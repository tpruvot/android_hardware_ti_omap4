//=============================================================================
//
// Copyright (c) 2011, Motorola Mobility Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.  Redistributions in binary
// form must reproduce the above copyright notice, this list of conditions and
// the following disclaimer in the documentation and/or other materials
// provided with the distribution.  Neither the name of Motorola Inc. nor the
// names of its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================

#define LOG_TAG "MotOverlay"

#include <hardware/hardware.h>
#include <hardware/overlay.h>

extern "C"
{
#include "v4l2_utils.h"
}

#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev.h>

#include <cutils/log.h>
#include <cutils/ashmem.h>
#include <cutils/atomic.h>
#include "overlay_common.h"
#include "MotOverlay.h"

#ifdef HAVE_LIBMIRROR
#include "IMirrorIpc.h"
#include "IMirrorIpcCB.h"
#endif
#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#define MAX_DISPLAY_CNT 1
#define MAX_MANAGER_CNT 3

#define LOG_FUNCTION_NAME_ENTRY LOGV("*** Calling %s() ++",  __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT  LOGV("*** Calling %s() --",  __FUNCTION__);

// This variable is to make sure that the overlay control device is opened
// only once from the surface flinger process;
// this will not protect the situation where the device is loaded from multiple processes
// and also, this is not valid in the data context, which is created in a process different
// from the surface flinger process

static overlay_control_context_t* TheOverlayControlDevice = NULL;

static struct hw_module_methods_t overlay_module_methods = {
    open: overlay_device_open
};

struct overlay_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: OVERLAY_HARDWARE_MODULE_ID,
        name: "Sample Overlay module",
        author: "The Android Open Source Project",
        methods: &overlay_module_methods,
        dso: NULL, /* remove compilation warnings */
        reserved: {0}, /* remove compilation warnings */
    }
};

namespace android {

// ----------------------------------------------------------------------------
// This class is really just a stub since we do all the reconfig'ing in the
// kernel, but we need the class to denote to the Mirror Service that an
// video/camera overlay is active

class IpcCB : public BnMirrorIpcCB
{
public:
             IpcCB();
    virtual ~IpcCB();

    virtual void notifyCallback(int32_t type, int32_t ext1, int32_t ext2);

    int clientID;
};

IpcCB::IpcCB()
{
    // Nothing to do
}

IpcCB::~IpcCB()
{
    // Nothing to do
}

void IpcCB::notifyCallback(int32_t type, int32_t ext1, int32_t ext2)
{
    // Nothing to do
}

// ----------------------------------------------------------------------------

}; // namespace android

android::sp<android::IBinder>    mIpcBinder = 0;
android::sp<android::IMirrorIpc> mIpcSrv    = 0;
android::sp<android::IpcCB>      mIpcClient;

static void connectToMirrorIpcServer( void );
static int  registerClientWithIpcServer( void );
static void unregisterClientWithIpcServer( void );

// ****************************************************************************
// Local static functions
// ****************************************************************************

#define MAIN_DISPLAY_PM_FB_SLEEP  "/sys/power/wait_for_fb_sleep"
#define MAIN_DISPLAY_PM_FB_AWAKE  "/sys/power/wait_for_fb_wake"

static int       sMainDisplayPMState   = 1; // 0 = sleeping, 1 = awake
static int       sMainDisplayJustAwake = 0;
static pthread_t sEventThrd            = 0;

static void* main_display_pm_event_loop( void* not_used )
{
    int  err = 0;
    char buf;
    int  fd;

    while ( 1 )
    {
        sMainDisplayPMState = 1;
        sMainDisplayJustAwake = 1;

        fd = open(MAIN_DISPLAY_PM_FB_SLEEP, O_RDONLY, 0);
        do
        {
            err = read(fd, &buf, 1);
        } while (err < 0 && errno == EINTR);
        close(fd);

        sMainDisplayPMState = 0;

        fd = open(MAIN_DISPLAY_PM_FB_AWAKE, O_RDONLY, 0);
        do
        {
            err = read(fd, &buf, 1);
        } while (err < 0 && errno == EINTR);
        close(fd);
    }

    // This thread should never exit
    return NULL;
}

#define MAIN_LCD_OVERLAY_ZORDER_PATH  "/sys/devices/platform/omapdss/overlay0/zorder"
#define MAIN_LCD_OVERLAY_ZORDER_VALUE "3"

static void init_main_display_path( void )
{
    int fd;

    // Set the main LCD overlay to the in-front overlay z-order (3)

    if ( (fd = open(MAIN_LCD_OVERLAY_ZORDER_PATH, O_RDWR)) < 0 )
    {
        LOGE("Failed open of LCD z-order SYSFS");
    }
    else
    {
        if ( (write(fd, MAIN_LCD_OVERLAY_ZORDER_VALUE, sizeof(MAIN_LCD_OVERLAY_ZORDER_VALUE))) <= 0 )
        {
            LOGE("Failed write of LCD z-order SYSFS");
        }

        close(fd);
    }

    if ( pthread_create( &sEventThrd, NULL, main_display_pm_event_loop, NULL) != 0 )
    {
        LOGW("Failed Main Display PM Thread Creation");
    }
}

static void connectToMirrorIpcServer()
{
    if ( mIpcBinder == 0 )
    {
        mIpcBinder = android::defaultServiceManager()->checkService(android::String16(MIRROR_SERVICE_NAME));

        if ( mIpcBinder == 0 )
        {
            LOGW("connectToMirrorIpcServer: Client Failed, No IPC Server");
        }
        else if ( (mIpcSrv = android::interface_cast<android::IMirrorIpc>(mIpcBinder)) == 0 )
        {
            LOGW("connectToMirrorIpcServer: Could not connect to server!");
        }
        else
        {
            LOGD("connectToMirrorIpcServer: Server connected!");
        }
    }
}

static int registerClientWithIpcServer()
{
    int rc = -1;

    if ( mIpcSrv == 0 )
    {
        LOGW("registerClientWithIpcServer: No IPC Server");
    }
    else if ( mIpcClient != 0 )
    {
        LOGW("registerClientWithIpcServer: Client Busy");
    }
    else
    {
        mIpcClient = new android::IpcCB();
        mIpcClient->clientID = mIpcSrv->registerCallback(mIpcClient);

        if ( mIpcClient->clientID < 0 )
        {
            LOGW("registerClientWithIpcServer: Register Failed");

            mIpcClient = 0;
        }
        else
        {
            rc = 0;
        }
    }

    return rc;
}

static void unregisterClientWithIpcServer()
{
    if ( mIpcSrv == 0 )
    {
        LOGW("unregisterClientWithIpcServer: No IPC Server");
    }
    else if ( mIpcClient == 0 )
    {
        LOGW("unregisterClientWithIpcServer: No Client");
    }
    else
    {
        mIpcSrv->unregisterCallback( mIpcClient->clientID );
        mIpcClient = 0;
    }
}

// ****************************************************************************
// Control context shared methods: to be used between control and data contexts
// ****************************************************************************
int overlay_control_context_t::create_shared_overlayobj(overlay_object** overlayobj)
{
    LOG_FUNCTION_NAME_ENTRY

    int fd;
    int rc = 0;
    int size = getpagesize() * 4; // NOTE: assuming sizeof(overlay_object) < 4 pages
    overlay_object *p;

    if ( (fd = ashmem_create_region("overlay_data", size)) < 0 )
    {
        LOGE("Failed to Create Overlay Data");
        return fd;
    }

    p = (overlay_object*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
    {
        LOGE("Failed to Map Overlay Data");
        close(fd);
        return -1;
    }

    memset(p, 0, size);
    p->marker = OVERLAY_DATA_MARKER;
    p->mControlHandle.overlayobj_size = size;
    p->mControlHandle.overlayobj_sharedfd = fd;
    p->refCnt = 1;

    if ( (rc = pthread_mutexattr_init(&p->attr)) != 0 )
    {
        LOGE("Failed to initialize overlay mutex attr");
    }
    else if ( (rc = pthread_mutexattr_setpshared(&p->attr, PTHREAD_PROCESS_SHARED)) != 0 )
    {
        LOGE("Failed to set the overlay mutex attr to be shared across-processes");
    }
    else if ( (rc = pthread_mutex_init(&p->lock, &p->attr)) != 0 )
    {
        LOGE("Failed to initialize overlay mutex");
    }

    if ( rc != 0 )
    {
        munmap(p, size);
        close(fd);
        return -1;
    }

    *overlayobj = p;

    LOG_FUNCTION_NAME_EXIT

    return fd;
}

void overlay_control_context_t::destroy_shared_overlayobj(overlay_object *overlayobj, bool isCtrlpath)
{
    LOG_FUNCTION_NAME_ENTRY

    if ( overlayobj == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return;
    }

    // Last side deallocated releases the mutex, otherwise the remaining
    // side will deadlock trying to use an already released mutex
    if ( android_atomic_dec(&(overlayobj->refCnt)) == 1 )
    {
        if ( pthread_mutex_destroy(&(overlayobj->lock)) )
        {
            LOGE("Failed to uninitialize overlay mutex");
        }

        if ( pthread_mutexattr_destroy(&(overlayobj->attr)) )
        {
            LOGE("Failed to uninitialize the overlay mutex attr");
        }

        overlayobj->marker = 0;
    }

    int overlayfd = -1;
    if ( isCtrlpath )
    {
        overlayfd = overlayobj->mControlHandle.overlayobj_sharedfd;
    }
    else
    {
        overlayfd = overlayobj->mDataHandle.overlayobj_sharedfd;
    }

    if ( munmap(overlayobj, overlayobj->mControlHandle.overlayobj_size) != 0 )
    {
        LOGE("Failed to Unmap Overlay Shared Data");
    }
    if ( close(overlayfd) != 0 )
    {
        LOGE("Failed to Close Overlay Shared Data");
    }

    LOG_FUNCTION_NAME_EXIT
}

overlay_object* overlay_control_context_t::open_shared_overlayobj(int ovlyfd, int ovlysize)
{
    LOG_FUNCTION_NAME_ENTRY

    int rc   = -1;
    int mode = PROT_READ | PROT_WRITE;
    overlay_object* overlay;

    overlay = (overlay_object*)mmap(0, ovlysize, mode, MAP_SHARED, ovlyfd, 0);

    if (overlay == MAP_FAILED)
    {
        LOGE("Failed to Map Overlay Shared Data!\n");
    }
    else if ( overlay->marker != OVERLAY_DATA_MARKER )
    {
        LOGE("Invalid Overlay Shared Marker!\n");
        munmap( overlay, ovlysize);
    }
    else if ( (int)overlay->mControlHandle.overlayobj_size != ovlysize )
    {
        LOGE("Invalid Overlay Shared Size!\n");
        munmap(overlay, ovlysize);
    }
    else
    {
        android_atomic_inc(&overlay->refCnt);
        overlay->mDataHandle.overlayobj_sharedfd = ovlyfd;
        rc = 0;
    }

    if ( rc != 0 )
    {
        overlay = NULL;
    }

    LOG_FUNCTION_NAME_EXIT

    return overlay;
}

void overlay_control_context_t::close_shared_overlayobj(overlay_object *overlayobj)
{
    destroy_shared_overlayobj(overlayobj, false);
    overlayobj = NULL;
}

int overlay_data_context_t::enable_streaming_locked(overlay_object* overlayobj, bool isDatapath)
{
    LOG_FUNCTION_NAME_ENTRY

    if ( overlayobj == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    int rc = 0;
    int fd;

    if ( !overlayobj->dataReady )
    {
        LOGI("Cannot enable stream without queuing at least one buffer");
        return -1;
    }

    overlayobj->streamEn = 1;

    fd = (isDatapath) ? overlayobj->getdata_videofd() : overlayobj->getctrl_videofd();

    if ( (rc = v4l2_overlay_stream_on(fd)) != 0 )
    {
        LOGE("Stream On Failed/%d", rc);
        overlayobj->streamEn = 0;
    }

    LOG_FUNCTION_NAME_EXIT

    return rc;
}

int overlay_data_context_t::enable_streaming(overlay_object* overlayobj, bool isDatapath)
{
    int rc = 0;

    if ( overlayobj == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    pthread_mutex_lock(&overlayobj->lock);

    rc = enable_streaming_locked(overlayobj, isDatapath);

    pthread_mutex_unlock(&overlayobj->lock);

    return rc;
}

int overlay_data_context_t::disable_streaming_locked(overlay_object* overlayobj, bool isDatapath)
{
    LOG_FUNCTION_NAME_ENTRY

    if ( overlayobj == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    int rc = 0;

    LOGI("disable_streaming_locked");

    if (overlayobj->streamEn)
    {
        int fd;

        fd = (isDatapath) ? overlayobj->getdata_videofd() : overlayobj->getctrl_videofd();

        if ( (rc = v4l2_overlay_stream_off( fd )) != 0 )
        {
            LOGE("Stream Off Failed!/%d\n", rc);
        }
        else
        {
            overlayobj->streamEn = 0;
            overlayobj->qd_buf_count = 0;
        }
    }

    LOG_FUNCTION_NAME_EXIT

    return rc;
}

// function not appropriate since it has mutex protection within.
int overlay_data_context_t::disable_streaming(overlay_object* overlayobj, bool isDatapath)
{
    int rc;

    if ( overlayobj == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    pthread_mutex_lock(&overlayobj->lock);

    rc = disable_streaming_locked(overlayobj, isDatapath);

    pthread_mutex_unlock(&overlayobj->lock);

    return rc;
}

// ****************************************************************************
// Control module context: used only in the control context
// ****************************************************************************

int overlay_control_context_t::overlay_get(struct overlay_control_device_t *dev, int name)
{
    int result = -1;

    switch (name)
    {
    case OVERLAY_MINIFICATION_LIMIT:   result = 0;  break; // 0 = no limit
    case OVERLAY_MAGNIFICATION_LIMIT:  result = 0;  break; // 0 = no limit
    case OVERLAY_SCALING_FRAC_BITS:    result = 0;  break; // 0 = infinite
    case OVERLAY_ROTATION_STEP_DEG:    result = 90; break; // 90 rotation steps (for instance)
    case OVERLAY_HORIZONTAL_ALIGNMENT: result = 1;  break; // 1-pixel alignment
    case OVERLAY_VERTICAL_ALIGNMENT:   result = 1;  break; // 1-pixel alignment
    case OVERLAY_WIDTH_ALIGNMENT:      result = 1;  break; // 1-pixel alignment
    case OVERLAY_HEIGHT_ALIGNMENT:     break;
    }

    return result;
}

overlay_t* overlay_control_context_t::overlay_createOverlay(struct overlay_control_device_t *dev,
                                                            uint32_t w, uint32_t h, int32_t  format,
                                                            int isS3D)
{
    LOG_FUNCTION_NAME_ENTRY;

    overlay_control_context_t *self = (overlay_control_context_t *)dev;
    overlay_object            *overlayobj = NULL;

    int err = 0;
    int fd;
    int overlay_fd;
    int pipelineId = -1;
    int index = 0;
    int overlayid = -1;
    uint32_t num = NUM_OVERLAY_BUFFERS_REQUESTED;

    connectToMirrorIpcServer();

    if ( format == OVERLAY_FORMAT_DEFAULT )
    {
        format = OVERLAY_FORMAT_CbYCrY_422_I;  // UYVY is the default format
    }

    // ONLY video/dev1 is allowed for camera and video playback.  
    // if video/dev1 aka mOmapOverlay[0] is not available, then surface flinger will retry
    if (self->mOmapOverlays[0] == NULL)
    {
        // this is succesful case to get video/dev1
        overlayid = 0;
    }

    if ( overlayid < 0 )
    {
        LOGE("No free overlay available");
    }
    // Validate the width and height are within a valid range for the video driver.
    else if (  w > MAX_OVERLAY_WIDTH_VAL
            || h > MAX_OVERLAY_HEIGHT_VAL
            || (w * h) > MAX_OVERLAY_RESOLUTION
            )
    {
        LOGE("Unsupported settings width %d, height %d (%d)", w, h, __LINE__);
    }
    else if ( w <= 0 || h <= 0 )
    {
        LOGE("Invalid settings width %d, height %d (%d)", w, h, __LINE__);
    }
    else if ( (fd = v4l2_overlay_open(overlayid)) < 0 )
    {
        LOGE("Failed to open overlay device[%d]\n", overlayid);
    }
    else if (  (overlay_fd = self->create_shared_overlayobj(&overlayobj)) < 0
            || overlayobj == NULL
            )
    {
        LOGE("Failed to create shared data");
        close(fd);
        overlayobj = NULL;
    }
    else if ( v4l2_overlay_init(fd, w, h, format) != 0 )
    {
        LOGE("Failed initializing overlays\n");
        err = 1;
    }
    else if ( v4l2_overlay_set_rotation(fd, 0, 0, 0) != 0 )
    {
        LOGE("Failed defaulting rotation\n");
        err = 1;
    }
    else if ( v4l2_overlay_set_crop(fd, 0, 0, w, h) != 0 )
    {
        LOGE("Failed defaulting crop window\n");
        err = 1;
    }
    else if ( v4l2_overlay_set_colorkey(fd, 0, 0, 0) != 0 )
    {
        LOGE("Failed disabling color key");
        err = 1;
    }
    else if ( v4l2_overlay_set_zorder(fd, 1) != 0 )
    {
        LOGE("Failed defaulting zorder\n");
        err = 1;
    }
    else if ( v4l2_overlay_req_buf(fd, &num, 0, 0, EMEMORY_MMAP) != 0 )
    {
        LOGE("Failed requesting buffers\n");
        err = 1;
    }
    else
    {
        registerClientWithIpcServer();

        LOGD("Enabling the OVERLAY[%d]", overlayid);
        overlayobj->init(fd, w, h, format, num, overlayid);
        overlayobj->controlReady = 0;
        overlayobj->streamEn = 0;
        overlayobj->dispW = LCD_WIDTH; // Need to determine this properly: change it in setParameter
        overlayobj->dispH = LCD_HEIGHT; // Need to determine this properly
        overlayobj->mData.cropX = 0;
        overlayobj->mData.cropY = 0;
        overlayobj->mData.cropW = w;
        overlayobj->mData.cropH = h;

        self->mOmapOverlays[overlayid] = overlayobj;
    }

    if ( err )
    {
        close(fd);
        self->destroy_shared_overlayobj(overlayobj);
        overlayobj = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;

    return overlayobj;
}

overlay_t* overlay_control_context_t::overlay_createOverlay(struct overlay_control_device_t *dev,
                                                            uint32_t w, uint32_t h, int32_t  format)
{
    // For Non-3D case
    return overlay_createOverlay(dev, w, h, format, 0);
}


void overlay_control_context_t::overlay_destroyOverlay(struct overlay_control_device_t *dev,
                                                       overlay_t* overlay)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL || overlay == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return;
    }

    overlay_control_context_t *self = (overlay_control_context_t *)dev;
    overlay_object *overlayobj = static_cast<overlay_object *>(overlay);

    int rc;
    int fd = overlayobj->getctrl_videofd();
    int index = overlayobj->getIndex();

    /* In case Surface Destroy happens first from SurfaceFlinger, remove constraint */
    if(overlayobj->take_constraint) {
        overlayobj->take_constraint = 0;
        rc = v4l2_overlay_take_constraint(fd, 0);
        if(rc < 0) {
               LOGE("Constraint cannot be reset");
        }
    }

    overlay_data_context_t::disable_streaming(overlayobj, false);

    LOGI("Destroying overlay/fd=%d/obj=%08lx", fd, (unsigned long)overlay);

    // Tell the Mirror Service the overlay is going away
    unregisterClientWithIpcServer();

    if ( close(fd) )
    {
        LOGI( "Error closing overly fd/%d", errno);
    }

    self->destroy_shared_overlayobj(overlayobj);

    // NOTE: Needs no protection as the calls are already serialized at Surfaceflinger level
    self->mOmapOverlays[index] = NULL;

    LOG_FUNCTION_NAME_EXIT;
}

int overlay_control_context_t::overlay_setPosition(struct overlay_control_device_t *dev,
                                                   overlay_t* overlay,
                                                   int x, int y, uint32_t w, uint32_t h)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL || overlay == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    overlay_object *overlayobj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *data  = overlayobj->data();
    overlay_ctrl_t *stage = overlayobj->staging();
    int fd = overlayobj->getctrl_videofd();
    int rc = 0;

    // FIXME:  This is a hack to deal with seemingly unintentional negative
    // offset that pop up now and again.  I believe the negative offsets are
    // due to a surface flinger bug that has not yet been found or fixed.
    //
    // This logic here is to return an error if the rectangle is not fully
    // within the display, unless we have not received a valid position yet,
    // in which case we will do our best to adjust the rectangle to be within
    // the display.

    // Require a minimum size
    if ( w < 2 || h < 2 )
    {
        LOGI("overlay_setPosition: size too small %d/%d/%d/%d", w, h, x, y);
        rc = -1;
    }
    else if ( !overlayobj->controlReady )
    {
        if ( x < 0 ) x = 0;
        if ( y < 0 ) y = 0;
        if ( w > overlayobj->dispW ) w = overlayobj->dispW;
        if ( h > overlayobj->dispH ) h = overlayobj->dispH;
        if ( (x + w) > overlayobj->dispW ) w = overlayobj->dispW - x;
        if ( (y + h) > overlayobj->dispH ) h = overlayobj->dispH - y;
    }
    else if (  x < 0
            || y < 0
            || (x + w) > overlayobj->dispW
            || (y + h) > overlayobj->dispH
            )
    {
        LOGI("overlay_setPosition: invalid size %d/%d/%d/%d", w, h, x, y);
        rc = -1;
    }

    if ( rc == 0 )
    {
        if (  data->posX == (unsigned int)x
           && data->posY == (unsigned int)y
           && data->posW == w
           && data->posH == h
           )
        {
            LOGI("Nothing to do (SP)");
        }
        else
        {
            stage->posX = x;
            stage->posY = y;
            stage->posW = w;
            stage->posH = h;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return rc;
}

int overlay_control_context_t::overlay_getPosition(struct overlay_control_device_t *dev,
                                                   overlay_t* overlay,
                                                   int* x, int* y, uint32_t* w, uint32_t* h)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL || overlay == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    int rc = 0;
    int fd = static_cast<overlay_object *>(overlay)->getctrl_videofd();

    if ( v4l2_overlay_get_position(fd, x, y, (int32_t*)w, (int32_t*)h) != 0 )
    {
        rc = -EINVAL;
    }

    return rc;
}

int overlay_control_context_t::overlay_setParameter(struct overlay_control_device_t *dev,
                                                    overlay_t* overlay, int param, int value)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL || overlay == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    overlay_object *overlayobj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *stage = static_cast<overlay_object *>(overlay)->staging();
    int rc = 0;

    switch (param)
    {
    case OVERLAY_DITHER:
        break;
    case OVERLAY_TRANSFORM:
        switch ( value )
        {
        case 0:
            stage->rotation = 0;
            stage->mirror   = false;
            break;
        case OVERLAY_TRANSFORM_ROT_90:
            stage->rotation = 90;
            stage->mirror   = false;
            break;
        case OVERLAY_TRANSFORM_ROT_180:
            stage->rotation = 180;
            stage->mirror   = false;
            break;
        case OVERLAY_TRANSFORM_ROT_270:
            stage->rotation = 270;
            stage->mirror   = false;
            break;
        case OVERLAY_TRANSFORM_FLIP_H:
            stage->rotation = 0;
            stage->mirror   = true;
            break;
        case OVERLAY_TRANSFORM_FLIP_V:
            stage->rotation = 180;
            stage->mirror   = true;
            break;

/* TEMPORARY MODIFIED THE TRANSFORM_FLIP_H and TRANSFORM_FLIP_V
 * To resolve preview rotation issues on Solana.  This is put
 * in under IKSTABLEFIVE-149 and is known to not work with TARGA
 * product.  After we enable the test teams, this WILL be revisited.
 * For now, the fix resolves the rotation issue of secondary sensor
 * for solana preview.
 */
        case (HAL_TRANSFORM_FLIP_H | OVERLAY_TRANSFORM_ROT_90): // was HAL_TRANSFORM_FLIP_V | OVERLAY_TRANSFORM_ROT_90
            stage->rotation = 90; 
            stage->mirror   = true;
            break;
        case (HAL_TRANSFORM_FLIP_V | OVERLAY_TRANSFORM_ROT_90): // was HAL_TRANSFORM_FLIP_H | OVERLAY_TRANSFORM_ROT_90
            stage->rotation = 270;
            stage->mirror   = true;
            break;
        default:
            rc = -EINVAL;
            break;
        }
        break;
    case OVERLAY_COLOR_KEY:
        LOGD("Overlay Colorkey set to 0x%02x", value);
//        stage->colorkey = value;
        break;
    case OVERLAY_PLANE_ALPHA:
        LOGD("Overlay Alpha set to 0x%02x", value);
        // Adjust the alpha to the HW limit
//        stage->alpha = MIN(value, 0xFF);
        break;
    case OVERLAY_PLANE_Z_ORDER:
        LOGD("Overlay z-order set to %d", value);
        // Limit the max value of z-order to HW limit (i.e. 3)
        // GFX is hardcoded to 3, make sure that video is behind GFX
        stage->zorder = MIN(value, 2);
        break;
    case OVERLAY_SET_DISPLAY_WIDTH:
        LOGD("Overlay Display width set to %d", value);
        overlayobj->dispW = value;
        break;
    case OVERLAY_SET_DISPLAY_HEIGHT:
        LOGD("Overlay Display height set to %d", value);
        overlayobj->dispH = value;
        break;
    case OVERLAY_SET_SCREEN_ID:
        break;
    default:
        rc = -1;
    }

    LOG_FUNCTION_NAME_EXIT;

    return rc;
}

int overlay_control_context_t::overlay_stage(struct overlay_control_device_t *dev,
                                             overlay_t* overlay)
{
    return 0;
}

int overlay_control_context_t::overlay_commit(struct overlay_control_device_t *dev,
                                              overlay_t* overlay)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL || overlay == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    overlay_object *overlayobj = static_cast<overlay_object *>(overlay);
    overlay_control_context_t *self = (overlay_control_context_t *)dev;
    overlay_ctrl_t *data   = overlayobj->data();
    overlay_ctrl_t *stage  = overlayobj->staging();

    int rc = 0;
    overlay_ctrl_t win;
    int fd = overlayobj->getctrl_videofd();
    overlay_data_t crp;
    int sameWin = (  data->posX == stage->posX && data->posY == stage->posY
                  && data->posW == stage->posW && data->posH == stage->posH) ? 1 : 0;

    pthread_mutex_lock(&overlayobj->lock);

    overlayobj->controlReady = 1;

    // If Rotation has changed but window has not changed yet, ignore this commit.
    // SurfaceFlinger will set the right window parameters and call commit again.
    // MOT ignores alpha, colorkey, and zorder since these are all fixed values.
    if (  sameWin
       && data->mirror == stage->mirror
       )
    {
        LOGI("Nothing to do (C/%d)", sMainDisplayJustAwake);
        pthread_mutex_unlock(&overlayobj->lock);

        // In this case V4L2 streaming would normally be left on, but in the case
        // where the LCD power was off and just woke up, go ahead and disable
        // streaming to ensure that no odd content is displayed
        if ( sMainDisplayJustAwake == 1 )
        {
            overlay_data_context_t::disable_streaming_locked(overlayobj, false);
            sMainDisplayJustAwake = 0;
        }

        return 0;
    }

    data->posX       = stage->posX;
    data->posY       = stage->posY;
    data->posW       = stage->posW;
    data->posH       = stage->posH;
    data->rotation   = stage->rotation;
    data->mirror     = stage->mirror;

    // Adjust the coordinate system to match the V4L change
    win.posX = data->posX;
    win.posY = data->posY;
    switch ( data->rotation )
    {
    case 90:
    case 270:
        win.posW = data->posH;
        win.posH = data->posW;
        break;
    case 180:
// This is just a hack that should be fixed in the overlay driver
//        win.posX = ((overlayobj->dispW - data->posX) - data->posW);
//        win.posY = ((overlayobj->dispH - data->posY) - data->posH);
//        win.posW = data->posW;
//        win.posH = data->posH;
//        break;
    default: // 0
        win.posW = data->posW;
        win.posH = data->posH;
        break;
    }

    LOGI("Pos/X%d/Y%d/W%d/H%d", data->posX, data->posY, data->posW, data->posH );
    LOGI("AdjPos/X%d/Y%d/W%d/H%d", win.posX, win.posY, win.posW, win.posH);
    LOGI("Rot/%d", data->rotation );
    LOGI("Mirror/%d", data->mirror );

    if ( (rc = v4l2_overlay_get_crop(fd, &crp.cropX, &crp.cropY, &crp.cropW, &crp.cropH)) != 0 )
    {
        LOGE("Get crop value Failed/%d", rc);
    }
    // Disable streaming to ensure that stream_on is called again which indirectly sets overlayenabled to 1
    else if ( (rc = overlay_data_context_t::disable_streaming_locked(overlayobj, false)) != 0 )
    {
        LOGE("Stream Off Failed/%d", rc);
    }
    else if ( (rc = v4l2_overlay_set_rotation(fd, data->rotation, 0, data->mirror)) != 0 )
    {
        LOGE("Set Rotation Failed/%d", rc);
    }
    else if ( (rc = v4l2_overlay_set_crop(fd, crp.cropX, crp.cropY, crp.cropW, crp.cropH)) != 0 )
    {
        LOGE("Set Cropping Failed/%d", rc);
    }
    else if ( (rc = v4l2_overlay_set_position(fd, win.posX, win.posY, win.posW, win.posH)) != 0 )
    {
        LOGE("Set Position Failed/%d", rc);
    }

    pthread_mutex_unlock(&overlayobj->lock);

    LOG_FUNCTION_NAME_EXIT;

    return rc;
}

int overlay_control_context_t::overlay_control_close(struct hw_device_t *dev)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    struct overlay_control_context_t* self = (struct overlay_control_context_t*)dev;

    if (self)
    {
        for (int i = 0; i < MAX_NUM_OVERLAYS; i++)
        {
            if (self->mOmapOverlays[i])
            {
                overlay_destroyOverlay((struct overlay_control_device_t *)self, (overlay_t*)(self->mOmapOverlays[i]));
            }
        }

        free(self);
        TheOverlayControlDevice = NULL;
    }

    return 0;
}

// ****************************************************************************
// Data module
// ****************************************************************************

int overlay_data_context_t::overlay_initialize(struct overlay_data_device_t *dev,
                                               overlay_handle_t handle)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL )
    {
        LOGE("Null Arguments[%d]", __LINE__);
        return -1;
    }

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    struct stat stat;

    int i;
    int rc = -1;

    int ovlyfd   = static_cast<const struct handle_t *>(handle)->overlayobj_sharedfd;
    int size     = static_cast<const struct handle_t *>(handle)->overlayobj_size;
    int video_fd = static_cast<const struct handle_t *>(handle)->video_fd;
    overlay_object* overlayobj = overlay_control_context_t::open_shared_overlayobj(ovlyfd, size);

    if (overlayobj == NULL)
    {
        LOGE("Overlay Initialization failed");
        return -1;
    }

    ctx->omap_overlay = overlayobj;
    overlayobj->mDataHandle.video_fd = video_fd;
    overlayobj->mDataHandle.overlayobj_sharedfd = ovlyfd;
    overlayobj->mDataHandle.overlayobj_size = size;

    ctx->omap_overlay->cacheable_buffers = 0;
    ctx->omap_overlay->maintain_coherency = 1;
    ctx->omap_overlay->optimalQBufCnt = NUM_BUFFERS_TO_BE_QUEUED_FOR_OPTIMAL_PERFORMANCE;
    ctx->omap_overlay->attributes_changed = 0;
    ctx->omap_overlay->take_constraint = 0;
    ctx->omap_overlay->mData.cropX = 0;
    ctx->omap_overlay->mData.cropY = 0;
    ctx->omap_overlay->mData.cropW = overlayobj->w;
    ctx->omap_overlay->mData.cropH = overlayobj->h;

    if ( fstat(video_fd, &stat) )
    {
        LOGE("Error = %s from overlay initialize", strerror(errno));
        return -1;
    }

    LOGD("Num of Buffers = %d", ctx->omap_overlay->num_buffers);

    ctx->omap_overlay->dataReady = 0;
    ctx->omap_overlay->qd_buf_count = 0;
    ctx->omap_overlay->mapping_data = new mapping_data_t;
    ctx->omap_overlay->buffers     = new void* [NUM_OVERLAY_BUFFERS_MAX];
    ctx->omap_overlay->buffers_len = new size_t[NUM_OVERLAY_BUFFERS_MAX];
    if (  !ctx->omap_overlay->buffers
       || !ctx->omap_overlay->buffers_len
       || !ctx->omap_overlay->mapping_data
       )
    {
        LOGE("Failed alloc'ing buffer arrays");
        overlay_control_context_t::close_shared_overlayobj(ctx->omap_overlay);
    }
    else
    {
        for (i = 0; i < ctx->omap_overlay->num_buffers; i++)
        {
            rc = v4l2_overlay_map_buf(video_fd, i, &ctx->omap_overlay->buffers[i],
                                       &ctx->omap_overlay->buffers_len[i]);
            if (rc)
            {
                LOGE("Failed mapping buffers");
                overlay_control_context_t::close_shared_overlayobj( ctx->omap_overlay );
                break;
            }
        }
    }
    ctx->omap_overlay->mappedbufcount = ctx->omap_overlay->num_buffers;

    LOG_FUNCTION_NAME_EXIT;
    LOGD("Initialize ret = %d", rc);

    return ( rc );
}

int overlay_data_context_t::overlay_resizeInput(struct overlay_data_device_t *dev, uint32_t w, uint32_t h)
{
    LOG_FUNCTION_NAME_ENTRY;

    if (dev == NULL)
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }

    overlay_data_t crp;
    int      rc     = 0;
    int      degree = 0;
    int32_t  win_x  = 0;
    int32_t  win_y  = 0;
    int32_t  win_w  = 0;
    int32_t  win_h  = 0;
    uint32_t mirror = 0;

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (!ctx->omap_overlay)
    {
        LOGE("Overlay Not Init'd! (%d)", __LINE__);
        return -1;
    }

    int fd = ctx->omap_overlay->getdata_videofd();
    overlay_ctrl_t *stage = ctx->omap_overlay->staging();

    // Validate the width and height are within a valid range for the video driver
    if (  w > MAX_OVERLAY_WIDTH_VAL
       || h > MAX_OVERLAY_HEIGHT_VAL
       || (w * h) > MAX_OVERLAY_RESOLUTION
       )
    {
       LOGE("%d: Error - Currently unsupported settings width %d, height %d", __LINE__, w, h);
       return -1;
    }
    if ( w <= 0 || h <= 0 )
    {
       LOGE("%d: Error - Invalid settings width %d, height %d", __LINE__, w, h);
       return -1;
    }

    if (  (ctx->omap_overlay->w == (unsigned int)w)
       && (ctx->omap_overlay->h == (unsigned int)h)
       && (ctx->omap_overlay->attributes_changed == 0))
    {
        LOGE("Nothing to do (RI)");

        // Lets reset the statemachine and disable the stream
        ctx->omap_overlay->dataReady = 0;
        if ( (rc = ctx->disable_streaming_locked(ctx->omap_overlay)) != 0 )
        {
            return -1;
        }
        return 0;
    }

    pthread_mutex_lock(&ctx->omap_overlay->lock);

    if ( (rc = ctx->disable_streaming_locked(ctx->omap_overlay)) != 0 )
    {
        LOGE("Disable streaming failed/%d", rc);
    }
    else if ( (rc = v4l2_overlay_get_crop(fd, &crp.cropX, &crp.cropY, &crp.cropW, &crp.cropH)) != 0 )
    {
        LOGE("Get crop value failed/%d", rc);
    }
    else if ( (rc = v4l2_overlay_get_position(fd, &win_x,  &win_y, &win_w, &win_h)) != 0 )
    {
        LOGE("Get position failed/%d", rc);
    }
    else if ( (rc = v4l2_overlay_get_rotation(fd, &degree, NULL, &mirror)) != 0 )
    {
        LOGE("Get rotation value failed/%d", rc);
    }
    else
    {
        for (int i = 0; i < ctx->omap_overlay->mappedbufcount; i++)
        {
            v4l2_overlay_unmap_buf(ctx->omap_overlay->buffers[i], ctx->omap_overlay->buffers_len[i]);
        }

        if ( (rc = v4l2_overlay_init(fd, w, h, ctx->omap_overlay->format)) != 0 )
        {
            LOGE("Error initializing overlay");
        }
        else if ( (rc = v4l2_overlay_set_rotation(fd, degree, 0, mirror)) != 0)
        {
            LOGE("Failed rotation");
        }
        else if ( (rc = v4l2_overlay_set_crop(fd, crp.cropX, crp.cropY, crp.cropW, crp.cropH)) != 0 )
        {
            LOGE("Failed crop window");
        }
        else if ( (rc = v4l2_overlay_set_position(fd, win_x,  win_y, win_w, win_h)) != 0 )
        {
            LOGE("Set position failed");
        }
        else if ( (rc = v4l2_overlay_req_buf( fd
                                            , (uint32_t *)(&ctx->omap_overlay->num_buffers)
                                            , ctx->omap_overlay->cacheable_buffers
                                            , ctx->omap_overlay->maintain_coherency
                                            , EMEMORY_MMAP)) != 0 )
        {
            LOGE("Error creating buffers");
        }
        else
        {
            // Update the overlay object with the new width and height
            ctx->omap_overlay->w = w;
            ctx->omap_overlay->h = h;

            for (int i = 0; i < ctx->omap_overlay->num_buffers; i++)
            {
                v4l2_overlay_map_buf(fd, i, &ctx->omap_overlay->buffers[i], &ctx->omap_overlay->buffers_len[i]);
            }

            ctx->omap_overlay->mappedbufcount = ctx->omap_overlay->num_buffers;
            ctx->omap_overlay->dataReady = 0;

            // The control pameters just got set
            ctx->omap_overlay->controlReady = 1;
            ctx->omap_overlay->attributes_changed = 0; // Reset it
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    pthread_mutex_unlock(&ctx->omap_overlay->lock);

    return rc;
}

int overlay_data_context_t::overlay_data_setParameter(struct overlay_data_device_t *dev,
                                     int param, int value)
{
    LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL )
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }

    int rc = 0;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;

    if ( ctx->omap_overlay == NULL )
    {
        LOGE("Overlay Not Init'd! (%d)", __LINE__);
        return -1;
    }

    switch ( param )
    {
    case CACHEABLE_BUFFERS:
        ctx->omap_overlay->cacheable_buffers = value;
        ctx->omap_overlay->attributes_changed = 1;
        break;
    case MAINTAIN_COHERENCY:
        ctx->omap_overlay->maintain_coherency = value;
        ctx->omap_overlay->attributes_changed = 1;
        break;
    case OPTIMAL_QBUF_CNT:
        if (value < 0)
        {
            LOGE("InValid number of optimal quebufffers requested[%d]", value);
            rc = -1;
        }
        else
        {
            ctx->omap_overlay->optimalQBufCnt = MIN(value, NUM_OVERLAY_BUFFERS_MAX);
        }
        break;
    case OVERLAY_NUM_BUFFERS:
        if (value <= 0)
        {
            LOGE("InValid number of Overlay bufffers requested[%d]", value);
            rc = -1;
        }
        else
        {
            ctx->omap_overlay->num_buffers = MIN(value, NUM_OVERLAY_BUFFERS_MAX);
            ctx->omap_overlay->attributes_changed = 1;
        }
        break;
    case TAKE_CONSTRAINT:
        if(value) {
           rc = v4l2_overlay_take_constraint(ctx->omap_overlay->mDataHandle.video_fd, 1);
           if(rc < 0) {
               LOGE("Constraint cannot be set");
           }
           else {
               ctx->omap_overlay->take_constraint = value;
           }
        }
        else {
           rc = v4l2_overlay_take_constraint(ctx->omap_overlay->mDataHandle.video_fd, 0);
           if(rc < 0) {
               LOGE("Constraint cannot be reset");
           }
           else {
               ctx->omap_overlay->take_constraint = value;
           }
        }
        break;
    default:
        rc = -1;
    }
    LOG_FUNCTION_NAME_EXIT;

    return ( rc );
}

int overlay_data_context_t::overlay_setCrop(struct overlay_data_device_t *dev,
                                           uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    //LOG_FUNCTION_NAME_ENTRY;

    if ( dev == NULL )
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }

    if (  ((int)x < 0)
       || ((int)y < 0)
       || ((int)w <= 0)
       || ((int)h <= 0)
       || (w > MAX_OVERLAY_WIDTH_VAL)
       || (h > MAX_OVERLAY_HEIGHT_VAL)
       )
    {
        LOGE("Invalid Arguments (%d)", __LINE__);
        return -1;
    }

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    int32_t  win_x  = 0;
    int32_t  win_y  = 0;
    int32_t  win_w  = 0;
    int32_t  win_h  = 0;
    int rc = 0;
    int fd;

    if ( ctx->omap_overlay == NULL )
    {
        LOGE("Overlay Not Init'd! (%d)", __LINE__);
        return -1;
    }

    if (  ctx->omap_overlay->mData.cropX == x
       && ctx->omap_overlay->mData.cropY == y
       && ctx->omap_overlay->mData.cropW == w
       && ctx->omap_overlay->mData.cropH == h
       )
    {
        LOGI("Nothing to do (SC)");
        return 0;
    }

    fd = ctx->omap_overlay->getdata_videofd();

    pthread_mutex_lock(&ctx->omap_overlay->lock);

    ctx->omap_overlay->dataReady = 1;

    if ( (rc = v4l2_overlay_get_position(fd, &win_x, &win_y, &win_w, &win_h)) != 0 )
    {
        LOGD("Could not get the position when setting the crop");
    }
    else if (  (ctx->omap_overlay->mData.cropW != w)
            || (ctx->omap_overlay->mData.cropH != h)
            )
    {
        ctx->omap_overlay->mData.cropX = x;
        ctx->omap_overlay->mData.cropY = y;
        ctx->omap_overlay->mData.cropW = w;
        ctx->omap_overlay->mData.cropH = h;
        LOGD("Crop Win/X%d/Y%d/W%d/H%d", x, y, w, h );

        // Since setCrop is called with crop window change, hence disable the stream first
        // and set the final window as well
        if ( (rc = ctx->disable_streaming_locked(ctx->omap_overlay)) )
        {
            LOGE("Disable streaming failed/%d", rc);
        }
        else if ( (rc = v4l2_overlay_set_crop(fd, x, y, w, h)) != 0 )
        {
            LOGE("Set crop window failed/%d", rc);
        }
        else if ( (rc = v4l2_overlay_set_position(fd, win_x, win_y, win_w, win_h)) )
        {
            LOGE("Set position failed/%d", rc);
        }
    }
    else
    {
        // setCrop is called only with the crop co-ordinates change
        // this is allowed on-the-fly. Hence no need of disabling the stream and
        // the final window remains the same
        ctx->omap_overlay->mData.cropX = x;
        ctx->omap_overlay->mData.cropY = y;
        ctx->omap_overlay->mData.cropW = w;
        ctx->omap_overlay->mData.cropH = h;

        if ( (rc = v4l2_overlay_set_crop(fd, x, y, w, h)) != 0 )
        {
            LOGE("Set crop window failed/%d", rc);
        }
    }

    pthread_mutex_unlock(&ctx->omap_overlay->lock);

    return rc;
}

int overlay_data_context_t::overlay_getCrop(struct overlay_data_device_t *dev , uint32_t* x,
                                                   uint32_t* y, uint32_t* w, uint32_t* h)
{
    if ( dev == NULL )
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    int fd = ctx->omap_overlay->getdata_videofd();
    return v4l2_overlay_get_crop(fd, x, y, w, h);
}

int overlay_data_context_t::overlay_dequeueBuffer(struct overlay_data_device_t *dev,
                                                         overlay_buffer_t *buffer)
{
    // blocks until a buffer is available and return an opaque structure
    // representing this buffer.
    if ( dev == NULL || buffer == NULL )
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    int fd = ctx->omap_overlay->getdata_videofd();
    int rc = 0;
    int i = -1;

    pthread_mutex_lock(&ctx->omap_overlay->lock);

    if ( ctx->omap_overlay->streamEn == 0 )
    {
        LOGE("Cannot dequeue when streaming is disabled (qd_buf_count = %d)", ctx->omap_overlay->qd_buf_count);
        rc = -EPERM;
    }
    else if ( ctx->omap_overlay->qd_buf_count < ctx->omap_overlay->optimalQBufCnt )
    {
        LOGV("Queue more buffers before attempting to dequeue");
        rc = -EPERM;
    }
    else if ( (rc = v4l2_overlay_dq_buf(fd, &i, EMEMORY_MMAP, NULL, 0 )) != 0 )
    {
        LOGE("Failed to DQ/%d\n", rc);
        // In order to recover from DQ failure scenario, let's disable the stream.
        // The stream gets re-enabled in the subsequent Q buffer call
        // if streamoff also fails just return the errorcode to the client
        rc = disable_streaming_locked(ctx->omap_overlay, true);
        if (rc == 0)
        {
            rc = -1;
        } // This is required for TIHardwareRenderer
    }
    else if ( i < 0 || i > ctx->omap_overlay->num_buffers )
    {
        LOGE("dqbuffer i=%d",i);
        rc = -EPERM;
    }
    else
    {
        *((int *)buffer) = i;
        ctx->omap_overlay->qd_buf_count --;
        LOGV("INDEX DEQUEUE = %d", i);
        LOGV("qd_buf_count --");
    }

    LOGV("qd_buf_count = %d", ctx->omap_overlay->qd_buf_count);

    pthread_mutex_unlock(&ctx->omap_overlay->lock);

    return ( rc );
}

int overlay_data_context_t::overlay_queueBuffer(struct overlay_data_device_t *dev,
                                                       overlay_buffer_t buffer)
{
    if (dev == NULL)
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (  ((int)buffer < 0)
       || ((int)buffer >= ctx->omap_overlay->num_buffers)
       )
    {
        LOGE("Invalid buffer Index [%d]@[%d]", (int)buffer, __LINE__);
        return -1;
    }

    int fd = ctx->omap_overlay->getdata_videofd();

    if ( !ctx->omap_overlay->controlReady )
    {
        LOGI("Control not ready but queue buffer requested");
    }

    pthread_mutex_lock(&ctx->omap_overlay->lock);

    int rc = v4l2_overlay_q_buf(fd, (int)buffer, EMEMORY_MMAP, NULL, 0);
    if (rc != 0)
    {
        LOGD("queueBuffer failed/%d", rc);
        rc = -EPERM;
    }
    else if (ctx->omap_overlay->qd_buf_count < ctx->omap_overlay->num_buffers)
    {
        ctx->omap_overlay->qd_buf_count ++;
        rc = ctx->omap_overlay->qd_buf_count;

        if (ctx->omap_overlay->streamEn == 0)
        {
            /* DSS2: 2 buffers need to be queue before enable streaming */
            ctx->omap_overlay->dataReady = 1;
            ctx->enable_streaming_locked(ctx->omap_overlay);
        }
    }

    pthread_mutex_unlock(&ctx->omap_overlay->lock);

    return ( rc );
}

void* overlay_data_context_t::overlay_getBufferAddress(struct overlay_data_device_t *dev,
                                                              overlay_buffer_t buffer)
{
    if (dev == NULL)
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return NULL;
    }

    int ret;
    struct v4l2_buffer buf;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (((int)buffer < 0)||((int)buffer >= ctx->omap_overlay->num_buffers))
    {
        LOGE("Invalid buffer Index [%d]@[%d]", (int)buffer, __LINE__);
        return NULL;
    }
    int fd = ctx->omap_overlay->getdata_videofd();
    ret = v4l2_overlay_query_buffer(fd, (int)buffer, &buf);
    if (ret)
        return NULL;

    // Initialize ctx->mapping_data
    memset(ctx->omap_overlay->mapping_data, 0, sizeof(mapping_data_t));

    ctx->omap_overlay->mapping_data->fd = fd;
    ctx->omap_overlay->mapping_data->length = buf.length;
    ctx->omap_overlay->mapping_data->offset = buf.m.offset;
    ctx->omap_overlay->mapping_data->ptr = NULL;

    if ((int)buffer >= 0 && (int)buffer < ctx->omap_overlay->num_buffers)
    {
        ctx->omap_overlay->mapping_data->ptr = ctx->omap_overlay->buffers[(int)buffer];
        LOGE("Buffer/%d/addr=%08lx/len=%d", (int)buffer, (unsigned long)ctx->omap_overlay->mapping_data->ptr,
                                                                           ctx->omap_overlay->buffers_len[(int)buffer]);
    }

    return (void *)ctx->omap_overlay->mapping_data;

}

int overlay_data_context_t::overlay_getBufferCount(struct overlay_data_device_t *dev)
{
    if (dev == NULL)
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    return (ctx->omap_overlay->num_buffers);
}

int overlay_data_context_t::overlay_data_close(struct hw_device_t *dev)
{

    LOG_FUNCTION_NAME_ENTRY;
    if (dev == NULL)
    {
        LOGE("Null Arguments (%d)", __LINE__);
        return -1;
    }

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    int rc;

    if (ctx)
    {
        int buf;
        int i;

        pthread_mutex_lock(&ctx->omap_overlay->lock);

        if ((rc = (ctx->disable_streaming_locked(ctx->omap_overlay))))
        {
            LOGE("Stream Off Failed/%d", rc);
        }

        for (i = 0; i < ctx->omap_overlay->mappedbufcount; i++)
        {
            LOGV("Unmap Buffer/%d/%08lx/%d", i, (unsigned long)ctx->omap_overlay->buffers[i],
                                                               ctx->omap_overlay->buffers_len[i] );
            rc = v4l2_overlay_unmap_buf(ctx->omap_overlay->buffers[i], ctx->omap_overlay->buffers_len[i]);
            if (rc != 0)
            {
                LOGE("Error unmapping the buffer/%d/%d", i, rc);
            }
        }
        delete (ctx->omap_overlay->mapping_data);
        delete [](ctx->omap_overlay->buffers);
        delete [](ctx->omap_overlay->buffers_len);
        ctx->omap_overlay->mapping_data = NULL;
        ctx->omap_overlay->buffers = NULL;
        ctx->omap_overlay->buffers_len = NULL;
        ctx->omap_overlay->dataReady = 0;

        pthread_mutex_unlock(&ctx->omap_overlay->lock);

        overlay_control_context_t::close_shared_overlayobj(ctx->omap_overlay);

        free(ctx);
    }

    return 0;
}

/*****************************************************************************/

static int overlay_device_open(const struct hw_module_t* module,
                               const char* name, struct hw_device_t** device)
{
    LOG_FUNCTION_NAME_ENTRY
    int status = -EINVAL;

    if (!strcmp(name, OVERLAY_HARDWARE_CONTROL))
    {
        if (TheOverlayControlDevice != NULL)
        {
            LOGE("Control device is already Open");
            *device = &(TheOverlayControlDevice->common);
            return 0;
        }

        overlay_control_context_t *dev;
        dev = (overlay_control_context_t*)malloc(sizeof(*dev));
        if (dev)
        {
            // initialize our state here
            memset(dev, 0, sizeof(*dev));

            // initialize the procs
            dev->common.tag     = HARDWARE_DEVICE_TAG;
            dev->common.version = 0;
            dev->common.module  = const_cast<hw_module_t*>(module);
            dev->common.close   = overlay_control_context_t::overlay_control_close;

            dev->get            = overlay_control_context_t::overlay_get;
            dev->createOverlay  = overlay_control_context_t::overlay_createOverlay;
            dev->destroyOverlay = overlay_control_context_t::overlay_destroyOverlay;
            dev->setPosition    = overlay_control_context_t::overlay_setPosition;
            dev->getPosition    = overlay_control_context_t::overlay_getPosition;
            dev->setParameter   = overlay_control_context_t::overlay_setParameter;
            dev->stage          = overlay_control_context_t::overlay_stage;
            dev->commit         = overlay_control_context_t::overlay_commit;

            *device = &dev->common;
            for (int i = 0; i < MAX_NUM_OVERLAYS; i++)
            {
                dev->mOmapOverlays[i] = NULL;
            }
            TheOverlayControlDevice = dev;

            init_main_display_path();
        }
        else
        {
            // Error: no memory available
            status = -ENOMEM;
        }

    }
    else if (!strcmp(name, OVERLAY_HARDWARE_DATA))
    {
        overlay_data_context_t *dev;
        dev = (overlay_data_context_t*)malloc(sizeof(*dev));
        if (dev)
        {
            // initialize our state here
            memset(dev, 0, sizeof(*dev));

            // initialize the procs
            dev->common.tag       = HARDWARE_DEVICE_TAG;
            dev->common.version   = 0;
            dev->common.module    = const_cast<hw_module_t*>(module);
            dev->common.close     = overlay_data_context_t::overlay_data_close;

            dev->initialize       = overlay_data_context_t::overlay_initialize;
            dev->resizeInput      = overlay_data_context_t::overlay_resizeInput;
            dev->setCrop          = overlay_data_context_t::overlay_setCrop;
            dev->getCrop          = overlay_data_context_t::overlay_getCrop;
            dev->setParameter     = overlay_data_context_t::overlay_data_setParameter;
            dev->dequeueBuffer    = overlay_data_context_t::overlay_dequeueBuffer;
            dev->queueBuffer      = overlay_data_context_t::overlay_queueBuffer;
            dev->getBufferAddress = overlay_data_context_t::overlay_getBufferAddress;
            dev->getBufferCount   = overlay_data_context_t::overlay_getBufferCount;

            *device = &dev->common;
            status = 0;
        }
        else
        {
            // Error: no memory available
            status = - ENOMEM;
        }
    }

    LOG_FUNCTION_NAME_EXIT
    return status;
}
