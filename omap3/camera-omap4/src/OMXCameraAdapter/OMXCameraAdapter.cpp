/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/**
* @file OMXCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to OMX.
*
*/



#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"
#include "TICameraParameters.h"
#include <signal.h>
#include <math.h>
#include <time.h>
#include <hardware_legacy/power.h>
#include <cutils/properties.h>

#include <cutils/properties.h>
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;

#define Q16_OFFSET 16

#define TOUCH_FOCUS_RANGE 0xFF

#define AF_CALLBACK_TIMEOUT 3000000 //3 seconds timeout
// FIXME: focus should not take more than 3sec
// Need to investigate the Ducati timeout logic

#define OMX_CAPTURE_TIMEOUT 3000000 //3 // 3 second timeout

#define OMX_CMD_TIMEOUT         3000000  //3 sec.

#define SENSOR_EEPROM_SIZE 256
#define DCC_HEADER_SIZE 84
#define OTP_BYTE_CAMERA_ID 0x28

typedef enum {
    modInfo_character = 0,
    modInfo_uint16,
    modInfo_uint32,
    modInfo_byte
} moduleInfoType ;

typedef struct {
    const char *name;
    uint16_t offset;
    uint16_t length; //in bytes
    moduleInfoType elemType;

} ModuleQueryFilter ;

static  ModuleQueryFilter moduleQueryFilter[] = {
    { "sensorSerial",         0,    5,  modInfo_byte },
    { "motPartNumber",        6,    8,  modInfo_character },
    { "lensIdentifier",       0xe,  1,  modInfo_character },
    { "manufactureId",        0x0f, 2,  modInfo_character },
    { "factoryId",            0x11, 2,  modInfo_character },
    { "manufactureDate",      0x13, 9,  modInfo_character },
    { "manufactureLine",      0x1C, 2,  modInfo_character },
    { "moduleSerial",         0x1e, 4,  modInfo_uint32    },
    { "autoFocusLiftOff",     0x22, 2,  modInfo_uint16    },
    { "autoFocusMacro",       0x24, 2,  modInfo_uint16    },
    { "shutterCalibration",   0x32, 16, modInfo_byte      },
    { "crc16",                0xF9, 2,  modInfo_uint16    },

};

#define HERE(Msg) {CAMHAL_LOGEB("--===line %d, %s===--\n", __LINE__, Msg);}

namespace android {

#undef LOG_TAG
///Maintain a separate tag for OMXCameraAdapter logs to isolate issues OMX specific
#define LOG_TAG "OMXCameraAdapter"

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

//ISO base for calculating gain in dB
#define ISO100 (100)

///Camera adapter wakelock
static const char kCameraAdapterWakelock[] = "OmxCameraAdapter";
static OMXCameraAdapter *gCameraAdapter = NULL;
static bool gWakeLockAcquired = false;
timer_t gAdapterTimeoutTimer = -1;
Mutex gAdapterLock;

struct timer_list ;


//Signal handler
static void SigHandler(int sig)
{
    Mutex::Autolock lock(gAdapterLock);

    if ( SIGTERM == sig )
        {
        CAMHAL_LOGDA("SIGTERM has been received");
        if ( NULL != gCameraAdapter )
            {
            delete gCameraAdapter;
            gCameraAdapter = NULL;
            }

        if(gWakeLockAcquired)
            {
            //Release the wakelock to allow system to go to suspend
            release_wake_lock(kCameraAdapterWakelock);
            gWakeLockAcquired = false;
            }

        if(gAdapterTimeoutTimer!=-1)
            {
            //Cancel and delete the timeout timer
            timer_delete(gAdapterTimeoutTimer);
            gAdapterTimeoutTimer = -1;
            }

        exit(0);
        }
}

static void adapterTimeout(int sig, siginfo_t *si, void *uc)
{
    Mutex::Autolock lock(gAdapterLock);
    if(SIGALRM == sig)
        {
        //Ignore the signal
        signal(sig, SIG_IGN);
        CAMHAL_LOGDA("adapterTimeout Received");
        if ( NULL != gCameraAdapter )
            {
            delete gCameraAdapter;
            gCameraAdapter = NULL;
            }

        if(gAdapterTimeoutTimer!=-1)
            {
            //Cancel and delete the timeout timer
            timer_delete(gAdapterTimeoutTimer);
            gAdapterTimeoutTimer = -1;
            }

        if(gWakeLockAcquired)
            {
            //Release the wakelock to allow system to go to suspend
            release_wake_lock(kCameraAdapterWakelock);
            gWakeLockAcquired = false;
            }

        }
}


/*--------------------Camera Adapter Class STARTS here-----------------------------*/

const char OMXCameraAdapter::EXIFASCIIPrefix [] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };

const int32_t OMXCameraAdapter::ZOOM_STEPS [ZOOM_STAGES] =  {
                                                                            65536, 68157, 70124, 72745,
                                                                            75366, 77988, 80609, 83231,
                                                                            86508, 89784, 92406, 95683,
                                                                            99615, 102892, 106168, 110100,
                                                                            114033, 117965, 122552, 126484,
                                                                            131072, 135660, 140247, 145490,
                                                                            150733, 155976, 161219, 167117,
                                                                            173015, 178913, 185467, 192020,
                                                                            198574, 205783, 212992, 220201,
                                                                            228065, 236585, 244449, 252969,
                                                                            262144, 271319, 281149, 290980,
                                                                            300810, 311951, 322437, 334234,
                                                                            346030, 357827, 370934, 384041,
                                                                            397148, 411566, 425984, 441057,
                                                                            456131, 472515, 488899, 506593,
                                                                            524288 };

status_t OMXCameraAdapter::initialize(int sensor_index)
{
    LOG_FUNCTION_NAME

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);

    TIMM_OSAL_ERRORTYPE osalError = OMX_ErrorNone;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    mLocalVersionParam.s.nVersionMajor = 0x1;
    mLocalVersionParam.s.nVersionMinor = 0x1;
    mLocalVersionParam.s.nRevision = 0x0 ;
    mLocalVersionParam.s.nStep =  0x0;

    mPending3Asettings = 0;//E3AsettingsAll;
    mEnLensPos = 0;

    mCaptureSem.Create(0);

    ///Event semaphore used for
    ret = mInitSem.Create(0);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Error in creating semaphore %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    if ( mComponentState != OMX_StateExecuting ){
        ///Update the preview and image capture port indexes
        mCameraAdapterParameters.mPrevPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;
        // temp changed in order to build OMX_CAMERA_PORT_VIDEO_OUT_IMAGE;
        mCameraAdapterParameters.mImagePortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;
        mCameraAdapterParameters.mMeasurementPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_MEASUREMENT;
        //currently not supported use preview port instead
        mCameraAdapterParameters.mVideoPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;

        if(!mCameraAdapterParameters.mHandleComp)
            {
            ///Initialize the OMX Core
            eError = OMX_Init();

            if(eError!=OMX_ErrorNone)
                {
                CAMHAL_LOGEB("OMX_Init -0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

            ///Setup key parameters to send to Ducati during init
            OMX_CALLBACKTYPE oCallbacks;

            /* Initialize the callback handles */
            oCallbacks.EventHandler    = android::OMXCameraAdapterEventHandler;
            oCallbacks.EmptyBufferDone = android::OMXCameraAdapterEmptyBufferDone;
            oCallbacks.FillBufferDone  = android::OMXCameraAdapterFillBufferDone;

            ///Get the handle to the OMX Component
            mCameraAdapterParameters.mHandleComp = NULL;
            eError = OMX_GetHandle(&(mCameraAdapterParameters.mHandleComp), //     previously used: OMX_GetHandle
                                        (OMX_STRING)"OMX.TI.DUCATI1.VIDEO.CAMERA" ///@todo Use constant instead of hardcoded name
                                        , this
                                        , &oCallbacks);

            if(eError!=OMX_ErrorNone)
                {
		mHDRInterface = NULL;
                CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

             eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                          OMX_CommandPortDisable,
                                          OMX_ALL,
                                          NULL);

             if(eError!=OMX_ErrorNone)
                 {
                 CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortDisable) -0x%x", eError);
                 }

             GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

             ///Register for port enable event
             ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                         OMX_EventCmdComplete,
                                         OMX_CommandPortEnable,
                                         mCameraAdapterParameters.mPrevPortIndex,
                                         mInitSem);
             if(ret!=NO_ERROR)
                 {
                 CAMHAL_LOGEB("Error in registering for event %d", ret);
                 goto EXIT;
                 }

            ///Enable PREVIEW Port
             eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                         OMX_CommandPortEnable,
                                         mCameraAdapterParameters.mPrevPortIndex,
                                         NULL);

             if(eError!=OMX_ErrorNone)
                 {
                 CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
                 }

             GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

             //Wait for the port enable event to occur
             ret = mInitSem.WaitTimeout(OMX_CMD_TIMEOUT);
             if ( NO_ERROR == ret )
                 {
                 CAMHAL_LOGDA("-Port enable event arrived");
                 }
             else
                 {
                 CAMHAL_LOGEA("Timeout for enabling preview port expired!");
                 goto EXIT;
                 }
            }
        else
            {
            OMXCameraPortParameters * mPreviewData =
                &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

            //Apply default configs before trying to swtich to a new sensor
            if ( NO_ERROR != setFormat(OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, *mPreviewData) )
                {
                CAMHAL_LOGEB("Error 0x%x while applying defaults", ret);
                goto EXIT;
                }
            }
    }
    ///Select the sensor
    OMX_CONFIG_SENSORSELECTTYPE sensorSelect;
    OMX_INIT_STRUCT_PTR (&sensorSelect, OMX_CONFIG_SENSORSELECTTYPE);
    sensorSelect.eSensor = (OMX_SENSORSELECT)sensor_index;
    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexConfigSensorSelect, &sensorSelect);

    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while selecting the sensor index as %d - 0x%x", sensor_index, eError);
        ret = -1;
        }
    else
        {
        CAMHAL_LOGEB("Sensor %d selected successfully", sensor_index);
        }
    ///Disable pre-announcement
    OMX_TI_PARAM_BUFFERPREANNOUNCE preAnnounce;
    OMX_INIT_STRUCT_PTR (&preAnnounce, OMX_TI_PARAM_BUFFERPREANNOUNCE);
    preAnnounce.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    preAnnounce.bEnabled = OMX_FALSE;

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE)OMX_TI_IndexParamBufferPreAnnouncement,&preAnnounce);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while setting pre-announcement 0x%x", eError);
        ret = -1;
        }
    else
        {
        CAMHAL_LOGEA("Preannouncement disabled successfully");
        }

    printComponentVersion(mCameraAdapterParameters.mHandleComp);

    mSensorIndex = sensor_index;
    mBracketingEnabled = false;
    mBracketingBuffersQueuedCount = 0;
    mBracketingRange = 1;
    mLastBracetingBufferIdx = 0;

    if ( mComponentState != OMX_StateExecuting ){
        mPreviewing = false;
        mPreviewInProgress = false;
        mCapturing = false;
        mCaptureSignalled = false;
        mCaptureConfigured = false;
        mRecording = false;
        mFlushBuffers = false;
        mWaitingForSnapshot = false;
        mSnapshotCount = 0;
        mComponentState = OMX_StateLoaded;

        mCapMode = OMXCameraAdapter::HIGH_QUALITY_NONZSL;
        mBurstFrames = 1;
        mCapturedFrames = 0;
        mPictureQuality = 100;
        mCurrentZoomIdx = 0;
        mTargetZoomIdx = 0;
        mReturnZoomStatus = false;
        mSmoothZoomEnabled = false;
        mZoomInc = 1;
        mZoomParameterIdx = 0;
        mExposureBracketingValidEntries = 0;
        mFaceDetectionThreshold = FACE_THRESHOLD_DEFAULT;

        mEXIFData.mGPSData.mAltitudeValid = false;
        mEXIFData.mGPSData.mDatestampValid = false;
        mEXIFData.mGPSData.mLatValid = false;
        mEXIFData.mGPSData.mLongValid = false;
        mEXIFData.mGPSData.mMapDatumValid = false;
        mEXIFData.mGPSData.mProcMethodValid = false;
        mEXIFData.mGPSData.mVersionIdValid = false;
        mEXIFData.mGPSData.mTimeStampValid = false;
        mEXIFData.mModelValid = false;
        mEXIFData.mMakeValid = false;

        mHDRCaptureEnabled = false;
        mHDRInterface = NULL;

        // initialize command handling thread
        if(mCommandHandler.get() == NULL)
            mCommandHandler = new CommandHandler(this);

        if ( NULL == mCommandHandler.get() )
        {
            CAMHAL_LOGEA("Couldn't create command handler");
            return NO_MEMORY;
        }

        ret = mCommandHandler->run("CallbackThread", PRIORITY_URGENT_DISPLAY);
        if ( ret != NO_ERROR )
        {
            if( ret == INVALID_OPERATION){
                CAMHAL_LOGDA("command handler thread already runnning!!");
            }else
            {
                CAMHAL_LOGEA("Couldn't run command handlerthread");
                return ret;
            }
        }

        //Remove any unhandled events
        if ( !mEventSignalQ.isEmpty() )
            {
            for (unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
                {
                Message *msg = mEventSignalQ.itemAt(i);
                //remove from queue and free msg
                mEventSignalQ.removeAt(i);
                if ( NULL != msg )
                    {
                    free(msg);
                    }
                }
            }

        //Setting this flag will that the first setParameter call will apply all 3A settings
        //and will not conditionally apply based on current values.
        mFirstTimeInit = true;

        memset(mExposureBracketingValues, 0, EXP_BRACKET_RANGE*sizeof(int));
        mTouchFocusPosX = 0;
        mTouchFocusPosY = 0;
        mMeasurementEnabled = false;
        mFaceDetectionRunning = false;

        if ( NULL != mFaceDectionResult )
            {
            memset(mFaceDectionResult, '\0', FACE_DETECTION_BUFFER_SIZE);
            }

        memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex], 0, sizeof(OMXCameraPortParameters));
        memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex], 0, sizeof(OMXCameraPortParameters));
    }
    return ErrorUtils::omxToAndroidError(eError);

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);

    if(mCameraAdapterParameters.mHandleComp)
        {
        ///Free the OMX component handle in case of error
        OMX_FreeHandle(mCameraAdapterParameters.mHandleComp);
        }

    ///De-init the OMX
    OMX_Deinit();

    LOG_FUNCTION_NAME_EXIT

    return ErrorUtils::omxToAndroidError(eError);
}

OMXCameraAdapter::OMXCameraPortParameters *OMXCameraAdapter::getPortParams(CameraFrame::FrameType frameType)
{
    OMXCameraAdapter::OMXCameraPortParameters *ret = NULL;

    switch ( frameType )
    {
    case CameraFrame::IMAGE_FRAME:
    case CameraFrame::RAW_FRAME:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        break;
    case CameraFrame::PREVIEW_FRAME_SYNC:
    case CameraFrame::SNAPSHOT_FRAME:
    case CameraFrame::VIDEO_FRAME_SYNC:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        break;
    case CameraFrame::FRAME_DATA_SYNC:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];
        break;
    default:
        break;
    };

    return ret;
}

status_t OMXCameraAdapter::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *port = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ( mComponentState != OMX_StateExecuting )
        {
        ret = -EINVAL;
        }

    if ( NULL == frameBuf )
        {
        ret = -EINVAL;
        }

    if ( ( NO_ERROR == ret ) && ( ( CameraFrame::IMAGE_FRAME == frameType ) ||
         ( CameraFrame::RAW_FRAME == frameType ) ) )
        {
        if ( ( 1 > mCapturedFrames ) && ( !mBracketingEnabled ) )
            {
            stopImageCapture();
            return NO_ERROR;
            }
        }

    if ( NO_ERROR == ret )
        {
        port = getPortParams(frameType);
        if ( NULL == port )
            {
            CAMHAL_LOGEB("Invalid frameType 0x%x", frameType);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {

        for ( int i = 0 ; i < port->mNumBufs ; i++)
            {
            if ( port->mBufferHeader[i]->pBuffer == frameBuf )
                {
                if(1)
                    {
                    port->mBufferHeader[i]->nFlags |= OMX_BUFFERHEADERFLAG_MODIFIED;
                    }
                eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp, port->mBufferHeader[i]);
                if ( eError != OMX_ErrorNone )
                    {
                    CAMHAL_LOGDB("OMX_FillThisBuffer 0x%x", eError);
                    ret = -1;
                    }
                break;
                }
            }

        }

    return ret;
}

status_t OMXCameraAdapter::setParameters(const CameraParameters &params)
{
    LOG_FUNCTION_NAME

    const char * str = NULL;
    int mode = 0;
    status_t ret = NO_ERROR;
    bool updateImagePortParams = false;
    int minFramerate, maxFramerate;
    const char *valstr = NULL;
    const char *oldstr = NULL;

   ///@todo Include more camera parameters
    int w, h;
    OMX_COLOR_FORMATTYPE pixFormat;
    if ( (valstr = params.getPreviewFormat()) != NULL )
        {
        if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
            {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
            {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
            {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            }
        else
            {
            CAMHAL_LOGDA("Invalid format, CbYCrY format selected as default");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            }
        }
    else
        {
        CAMHAL_LOGEA("Preview format is NULL, defaulting to CbYCrY");
        pixFormat = OMX_COLOR_FormatCbYCrY;
        }

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    params.getPreviewSize(&w, &h);

    /// OMX Camera Adapter uses only MIN and MAX frame rates passed from CameraHAL
    /// In case of variable frame rate MIN < MAX
    /// In case of constant frame rate MIN = MAX
    minFramerate = params.getInt(TICameraParameters::KEY_MINFRAMERATE);
    maxFramerate = params.getInt(TICameraParameters::KEY_MAXFRAMERATE);
    if ( ( 0 < minFramerate ) &&
         ( 0 < maxFramerate ) )
        {
        if ( minFramerate > maxFramerate )
            {
             CAMHAL_LOGEA(" Min FPS set higher than MAX. So setting MIN and MAX to the higher value");
             maxFramerate = minFramerate;
            }

        if( ( cap->mMinFrameRate != minFramerate ) ||
            ( cap->mMaxFrameRate != maxFramerate ) )
            {
            cap->mMinFrameRate = minFramerate;
            cap->mMaxFrameRate = maxFramerate;

            // We support variable frame rate configuration on the fly through this.
            if(mPreviewing)
                {
                setVFramerate(cap->mMinFrameRate, cap->mMaxFrameRate);
                }
            }

        cap->mColorFormat = pixFormat;
        cap->mWidth = w;
        cap->mHeight = h;
        cap->mFrameRate = maxFramerate;

        CAMHAL_LOGVB("Prev: cap.mColorFormat = %d", (int)cap->mColorFormat);
        CAMHAL_LOGVB("Prev: cap.mWidth = %d", (int)cap->mWidth);
        CAMHAL_LOGVB("Prev: cap.mHeight = %d", (int)cap->mHeight);
        CAMHAL_LOGVB("Prev: cap.mFrameRate = %d", (int)cap->mFrameRate);

        //TODO: Add an additional parameter for video resolution
       //use preview resolution for now
        cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        cap->mColorFormat = pixFormat;
        cap->mWidth = w;
        cap->mHeight = h;
        cap->mFrameRate = maxFramerate;

        CAMHAL_LOGVB("Video: cap.mColorFormat = %d", (int)cap->mColorFormat);
        CAMHAL_LOGVB("Video: cap.mWidth = %d", (int)cap->mWidth);
        CAMHAL_LOGVB("Video: cap.mHeight = %d", (int)cap->mHeight);
        CAMHAL_LOGVB("Video: cap.mFrameRate = %d", (int)cap->mFrameRate);

        ///mStride is set from setBufs() while passing the APIs
        cap->mStride = 4096;
        cap->mBufSize = cap->mStride * cap->mHeight;
        }

    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    params.getPictureSize(&w, &h);

    if ( ( w != ( int ) cap->mWidth ) ||
          ( h != ( int ) cap->mHeight ) )
        {
        updateImagePortParams = true;
        }

    cap->mWidth = w;
    cap->mHeight = h;
    //TODO: Support more pixelformats
    cap->mStride = 2;

    CAMHAL_LOGVB("Image: cap.mWidth = %d", (int)cap->mWidth);
    CAMHAL_LOGVB("Image: cap.mHeight = %d", (int)cap->mHeight);

    if ( (valstr = params.getPictureFormat()) != NULL )
        {
        if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
            {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
            {
            CAMHAL_LOGDA("YUV420SP format selected");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
            {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_JPEG) == 0)
            {
            CAMHAL_LOGDA("JPEG format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingNone;
            }
        else if(strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_JPS) == 0)
            {
            CAMHAL_LOGDA("JPS format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingJPS;
            }
        else if(strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_MPO) == 0)
            {
            CAMHAL_LOGDA("MPO format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingMPO;
            }
        else if(strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_RAW_JPEG) == 0)
            {
            CAMHAL_LOGDA("RAW + JPEG format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingRAWJPEG;
            }
        else if(strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_RAW_MPO) == 0)
            {
            CAMHAL_LOGDA("RAW + MPO format selected");
            pixFormat = OMX_COLOR_FormatUnused;
            mCodingMode = CodingRAWMPO;
            }
        else if(strcmp(valstr, (const char *) TICameraParameters::PIXEL_FORMAT_RAW) == 0)
            {
            CAMHAL_LOGDA("RAW Picture format selected");
            pixFormat = OMX_COLOR_FormatRawBayer10bit;
            }
        else
            {
            CAMHAL_LOGEA("Invalid format, JPEG format selected as default");
            pixFormat = OMX_COLOR_FormatUnused;
            }
        }
    else
        {
        CAMHAL_LOGEA("Picture format is NULL, defaulting to JPEG");
        pixFormat = OMX_COLOR_FormatUnused;
        }

    if ( pixFormat != cap->mColorFormat )
        {
        updateImagePortParams = true;
        cap->mColorFormat = pixFormat;
        }

    if ( updateImagePortParams )
        {
        Mutex::Autolock lock(mLock);
        if ( !mCapturing )
            {
            setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *cap);
            }
        }

    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    setFormat(OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, *cap);

    str = params.get(TICameraParameters::KEY_EXPOSURE_MODE);
    mode = getLUTvalue_HALtoOMX( str, ExpLUT);
    if ( ( str != NULL ) && ( mParameters3A.Exposure != mode ) )
        {
        mParameters3A.Exposure = mode;
        CAMHAL_LOGEB("Exposure mode %d", mode);
        if ( 0 <= mParameters3A.Exposure )
            {
            mPending3Asettings |= SetExpMode;
            }
        }

    str = params.get(CameraParameters::KEY_WHITE_BALANCE);
    mode = getLUTvalue_HALtoOMX( str, WBalLUT);
    if ( ( mFirstTimeInit || ( str != NULL ) ) && ( mode != mParameters3A.WhiteBallance ) )
        {
        mParameters3A.WhiteBallance = mode;
        CAMHAL_LOGEB("Whitebalance mode %d", mode);
        if ( 0 <= mParameters3A.WhiteBallance )
            {
            mPending3Asettings |= SetWhiteBallance;
            }
        }

    if ( 0 <= params.getInt(TICameraParameters::KEY_CONTRAST) )
        {
        if ( mFirstTimeInit || ( (mParameters3A.Contrast  + CONTRAST_OFFSET) != params.getInt(TICameraParameters::KEY_CONTRAST)) )
            {
            mParameters3A.Contrast = params.getInt(TICameraParameters::KEY_CONTRAST) - CONTRAST_OFFSET;
            CAMHAL_LOGEB("Contrast %d", mParameters3A.Contrast);
            mPending3Asettings |= SetContrast;
            }
        }

    if ( 0 <= params.getInt(TICameraParameters::KEY_SHARPNESS) )
        {
        if ( mFirstTimeInit || ((mParameters3A.Sharpness + SHARPNESS_OFFSET) != params.getInt(TICameraParameters::KEY_SHARPNESS)))
            {
            mParameters3A.Sharpness = params.getInt(TICameraParameters::KEY_SHARPNESS) - SHARPNESS_OFFSET;
            CAMHAL_LOGEB("Sharpness %d", mParameters3A.Sharpness);
            mPending3Asettings |= SetSharpness;
            }
        }

    if ( 0 <= params.getInt(TICameraParameters::KEY_SATURATION) )
        {
        if ( mFirstTimeInit || ((mParameters3A.Saturation + SATURATION_OFFSET) != params.getInt(TICameraParameters::KEY_SATURATION)) )
            {
            mParameters3A.Saturation = params.getInt(TICameraParameters::KEY_SATURATION) - SATURATION_OFFSET;
            CAMHAL_LOGEB("Saturation %d", mParameters3A.Saturation);
            mPending3Asettings |= SetSaturation;
            }
        }

    if ( 0 <= params.getInt(TICameraParameters::KEY_BRIGHTNESS) )
        {
        if ( mFirstTimeInit || (( mParameters3A.Brightness !=  ( unsigned int ) params.getInt(TICameraParameters::KEY_BRIGHTNESS))) )
            {
            mParameters3A.Brightness = (unsigned)params.getInt(TICameraParameters::KEY_BRIGHTNESS);
            CAMHAL_LOGEB("Brightness %d", mParameters3A.Brightness);
            mPending3Asettings |= SetBrightness;
            }
        }

    str = params.get(CameraParameters::KEY_ANTIBANDING);
    mode = getLUTvalue_HALtoOMX(str,FlickerLUT);
    if ( mFirstTimeInit || ( ( str != NULL ) && ( mParameters3A.Flicker != mode ) ))
        {
        mParameters3A.Flicker = mode;
        CAMHAL_LOGEB("Flicker %d", mParameters3A.Flicker);
        if ( 0 <= mParameters3A.Flicker )
            {
            mPending3Asettings |= SetFlicker;
            }
        }

    str = params.get(TICameraParameters::KEY_ISO);
    mode = getLUTvalue_HALtoOMX(str, IsoLUT);
    CAMHAL_LOGEB("ISO mode arrived in HAL : %s", str);
    if ( mFirstTimeInit || (  ( str != NULL ) && ( mParameters3A.ISO != mode )) )
        {
        mParameters3A.ISO = mode;
        CAMHAL_LOGEB("ISO %d", mParameters3A.ISO);
        if ( 0 <= mParameters3A.ISO )
            {
            mPending3Asettings |= SetISO;
            }
        }

    str = params.get(CameraParameters::KEY_FOCUS_MODE);
    mode = getLUTvalue_HALtoOMX(str, FocusLUT);
    if ( mFirstTimeInit || ( ( str != NULL ) && ( mParameters3A.Focus != mode ) ) )
        {
        // Apply focus mode immediately only if  CAF  or Inifinity are selected
        if ( ( mode == OMX_IMAGE_FocusControlAuto ) ||
             ( mode == OMX_IMAGE_FocusControlAutoInfinity ) )
            {
            mPending3Asettings |= SetFocus;
            mParameters3A.Focus = mode;
            }
        else if ( mParameters3A.Focus == OMX_IMAGE_FocusControlAuto )
            {
            //If we switch from CAF to something else, then disable CAF
            mPending3Asettings |= SetFocus;
            mParameters3A.Focus = OMX_IMAGE_FocusControlOff;
            }

        mParameters3A.Focus = mode;
        CAMHAL_LOGEB("Focus %x", mParameters3A.Focus);
        }

    str = params.get(TICameraParameters::KEY_TOUCH_FOCUS_POS);
    if ( NULL != str ) {
        strncpy(mTouchCoords, str, TOUCH_DATA_SIZE-1);
        parseTouchFocusPosition(mTouchCoords, mTouchFocusPosX, mTouchFocusPosY);
        CAMHAL_LOGEB("Touch focus position %d,%d", mTouchFocusPosX, mTouchFocusPosY);
    }

    if( (str = params.get(TICameraParameters::KEY_MOT_HDR_MODE)) != NULL )
        {
        if ( strcmp(str, TICameraParameters::MOT_HDR_MODE_ON) == 0 )
            {
            mHDRCaptureEnabled = true;
            CAMHAL_LOGDB(" mExposureBracketingValidEntries = %d\n", mExposureBracketingValidEntries);
            if(mHDRInterface == NULL)
                {
                mHDRInterface = new MotHDRInterface(this);
                mHDRInterface->init();
                }
            }
        else if ( strcmp(str, TICameraParameters::MOT_HDR_MODE_OFF) == 0 )
            {
            mHDRCaptureEnabled = false;
            if(mHDRInterface != NULL)
                {
                CAMHAL_LOGDA("Destroy HDR Interface\n");
                delete mHDRInterface;
                mHDRInterface = NULL;
                }
            }
        else
            {
            CAMHAL_LOGDA("Invalid HDR mode\n");
            }
        }

    str = params.get(TICameraParameters::KEY_EXP_BRACKETING_RANGE);
    if ( NULL != str ) {
        parseExpRange(str, mExposureBracketingValues, EXP_BRACKET_RANGE, mExposureBracketingValidEntries);
    } else {
        mExposureBracketingValidEntries = 0;
    }

    str = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    if ( mFirstTimeInit || (( str != NULL ) && (mParameters3A.EVCompensation != params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION))))
        {
        CAMHAL_LOGEB("Setting EV Compensation to %d", params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION));

        mParameters3A.EVCompensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
        mPending3Asettings |= SetEVCompensation;
        }

    str = params.get(CameraParameters::KEY_SCENE_MODE);
    mode = getLUTvalue_HALtoOMX( str, SceneLUT);
    if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.SceneMode != mode )) )
        {
        if ( 0 <= mode )
            {
            mParameters3A.SceneMode = mode;
            mPending3Asettings |= SetSceneMode;
            }
        else
            {
            mParameters3A.SceneMode = OMX_Manual;
            }

        CAMHAL_LOGEB("SceneMode %d", mParameters3A.SceneMode);
        }

    str = params.get(CameraParameters::KEY_FLASH_MODE);
    mode = getLUTvalue_HALtoOMX( str, FlashLUT);
    if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.FlashMode != mode )) )
        {
        if ( 0 <= mode )
            {
            mParameters3A.FlashMode = mode;
            mPending3Asettings |= SetFlash;
            }
        else
            {
            mParameters3A.FlashMode = OMX_Manual;
            }
        }

    LOGE("Flash Setting %s", str);
    LOGE("FlashMode %d", mParameters3A.FlashMode);

    str = params.get(CameraParameters::KEY_EFFECT);
    mode = getLUTvalue_HALtoOMX( str, EffLUT);
    if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.Effect != mode )) )
        {
        mParameters3A.Effect = mode;
        CAMHAL_LOGEB("Effect %d", mParameters3A.Effect);
        if ( 0 <= mParameters3A.Effect )
            {
            mPending3Asettings |= SetEffect;
            }
        }

    if ( params.getInt(CameraParameters::KEY_ROTATION) != -1 )
        {
        mPictureRotation = params.getInt(CameraParameters::KEY_ROTATION);
        }
    else
        {
        mPictureRotation = 0;
        }

    CAMHAL_LOGVB("Picture Rotation set %d", mPictureRotation);

    if ( (valstr = params.get(TICameraParameters::KEY_CAP_MODE)) != NULL )
        {
        if (strcmp(valstr, (const char *) TICameraParameters::HIGH_PERFORMANCE_MODE) == 0)
            {
            mCapMode = OMXCameraAdapter::HIGH_SPEED;
            }
        else if (strcmp(valstr, (const char *) TICameraParameters::HIGH_QUALITY_MODE) == 0)
            {
            mCapMode = OMXCameraAdapter::HIGH_QUALITY;
            }
        else if (strcmp(valstr, (const char *) TICameraParameters::VIDEO_MODE) == 0)
            {
            mCapMode = OMXCameraAdapter::VIDEO_MODE;
            }
        else if (strcmp(valstr, (const char *) TICameraParameters::HIGH_QUALITY_NONZSL_MODE) == 0)
            {
            mCapMode = OMXCameraAdapter::HIGH_QUALITY_NONZSL;
            }
        else
            {
            mCapMode = OMXCameraAdapter::HIGH_QUALITY_NONZSL;
            }
        }
    else
        {
        mCapMode = OMXCameraAdapter::HIGH_QUALITY_NONZSL;
        }

    //use HIGH_SPEED mode for BURST capture usecase
    if( (params.get(TICameraParameters::KEY_BURST) != NULL ))
        {
        if (params.getInt(TICameraParameters::KEY_BURST) >= 1 )
            {
            mCapMode = OMXCameraAdapter::HIGH_SPEED;
            }
       }

    LOGE("Capture Mode is %d", mCapMode);

    /// Configure IPP, LDCNSF, GBCE and GLBCE only in HQ mode
    if((mCapMode == OMXCameraAdapter::HIGH_QUALITY) ||
       (mCapMode == OMXCameraAdapter::HIGH_QUALITY_NOTSET) ||
       (mCapMode == OMXCameraAdapter::HIGH_QUALITY_NONZSL))
        {
          if ( (valstr = params.get(TICameraParameters::KEY_IPP)) != NULL )
            {
            if (strcmp(valstr, (const char *) TICameraParameters::IPP_LDCNSF) == 0)
                {
                mIPP = OMXCameraAdapter::IPP_LDCNSF;
                }
            else if (strcmp(valstr, (const char *) TICameraParameters::IPP_LDC) == 0)
                {
                mIPP = OMXCameraAdapter::IPP_LDC;
                }
            else if (strcmp(valstr, (const char *) TICameraParameters::IPP_NSF) == 0)
                {
                mIPP = OMXCameraAdapter::IPP_NSF;
                }
            else if (strcmp(valstr, (const char *) TICameraParameters::IPP_NONE) == 0)
                {
                mIPP = OMXCameraAdapter::IPP_NONE;
                }
            else
                {
                mIPP = OMXCameraAdapter::IPP_NONE;
                }
            }
        else
            {
            mIPP = OMXCameraAdapter::IPP_NONE;
            }

        CAMHAL_LOGEB("IPP Mode set %d", mIPP);

        if (((valstr = params.get(TICameraParameters::KEY_GBCE)) != NULL) )
            {
            // Configure GBCE only if the setting has changed since last time
            oldstr = mParams.get(TICameraParameters::KEY_GBCE);
            bool cmpRes = true;
            if ( NULL != oldstr )
                {
                cmpRes = strcmp(valstr, oldstr) != 0;
                }
            else
                {
                cmpRes = true;
                }


            if( cmpRes )
                {
                if (strcmp(valstr, ( const char * ) TICameraParameters::GBCE_ENABLE ) == 0)
                    {
                    setGBCE(OMXCameraAdapter::BRIGHTNESS_ON);
                    }
                else if (strcmp(valstr, ( const char * ) TICameraParameters::GBCE_DISABLE ) == 0)
                    {
                    setGBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
                    }
                else
                    {
                    setGBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
                    }
                }
            }
        else
            {
            //Disable GBCE by default
            setGBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
            }

        if ( ( valstr = params.get(TICameraParameters::KEY_GLBCE) ) != NULL )
            {
            // Configure GLBCE only if the setting has changed since last time

            oldstr = mParams.get(TICameraParameters::KEY_GLBCE);
            bool cmpRes = true;
            if ( NULL != oldstr )
                {
                cmpRes = strcmp(valstr, oldstr) != 0;
                }
            else
                {
                cmpRes = true;
                }


            if( cmpRes )
                {
                if (strcmp(valstr, ( const char * ) TICameraParameters::GLBCE_ENABLE ) == 0)
                    {
                    setGLBCE(OMXCameraAdapter::BRIGHTNESS_ON);
                    }
                else if (strcmp(valstr, ( const char * ) TICameraParameters::GLBCE_DISABLE ) == 0)
                    {
                    setGLBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
                    }
                else
                    {
                    setGLBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
                    }
                }
            }
        else
            {
            //Disable GLBCE by default
            setGLBCE(OMXCameraAdapter::BRIGHTNESS_OFF);
            }
        }
    else
        {
        mIPP = OMXCameraAdapter::IPP_NONE;
        }


    if ( params.getInt(TICameraParameters::KEY_BURST)  >= 1 )
        {
        mBurstFrames = params.getInt(TICameraParameters::KEY_BURST);
        }
    else
        {
        mBurstFrames = 1;
        }

    CAMHAL_LOGVB("Burst Frames set %d", mBurstFrames);

    if ( ((valstr = params.get(TICameraParameters::KEY_FACE_DETECTION_ENABLE)) != NULL) )
     {
      // Configure FD only if the setting has changed since last time
      oldstr = mParams.get(TICameraParameters::KEY_FACE_DETECTION_ENABLE);
      bool cmpRes = true;
      if ( NULL != oldstr )
           {
           cmpRes = strcmp(valstr, oldstr) != 0;
           }
      else
           {
           cmpRes = true;
           }
      if ( cmpRes )
           {
      if (strcmp(valstr, (const char *) TICameraParameters::FACE_DETECTION_ENABLE) == 0)
           {
           setFaceDetection(true);
           }
      else if (strcmp(valstr, (const char *) TICameraParameters::FACE_DETECTION_DISABLE) == 0)
           {
           setFaceDetection(false);
           }
      else
           {
           setFaceDetection(false);
           }
       }
   }

    if ( 0 <= params.getInt(TICameraParameters::KEY_FACE_DETECTION_THRESHOLD ) )
        {
        mFaceDetectionThreshold = params.getInt(TICameraParameters::KEY_FACE_DETECTION_THRESHOLD);
        }
    else
        {
        mFaceDetectionThreshold = FACE_THRESHOLD_DEFAULT;
        }

    if ( (valstr = params.get(TICameraParameters::KEY_MEASUREMENT_ENABLE)) != NULL )
        {
        if (strcmp(valstr, (const char *) TICameraParameters::MEASUREMENT_ENABLE) == 0)
            {
            mMeasurementEnabled = true;
            }
        else if (strcmp(valstr, (const char *) TICameraParameters::MEASUREMENT_DISABLE) == 0)
            {
            mMeasurementEnabled = false;
            }
        else
            {
            mMeasurementEnabled = false;
            }
        }
    else
        {
        //Disable measurement data by default
        mMeasurementEnabled = false;
        }

    if ( ( params.getInt(CameraParameters::KEY_JPEG_QUALITY)  >= MIN_JPEG_QUALITY ) &&
         ( params.getInt(CameraParameters::KEY_JPEG_QUALITY)  <= MAX_JPEG_QUALITY ) )
        {
        mPictureQuality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
        }
    else
        {
        mPictureQuality = MAX_JPEG_QUALITY;
        }

    CAMHAL_LOGVB("Picture Quality set %d", mPictureQuality);

    if ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH)  >= 0 )
        {
        mThumbWidth = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
        }
    else
        {
        mThumbWidth = DEFAULT_THUMB_WIDTH;
        }


    CAMHAL_LOGVB("Picture Thumb width set %d", mThumbWidth);

    if ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT)  >= 0 )
        {
        mThumbHeight = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
        }
    else
        {
        mThumbHeight = DEFAULT_THUMB_HEIGHT;
        }


    CAMHAL_LOGVB("Picture Thumb height set %d", mThumbHeight);


    ///Set VNF Configuration
    if ( params.getInt(TICameraParameters::KEY_VNF)  > 0 )
        {
        CAMHAL_LOGDA("VNF Enabled");
        mVnfEnabled = true;
        }
    else
        {
        CAMHAL_LOGDA("VNF Disabled");
        mVnfEnabled = false;
        }

    ///Set VSTAB Configuration
    if ( params.getInt(TICameraParameters::KEY_VSTAB)  > 0 )
        {
        CAMHAL_LOGDA("VSTAB Enabled");
        mVstabEnabled = true;
        }
    else
        {
        CAMHAL_LOGDA("VSTAB Disabled");
        mVstabEnabled = false;
        }

	 //Set Auto Convergence Mode
    str = params.get((const char *) TICameraParameters::KEY_AUTOCONVERGENCE);
    if ( str != NULL )
        {
        // Set ManualConvergence default value
        OMX_S32 manualconvergence = -30;
        if ( strcmp (str, (const char *) TICameraParameters::AUTOCONVERGENCE_MODE_DISABLE) == 0 )
            {
            setAutoConvergence(OMX_TI_AutoConvergenceModeDisable, manualconvergence);
            }
        else if ( strcmp (str, (const char *) TICameraParameters::AUTOCONVERGENCE_MODE_FRAME) == 0 )
                {
                setAutoConvergence(OMX_TI_AutoConvergenceModeFrame, manualconvergence);
                }
        else if ( strcmp (str, (const char *) TICameraParameters::AUTOCONVERGENCE_MODE_CENTER) == 0 )
                {
                setAutoConvergence(OMX_TI_AutoConvergenceModeCenter, manualconvergence);
                }
        else if ( strcmp (str, (const char *) TICameraParameters::AUTOCONVERGENCE_MODE_FFT) == 0 )
                {
                setAutoConvergence(OMX_TI_AutoConvergenceModeFocusFaceTouch, manualconvergence);
                }
        else if ( strcmp (str, (const char *) TICameraParameters::AUTOCONVERGENCE_MODE_MANUAL) == 0 )
                {
                manualconvergence = (OMX_S32)params.getInt(TICameraParameters::KEY_MANUALCONVERGENCE_VALUES);
                setAutoConvergence(OMX_TI_AutoConvergenceModeManual, manualconvergence);
                }
        CAMHAL_LOGEB("AutoConvergenceMode %s, value = %d", str, (int) manualconvergence);
        }

    if ( ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)  >= MIN_JPEG_QUALITY ) &&
         ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)  <= MAX_JPEG_QUALITY ) )
        {
        mThumbQuality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
        }
    else
        {
        mThumbQuality = MAX_JPEG_QUALITY;
        }

    CAMHAL_LOGDB("Thumbnail Quality set %d", mThumbQuality);

        {
        Mutex::Autolock lock(mZoomLock);

        //Immediate zoom should not be avaialable while smooth zoom is running
        if ( !mSmoothZoomEnabled )
            {
            int zoom = params.getInt(CameraParameters::KEY_ZOOM);
            if( (zoom >= 0) && ( zoom < ZOOM_STAGES) ){
                mTargetZoomIdx = zoom;
            } else {
                mTargetZoomIdx = 0;
            }
            //Immediate zoom should be applied instantly ( CTS requirement )
            mCurrentZoomIdx = mTargetZoomIdx;
            doZoom(mCurrentZoomIdx);

            CAMHAL_LOGDB("Zoom by App %d", zoom);
            }
        }

    double gpsPos;

    if( ( valstr = params.get(CameraParameters::KEY_GPS_LATITUDE) ) != NULL )
        {
        gpsPos = strtod(valstr, NULL);

        if ( convertGPSCoord( gpsPos, &mEXIFData.mGPSData.mLatDeg,
                                        &mEXIFData.mGPSData.mLatMin,
                                        &mEXIFData.mGPSData.mLatSec ) == NO_ERROR )
            {

            if ( 0 < gpsPos )
                {
                strncpy(mEXIFData.mGPSData.mLatRef, GPS_NORTH_REF, GPS_REF_SIZE);
                }
            else
                {
                strncpy(mEXIFData.mGPSData.mLatRef, GPS_SOUTH_REF, GPS_REF_SIZE);
                }


            mEXIFData.mGPSData.mLatValid = true;
            }
        else
            {
            mEXIFData.mGPSData.mLatValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mLatValid = false;
        }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_LONGITUDE) ) != NULL )
        {
        gpsPos = strtod(valstr, NULL);

        if ( convertGPSCoord( gpsPos, &mEXIFData.mGPSData.mLongDeg,
                                        &mEXIFData.mGPSData.mLongMin,
                                        &mEXIFData.mGPSData.mLongSec ) == NO_ERROR )
            {

            if ( 0 < gpsPos )
                {
                strncpy(mEXIFData.mGPSData.mLongRef, GPS_EAST_REF, GPS_REF_SIZE);
                }
            else
                {
                strncpy(mEXIFData.mGPSData.mLongRef, GPS_WEST_REF, GPS_REF_SIZE);
                }

            mEXIFData.mGPSData.mLongValid= true;
            }
        else
            {
            mEXIFData.mGPSData.mLongValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mLongValid = false;
        }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_ALTITUDE) ) != NULL )
        {
        gpsPos = strtod(valstr, NULL);
        mEXIFData.mGPSData.mAltitude = gpsPos;
        mEXIFData.mGPSData.mAltitudeValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mAltitudeValid= false;
        }

    if( (valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
        {
        long gpsTimestamp = strtol(valstr, NULL, 10);
        struct tm *timeinfo = localtime( ( time_t * ) & (gpsTimestamp) );
        if ( NULL != timeinfo )
            {
            mEXIFData.mGPSData.mTimeStampHour = timeinfo->tm_hour;
            mEXIFData.mGPSData.mTimeStampMin = timeinfo->tm_min;
            mEXIFData.mGPSData.mTimeStampSec = timeinfo->tm_sec;
            mEXIFData.mGPSData.mTimeStampValid = true;
            }
        else
            {
            mEXIFData.mGPSData.mTimeStampValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mTimeStampValid = false;
        }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP) ) != NULL )
        {
        long gpsDatestamp = strtol(valstr, NULL, 10);
        struct tm *timeinfo = localtime( ( time_t * ) & (gpsDatestamp) );
        if ( NULL != timeinfo )
            {
            strftime(mEXIFData.mGPSData.mDatestamp, GPS_DATESTAMP_SIZE, "%Y:%m:%d", timeinfo);
            mEXIFData.mGPSData.mDatestampValid = true;
            }
        else
            {
            mEXIFData.mGPSData.mDatestampValid = false;
            }
        }
    else
        {
        mEXIFData.mGPSData.mDatestampValid = false;
        }

    if( ( valstr = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mProcMethod, valstr, GPS_PROCESSING_SIZE-1);
        mEXIFData.mGPSData.mProcMethodValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mProcMethodValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_GPS_ALTITUDE_REF) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mAltitudeRef, valstr, GPS_REF_SIZE-1);
        mEXIFData.mGPSData.mAltitudeValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mAltitudeValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_GPS_MAPDATUM) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mMapDatum, valstr, GPS_MAPDATUM_SIZE-1);
        mEXIFData.mGPSData.mMapDatumValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mMapDatumValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_GPS_VERSION) ) != NULL )
        {
        strncpy(mEXIFData.mGPSData.mVersionId, valstr, GPS_VERSION_SIZE-1);
        mEXIFData.mGPSData.mVersionIdValid = true;
        }
    else
        {
        mEXIFData.mGPSData.mVersionIdValid = false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_EXIF_MODEL ) ) != NULL )
        {
        CAMHAL_LOGEB("EXIF Model: %s", valstr);
        mEXIFData.mModelValid= true;
        }
    else
        {
        mEXIFData.mModelValid= false;
        }

    if( ( valstr = params.get(TICameraParameters::KEY_EXIF_MAKE ) ) != NULL )
        {
        CAMHAL_LOGEB("EXIF Make: %s", valstr);
        mEXIFData.mMakeValid = true;
        }
    else
        {
        mEXIFData.mMakeValid= false;
        }


// Motorola specific - begin
    CAMHAL_LOGDA("Start setting of Motorola specific parameters");
    if (NULL != params.get(TICameraParameters::KEY_TESTPATTERN1_COLORBARS))
        {
        if (strcmp( params.get(TICameraParameters::KEY_TESTPATTERN1_COLORBARS),
                    (const char *) TICameraParameters::TESTPATTERN1_ENABLE) == 0)
            {
            setEnableColorBars(true);
            }
        else if (strcmp( params.get(TICameraParameters::KEY_TESTPATTERN1_COLORBARS),
                    (const char *) TICameraParameters::TESTPATTERN1_DISABLE) == 0)
            {
            setEnableColorBars(false);
            }
        else
            {
            CAMHAL_LOGEB("Value of key %s should be \"%s\" or \"%s\"",
                TICameraParameters::KEY_TESTPATTERN1_COLORBARS,
                TICameraParameters::TESTPATTERN1_ENABLE,
                TICameraParameters::TESTPATTERN1_DISABLE);
            }
        }

    if (NULL != params.get(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE))
        {
        if (strcmp( params.get(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE),
                    (const char *) TICameraParameters::TESTPATTERN1_ENABLE) == 0)
            {
            setEnManualExposure(true);
            }
        else if (strcmp( params.get(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE),
                    (const char *) TICameraParameters::TESTPATTERN1_DISABLE) == 0)
            {
            setEnManualExposure(false);
            }
        else
            {
            CAMHAL_LOGEB("Value of key %s should be \"%s\" or \"%s\"",
                TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE,
                TICameraParameters::TESTPATTERN1_ENABLE,
                TICameraParameters::TESTPATTERN1_DISABLE);
            }
        }
    if (NULL != params.get(TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME))
        {
        setExposureTime((unsigned int) (params.getInt(TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME)));
        }

    if (NULL != params.get(TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN))
        {
        setExposureGain((int) (params.getInt(TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN)));
        }
    if (NULL != params.get(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE))
        {
        if (strcmp(params.get(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE),
                   (const char *) TICameraParameters::TESTPATTERN1_ENABLE) == 0)
            {
            setEnableTargetedExposure(true);
            }
        else if (strcmp(params.get(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE),
                             (const char *) TICameraParameters::TESTPATTERN1_DISABLE) == 0)
            {
            setEnableTargetedExposure(false);
            }
        else
            {
            CAMHAL_LOGEB("Value of key %s should be \"%s\" or \"%s\"",
                TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE,
                TICameraParameters::TESTPATTERN1_ENABLE,
                TICameraParameters::TESTPATTERN1_DISABLE);
            }
        }

    if (NULL != params.get(TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE))
        {
        setTargetExpValue((unsigned char) (params.getInt(TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE)));
        }

    if (NULL != params.get(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET))
        {
        setEnLensPosGetSet(params.getInt(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET));
        }

    if (NULL != params.get(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION))
        {
        setLensPosition(params.getInt(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION));
        }

    // The following parameters are get-only. Nothing to set.
    // KEY_DEBUGATTRIB_MIPIFRAMECOUNT
    // KEY_DEBUGATTRIB_MIPIECCERRORS
    // KEY_DEBUGATTRIB_MIPICRCERRORS

    if (NULL != params.get(TICameraParameters::KEY_DEBUGATTRIB_MIPIRESET))
        {
        setMIPIReset();
        }

    if (NULL != params.get(TICameraParameters::KEY_MOT_LEDFLASH))
    {
    setLedFlash((unsigned int) (params.getInt(TICameraParameters::KEY_MOT_LEDFLASH)));
    }

    if (NULL != params.get(TICameraParameters::KEY_MOT_LEDTORCH))
    {
    setLedTorch((unsigned int) (params.getInt(TICameraParameters::KEY_MOT_LEDTORCH)));
    }

    // Needed by camera_test application.
    if (NULL != params.get(TICameraParameters::KEY_MANUAL_EXPOSURE_TIME_MS))
    {
    setManualExposureTimeMs((unsigned int) (params.getInt(TICameraParameters::KEY_MANUAL_EXPOSURE_TIME_MS)));
    }
// Motorola specific - end

    mParams = params;
    mFirstTimeInit = false;

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

//Only Geo-tagging is currently supported
status_t OMXCameraAdapter::setupEXIF()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_SHAREDBUFFER sharedBuffer;
    OMX_TI_CONFIG_EXIF_TAGS *exifTags;
    unsigned char *sharedPtr = NULL;
    struct timeval sTv;
    struct tm *pTime;
    OMXCameraPortParameters * capData = NULL;

    LOG_FUNCTION_NAME

    sharedBuffer.pSharedBuff = NULL;
    capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&sharedBuffer, OMX_TI_CONFIG_SHAREDBUFFER);
        sharedBuffer.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        //We allocate the shared buffer dynamically based on the
        //requirements of the EXIF tags. The additional buffers will
        //get stored after the EXIF configuration structure and the pointers
        //will contain offsets within the shared buffer itself.
        sharedBuffer.nSharedBuffSize = sizeof(OMX_TI_CONFIG_EXIF_TAGS) +
                                                       ( EXIF_MODEL_SIZE ) +
                                                       ( EXIF_MAKE_SIZE ) +
                                                       ( EXIF_DATE_TIME_SIZE ) +
                                                       ( GPS_MAPDATUM_SIZE ) +
                                                       ( GPS_DATESTAMP_SIZE );


        sharedBuffer.pSharedBuff =  ( OMX_U8 * ) malloc (sharedBuffer.nSharedBuffSize);
        if ( NULL == sharedBuffer.pSharedBuff )
            {
            CAMHAL_LOGEA("No resources to allocate OMX shared buffer");
            ret = -1;
            }

        //Extra data begins right after the EXIF configuration structure.
        sharedPtr = sharedBuffer.pSharedBuff + sizeof(OMX_TI_CONFIG_EXIF_TAGS);
        }

    if ( NO_ERROR == ret )
        {
        exifTags = ( OMX_TI_CONFIG_EXIF_TAGS * ) sharedBuffer.pSharedBuff;
        OMX_INIT_STRUCT_PTR (exifTags, OMX_TI_CONFIG_EXIF_TAGS);
        exifTags->nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                                                ( OMX_INDEXTYPE ) OMX_TI_IndexConfigExifTags,
                                                &sharedBuffer );
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while retrieving EXIF configuration structure 0x%x", eError);
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusModel ) &&
              ( mEXIFData.mModelValid ) )
            {
            strncpy( ( char * ) sharedPtr,
                         ( char * ) mParams.get(TICameraParameters::KEY_EXIF_MODEL ),
                         EXIF_MODEL_SIZE - 1);

            exifTags->pModelBuff = ( OMX_S8 * ) ( sharedPtr - sharedBuffer.pSharedBuff );
            sharedPtr += EXIF_MODEL_SIZE;
            exifTags->ulModelBuffSizeBytes = EXIF_MODEL_SIZE;
            exifTags->eStatusModel = OMX_TI_TagUpdated;
            }

         if ( ( OMX_TI_TagReadWrite == exifTags->eStatusMake) &&
               ( mEXIFData.mMakeValid ) )
             {
             strncpy( ( char * ) sharedPtr,
                          ( char * ) mParams.get(TICameraParameters::KEY_EXIF_MAKE ),
                          EXIF_MAKE_SIZE - 1);

             exifTags->pMakeBuff = ( OMX_S8 * ) ( sharedPtr - sharedBuffer.pSharedBuff );
             sharedPtr += EXIF_MAKE_SIZE;
             exifTags->ulMakeBuffSizeBytes = EXIF_MAKE_SIZE;
             exifTags->eStatusMake = OMX_TI_TagUpdated;
             }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusFocalLength ))
        {
                char *ctx;
                int len;
                char* temp = (char*) mParams.get(CameraParameters::KEY_FOCAL_LENGTH);
                char * tempVal = NULL;
                if(temp != NULL)
                {
                    len = strlen(temp);
                    tempVal = (char*) malloc( sizeof(char) * (len + 1));
                }
                if(tempVal != NULL)
                {
                    memset(tempVal, '\0', len + 1);
                    strncpy(tempVal, temp, len);
                    CAMHAL_LOGDB("KEY_FOCAL_LENGTH = %s", tempVal);

                    // convert the decimal string into a rational
                    size_t den_len;
                    OMX_U32 numerator = 0;
                    OMX_U32 denominator = 0;
                    char* temp = strtok_r(tempVal, ".", &ctx);

                    if(temp != NULL)
                        numerator = atoi(temp);

                    temp = strtok_r(NULL, ".", &ctx);
                    if(temp != NULL)
                    {
                        den_len = strlen(temp);
                        if(HUGE_VAL == den_len )
                            {
                            den_len = 0;
                            }
                        denominator = static_cast<OMX_U32>(pow(10, den_len));
                        if(HUGE_VAL == denominator )
                            {
                            denominator = 1;
                            }
                        numerator = numerator*denominator + atoi(temp);
                    }else{
                        denominator = 1;
                    }

                    free(tempVal);

                    exifTags->ulFocalLength[0] = numerator;
                    exifTags->ulFocalLength[1] = denominator;
                    CAMHAL_LOGVB("exifTags->ulFocalLength = [%u] [%u]", (unsigned int)(exifTags->ulFocalLength[0]), (unsigned int)(exifTags->ulFocalLength[1]));
                    exifTags->eStatusFocalLength = OMX_TI_TagUpdated;
                }
             }

         if ( OMX_TI_TagReadWrite == exifTags->eStatusDateTime )
             {
             int status = gettimeofday (&sTv, NULL);
             pTime = localtime (&sTv.tv_sec);
             if ( ( 0 == status ) && ( NULL != pTime ) )
                {
                snprintf( ( char * ) sharedPtr, EXIF_DATE_TIME_SIZE, "%04d:%02d:%02d %02d:%02d:%02d",
                  pTime->tm_year + 1900,
                  pTime->tm_mon + 1,
                  pTime->tm_mday,
                  pTime->tm_hour,
                  pTime->tm_min,
                  pTime->tm_sec );
                }

             exifTags->pDateTimeBuff = ( OMX_S8 * ) ( sharedPtr - sharedBuffer.pSharedBuff );
             sharedPtr += EXIF_DATE_TIME_SIZE;
             exifTags->ulDateTimeBuffSizeBytes = EXIF_DATE_TIME_SIZE;
             exifTags->eStatusDateTime = OMX_TI_TagUpdated;
             }

         if ( OMX_TI_TagReadWrite == exifTags->eStatusImageWidth )
             {
             exifTags->ulImageWidth = capData->mWidth;
             exifTags->eStatusImageWidth = OMX_TI_TagUpdated;
             }

         if ( OMX_TI_TagReadWrite == exifTags->eStatusImageHeight )
             {
             exifTags->ulImageHeight = capData->mHeight;
             exifTags->eStatusImageHeight = OMX_TI_TagUpdated;
             }

         if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsLatitude ) &&
              ( mEXIFData.mGPSData.mLatValid ) )
            {
            exifTags->ulGpsLatitude[0] = abs(mEXIFData.mGPSData.mLatDeg);
            exifTags->ulGpsLatitude[2] = abs(mEXIFData.mGPSData.mLatMin);
            exifTags->ulGpsLatitude[4] = abs(mEXIFData.mGPSData.mLatSec);
            exifTags->ulGpsLatitude[1] = 1;
            exifTags->ulGpsLatitude[3] = 1;
            exifTags->ulGpsLatitude[5] = 1;
            exifTags->eStatusGpsLatitude = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpslatitudeRef ) &&
             ( mEXIFData.mGPSData.mLatValid ) )
            {
            exifTags->cGpslatitudeRef[0] = ( OMX_S8 ) mEXIFData.mGPSData.mLatRef[0];
            exifTags->cGpslatitudeRef[1] = '\0';
            exifTags->eStatusGpslatitudeRef = OMX_TI_TagUpdated;
            }

         if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsLongitude ) &&
              ( mEXIFData.mGPSData.mLongValid ) )
            {
            exifTags->ulGpsLongitude[0] = abs(mEXIFData.mGPSData.mLongDeg);
            exifTags->ulGpsLongitude[2] = abs(mEXIFData.mGPSData.mLongMin);
            exifTags->ulGpsLongitude[4] = abs(mEXIFData.mGPSData.mLongSec);
            exifTags->ulGpsLongitude[1] = 1;
            exifTags->ulGpsLongitude[3] = 1;
            exifTags->ulGpsLongitude[5] = 1;
            exifTags->eStatusGpsLongitude = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsLongitudeRef ) &&
             ( mEXIFData.mGPSData.mLongValid ) )
            {
            exifTags->cGpsLongitudeRef[0] = ( OMX_S8 ) mEXIFData.mGPSData.mLongRef[0];
            exifTags->cGpsLongitudeRef[1] = '\0';
            exifTags->eStatusGpsLongitudeRef = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsAltitude ) &&
             ( mEXIFData.mGPSData.mAltitudeValid) )
            {
            exifTags->ulGpsAltitude[0] = ( OMX_U32 ) mEXIFData.mGPSData.mAltitude;
            exifTags->eStatusGpsAltitude = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsMapDatum ) &&
             ( mEXIFData.mGPSData.mMapDatumValid ) )
            {
            memcpy(sharedPtr, mEXIFData.mGPSData.mMapDatum, GPS_MAPDATUM_SIZE);

            exifTags->pGpsMapDatumBuff = ( OMX_S8 * ) ( sharedPtr - sharedBuffer.pSharedBuff );
            exifTags->ulGpsMapDatumBuffSizeBytes = GPS_MAPDATUM_SIZE;
            exifTags->eStatusGpsMapDatum = OMX_TI_TagUpdated;
            sharedPtr += GPS_MAPDATUM_SIZE;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsProcessingMethod ) &&
             ( mEXIFData.mGPSData.mProcMethodValid ) )
            {
            exifTags->pGpsProcessingMethodBuff = ( OMX_S8 * ) ( sharedPtr - sharedBuffer.pSharedBuff );
            memcpy(sharedPtr, EXIFASCIIPrefix, sizeof(EXIFASCIIPrefix));
            sharedPtr += sizeof(EXIFASCIIPrefix);

            memcpy(sharedPtr, mEXIFData.mGPSData.mProcMethod, ( GPS_PROCESSING_SIZE - sizeof(EXIFASCIIPrefix) ) );
            exifTags->ulGpsProcessingMethodBuffSizeBytes = GPS_PROCESSING_SIZE;
            exifTags->eStatusGpsProcessingMethod = OMX_TI_TagUpdated;
            sharedPtr += GPS_PROCESSING_SIZE;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsVersionId ) &&
             ( mEXIFData.mGPSData.mVersionIdValid ) )
            {
            exifTags->ucGpsVersionId[0] = ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[0];
            exifTags->ucGpsVersionId[1] =  ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[1];
            exifTags->ucGpsVersionId[2] = ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[2];
            exifTags->ucGpsVersionId[3] = ( OMX_U8 ) mEXIFData.mGPSData.mVersionId[3];
            exifTags->eStatusGpsVersionId = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsTimeStamp ) &&
             ( mEXIFData.mGPSData.mTimeStampValid ) )
            {
            exifTags->ulGpsTimeStamp[0] = mEXIFData.mGPSData.mTimeStampHour;
            exifTags->ulGpsTimeStamp[2] = mEXIFData.mGPSData.mTimeStampMin;
            exifTags->ulGpsTimeStamp[4] = mEXIFData.mGPSData.mTimeStampSec;
            exifTags->ulGpsTimeStamp[1] = 1;
            exifTags->ulGpsTimeStamp[3] = 1;
            exifTags->ulGpsTimeStamp[5] = 1;
            exifTags->eStatusGpsTimeStamp = OMX_TI_TagUpdated;
            }

        if ( ( OMX_TI_TagReadWrite == exifTags->eStatusGpsDateStamp ) &&
             ( mEXIFData.mGPSData.mDatestampValid ) )
            {
            strncpy( ( char * ) exifTags->cGpsDateStamp,
                         ( char * ) mEXIFData.mGPSData.mDatestamp,
                         GPS_DATESTAMP_SIZE );
            exifTags->eStatusGpsDateStamp = OMX_TI_TagUpdated;
            }

        eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigExifTags,
                                               &sharedBuffer );
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting EXIF configuration 0x%x", eError);
            ret = -1;
            }
        }

    if ( NULL != sharedBuffer.pSharedBuff )
        {
        free(sharedBuffer.pSharedBuff);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::convertGPSCoord(double coord, int *deg, int *min, int *sec)
{
    double tmp;

    LOG_FUNCTION_NAME

    if ( coord == 0 ) {

        LOGE("Invalid GPS coordinate");

        return -EINVAL;
    }

    *deg = (int) floor(coord);
    tmp = ( coord - floor(coord) )*60;
    *min = (int) floor(tmp);
    tmp = ( tmp - floor(tmp) )*60;
    *sec = (int) floor(tmp);

    if( *sec >= 60 ) {
        *sec = 0;
        *min += 1;
    }

    if( *min >= 60 ) {
        *min = 0;
        *deg += 1;
    }

    LOG_FUNCTION_NAME_EXIT

    return NO_ERROR;
}


void saveFile(unsigned char   *buff, int width, int height, int format) {
    static int      counter = 1;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME

    fn[0] = 0;
    sprintf(fn, "/preview%03d.yuv", counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    if(fd < 0) {
        LOGE("Unable to open file %s: %s", fn, strerror(fd));
        return;
    }

    CAMHAL_LOGVB("Copying from 0x%x, size=%d x %d", buff, width, height);

    //method currently supports only nv12 dumping
    int stride = width;
    uint8_t *bf = (uint8_t*) buff;
    for(int i=0;i<height;i++)
        {
        write(fd, bf, width);
        bf += 4096;
        }

    for(int i=0;i<height/2;i++)
        {
        write(fd, bf, stride);
        bf += 4096;
        }

    close(fd);


    counter++;

    LOG_FUNCTION_NAME_EXIT
}

status_t OMXCameraAdapter::getFocusDistances(OMX_U32 &near,OMX_U32 &optimal, OMX_U32 &far)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError;

    OMX_TI_CONFIG_FOCUSDISTANCETYPE focusDist;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = UNKNOWN_ERROR;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR(&focusDist, OMX_TI_CONFIG_FOCUSDISTANCETYPE);
        focusDist.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigFocusDistance,
                               &focusDist);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while querying focus distances 0x%x", eError);
            ret = UNKNOWN_ERROR;
            }

        }

    if ( NO_ERROR == ret )
        {
        near = focusDist.nFocusDistanceNear;
        optimal = focusDist.nFocusDistanceOptimal;
        far = focusDist.nFocusDistanceFar;
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::encodeFocusDistance(OMX_U32 dist, char *buffer, size_t length)
{
    status_t ret = NO_ERROR;
    uint32_t focusScale = 1000;
    float distFinal;

    LOG_FUNCTION_NAME

    if(mParameters3A.Focus == OMX_IMAGE_FocusControlAutoInfinity)
        {
        dist=0;
        }

    if ( NO_ERROR == ret )
        {
        if ( 0 == dist )
            {
            strncpy(buffer, CameraParameters::FOCUS_DISTANCE_INFINITY, ( length - 1 ));
            }
        else
            {
            distFinal = dist;
            distFinal /= focusScale;
            snprintf(buffer, ( length - 1 ) , "%5.3f", distFinal);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::addFocusDistances(OMX_U32 &near,
                                             OMX_U32 &optimal,
                                             OMX_U32 &far,
                                             CameraParameters& params)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if ( NO_ERROR == ret )
        {
        ret = encodeFocusDistance(near, mFocusDistNear, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        ret = encodeFocusDistance(optimal, mFocusDistOptimal, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        ret = encodeFocusDistance(far, mFocusDistFar, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        snprintf(mFocusDistBuffer, ( FOCUS_DIST_BUFFER_SIZE - 1) ,"%s,%s,%s", mFocusDistNear,
                                                                              mFocusDistOptimal,
                                                                              mFocusDistFar);

        params.set(CameraParameters::KEY_FOCUS_DISTANCES, mFocusDistBuffer);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}


void OMXCameraAdapter::getParameters(CameraParameters& params)
{
    OMX_CONFIG_EXPOSUREVALUETYPE exp;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 focusNear, focusOptimal, focusFar;
    status_t stat = NO_ERROR;

    LOG_FUNCTION_NAME

#ifdef PARAM_FEEDBACK

    OMX_CONFIG_WHITEBALCONTROLTYPE wb;
    OMX_CONFIG_FLICKERCANCELTYPE flicker;
    OMX_CONFIG_SCENEMODETYPE scene;
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE flash;
    OMX_CONFIG_BRIGHTNESSTYPE brightness;
    OMX_CONFIG_CONTRASTTYPE contrast;
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE procSharpness;
    OMX_CONFIG_SATURATIONTYPE saturation;
    OMX_CONFIG_IMAGEFILTERTYPE effect;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;

    exp.nSize = sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE);
    exp.nVersion = mLocalVersionParam;
    exp.nPortIndex = OMX_ALL;

    expValues.nSize = sizeof(OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nVersion = mLocalVersionParam;
    expValues.nPortIndex = OMX_ALL;

    wb.nSize = sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE);
    wb.nVersion = mLocalVersionParam;
    wb.nPortIndex = OMX_ALL;

    flicker.nSize = sizeof(OMX_CONFIG_FLICKERCANCELTYPE);
    flicker.nVersion = mLocalVersionParam;
    flicker.nPortIndex = OMX_ALL;

    scene.nSize = sizeof(OMX_CONFIG_SCENEMODETYPE);
    scene.nVersion = mLocalVersionParam;
    scene.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    flash.nSize = sizeof(OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    flash.nVersion = mLocalVersionParam;
    flash.nPortIndex = OMX_ALL;


    brightness.nSize = sizeof(OMX_CONFIG_BRIGHTNESSTYPE);
    brightness.nVersion = mLocalVersionParam;
    brightness.nPortIndex = OMX_ALL;

    contrast.nSize = sizeof(OMX_CONFIG_CONTRASTTYPE);
    contrast.nVersion = mLocalVersionParam;
    contrast.nPortIndex = OMX_ALL;

    procSharpness.nSize = sizeof( OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE );
    procSharpness.nVersion = mLocalVersionParam;
    procSharpness.nPortIndex = OMX_ALL;

    saturation.nSize = sizeof(OMX_CONFIG_SATURATIONTYPE);
    saturation.nVersion = mLocalVersionParam;
    saturation.nPortIndex = OMX_ALL;

    effect.nSize = sizeof(OMX_CONFIG_IMAGEFILTERTYPE);
    effect.nVersion = mLocalVersionParam;
    effect.nPortIndex = OMX_ALL;

    focus.nSize = sizeof(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focus.nVersion = mLocalVersionParam;
    focus.nPortIndex = OMX_ALL;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,OMX_IndexConfigCommonExposure, &exp);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonWhiteBalance, &wb);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexConfigFlickerCancel, &flicker );
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexParamSceneMode, &scene);
    OMX_GetParameter( mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexParamFlashControl, &flash);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonBrightness, &brightness);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonContrast, &contrast);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexConfigSharpeningLevel, &procSharpness);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonSaturation, &saturation);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonImageFilter, &effect);
    OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);

    char * str = NULL;

    for(int i = 0; i < ExpLUT.size; i++)
        if( ExpLUT.Table[i].omxDefinition == exp.eExposureControl )
            str = (char*)ExpLUT.Table[i].userDefinition;
    params.set( TICameraParameters::KEY_EXPOSURE_MODE , str);

    for(int i = 0; i < WBalLUT.size; i++)
        if( WBalLUT.Table[i].omxDefinition == wb.eWhiteBalControl )
            str = (char*)WBalLUT.Table[i].userDefinition;
    params.set( CameraParameters::KEY_WHITE_BALANCE , str );

    for(int i = 0; i < FlickerLUT.size; i++)
        if( FlickerLUT.Table[i].omxDefinition == flicker.eFlickerCancel )
            str = (char*)FlickerLUT.Table[i].userDefinition;
    params.set( CameraParameters::KEY_ANTIBANDING , str );

    for(int i = 0; i < SceneLUT.size; i++)
        if( SceneLUT.Table[i].omxDefinition == scene.eSceneMode )
            str = (char*)SceneLUT.Table[i].userDefinition;
    params.set( CameraParameters::KEY_SCENE_MODE , str );

    for(int i = 0; i < FlashLUT.size; i++)
        if( FlashLUT.Table[i].omxDefinition == flash.eFlashControl )
            str = (char*)FlashLUT.Table[i].userDefinition;
    params.set( CameraParameters::KEY_FLASH_MODE, str );

    for(int i = 0; i < EffLUT.size; i++)
        if( EffLUT.Table[i].omxDefinition == effect.eImageFilter )
            str = (char*)EffLUT.Table[i].userDefinition;
    params.set( CameraParameters::KEY_EFFECT , str );

    for(int i = 0; i < FocusLUT.size; i++)
        if( FocusLUT.Table[i].omxDefinition == focus.eFocusControl )
            str = (char*)FocusLUT.Table[i].userDefinition;

    params.set( CameraParameters::KEY_FOCUS_MODE , str );

    int comp = ((expValues.xEVCompensation * 10) >> Q16_OFFSET);

    params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, comp );
    params.set( TICameraParameters::KEY_MAN_EXPOSURE, expValues.nShutterSpeedMsec);
    params.set( TICameraParameters::KEY_BRIGHTNESS, brightness.nBrightness);
    params.set( TICameraParameters::KEY_CONTRAST, contrast.nContrast );
    params.set( TICameraParameters::KEY_SHARPNESS, procSharpness.nLevel);
    params.set( TICameraParameters::KEY_SATURATION, saturation.nSaturation);

#else

    stat = getFocusDistances(focusNear, focusOptimal, focusFar);
    if ( NO_ERROR == stat)
        {
        stat = addFocusDistances(focusNear, focusOptimal, focusFar, params);
            if ( NO_ERROR != stat )
                {
                CAMHAL_LOGEB("Error in call to addFocusDistances() 0x%x", stat);
                }
        }
    else
        {
        CAMHAL_LOGEB("Error in call to getFocusDistances() 0x%x", stat);
        }

    OMX_INIT_STRUCT_PTR (&exp, OMX_CONFIG_EXPOSUREVALUETYPE);
    exp.nPortIndex = OMX_ALL;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,OMX_IndexConfigCommonExposureValue, &exp);
    if ( OMX_ErrorNone == eError )
        {
        params.set(TICameraParameters::KEY_CURRENT_ISO, exp.nSensitivity);
        }
    else
        {
        CAMHAL_LOGEB("OMX error 0x%x, while retrieving current ISO value", eError);
        }

        {
        Mutex::Autolock lock(mZoomLock);
        if ( ( mSmoothZoomEnabled ) )
            {
            if ( mZoomParameterIdx != mCurrentZoomIdx )
                {
                mZoomParameterIdx += mZoomInc;
                }

            params.set( CameraParameters::KEY_ZOOM, mZoomParameterIdx);
            if ( ( mCurrentZoomIdx == mTargetZoomIdx ) && ( mZoomParameterIdx == mCurrentZoomIdx ) )
                {
                mSmoothZoomEnabled = false;
                }
            CAMHAL_LOGDB("CameraParameters Zoom = %d , mSmoothZoomEnabled = %d", mCurrentZoomIdx, mSmoothZoomEnabled);
            }
        else
            {
            params.set( CameraParameters::KEY_ZOOM, mCurrentZoomIdx);
            }
        }

        //Face detection coordinates go in here
        {
        Mutex::Autolock lock(mFaceDetectionLock);
        if ( mFaceDetectionRunning )
            {
            params.set( TICameraParameters::KEY_FACE_DETECTION_DATA, mFaceDectionResult);
            }
        else
            {
            params.set( TICameraParameters::KEY_FACE_DETECTION_DATA, NULL);
            }
        }

#endif

// Motorola specific - begin
// TICameraParameters::KEY_TESTPATTERN1_COLORBARS "TestPattern1_ColorBars"; // enable/disable
    OMX_CONFIG_BOOLEANTYPE bColorBarsEnabled;
// TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE "TestPattern1_EnManualExposure"; // enable/disable
// TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME "DebugAttrib_ExposureTime"; // miscoseconds, default 0 - auto mode
// TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN "DebugAttrib_ExposureGain"; // -600..2000, default 0. means -6..20dB
    OMX_CONFIG_EXPOSUREVALUETYPE exposure;
// TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE "TestPattern1_TargetedExposure"; // enable/disable
// TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE "DebugAttrib_TargetExpValue"; // 0..255, default 128
    OMX_CONFIG_TARGETEXPOSURE targetExposure;
// TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET "DebugAttrib_EnLensPosGetSet"; // int: 0-disabled, 1-enabled, default 0
// TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION "DebugAttrib_LensPosition"; // 0..100, default 0
// This variable is defined above: OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;
// TICameraParameters::KEY_DEBUGATTRIB_AFSHARPNESSSCORE[] = "DebugAttrib_AFSharpnessScore"; // U32, default 0;
    OMX_CONFIG_AUTOFOCUSSCORE af_score;
// TICameraParameters::KEY_DEBUGATTRIB_MIPIFRAMECOUNT "DebugAttrib_MipiFrameCount"; // U32, default 0
// TICameraParameters::KEY_DEBUGATTRIB_MIPIECCERRORS "DebugAttrib_MipiEccErrors";   // U32, default 0
// TICameraParameters::KEY_DEBUGATTRIB_MIPICRCERRORS "DebugAttrib_MipiCrcErrors";   // U32, default 0
    OMX_CONFIG_MIPICOUNTERS mipiCounters;
// TICameraParameters::KEY_MOT_LEDFLASH "mot-led-flash"; // U32, default 100, percent
// TICameraParameters::KEY_MOT_LEDTORCH "mot-led-torch"; // U32, default 100, percent
    OMX_CONFIG_LEDINTESITY LedIntens;


    OMX_INIT_STRUCT_PTR (&bColorBarsEnabled, OMX_CONFIG_BOOLEANTYPE);
    OMX_INIT_STRUCT_PTR (&exposure, OMX_CONFIG_EXPOSUREVALUETYPE);
    exposure.nPortIndex = OMX_ALL;
    OMX_INIT_STRUCT_PTR (&targetExposure, OMX_CONFIG_TARGETEXPOSURE);
    targetExposure.nPortIndex = OMX_ALL;
    OMX_INIT_STRUCT_PTR (&focus, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focus.nPortIndex = OMX_ALL;
    OMX_INIT_STRUCT_PTR (&af_score, OMX_CONFIG_AUTOFOCUSSCORE);
    af_score.nPortIndex = OMX_ALL;
    OMX_INIT_STRUCT_PTR (&mipiCounters, OMX_CONFIG_MIPICOUNTERS);
    mipiCounters.nPortIndex = OMX_ALL;
    OMX_INIT_STRUCT_PTR (&LedIntens, OMX_CONFIG_LEDINTESITY);
    LedIntens.nPortIndex = OMX_ALL;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigColorBars, &bColorBarsEnabled);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigColorBars) returned error 0x%x", eError);
        }
    else
        {
        if (bColorBarsEnabled.bEnabled != 0)
            {
            params.set( TICameraParameters::KEY_TESTPATTERN1_COLORBARS,
                        TICameraParameters::TESTPATTERN1_ENABLE);
            }
        else
            {
            params.set( TICameraParameters::KEY_TESTPATTERN1_COLORBARS,
                        TICameraParameters::TESTPATTERN1_DISABLE);
            }
        }

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigCommonExposureValue) returned error 0x%x", eError);
        }
    else
        {
        if ( (false == exposure.bAutoSensitivity) &&
             (false == exposure.bAutoShutterSpeed))
            {
            params.set( TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE,
                        (const char *)(TICameraParameters::TESTPATTERN1_ENABLE) );
            }
        else
            {
            params.set( TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE,
                        (const char *)(TICameraParameters::TESTPATTERN1_DISABLE) );
            }

        CAMHAL_LOGDB("OMXCameraAdapter - getParametersOMX - exposure.nShutterSpeedMsec = %ld", exposure.nShutterSpeedMsec);
        CAMHAL_LOGDB("OMXCameraAdapter - getParametersOMX - exposure.nSensitivity = %ld", exposure.nSensitivity);
        params.set( TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME, (int)(exposure.nShutterSpeedMsec * 1000));
        params.set( TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN,
                    (int)(10 * log10( ((double)(exposure.nSensitivity))/((double)ISO100) )  ));
        if (OMX_FALSE == exposure.bAutoShutterSpeed)
            {
            params.set( TICameraParameters::KEY_MANUAL_EXPOSURE_TIME_MS, (int)(exposure.nShutterSpeedMsec));
            }
        else
            {
            params.set( TICameraParameters::KEY_MANUAL_EXPOSURE_TIME_MS, 0);
            }

        }

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigTargetExposure, &targetExposure);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigTargetExposure) returned error 0x%x", eError);
        }
    else
        {
        if ( false == targetExposure.bUseTargetExposure)
            {
            params.set( TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE,
                        (const char *)(TICameraParameters::TESTPATTERN1_ENABLE) );
            }
        else
            {
            params.set( TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE,
                        (const char *)(TICameraParameters::TESTPATTERN1_DISABLE) );
            }

        params.set( TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE, (int)(targetExposure.nTargetExposure));
        }


    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigFocusControl) returned error 0x%x", eError);
        }
    else
        {
        // Get this parameter only if it had been set before via the SetDebugAttrib() interface
        if (params.get(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET) != NULL)
            {
            if (1 == mEnLensPos)
                {
                params.set(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET, 1);
                }
            else
                {
                params.set(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET, 0);
                }
            }
        // Get this parameter only if it had been set before via the SetDebugAttrib() interface
        if (params.get(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION) != NULL)
            {
            // Read the focus position in percents
            params.set(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION, (int)(focus.nFocusSteps));
            }
        }

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigAFScore, &af_score);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigAFScore) returned error 0x%x", eError);
        }
    else
        {
        params.set( TICameraParameters::KEY_DEBUGATTRIB_AFSHARPNESSSCORE, (int)(af_score.nAutoFocusScore));
        }

    // OMX_IndexConfigMipiCounters,                         /**< reference: OMX_CONFIG_MIPICOUNTERS */
    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigMipiCounters, &mipiCounters);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x when getting OMX_IndexConfigMipiCounters", eError);
        }
    else
        {
        params.set(TICameraParameters::KEY_DEBUGATTRIB_MIPIFRAMECOUNT, mipiCounters.nMIPICounter);
        params.set(TICameraParameters::KEY_DEBUGATTRIB_MIPIECCERRORS, mipiCounters.nECCCounter);
        params.set(TICameraParameters::KEY_DEBUGATTRIB_MIPICRCERRORS, mipiCounters.nCRCCounter);
        // Clear the MIPI Reset parameter. It is set with setParameters to 1, to reset the  MIPI counters.
        params.set(TICameraParameters::KEY_DEBUGATTRIB_MIPIRESET, 0);
        }

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigLedIntensity, &LedIntens);
    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigLedIntensity) returned error 0x%x", eError);
        }
    else
        {
        params.set( TICameraParameters::KEY_MOT_LEDFLASH, (int)(LedIntens.nLedFlashIntens));
        params.set( TICameraParameters::KEY_MOT_LEDTORCH, (int)(LedIntens.nLedTorchIntens));
        }

// Motorola specific - end

    LOG_FUNCTION_NAME_EXIT
}


status_t OMXCameraAdapter::setVFramerate(OMX_U32 minFrameRate, OMX_U32 maxFrameRate)
{
     status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_VARFRMRANGETYPE vfr;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
    OMX_INIT_STRUCT_PTR (&vfr, OMX_TI_CONFIG_VARFRMRANGETYPE);

    vfr.xMin = minFrameRate<<16;
    vfr.xMax = maxFrameRate<<16;

    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_TI_IndexConfigVarFrmRange, &vfr);
     if ( OMX_ErrorNone != eError )
     {
            CAMHAL_LOGEB("Error while setting VFR min = %d, max = %d, error = 0x%x",
                         ( unsigned int ) minFrameRate,
                         ( unsigned int ) maxFrameRate,
                         eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGEB("VFR Configured Successfully [%d:%d]",
                        ( unsigned int ) minFrameRate,
                        ( unsigned int ) maxFrameRate);
            }
        }

    return ret;
 }

status_t OMXCameraAdapter::setFormat(OMX_U32 port, OMXCameraPortParameters &portParams)
{
    size_t bufferCount;

    LOG_FUNCTION_NAME

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;

    OMX_INIT_STRUCT_PTR (&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);

    portCheck.nPortIndex = port;

    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                                OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    if ( OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW == port )
        {
        portCheck.format.video.nFrameWidth      = portParams.mWidth;
        portCheck.format.video.nFrameHeight     = portParams.mHeight;
        portCheck.format.video.eColorFormat     = portParams.mColorFormat;
        portCheck.format.video.nStride          = portParams.mStride;
        if( ( portCheck.format.video.nFrameWidth == 1920 ) && ( portCheck.format.video.nFrameHeight == 1080 ) &&
            ( portParams.mFrameRate>=FRAME_RATE_FULL_HD ) )
            {
            //Confugure overclocking for this framerate
            setSensorOverclock(true);
            }
        else
            {
            setSensorOverclock(false);
            }

        portCheck.format.video.xFramerate       = portParams.mFrameRate<<16;
        portCheck.nBufferSize                   = portParams.mStride * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
        mFocusThreshold = FOCUS_THRESHOLD * portParams.mFrameRate;
        }
    else if ( OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port )
        {
        portCheck.format.image.nFrameWidth      = portParams.mWidth;
        portCheck.format.image.nFrameHeight     = portParams.mHeight;
        if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingNone )
            {
            portCheck.format.image.eColorFormat     = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingJPS )
            {
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = (OMX_IMAGE_CODINGTYPE) OMX_TI_IMAGE_CodingJPS;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingMPO )
            {
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = (OMX_IMAGE_CODINGTYPE) OMX_TI_IMAGE_CodingMPO;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingRAWJPEG )
            {
            //TODO: OMX_IMAGE_CodingJPEG should be changed to OMX_IMAGE_CodingRAWJPEG when
            // RAW format is supported
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
        else if ( OMX_COLOR_FormatUnused == portParams.mColorFormat && mCodingMode == CodingRAWMPO )
            {
            //TODO: OMX_IMAGE_CodingJPEG should be changed to OMX_IMAGE_CodingRAWMPO when
            // RAW format is supported
            portCheck.format.image.eColorFormat       = OMX_COLOR_FormatCbYCrY;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
        else
            {
            portCheck.format.image.eColorFormat     = portParams.mColorFormat;
            portCheck.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
            }

        portCheck.format.image.nStride          = portParams.mStride * portParams.mWidth;
        portCheck.nBufferSize                   =  portParams.mStride * portParams.mWidth * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
        }
    else
        {
        CAMHAL_LOGEB("Unsupported port index 0x%x", (unsigned int)port);
        }

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    /* check if parameters are set correctly by calling GetParameter() */
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
                                        OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    portParams.mBufSize = portCheck.nBufferSize;

    if ( OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port )
        {
        LOGD("\n *** IMG Width = %ld", portCheck.format.image.nFrameWidth);
        LOGD("\n ***IMG Height = %ld", portCheck.format.image.nFrameHeight);

        LOGD("\n ***IMG IMG FMT = %x", portCheck.format.image.eColorFormat);
        LOGD("\n ***IMG portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        LOGD("\n ***IMG portCheck.nBufferCountMin = %ld\n",
                                                portCheck.nBufferCountMin);
        LOGD("\n ***IMG portCheck.nBufferCountActual = %ld\n",
                                                portCheck.nBufferCountActual);
        LOGD("\n ***IMG portCheck.format.image.nStride = %ld\n",
                                                portCheck.format.image.nStride);
        }
    else
        {
        LOGD("\n *** PRV Width = %ld", portCheck.format.video.nFrameWidth);
        LOGD("\n ***PRV Height = %ld", portCheck.format.video.nFrameHeight);

        LOGD("\n ***PRV IMG FMT = %x", portCheck.format.video.eColorFormat);
        LOGD("\n ***PRV portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        LOGD("\n ***PRV portCheck.nBufferCountMin = %ld\n",
                                                portCheck.nBufferCountMin);
        LOGD("\n ***PRV portCheck.nBufferCountActual = %ld\n",
                                                portCheck.nBufferCountActual);
        LOGD("\n ***PRV portCheck.format.video.nStride = %ld\n",
                                                portCheck.format.video.nStride);
        }

    LOG_FUNCTION_NAME_EXIT

    return ErrorUtils::omxToAndroidError(eError);

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError=%x", __FUNCTION__, eError);

    LOG_FUNCTION_NAME_EXIT

    return ErrorUtils::omxToAndroidError(eError);
}


status_t OMXCameraAdapter::enableVideoStabilization(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_FRAMESTABTYPE frameStabCfg;


    LOG_FUNCTION_NAME

    if ( NO_ERROR == ret )
        {
        OMX_CONFIG_BOOLEANTYPE vstabp;
        OMX_INIT_STRUCT_PTR (&vstabp, OMX_CONFIG_BOOLEANTYPE);
        if(enable)
            {
            vstabp.bEnabled = OMX_TRUE;
            }
        else
            {
            vstabp.bEnabled = OMX_FALSE;
            }

        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexParamFrameStabilisation, &vstabp);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring video stabilization param 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Video stabilization param configured successfully");
            }

        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&frameStabCfg, OMX_CONFIG_FRAMESTABTYPE);


        eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                                            ( OMX_INDEXTYPE ) OMX_IndexConfigCommonFrameStabilisation, &frameStabCfg);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while getting video stabilization mode 0x%x", (unsigned int)eError);
            ret = -1;
            }

        CAMHAL_LOGDB("VSTAB Port Index = %d", (int)frameStabCfg.nPortIndex);

        frameStabCfg.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        if ( enable )
            {
            CAMHAL_LOGDA("VSTAB is enabled");
            frameStabCfg.bStab = OMX_TRUE;
            }
        else
            {
            CAMHAL_LOGDA("VSTAB is disabled");
            frameStabCfg.bStab = OMX_FALSE;

            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                            ( OMX_INDEXTYPE ) OMX_IndexConfigCommonFrameStabilisation, &frameStabCfg);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring video stabilization mode 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Video stabilization mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}


status_t OMXCameraAdapter::enableVideoNoiseFilter(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_VIDEONOISEFILTERTYPE vnfCfg;


    LOG_FUNCTION_NAME

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&vnfCfg, OMX_PARAM_VIDEONOISEFILTERTYPE);

        if ( enable )
            {
            CAMHAL_LOGDA("VNF is enabled");
            vnfCfg.eMode = OMX_VideoNoiseFilterModeOn;
            }
        else
            {
            CAMHAL_LOGDA("VNF is disabled");
            vnfCfg.eMode = OMX_VideoNoiseFilterModeOff;
            }

        eError =  OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                            ( OMX_INDEXTYPE ) OMX_IndexParamVideoNoiseFilter, &vnfCfg);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring video noise filter 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Video noise filter is configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;

}


status_t OMXCameraAdapter::flushBuffers()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    TIMM_OSAL_ERRORTYPE err;
    TIMM_OSAL_U32 uRequestedEvents = OMXCameraAdapter::CAMERA_PORT_FLUSH;
    TIMM_OSAL_U32 pRetrievedEvents;

    ret = mFlushSem.Create(0);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in creating semaphore %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    LOG_FUNCTION_NAME

    OMXCameraPortParameters * mPreviewData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    Mutex::Autolock lock(mLock);

    if(!mPreviewing || mFlushBuffers)
        {
        LOG_FUNCTION_NAME_EXIT
        return NO_ERROR;
        }

    ///If preview is ongoing and we get a new set of buffers, flush the o/p queue,
    ///wait for all buffers to come back and then queue the new buffers in one shot
    ///Stop all callbacks when this is ongoing
    mFlushBuffers = true;

    ///Register for the FLUSH event
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandFlush,
                                OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                                mFlushSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    ///Send FLUSH command to preview port
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp, OMX_CommandFlush,
                                                    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                                                    NULL);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandFlush)-0x%x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    ///Wait for the FLUSH event to occur
    ret = mFlushSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("Flush event received");
        }
    else
        {
        CAMHAL_LOGDA("Flush event timeout expired");
        goto EXIT;
        }

    LOG_FUNCTION_NAME_EXIT

    return (ret | ErrorUtils::omxToAndroidError(eError));

    EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    LOG_FUNCTION_NAME_EXIT

    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::completeHDRProcessing(void* jpegBuf, int len)
{
    CAMHAL_LOGEA("CameraAdapter::CAMERA_COMPLETE_HDR_PROCESSING\n");
    status_t ret = NO_ERROR;
    CameraFrame frame;
    OMXCameraPortParameters  *pPortParam = NULL;

    pPortParam = &(mCameraAdapterParameters.mCameraPortParams[OMX_CAMERA_PORT_IMAGE_OUT_IMAGE]);

    frame.mFrameType = CameraFrame::IMAGE_FRAME;
    frame.mBuffer = jpegBuf;
    frame.mLength = len;
    frame.mAlignment = pPortParam->mStride;
    frame.mOffset = 0;
    frame.mWidth = pPortParam->mWidth;
    frame.mHeight = pPortParam->mHeight;
    frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    stopImageCapture();
    ret = sendFrameToSubscribers(&frame);

    return ret;
}

///API to give the buffers to Adapter
status_t OMXCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mLock);

    switch(mode)
        {
        case CAMERA_PREVIEW:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mNumBufs =  num;
            ret = UseBuffersPreview(bufArr, num);
            break;

        case CAMERA_IMAGE_CAPTURE:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mNumBufs = num;
            ret = UseBuffersCapture(bufArr, num);
            break;

        case CAMERA_VIDEO:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mNumBufs =  num;
            ret = UseBuffersPreview(bufArr, num);
            break;

        case CAMERA_MEASUREMENT:
            mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex].mNumBufs = num;
            ret = UseBuffersPreviewData(bufArr, num);
            break;

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::UseBuffersPreviewData(void* bufArr, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * measurementData = NULL;
    uint32_t *buffers;
    Semaphore eventSem;

    LOG_FUNCTION_NAME

    if ( mComponentState != OMX_StateLoaded )
        {
        CAMHAL_LOGEA("Calling UseBuffersPreviewData() when not in LOADED state");
        ret = -1;
        }

    if ( NULL == bufArr )
        {
        CAMHAL_LOGEA("NULL pointer passed for buffArr");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];
        measurementData->mNumBufs = num ;
        buffers= (uint32_t*) bufArr;
        ret = mUsePreviewDataSem.Create(0);
        }

    if ( NO_ERROR == ret )
        {
         ///Register for port enable event on measurement port
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                      OMX_EventCmdComplete,
                                      OMX_CommandPortEnable,
                                      mCameraAdapterParameters.mMeasurementPortIndex,
                                      mUsePreviewDataSem);

        if ( ret == NO_ERROR )
            {
            CAMHAL_LOGDB("Registering for event %d", ret);
            }
        else
            {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
         ///Enable MEASUREMENT Port
         eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                      OMX_CommandPortEnable,
                                      mCameraAdapterParameters.mMeasurementPortIndex,
                                      NULL);

            if ( eError == OMX_ErrorNone )
                {
                CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
                }
            else
                {
                CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
                ret = -1;
                }
        }

    if ( NO_ERROR == ret )
        {
        //Wait for the port enable event to occur
        ret = mUsePreviewDataSem.WaitTimeout(OMX_CMD_TIMEOUT);

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Port enable event arrived on measurement port");
            }
        else
            {
            CAMHAL_LOGEA("Timeout expoired during port enable on measurement port");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::UseBuffersPreview(void* bufArr, int num)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME

    //mLock is already held in useBuffers
    mPreviewInProgress = true;

    ret = doUseBuffersPreview(bufArr, num);

    LOG_FUNCTION_NAME_EXIT

    return ret;

}



status_t OMXCameraAdapter::doUseBuffersPreview(void* bufArr, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMXCameraPortParameters * mPreviewData = NULL;
    OMXCameraPortParameters *measurementData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];
    mPreviewData->mNumBufs = num ;
    uint32_t *buffers = (uint32_t*)bufArr;

    Semaphore eventSem;
    ret = mUsePreviewSem.Create(0);

    LOG_FUNCTION_NAME

    ///Flag to determine whether it is 3D camera or not
    bool isS3d = false;
    const char *valstr = NULL;
    if ( (valstr = mParams.get(TICameraParameters::KEY_S3D_SUPPORTED)) != NULL) {
        isS3d = (strcmp(valstr, "true") == 0);
    }

    if(bufArr && (mComponentState == OMX_StateExecuting))
        {
        CAMHAL_LOGEA("Queueing buffers to Camera");

        for(int index=0;index<num;index++)
            {
            CAMHAL_LOGDB("Queuing new buffers to Ducati 0x%x",((int32_t*)bufArr)[index]);

            mPreviewData->mBufferHeader[index]->nFlags |= OMX_BUFFERHEADERFLAG_MODIFIED;
            mPreviewData->mBufferHeader[index]->pBuffer = (OMX_U8*)((int32_t*)bufArr)[index];

            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)mPreviewData->mBufferHeader[index]);

            if(eError!=OMX_ErrorNone)
                {
                CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

             }

        LOG_FUNCTION_NAME_EXIT
        return eError;
        }
    else if(!bufArr && ((mComponentState == OMX_StateIdle) ||(mComponentState == OMX_StateExecuting) ))
        {
        //Nothing to do
        return NO_ERROR;
        }
    else if(bufArr && (mComponentState == OMX_StateIdle))
        {
        for(int index=0;index<num;index++)
            {
            CAMHAL_LOGDB("Queuing new buffers to Ducati 0x%x",((int32_t*)bufArr)[index]);


            mPreviewData->mBufferHeader[index]->pBuffer = (OMX_U8*)((int32_t*)bufArr)[index];
            }
        }



     if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in creating semaphore %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    if(mPreviewing && (mPreviewData->mNumBufs!=num))
        {
        CAMHAL_LOGEA("Current number of buffers doesnt equal new num of buffers passed!");
        LOG_FUNCTION_NAME_EXIT
        return BAD_VALUE;
        }
    ///If preview is ongoing
    if(mPreviewing)
        {
        ///If preview is ongoing and we get a new set of buffers, flush the o/p queue,
        ///wait for all buffers to come back and then queue the new buffers in one shot
        ///Stop all callbacks when this is ongoing
        mFlushBuffers = true;

        ///Register for the FLUSH event
        ///This method just inserts a message in Event Q, which is checked in the callback
        ///The sempahore passed is signalled by the callback
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                    OMX_EventCmdComplete,
                                    OMX_CommandFlush,
                                    OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                                    mUsePreviewSem);
        if(ret!=NO_ERROR)
            {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
            }
        ///Send FLUSH command to preview port
        eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp, OMX_CommandFlush,
                                                        OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                                                        NULL);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandFlush)- %x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        ///Wait for the FLUSH event to occur
        ret = mUsePreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Flush event received");
            }
        else
            {
            CAMHAL_LOGEA("Timeout expired on flush event");
            goto EXIT;
            }

        ///If flush has already happened, we need to update the pBuffer pointer in
        ///the buffer header and call OMX_FillThisBuffer to queue all the new buffers
        for(int index=0;index<num;index++)
            {
            CAMHAL_LOGDB("Queuing new buffers to Ducati 0x%x",((int32_t*)bufArr)[index]);

            mPreviewData->mBufferHeader[index]->pBuffer = (OMX_U8*)((int32_t*)bufArr)[index];

            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)mPreviewData->mBufferHeader[index]);

            if(eError!=OMX_ErrorNone)
                {
                CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

            }
        ///Once we queued new buffers, we set the flushBuffers flag to false
        mFlushBuffers = false;

        ret = ErrorUtils::omxToAndroidError(eError);

        ///Return from here
        return ret;
        }

    ret = setLDC(mIPP);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("setLDC() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    ret = setNSF(mIPP);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("setNSF() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    ret = setCaptureMode(mCapMode);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("setCaptureMode() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    CAMHAL_LOGDB("Camera Mode = %d", mCapMode);

    ret = setFormat(OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW, *mPreviewData);
    if ( ret != NO_ERROR )
        {
        CAMHAL_LOGEB("setFormat() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    setVFramerate(mPreviewData->mMinFrameRate, mPreviewData->mMaxFrameRate);

    if(mCapMode == OMXCameraAdapter::VIDEO_MODE)
        {
        ///Enable/Disable Video Noise Filter
        ret = enableVideoNoiseFilter(mVnfEnabled);
        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VNF %x", ret);
            return ret;
            }

        ///Enable/Disable Video Stabilization
        ret = enableVideoStabilization(mVstabEnabled);
        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
            return ret;
            }
        }
    else
        {
        mVnfEnabled = false;
        ret = enableVideoNoiseFilter(mVnfEnabled);
        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VNF %x", ret);
            return ret;
            }
        mVstabEnabled = false;
        ///Enable/Disable Video Stabilization
        ret = enableVideoStabilization(mVstabEnabled);
        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
            return ret;
            }
        }
    ///Register for IDLE state switch event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                mUsePreviewSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }


    ///Once we get the buffers, move component state to idle state and pass the buffers to OMX comp using UseBuffer
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp , OMX_CommandStateSet, OMX_StateIdle, NULL);

    CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandStateSet) 0x%x", eError);
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    for(int index=0;index<num;index++)
        {
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        OMX_U8* ptr = (((uint32_t)bufArr) != NULL)?(OMX_U8*)buffers[index]:(OMX_U8*)0x0;
        CAMHAL_LOGDB("OMX_UseBuffer(0x%x)", ptr);
        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mPrevPortIndex,
                                0,
                                mPreviewData->mBufSize,
                                (OMX_U8*)ptr);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_UseBuffer-0x%x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        //pBufferHdr->pAppPrivate =  (OMX_PTR)pBufferHdr;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0 ;
        pBufferHdr->nVersion.s.nStep =  0;
        mPreviewData->mBufferHeader[index] = pBufferHdr;
        }

    if ( mMeasurementEnabled )
        {

        for( int i = 0; i < num; i++ )
            {
            OMX_BUFFERHEADERTYPE *pBufHdr;
            eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                    &pBufHdr,
                                    mCameraAdapterParameters.mMeasurementPortIndex,
                                    0,
                                    measurementData->mBufSize,
                                    (OMX_U8*)(mPreviewDataBuffers[i]));

             if ( eError == OMX_ErrorNone )
                {
                pBufHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
                pBufHdr->nVersion.s.nVersionMajor = 1 ;
                pBufHdr->nVersion.s.nVersionMinor = 1 ;
                pBufHdr->nVersion.s.nRevision = 0 ;
                pBufHdr->nVersion.s.nStep =  0;
                pBufHdr->nFlags |= OMX_BUFFERHEADERFLAG_MODIFIED;
                measurementData->mBufferHeader[i] = pBufHdr;
                }
            else
                {
                CAMHAL_LOGEB("OMX_UseBuffer -0x%x", eError);
                ret = -1;
                break;
                }
            }

        }

    CAMHAL_LOGDA("Waiting LOADED->IDLE state transition");
    ///Wait for state to switch to idle
    ret = mUsePreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("LOADED->IDLE state changed");
        }
    else
        {
        CAMHAL_LOGEA("Timeout expired on LOADED->IDLE state change");
        goto EXIT;
        }

    mComponentState = OMX_StateIdle;

    LOG_FUNCTION_NAME_EXIT
    return (ret | ErrorUtils::omxToAndroidError(eError));

    ///If there is any failure, we reach here.
    ///Here, we do any resource freeing and convert from OMX error code to Camera Hal error code
    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);

    LOG_FUNCTION_NAME_EXIT

    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::UseBuffersCapture(void* bufArr, int num)
{
    LOG_FUNCTION_NAME

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError;
    OMXCameraPortParameters * imgCaptureData = NULL;
    uint32_t *buffers = (uint32_t*)bufArr;
    OMXCameraPortParameters cap;

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    ret = mUseCaptureSemPortDisable.Create(0);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEA("Semaphore mUseCaptureSemPortDisable failed to initialize!");
        goto EXIT;
        }
     ret = mUseCaptureSemPortEnable.Create(0);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEA("Semaphore mUseCaptureSemPortEnable  failed to initialize!");
        goto EXIT;
        }

    if ( mCapturing )
        {
        ///Register for Image port Disable event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                    OMX_EventCmdComplete,
                                    OMX_CommandPortDisable,
                                    mCameraAdapterParameters.mImagePortIndex,
                                    mUseCaptureSemPortDisable);

        ///Disable Capture Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                    OMX_CommandPortDisable,
                                    mCameraAdapterParameters.mImagePortIndex,
                                    NULL);

        CAMHAL_LOGDA("Waiting for port disable");
        //Wait for the image port enable event
        ret = mUseCaptureSemPortDisable.WaitTimeout(OMX_CMD_TIMEOUT);
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Port disabled");
            }
        else
            {
            CAMHAL_LOGEA("Timeout expired on port disable");
            goto EXIT;
            }
        }

    imgCaptureData->mNumBufs = num;

    //TODO: Support more pixelformats

    LOGE("Params Width = %d", (int)imgCaptureData->mWidth);
    LOGE("Params Height = %d", (int)imgCaptureData->mWidth);

    ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
    if ( ret != NO_ERROR )
        {
        CAMHAL_LOGEB("setFormat() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    ret = setThumbnailParams(mThumbWidth, mThumbHeight, mThumbQuality);
    if ( NO_ERROR != ret)
        {
        CAMHAL_LOGEB("Error configuring thumbnail size %x", ret);
        return ret;
        }

    ret = setExposureBracketing( mExposureBracketingValues,
                                 mExposureBracketingValidEntries, mBurstFrames);
    if ( ret != NO_ERROR )
        {
        CAMHAL_LOGEB("setExposureBracketing() failed %d", ret);
        return ret;
        }

    ret = setImageQuality(mPictureQuality);
    if ( NO_ERROR != ret)
        {
        CAMHAL_LOGEB("Error configuring image quality %x", ret);
        return ret;
        }

    if(mHDRCaptureEnabled)
        {
        mHDRInterface->setImageSize(imgCaptureData->mWidth, imgCaptureData->mHeight);
        }

    ///Register for Image port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mImagePortIndex,
                                mUseCaptureSemPortEnable);

    ///Enable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mImagePortIndex,
                                NULL);

    for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++ )
    {
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d", (unsigned int)buffers[index], (int)imgCaptureData->mBufSize);

        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mImagePortIndex,
                                0,
                                mCaptureBuffersLength,
                                (OMX_U8*)buffers[index]);

        CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);

        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        pBufferHdr->pAppPrivate = (OMX_PTR) index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = 1 ;
        pBufferHdr->nVersion.s.nVersionMinor = 1 ;
        pBufferHdr->nVersion.s.nRevision = 0;
        pBufferHdr->nVersion.s.nStep =  0;
        imgCaptureData->mBufferHeader[index] = pBufferHdr;
    }

    //Wait for the image port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    ret = mUseCaptureSemPortEnable.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( ret == NO_ERROR )
        {
        CAMHAL_LOGDA("Port enabled");
        }
    else
        {
        CAMHAL_LOGDA("Timeout expired on port enable");
        goto EXIT;
        }

    if ( NO_ERROR == ret )
        {
        ret = setupEXIF();
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
            }
        }


    mCapturedFrames = mBurstFrames;
    mCaptureConfigured = true;


    EXIT:

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::startSmoothZoom(int targetIdx)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mZoomLock);

    CAMHAL_LOGDB("Start smooth zoom target = %d, mCurrentIdx = %d", targetIdx, mCurrentZoomIdx);

    if ( ( targetIdx >= 0 ) && ( targetIdx < ZOOM_STAGES ) )
        {
        mTargetZoomIdx = targetIdx;
        mZoomParameterIdx = mCurrentZoomIdx;
        mSmoothZoomEnabled = true;
        }
    else
        {
        CAMHAL_LOGEB("Smooth value out of range %d!", targetIdx);
        ret = -EINVAL;
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::stopSmoothZoom()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mZoomLock);

    if ( mSmoothZoomEnabled )
        {
        mTargetZoomIdx = mCurrentZoomIdx;
        mReturnZoomStatus = true;
        CAMHAL_LOGDB("Stop smooth zoom mCurrentZoomIdx = %d, mTargetZoomIdx = %d", mCurrentZoomIdx, mTargetZoomIdx);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::startPreview()
{

    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME

    ret = doStartPreview();

    LOG_FUNCTION_NAME_EXIT
    return ret;

}

status_t OMXCameraAdapter::doStartPreview()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if(!mPreviewInProgress)
        {
        //Preview start is cancelled
        return NO_ERROR;
        }

    mOnlyOnce = true;

    Semaphore eventSem;
    ret = mStartPreviewSem.Create(0);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in creating semaphore %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    if(mComponentState!=OMX_StateIdle)
        {
        CAMHAL_LOGEA("Calling UseBuffersPreview() when not in IDLE state");
        LOG_FUNCTION_NAME_EXIT
        return NO_INIT;
        }

    LOG_FUNCTION_NAME

    OMXCameraPortParameters *mPreviewData = NULL;
    OMXCameraPortParameters *measurementData = NULL;

    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];

    ///Register for EXECUTING state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateExecuting,
                                mStartPreviewSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }


    ///Switch to EXECUTING state
    ret = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandStateSet,
                                OMX_StateExecuting,
                                NULL);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_StateExecuting)-0x%x", eError);
        }
    if( NO_ERROR == ret)
        {
        Mutex::Autolock lock(mLock);
        mPreviewing = true;
        }
    else
        {
        goto EXIT;
        }

    CAMHAL_LOGDA("+Waiting for component to go into EXECUTING state");
    ret = mStartPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("+Great. Component went into executing state!!");
        }
    else
        {
        CAMHAL_LOGDA("Timeout expired on executing state switch!");
        goto EXIT;
        }

    ///Queue all the buffers on preview port
    for(int index=0;index< mPreviewData->mNumBufs;index++)
        {
        mPreviewData->mBufferHeader[index]->nFlags |= OMX_BUFFERHEADERFLAG_MODIFIED;
        CAMHAL_LOGDB("Queuing buffer on Preview port - 0x%x", (uint32_t)mPreviewData->mBufferHeader[index]->pBuffer);
        if(mPreviewData->mBufferHeader[index]->pBuffer!=NULL)
            {
        eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                    (OMX_BUFFERHEADERTYPE*)mPreviewData->mBufferHeader[index]);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }
        }

    if ( mMeasurementEnabled )
        {

        for(int index=0;index< mPreviewData->mNumBufs;index++)
            {
            CAMHAL_LOGDB("Queuing buffer on Measurement port - 0x%x", (uint32_t) measurementData->mBufferHeader[index]->pBuffer);
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                            (OMX_BUFFERHEADERTYPE*) measurementData->mBufferHeader[index]);
            if(eError!=OMX_ErrorNone)
                {
                CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
                }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }

        }


    if ( mPending3Asettings )
        apply3Asettings(mParameters3A);

    mComponentState = OMX_StateExecuting;

    //reset frame rate estimates
    mFPS = 0.0f;
    mLastFPS = 0.0f;
    mFrameCount = 0;
    mLastFrameCount = 0;
    mIter = 1;
    mLastFPSTime = systemTime();

    LOG_FUNCTION_NAME_EXIT

    return ret;

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);

    LOG_FUNCTION_NAME_EXIT

    return (ret | ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::stopPreview()
{
    LOG_FUNCTION_NAME

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    OMXCameraPortParameters *mCaptureData , *mPreviewData, *measurementData;
    mCaptureData = mPreviewData = measurementData = NULL;

    if(!mPreviewInProgress || !mPreviewing)
        {
        return NO_ERROR;
        }

    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    mCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    measurementData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mMeasurementPortIndex];

    ret = cancelAutoFocus();
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error canceling autofocus %d", ret);
        // Error, but we probably still want to continue to stop preview
    }

    ret = mStopPreviewSem.Create(0);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("Error in creating semaphore %d", ret);
        LOG_FUNCTION_NAME_EXIT
        return ret;
        }

    if ( mComponentState != OMX_StateExecuting )
        {
        CAMHAL_LOGEA("Calling StopPreview() when not in EXECUTING state");
        LOG_FUNCTION_NAME_EXIT
        return NO_INIT;
        }

    mComponentState = OMX_StateLoaded;
    ///Clear the previewing flag, we are no longer previewing
    mPreviewing = false;
    mPreviewInProgress = false;


    CAMHAL_LOGEB("Average framerate: %f", mFPS);

    ///Register for EXECUTING state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                mStopPreviewSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                                OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_StateIdle) - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    ///Wait for the EXECUTING ->IDLE transition to arrive

    CAMHAL_LOGDA("EXECUTING->IDLE state changed");
    ret = mStopPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("EXECUTING->IDLE state changed");
        }
    else
        {
        CAMHAL_LOGEA("Timeout expired on EXECUTING->IDLE state changed");
        goto EXIT;
        }

    ///Register for LOADED state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateLoaded,
                                mStopPreviewSem);
    if(ret!=NO_ERROR)
        {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
        }

    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                            OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(eError!=OMX_ErrorNone)
        {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_StateLoaded) - %x", eError);
        }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    ///Free the OMX Buffers
    for ( int i = 0 ; i < mPreviewData->mNumBufs ; i++ )
        {
        eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                            mCameraAdapterParameters.mPrevPortIndex,
                            mPreviewData->mBufferHeader[i]);
        if(eError!=OMX_ErrorNone)
            {
            CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
            }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

    if ( mMeasurementEnabled )
        {

            for ( int i = 0 ; i < measurementData->mNumBufs ; i++ )
                {
                eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                    mCameraAdapterParameters.mMeasurementPortIndex,
                                    measurementData->mBufferHeader[i]);
                if(eError!=OMX_ErrorNone)
                    {
                    CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
                    }
                GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
                }

            {
            Mutex::Autolock lock(mPreviewDataBufferLock);
            mPreviewDataBuffersAvailable.clear();
            }

        }

    ///Wait for the IDLE -> LOADED transition to arrive
    CAMHAL_LOGDA("IDLE->LOADED state changed");
    ret = mStopPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("IDLE->LOADED state changed");
        }
    else
        {
        CAMHAL_LOGEA("Timeout expired on IDLE->LOADED state change");
        goto EXIT;
        }

        {
        Mutex::Autolock lock(mPreviewBufferLock);
        ///Clear all the available preview buffers
        mPreviewBuffersAvailable.clear();
        }



    LOG_FUNCTION_NAME_EXIT

    return (ret | ErrorUtils::omxToAndroidError(eError));

    EXIT:
    CAMHAL_LOGEB("Exiting function because of eError= %x", eError);

    LOG_FUNCTION_NAME_EXIT

    return (ret | ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::setTimeOut(int sec)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(gAdapterLock);

    if( (mComponentState == OMX_StateInvalid) || (0 == sec))
      {
        delete this;
        gCameraAdapter = NULL;
        return 0;
      }
    else
      {
      if(!gWakeLockAcquired)
        {
        //Acquire wakelock to prevent system from suspending till we release the adapter
        acquire_wake_lock(PARTIAL_WAKE_LOCK, kCameraAdapterWakelock);
        gWakeLockAcquired = true;
        }

      struct sigaction sa;
      sa.sa_flags = SA_SIGINFO;
      sa.sa_sigaction = adapterTimeout;
      sigemptyset(&sa.sa_mask);
      if (sigaction(SIGALRM, &sa, NULL) == -1)
         {
         ///Delete the adapter but return error
         delete this;
         gCameraAdapter = NULL;
         release_wake_lock(kCameraAdapterWakelock);
         gWakeLockAcquired = false;
         return -1;
         }


      /* Create the timer */
      struct sigevent sev;
      sev.sigev_notify = SIGEV_SIGNAL;
      sev.sigev_signo = SIGALRM;
      sev.sigev_value.sival_ptr = &gAdapterTimeoutTimer;
      if (timer_create(CLOCK_REALTIME, &sev, &gAdapterTimeoutTimer) == -1)
        {
        ///Delete the adapter but return error
        delete this;
        gCameraAdapter = NULL;
        release_wake_lock(kCameraAdapterWakelock);
        gWakeLockAcquired = false;
        return -1;
        }

      /* Start the timer */
      struct itimerspec its;
      its.it_value.tv_sec = sec;
      its.it_value.tv_nsec = 0;
      its.it_interval.tv_sec = its.it_value.tv_sec;
      its.it_interval.tv_nsec = its.it_value.tv_nsec;

      if (timer_settime(gAdapterTimeoutTimer, 0, &its, NULL) == -1)
        {
        ///Delete the adapter but return error
        delete this;
        gCameraAdapter = NULL;
        release_wake_lock(kCameraAdapterWakelock);
        gWakeLockAcquired = false;
        return -1;
        }

      }

    //At this point ErrorNotifier becomes invalid
    mErrorNotifier = NULL;

    LOG_FUNCTION_NAME_EXIT

    return BaseCameraAdapter::setTimeOut(sec);
}

status_t OMXCameraAdapter::cancelTimeOut()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if(gAdapterTimeoutTimer!=-1)
        {
        //Cancel and delete the timeout timer
        timer_delete(gAdapterTimeoutTimer);
        gAdapterTimeoutTimer = -1;
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setThumbnailParams(unsigned int width, unsigned int height, unsigned int quality)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_THUMBNAILTYPE thumbConf;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT(thumbConf, OMX_PARAM_THUMBNAILTYPE);
        thumbConf.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexParamThumbnail, &thumbConf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while retrieving thumbnail size 0x%x", eError);
            ret = -1;
            }

        //CTS Requirement: width or height equal to zero should
        //result in absent EXIF thumbnail
        if ( ( 0 == width ) || ( 0 == height ) )
            {
            thumbConf.nWidth = mThumbRes[0].width;
            thumbConf.nHeight = mThumbRes[0].height;
            thumbConf.eCompressionFormat = OMX_IMAGE_CodingUnused;
            }
        else
            {
            thumbConf.nWidth = width;
            thumbConf.nHeight = height;
            thumbConf.nQuality = quality;
            thumbConf.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }

        CAMHAL_LOGDB("Thumbnail width = %d, Thumbnail Height = %d", width, height);

        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexParamThumbnail, &thumbConf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring thumbnail size 0x%x", eError);
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setImageQuality(unsigned int quality)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_QFACTORTYPE jpegQualityConf;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT(jpegQualityConf, OMX_IMAGE_PARAM_QFACTORTYPE);
        jpegQualityConf.nQFactor = quality;
        jpegQualityConf.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamQFactor, &jpegQualityConf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring jpeg Quality 0x%x", eError);
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

OMX_ERRORTYPE OMXCameraAdapter::setExposureMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSURECONTROLTYPE exp;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        eError = OMX_ErrorInvalidState;
        }

    if ( OMX_ErrorNone == eError )
        {

        if ( EXPOSURE_FACE_PRIORITY == Gen3A.Exposure )
                {
                //Disable Region priority and enable Face priority
                setAlgoPriority(REGION_PRIORITY, EXPOSURE_ALGO, false);
                setAlgoPriority(FACE_PRIORITY, EXPOSURE_ALGO, true);

                //Then set the mode to auto
                Gen3A.WhiteBallance = OMX_ExposureControlAuto;
                }
            else
                {
                //Disable Face and Region priority
                setAlgoPriority(FACE_PRIORITY, EXPOSURE_ALGO, false);
                setAlgoPriority(REGION_PRIORITY, EXPOSURE_ALGO, false);
                }

        OMX_INIT_STRUCT_PTR (&exp, OMX_CONFIG_EXPOSURECONTROLTYPE);
        exp.nPortIndex = OMX_ALL;
        exp.eExposureControl = (OMX_EXPOSURECONTROLTYPE)Gen3A.Exposure;

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposure, &exp);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring exposure mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera exposure mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return eError;
}

status_t OMXCameraAdapter::setAlgoPriority(AlgoPriority priority, Algorithm3A algo, bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_3A_REGION_PRIORITY regionPriority;
    OMX_TI_CONFIG_3A_FACE_PRIORITY facePriority;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {

        if ( FACE_PRIORITY == priority )
            {
            OMX_INIT_STRUCT_PTR (&facePriority, OMX_TI_CONFIG_3A_FACE_PRIORITY);
            facePriority.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

            if ( algo & WHITE_BALANCE_ALGO )
                {
                if ( enable )
                    {
                    facePriority.bAwbFaceEnable = OMX_TRUE;
                    }
                else
                    {
                    facePriority.bAwbFaceEnable = OMX_FALSE;
                    }
                }

            if ( algo & EXPOSURE_ALGO )
                {
                if ( enable )
                    {
                    facePriority.bAeFaceEnable = OMX_TRUE;
                    }
                else
                    {
                    facePriority.bAeFaceEnable = OMX_FALSE;
                    }
                }

            if ( algo & FOCUS_ALGO )
                {
                if ( enable )
                    {
                    facePriority.bAfFaceEnable= OMX_TRUE;
                    }
                else
                    {
                    facePriority.bAfFaceEnable = OMX_FALSE;
                    }
                }

            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexConfigFacePriority3a, &facePriority);
            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring face priority 0x%x", eError);
                }
            else
                {
                CAMHAL_LOGEA("Face priority for algorithms set successfully");
                }

            }
        else if ( REGION_PRIORITY == priority )
            {

            OMX_INIT_STRUCT_PTR (&regionPriority, OMX_TI_CONFIG_3A_REGION_PRIORITY);
            regionPriority.nPortIndex =  mCameraAdapterParameters.mImagePortIndex;

            if ( algo & WHITE_BALANCE_ALGO )
                {
                if ( enable )
                    {
                    regionPriority.bAwbRegionEnable= OMX_TRUE;
                    }
                else
                    {
                    regionPriority.bAwbRegionEnable = OMX_FALSE;
                    }
                }

            if ( algo & EXPOSURE_ALGO )
                {
                if ( enable )
                    {
                    regionPriority.bAeRegionEnable = OMX_TRUE;
                    }
                else
                    {
                    regionPriority.bAeRegionEnable = OMX_FALSE;
                    }
                }

            if ( algo & FOCUS_ALGO )
                {
                if ( enable )
                    {
                    regionPriority.bAfRegionEnable = OMX_TRUE;
                    }
                else
                    {
                    regionPriority.bAfRegionEnable = OMX_FALSE;
                    }
                }

            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexConfigRegionPriority3a, &regionPriority);
            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring region priority 0x%x", eError);
                }
            else
                {
                CAMHAL_LOGEA("Region priority for algorithms set successfully");
                }

            }

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::parseExpRange(const char *rangeStr, int * expRange, size_t count, size_t &validEntries)
{
    status_t ret = NO_ERROR;
    char *ctx, *expVal;
    char *tmp = NULL;
    size_t i = 0;

    LOG_FUNCTION_NAME

    if ( NULL == rangeStr )
        {
        return -EINVAL;
        }

    if ( NULL == expRange )
        {
        return -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        tmp = ( char * ) malloc( strlen(rangeStr) + 1 );

        if ( NULL == tmp )
            {
            CAMHAL_LOGEA("No resources for temporary buffer");
            return -1;
            }
        memset(tmp, '\0', strlen(rangeStr) + 1);

        }

    if ( NO_ERROR == ret )
        {
        strncpy(tmp, rangeStr, strlen(rangeStr) );
        expVal = strtok_r( (char *) tmp, CameraHal::PARAMS_DELIMITER, &ctx);

        i = 0;
        while ( ( NULL != expVal ) && ( i < count ) )
            {
            expRange[i] = atoi(expVal);
            expVal = strtok_r(NULL, CameraHal::PARAMS_DELIMITER, &ctx);
            i++;
            }
        validEntries = i;
        }

    if ( NULL != tmp )
        {
        free(tmp);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setExposureBracketing(int *evValues, size_t evCount, size_t frameCount)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAPTUREMODETYPE expCapMode;
    OMX_CONFIG_EXTCAPTUREMODETYPE extExpCapMode;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NULL == evValues )
        {
        CAMHAL_LOGEA("Exposure compensation values pointer is invalid");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&expCapMode, OMX_CONFIG_CAPTUREMODETYPE);
        expCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        if ( 0 == evCount && 0 == frameCount )
            {
            expCapMode.bFrameLimited = OMX_FALSE;
            }
        else
            {
            expCapMode.bFrameLimited = OMX_TRUE;
            expCapMode.nFrameLimit = frameCount;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCaptureMode, &expCapMode);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring capture mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera capture mode configured successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&extExpCapMode, OMX_CONFIG_EXTCAPTUREMODETYPE);
        extExpCapMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        if ( 0 == evCount )
            {
            extExpCapMode.bEnableBracketing = OMX_FALSE;
            }
        else
            {
            extExpCapMode.bEnableBracketing = OMX_TRUE;
            extExpCapMode.tBracketConfigType.eBracketMode = OMX_BracketExposureRelativeInEV;
            extExpCapMode.tBracketConfigType.nNbrBracketingValues = evCount - 1;
            }

        for ( unsigned int i = 0 ; i < evCount ; i++ )
            {
            extExpCapMode.tBracketConfigType.nBracketValues[i]  =  ( evValues[i] * ( 1 << Q16_OFFSET ) )  / 10;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexConfigExtCaptureMode, &extExpCapMode);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring extended capture mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Extended camera capture mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setFaceDetection(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXTRADATATYPE extraDataControl;
    OMX_CONFIG_OBJDETECTIONTYPE objDetection;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&objDetection, OMX_CONFIG_OBJDETECTIONTYPE);
        objDetection.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        if  ( enable )
            {
            objDetection.bEnable = OMX_TRUE;
            }
        else
            {
            objDetection.bEnable = OMX_FALSE;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexConfigImageFaceDetection, &objDetection);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring face detection 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGEA("Face detection configuring successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&extraDataControl, OMX_CONFIG_EXTRADATATYPE);
        extraDataControl.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        extraDataControl.eExtraDataType = OMX_FaceDetection;
        extraDataControl.eCameraView = OMX_2D;
        if  ( enable )
            {
            extraDataControl.bEnable = OMX_TRUE;
            }
        else
            {
            extraDataControl.bEnable = OMX_FALSE;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexConfigOtherExtraDataControl, &extraDataControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring face detection extra data 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGEA("Face detection extra data configuring successfully");
            }
        }

    if ( NO_ERROR == ret )
        {
        Mutex::Autolock lock(mFaceDetectionLock);
        mFaceDetectionRunning = enable;
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::detectFaces(OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_FACERESULT *faceResult;
    OMX_OTHER_EXTRADATATYPE *extraData;
    OMX_FACEDETECTIONTYPE *faceData;
    OMX_TI_PLATFORMPRIVATE *platformPrivate;

    LOG_FUNCTION_NAME

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -1;
        }

    if ( NULL == pBuffHeader )
        {
        CAMHAL_LOGEA("Invalid Buffer header");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {

        platformPrivate = (OMX_TI_PLATFORMPRIVATE *) (pBuffHeader->pPlatformPrivate);
        if ( NULL != platformPrivate )
            {

            if ( sizeof(OMX_TI_PLATFORMPRIVATE) == platformPrivate->nSize )
                {
                CAMHAL_LOGVB("Size = %d, sizeof = %d, pAuxBuf = 0x%x, pAuxBufSize= %d, pMetaDataBufer = 0x%x, nMetaDataSize = %d",
                                        platformPrivate->nSize,
                                        sizeof(OMX_TI_PLATFORMPRIVATE),
                                        platformPrivate->pAuxBuf1,
                                        platformPrivate->pAuxBufSize1,
                                        platformPrivate->pMetaDataBuffer,
                                        platformPrivate->nMetaDataSize
                                        );
                }
            else
                {
                CAMHAL_LOGEB("OMX_TI_PLATFORMPRIVATE size mismatch: expected = %d, received = %d",
                                            ( unsigned int ) sizeof(OMX_TI_PLATFORMPRIVATE),
                                            ( unsigned int ) platformPrivate->nSize);
                ret = -EINVAL;
                }
            }
        else
            {
            CAMHAL_LOGEA("Invalid OMX_TI_PLATFORMPRIVATE");
            ret = -EINVAL;
            }

    }

    if ( NO_ERROR == ret )
        {

        if ( 0 >= platformPrivate->nMetaDataSize )
            {
            CAMHAL_LOGEB("OMX_TI_PLATFORMPRIVATE nMetaDataSize is size is %d",
                                            ( unsigned int ) platformPrivate->nMetaDataSize);
            ret = -EINVAL;
            }

        }

    if ( NO_ERROR == ret )
        {

        extraData = (OMX_OTHER_EXTRADATATYPE *) (platformPrivate->pMetaDataBuffer);
        if ( NULL != extraData )
            {
            CAMHAL_LOGVB("Size = %d, sizeof = %d, eType = 0x%x, nDataSize= %d, nPortIndex = 0x%x, nVersion = 0x%x",
                                        extraData->nSize,
                                        sizeof(OMX_OTHER_EXTRADATATYPE),
                                        extraData->eType,
                                        extraData->nDataSize,
                                        extraData->nPortIndex,
                                        extraData->nVersion
                                        );
            }
        else
            {
            CAMHAL_LOGEA("Invalid OMX_OTHER_EXTRADATATYPE");
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {

        faceData = ( OMX_FACEDETECTIONTYPE * ) extraData->data;
        if ( NULL != faceData )
            {
            if ( sizeof(OMX_FACEDETECTIONTYPE) == faceData->nSize )
                {
                CAMHAL_LOGVB("Faces detected %d",
                                            faceData->ulFaceCount,
                                            faceData->nSize,
                                            sizeof(OMX_FACEDETECTIONTYPE),
                                            faceData->eCameraView,
                                            faceData->nPortIndex,
                                            faceData->nVersion);
                }
            else
                {
                CAMHAL_LOGEB("OMX_FACEDETECTIONTYPE size mismatch: expected = %d, received = %d",
                                            ( unsigned int ) sizeof(OMX_FACEDETECTIONTYPE),
                                            ( unsigned int ) faceData->nSize);
                ret = -EINVAL;
                }

            }
        else
            {
            CAMHAL_LOGEA("Invalid OMX_FACEDETECTIONTYPE");
            ret = -EINVAL;
            }

        }

    if ( NO_ERROR == ret )
        {
        ret = encodeFaceCoordinates(faceData, mFaceDectionResult, FACE_DETECTION_BUFFER_SIZE);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::encodeFaceCoordinates(const OMX_FACEDETECTIONTYPE *faceData, char *faceString, size_t faceStringSize)
{
    status_t ret = NO_ERROR;
    OMX_TI_FACERESULT *faceResult;
    size_t faceResultSize;
    int count = 0;
    char *p;

    LOG_FUNCTION_NAME

    if ( NULL == faceData )
        {
        CAMHAL_LOGEA("Invalid OMX_FACEDETECTIONTYPE parameter");
        ret = -EINVAL;
        }

    if ( NULL == faceString )
        {
        CAMHAL_LOGEA("Invalid faceString parameter");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {

        p = mFaceDectionResult;
        faceResultSize = faceStringSize;
        faceResult = ( OMX_TI_FACERESULT * ) faceData->tFacePosition;
        if ( 0 < faceData->ulFaceCount )
            {
            for ( int i = 0  ; i < faceData->ulFaceCount ; i++)
                {

                if ( mFaceDetectionThreshold <= faceResult->nScore )
                    {
                    CAMHAL_LOGVB("Face %d: left = %d, top = %d, width = %d, height = %d", i,
                                                           ( unsigned int ) faceResult->nLeft,
                                                           ( unsigned int ) faceResult->nTop,
                                                           ( unsigned int ) faceResult->nWidth,
                                                           ( unsigned int ) faceResult->nHeight);

                    count = snprintf(p, faceResultSize, "%d,%dx%d,%dx%d,",
                                                           ( unsigned int ) faceResult->nOrientationRoll,
                                                           ( unsigned int ) faceResult->nLeft,
                                                           ( unsigned int ) faceResult->nTop,
                                                           ( unsigned int ) faceResult->nWidth,
                                                           ( unsigned int ) faceResult->nHeight);
                    }

                p += count;
                faceResultSize -= count;
                faceResult++;
                }
            }
        else
            {
            memset(mFaceDectionResult, '\0', faceStringSize);
            }
        }
    else
        {
        memset(mFaceDectionResult, '\0', faceStringSize);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

OMX_ERRORTYPE OMXCameraAdapter::setScene(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SCENEMODETYPE scene;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        eError = OMX_ErrorInvalidState;
        }

    if ( OMX_ErrorNone == eError )
        {
        OMX_INIT_STRUCT_PTR (&scene, OMX_CONFIG_SCENEMODETYPE);
        scene.nPortIndex = OMX_ALL;
        scene.eSceneMode = ( OMX_SCENEMODETYPE ) Gen3A.SceneMode;

        CAMHAL_LOGEB("Configuring scene mode 0x%x", scene.eSceneMode);
        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexConfigSceneMode, &scene);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring scene mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera scene configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return eError;
}

OMX_ERRORTYPE OMXCameraAdapter::setFlashMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE flash;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&flash, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
        flash.nPortIndex = OMX_ALL;
        flash.eFlashControl = ( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode;

        CAMHAL_LOGEB("Configuring flash mode 0x%x", flash.eFlashControl);
        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigFlashControl, &flash);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring flash mode 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("Camera flash mode configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return eError;
}


status_t OMXCameraAdapter::setPictureRotation(unsigned int degree)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_ROTATIONTYPE rotation;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT(rotation, OMX_CONFIG_ROTATIONTYPE);

        rotation.nRotation = degree;
        rotation.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonRotate, &rotation);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring rotation 0x%x", eError);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setSensorOverclock(bool enable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in loaded state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);

        if ( enable )
            {
            bOMX.bEnabled = OMX_TRUE;
            }
        else
            {
            bOMX.bEnabled = OMX_FALSE;
            }

        CAMHAL_LOGEB("Configuring Sensor overclock mode 0x%x", bOMX.bEnabled);
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexParamSensorOverClockMode, &bOMX);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting Sensor overclock 0x%x", eError);
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setLDC(OMXCameraAdapter::IPPMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in loaded state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);

        switch ( mode )
            {
            case OMXCameraAdapter::IPP_LDCNSF:
            case OMXCameraAdapter::IPP_LDC:
                {
                bOMX.bEnabled = OMX_TRUE;
                break;
                }
            case OMXCameraAdapter::IPP_NONE:
            case OMXCameraAdapter::IPP_NSF:
            default:
                {
                bOMX.bEnabled = OMX_FALSE;
                break;
                }
            }

        CAMHAL_LOGEB("Configuring LDC mode 0x%x", bOMX.bEnabled);
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexParamLensDistortionCorrection, &bOMX);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEA("Error while setting LDC");
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setNSF(OMXCameraAdapter::IPPMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_ISONOISEFILTERTYPE nsf;

    LOG_FUNCTION_NAME

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in loaded state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&nsf, OMX_PARAM_ISONOISEFILTERTYPE);

        switch ( mode )
            {
            case OMXCameraAdapter::IPP_LDCNSF:
            case OMXCameraAdapter::IPP_NSF:
                {
                nsf.eMode = OMX_ISONoiseFilterModeOn;
                break;
                }
            case OMXCameraAdapter::IPP_LDC:
            case OMXCameraAdapter::IPP_NONE:
            default:
                {
                nsf.eMode = OMX_ISONoiseFilterModeOff;
                break;
                }
            }

        CAMHAL_LOGEB("Configuring NSF mode 0x%x", nsf.eMode);
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexParamHighISONoiseFiler, &nsf);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEA("Error while setting NSF");
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

int OMXCameraAdapter::getRevision()
{
    LOG_FUNCTION_NAME

    LOG_FUNCTION_NAME_EXIT

    return mCompRevision.nVersion;
}

status_t OMXCameraAdapter::printComponentVersion(OMX_HANDLETYPE handle)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_VERSIONTYPE compVersion;
    char compName[OMX_MAX_STRINGNAME_SIZE];
    char *currentUUID = NULL;
    size_t offset = 0;

    LOG_FUNCTION_NAME

    if ( NULL == handle )
        {
        CAMHAL_LOGEB("Invalid OMX Handle =0x%x",  ( unsigned int ) handle);
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        eError = OMX_GetComponentVersion(handle,
                                      compName,
                                      &compVersion,
                                      &mCompRevision,
                                      &mCompUUID
                                    );
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_GetComponentVersion returned 0x%x", eError);
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDB("OMX Component name: [%s]", compName);
        CAMHAL_LOGDB("OMX Component version: [%u]", ( unsigned int ) compVersion.nVersion);
        CAMHAL_LOGDB("Spec version: [%u]", ( unsigned int ) mCompRevision.nVersion);
        CAMHAL_LOGDB("Git Commit ID: [%s]", mCompUUID);
        currentUUID = ( char * ) mCompUUID;
        }

    if ( NULL != currentUUID )
        {
        offset = strlen( ( const char * ) mCompUUID) + 1;
        if ( offset < OMX_MAX_STRINGNAME_SIZE )
            {
            currentUUID += offset;
            CAMHAL_LOGDB("Git Branch: [%s]", currentUUID);
            }
        else
            {
            ret = -1;
            }
    }

    if ( NO_ERROR == ret )
        {
        offset += strlen( ( const char * ) currentUUID) + 1;

        if ( offset < OMX_MAX_STRINGNAME_SIZE )
            {
            currentUUID += offset;
            CAMHAL_LOGDB("Build date and time: [%s]", currentUUID);
            }
        else
            {
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
        offset += strlen( ( const char * ) currentUUID) + 1;

        if ( offset < OMX_MAX_STRINGNAME_SIZE )
            {
            currentUUID += offset;
            CAMHAL_LOGDB("Build description: [%s]", currentUUID);
            }
        else
            {
            ret = -1;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setGBCE(OMXCameraAdapter::BrightnessMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE bControl;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bControl,
                                               OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE);

        bControl.nPortIndex = OMX_ALL;

        switch ( mode )
            {
            case OMXCameraAdapter::BRIGHTNESS_ON:
                {
                bControl.eControl = OMX_TI_BceModeOn;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_AUTO:
                {
                bControl.eControl = OMX_TI_BceModeAuto;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_OFF:
            default:
                {
                bControl.eControl = OMX_TI_BceModeOff;
                break;
                }
            }

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigGlobalBrightnessContrastEnhance,
                                               &bControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while setting GBCE 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDB("GBCE configured successfully 0x%x", mode);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setGLBCE(OMXCameraAdapter::BrightnessMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE bControl;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&bControl,
                                               OMX_TI_CONFIG_LOCAL_AND_GLOBAL_BRIGHTNESSCONTRASTTYPE);

        bControl.nPortIndex = OMX_ALL;

        switch ( mode )
            {
            case OMXCameraAdapter::BRIGHTNESS_ON:
                {
                bControl.eControl = OMX_TI_BceModeOn;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_AUTO:
                {
                bControl.eControl = OMX_TI_BceModeAuto;
                break;
                }
            case OMXCameraAdapter::BRIGHTNESS_OFF:
            default:
                {
                bControl.eControl = OMX_TI_BceModeOff;
                break;
                }
            }

        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                               ( OMX_INDEXTYPE ) OMX_TI_IndexConfigLocalBrightnessContrastEnhance,
                                               &bControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configure GLBCE 0x%x", eError);
            }
        else
            {
            CAMHAL_LOGDA("GLBCE configured successfully");
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setCaptureMode(OMXCameraAdapter::CaptureMode mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CAMOPERATINGMODETYPE camMode;
    OMX_CONFIG_BOOLEANTYPE bCAC;

    LOG_FUNCTION_NAME

    OMX_INIT_STRUCT_PTR (&bCAC, OMX_CONFIG_BOOLEANTYPE);
    bCAC.bEnabled = OMX_FALSE;

    if (mHDRCaptureEnabled && (OMXCameraAdapter::VIDEO_MODE != mode))
    {
        mode = OMXCameraAdapter::HIGH_QUALITY_NONZSL;
    }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&camMode, OMX_CONFIG_CAMOPERATINGMODETYPE);
        if ( mSensorIndex == OMX_TI_StereoSensor )
            {
            CAMHAL_LOGDA("Camera mode: STEREO");
            camMode.eCamOperatingMode = OMX_CaptureStereoImageCapture;
            }
        else if ( OMXCameraAdapter::HIGH_SPEED == mode )
            {
            CAMHAL_LOGDA("Camera mode: HIGH SPEED");
            camMode.eCamOperatingMode = OMX_CaptureImageHighSpeedTemporalBracketing;
            }
        else if( OMXCameraAdapter::HIGH_QUALITY == mode )
            {
            CAMHAL_LOGDA("Camera mode: HIGH QUALITY: setting OMX_CaptureImageProfileBasePlusSOC");
            camMode.eCamOperatingMode = OMX_CaptureImageProfileBasePlusSOC;
            bCAC.bEnabled = OMX_TRUE;
            }
        else if( OMXCameraAdapter::VIDEO_MODE == mode )
            {
            CAMHAL_LOGDA("Camera mode: VIDEO MODE");
            camMode.eCamOperatingMode = OMX_CaptureVideo;
            }
        else if( OMXCameraAdapter::HIGH_QUALITY_NONZSL == mode )
            {
            if ( mSensorIndex == OMX_PrimarySensor )
                {
                CAMHAL_LOGDA("Camera mode: HIGH QUALITY_NONZSL for primary sensor uses OMX_CaptureImageProfileBase");
                camMode.eCamOperatingMode = OMX_CaptureImageProfileBase;
                }
            else if ( mSensorIndex == OMX_SecondarySensor )
                {
                CAMHAL_LOGDA("Camera mode: HIGH QUALITY_NONZSL for secondary sensor uses OMX_CaptureImageProfileBasePlusSOC");
                camMode.eCamOperatingMode = OMX_CaptureImageProfileBasePlusSOC;
                }
            else
                {
                CAMHAL_LOGDA("Camera mode: HIGH QUALITY_NONZSL for another future sensor uses default OMX_CaptureImageProfileBase");
                camMode.eCamOperatingMode = OMX_CaptureImageProfileBase;
                }
            bCAC.bEnabled = OMX_TRUE;
            }
        else if( OMXCameraAdapter::HIGH_QUALITY_NOTSET == mode )
            {
            if ( mSensorIndex == OMX_PrimarySensor )
                {
                CAMHAL_LOGDA("Camera mode not selected, for primary sensor use OMX_CaptureImageProfileBase");
                camMode.eCamOperatingMode = OMX_CaptureImageProfileBase;
                }
            else if ( mSensorIndex == OMX_SecondarySensor )
                {
                CAMHAL_LOGDA("Camera mode not selected, for secondary sensor use OMX_CaptureImageProfileBasePlusSOC");
                camMode.eCamOperatingMode = OMX_CaptureImageProfileBasePlusSOC;
                }
            else
                {
                CAMHAL_LOGDA("Camera mode not selected, for another future sensor use default OMX_CaptureImageProfileBase");
                camMode.eCamOperatingMode = OMX_CaptureImageProfileBase;
                }
            bCAC.bEnabled = OMX_TRUE;
            }
        else
            {
            CAMHAL_LOGEA("Camera mode: INVALID mode passed!");
            return BAD_VALUE;
            }

        if(ret != -1)
            {
            eError =  OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexCameraOperatingMode, &camMode);
            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring camera mode 0x%x", eError);
                ret = -1;
                }
            else
                {
                CAMHAL_LOGDA("Camera mode configured successfully");
                }
            }

        if(ret != -1)
            {
            //Configure CAC
            eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                    ( OMX_INDEXTYPE ) OMX_IndexConfigChromaticAberrationCorrection, &bCAC);
            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring CAC 0x%x", eError);
                ret = -1;
                }
            else
                {
                CAMHAL_LOGDA("CAC configured successfully");
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::doZoom(int index)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SCALEFACTORTYPE zoomControl;
    static int prevIndex = 0;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if (  ( 0 > index) || ( ( ZOOM_STAGES - 1 ) < index ) )
        {
        CAMHAL_LOGEB("Zoom index %d out of range", index);
        ret = -EINVAL;
        }

    if ( prevIndex == index )
        {
        return NO_ERROR;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&zoomControl, OMX_CONFIG_SCALEFACTORTYPE);
        zoomControl.nPortIndex = OMX_ALL;
        zoomControl.xHeight = ZOOM_STEPS[index];
        zoomControl.xWidth = ZOOM_STEPS[index];

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonDigitalZoom, &zoomControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while applying digital zoom 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Digital zoom applied successfully");
            prevIndex = index;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::parseTouchFocusPosition(const char *pos, unsigned int &posX, unsigned int &posY)
{

    status_t ret = NO_ERROR;
    char *ctx, *pX, *pY;
    const char *sep = ",";

    LOG_FUNCTION_NAME

    if ( NULL == pos )
        {
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        pX = strtok_r( (char *) pos, sep, &ctx);

        if ( NULL != pX )
            {
            posX = atoi(pX);
            }
        else
            {
            CAMHAL_LOGEB("Invalid touch focus position %s", pos);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {
        pY = strtok_r(NULL, sep, &ctx);

        if ( NULL != pY )
            {
            posY = atoi(pY);
            }
        else
            {
            CAMHAL_LOGEB("Invalid touch focus position %s", pos);
            ret = -EINVAL;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setTouchFocus(unsigned int posX, unsigned int posY, size_t width, size_t height)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXTFOCUSREGIONTYPE touchControl;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&touchControl, OMX_CONFIG_EXTFOCUSREGIONTYPE);
        touchControl.nLeft = ( posX * TOUCH_FOCUS_RANGE ) / width;
        touchControl.nTop =  ( posY * TOUCH_FOCUS_RANGE ) / height;
        touchControl.nWidth = 0;
        touchControl.nHeight = 0;

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexConfigExtFocusRegion, &touchControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while configuring touch focus 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDB("Touch focus nLeft = %d, nTop = %d configured successfuly", ( int ) touchControl.nLeft, ( int ) touchControl.nTop);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;
    Message msg = {0,0,0,0,0,0};

    LOG_FUNCTION_NAME

    {
        Mutex::Autolock lock(mFocusLock);
        mFocusStarted = true;
    }

    msg.command = CommandHandler::CAMERA_PERFORM_AUTOFOCUS;
    msg.arg1 = mErrorNotifier;
    ret = mCommandHandler->put(&msg);

    LOG_FUNCTION_NAME

    return ret;
}

status_t OMXCameraAdapter::doAutoFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusControl;
    Semaphore eventSem;

    LOG_FUNCTION_NAME
    
    mFocusLock.lock();

    // Motorola specific - begin
    // Disabling doAutoFocus when "focus-mode" is "fixed".
    // The LUT in General3A_Settings.h sets
    // OMX_IMAGE_FocusControlOff when the "focus-mode" is set to "fixed".
    if (mParameters3A.Focus == OMX_IMAGE_FocusControlOff)
        {
        mFocusStarted = true;
        returnFocusStatus(false);
        mFocusLock.unlock();
        return NO_ERROR;
        }
    // Motorola specific - end

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        mFocusStarted = false;
        ret = -1;
        }

    if ( !mFocusStarted )
        {
        CAMHAL_LOGVA("Focus canceled before we could start");
        ret = NO_ERROR;
        mFocusLock.unlock();
        return ret;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&focusControl, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
        focusControl.eFocusControl = ( OMX_IMAGE_FOCUSCONTROLTYPE ) mParameters3A.Focus;
        //If touch AF is set, then necessary configuration first
        if ( FOCUS_REGION_PRIORITY == focusControl.eFocusControl )
            {

            //Disable face priority first
            setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, false);

            //Enable region algorithm priority
            setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, true);

            //Set position
            OMXCameraPortParameters * mPreviewData = NULL;
            mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
            setTouchFocus(mTouchFocusPosX, mTouchFocusPosY, mPreviewData->mWidth, mPreviewData->mHeight);

            //Do normal focus afterwards
            focusControl.eFocusControl = ( OMX_IMAGE_FOCUSCONTROLTYPE ) OMX_IMAGE_FocusControlExtended;

            }
        else if ( FOCUS_FACE_PRIORITY == focusControl.eFocusControl )
            {

            //Disable region priority first
            setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, false);

            //Enable face algorithm priority
            setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, true);

            //Do normal focus afterwards
            focusControl.eFocusControl = ( OMX_IMAGE_FOCUSCONTROLTYPE ) OMX_IMAGE_FocusControlExtended;

            }
        else
            {

            //Disable both region and face priority
            setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, false);

            setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, false);

            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focusControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while starting focus 0x%x", eError);
            mFocusStarted = false;
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Autofocus started successfully");

            mFocusStarted = true;
            }
        }

        // We will send a callback to the application for both auto focus and CAF
        if ( mParameters3A.Focus != OMX_IMAGE_FocusControlAutoInfinity ) 
        {
        if ( NO_ERROR == ret )
            {
            ret = eventSem.Create(0);
            }

        if ( NO_ERROR == ret )
            {
            ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                        (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                                        OMX_ALL,
                                        OMX_IndexConfigCommonFocusStatus,
                                        eventSem);
            }

        if ( NO_ERROR == ret )
            {
            ret = setFocusCallback(true);
            }

        if ( NO_ERROR == ret )
            {
            ret = eventSem.WaitTimeout(AF_CALLBACK_TIMEOUT);
            //Disable auto focus callback from Ducati
            ret |= setFocusCallback(false);
            //Signal a dummy AF event so that in case the callback from ducati does come then it doesnt crash after
            //exiting this function since eventSem will go out of scope.
            if(ret != NO_ERROR)
                {
                ret |= SignalEvent(mCameraAdapterParameters.mHandleComp,
                                            (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                                            OMX_ALL,
                                            OMX_IndexConfigCommonFocusStatus,
                                            NULL );
                }
            }

        {
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Autofocus callback received");
            ret = returnFocusStatus(false);
            }
        else
            {
            CAMHAL_LOGEA("Autofocus callback timeout expired");
            ret = returnFocusStatus(true);
            }
        }

        // if focus is cancelled when waiting for AF callabck, CTS fails
        // so hold the lock till AF callback is received
        mFocusLock.unlock();
        }
    else
        {
        if ( NO_ERROR == ret )
            {
            ret = returnFocusStatus(true);
            }

        mFocusLock.unlock();
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::stopAutoFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusControl;

    LOG_FUNCTION_NAME

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
       {
       //Disable the callback first
       ret = setFocusCallback(false);
       }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&focusControl, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
        focusControl.eFocusControl = OMX_IMAGE_FocusControlOff;

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focusControl);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while stopping focus 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDA("Autofocus stopped successfully");
            }
        }

    mFocusStarted = false;

    LOG_FUNCTION_NAME_EXIT

    return ret;

}

status_t OMXCameraAdapter::cancelAutoFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mFocusLock);

    if(mFocusStarted)
    {
        stopAutoFocus();
        //Signal a dummy AF event so that in case the callback from ducati does come then it doesnt crash after
        //exiting this function since eventSem will go out of scope.
        ret |= SignalEvent(mCameraAdapterParameters.mHandleComp,
                                    (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                                    OMX_ALL,
                                    OMX_IndexConfigCommonFocusStatus,
                                    NULL );
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

}

status_t OMXCameraAdapter::setFocusCallback(bool enabled)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CALLBACKREQUESTTYPE focusRequstCallback;

    LOG_FUNCTION_NAME

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&focusRequstCallback, OMX_CONFIG_CALLBACKREQUESTTYPE);
        focusRequstCallback.nPortIndex = OMX_ALL;
        focusRequstCallback.nIndex = OMX_IndexConfigCommonFocusStatus;

        if ( enabled )
            {
            focusRequstCallback.bEnable = OMX_TRUE;
            }
        else
            {
            focusRequstCallback.bEnable = OMX_FALSE;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,  (OMX_INDEXTYPE) OMX_IndexConfigCallbackRequest, &focusRequstCallback);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error registering focus callback 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDB("Autofocus callback for index 0x%x registered successfully", OMX_IndexConfigCommonFocusStatus);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::returnFocusStatus(bool timeoutReached)
{
    status_t ret = NO_ERROR;
    OMX_PARAM_FOCUSSTATUSTYPE eFocusStatus;
    bool focusStatus = false;

    LOG_FUNCTION_NAME

    OMX_INIT_STRUCT(eFocusStatus, OMX_PARAM_FOCUSSTATUSTYPE);

    if(!mFocusStarted)
       {
        /// We don't send focus callback if focus was not started
       return NO_ERROR;
       }

    if ( NO_ERROR == ret )
        {

        if ( ( !timeoutReached ) &&
             ( mFocusStarted ) )
            {
            ret = checkFocus(&eFocusStatus);

            if ( NO_ERROR != ret )
                {
                CAMHAL_LOGEA("Focus status check failed!");
                }
            }
        }

    if ( NO_ERROR == ret )
        {

        if ( timeoutReached )
            {
            focusStatus = false;
            }
        else
            {

            switch (eFocusStatus.eFocusStatus)
                {
                    case OMX_FocusStatusReached:
                        {
                        focusStatus = true;
                        break;
                        }
                    case OMX_FocusStatusRequest:
                        {
                        // In CAF mode, we are seeing the auto focus driver return this value upon
                        // initial covergence. For the application sake, handle it as a convergence.
                        if (mParameters3A.Focus == OMX_IMAGE_FocusControlAuto)
                        {
                            focusStatus = true;
                            CAMHAL_LOGEA("CAF returnFocusStatus of good for request state");
                        }
                        else
                        {
                            focusStatus = false;
                        }
                        break;
                        }
                    case OMX_FocusStatusUnableToReach:
                       {
                        // In CAF mode, we need to make sure that we kick the focus sweep again. Poke
                        // the application to trigger another initial request by making a transition to
                        // focus request mode from paused state without notifying of the failure.
                        if (mParameters3A.Focus == OMX_IMAGE_FocusControlAuto)
                        {
                            mCafNotifier->cafNotify(AppCallbackNotifier::CAF_MSG_PAUSED);
                            mCafNotifier->cafNotify(AppCallbackNotifier::CAF_MSG_FOCUS_REQ);
                            CAMHAL_LOGEA("CAF unable to reach focus");
                            return ret;
                        }
                        focusStatus = false;
                        break;
                        }
                    case OMX_FocusStatusOff:
                    default:
                        {
                        focusStatus = false;
                        break;
                        }
                }

            // Stop auto focus, but not CAF
            if (mParameters3A.Focus != OMX_IMAGE_FocusControlAuto)
            { 
                stopAutoFocus();
            }
        }
    }

    if ( NO_ERROR == ret )
        {
        ret = notifyFocusSubscribers(focusStatus);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::checkFocus(OMX_PARAM_FOCUSSTATUSTYPE *eFocusStatus)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME

    if ( NULL == eFocusStatus )
        {
        CAMHAL_LOGEA("Invalid focus status");
        ret = -EINVAL;
        }

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = -EINVAL;
        }

    if ( !mFocusStarted )
        {
        CAMHAL_LOGEA("Focus was not started");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (eFocusStatus, OMX_PARAM_FOCUSSTATUSTYPE);

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonFocusStatus, eFocusStatus);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error while retrieving focus status: 0x%x", eError);
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDB("Focus Status: %d", eFocusStatus->eFocusStatus);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::setShutterCallback(bool enabled)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CALLBACKREQUESTTYPE shutterRequstCallback;

    LOG_FUNCTION_NAME

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {

        OMX_INIT_STRUCT_PTR (&shutterRequstCallback, OMX_CONFIG_CALLBACKREQUESTTYPE);
        shutterRequstCallback.nPortIndex = OMX_ALL;

        if ( enabled )
            {
            shutterRequstCallback.bEnable = OMX_TRUE;
            shutterRequstCallback.nIndex = ( OMX_INDEXTYPE ) OMX_TI_IndexConfigShutterCallback;
            }
        else
            {
            shutterRequstCallback.bEnable = OMX_FALSE;
            shutterRequstCallback.nIndex = ( OMX_INDEXTYPE ) OMX_TI_IndexConfigShutterCallback;
            }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,  ( OMX_INDEXTYPE ) OMX_IndexConfigCallbackRequest, &shutterRequstCallback);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("Error registering shutter callback 0x%x", eError);
            ret = -1;
            }
        else
            {
            CAMHAL_LOGDB("Shutter callback for index 0x%x registered successfully", OMX_TI_IndexConfigShutterCallback);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::startBracketing(int range)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

        {
        Mutex::Autolock lock(mBracketingLock);

        if ( mBracketingEnabled )
            {
            return ret;
            }
        }

    if ( 0 == imgCaptureData->mNumBufs )
        {
        CAMHAL_LOGEB("Image capture buffers set to %d", imgCaptureData->mNumBufs);
        ret = -EINVAL;
        }

    if ( mPending3Asettings )
        apply3Asettings(mParameters3A);

    if ( NO_ERROR == ret )
        {
        Mutex::Autolock lock(mBracketingLock);

        mBracketingRange = range;
        mBracketingBuffersQueued = new bool[imgCaptureData->mNumBufs];
        if ( NULL == mBracketingBuffersQueued )
            {
            CAMHAL_LOGEA("Unable to allocate bracketing management structures");
            ret = -1;
            }

        if ( NO_ERROR == ret )
            {
            mBracketingBuffersQueuedCount = imgCaptureData->mNumBufs;
            mLastBracetingBufferIdx = mBracketingBuffersQueuedCount - 1;

            for ( int i = 0 ; i  < imgCaptureData->mNumBufs ; i++ )
                {
                mBracketingBuffersQueued[i] = true;
                }

            }
        }

    if ( NO_ERROR == ret )
        {

        ret = startImageCapture();
            {
            Mutex::Autolock lock(mBracketingLock);

            if ( NO_ERROR == ret )
                {
                mBracketingEnabled = true;
                }
            else
                {
                mBracketingEnabled = false;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::doHDRProcessing(OMX_BUFFERHEADERTYPE *pBuffHeader, int outputPortIndex)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters * imgCaptureData = NULL;
    CameraFrame::FrameType typeOfFrame = CameraFrame::ALL_FRAMES;

    LOG_FUNCTION_NAME

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[outputPortIndex];

    ret = mHDRInterface->setDataBuffers(pBuffHeader->pBuffer, pBuffHeader->nFilledLen, pBuffHeader->nOffset);
    if ( ret == 0 )
        {
        CAMHAL_LOGEA("HDR Inteface setDataBuffers failed\n");
        }

    if ( OMX_COLOR_FormatUnused == mCameraAdapterParameters.mCameraPortParams[outputPortIndex].mColorFormat )
        {
        typeOfFrame = CameraFrame::IMAGE_FRAME;
        }
    else
        {
        typeOfFrame = CameraFrame::RAW_FRAME;
        }
    CAMHAL_LOGEB("mCapturedFrames = %d\n", mCapturedFrames);
    if ( mCapturedFrames == 3 )
       {
       returnFrame(pBuffHeader->pBuffer, typeOfFrame);
       }
    return ret;
}

status_t OMXCameraAdapter::doBracketing(OMX_BUFFERHEADERTYPE *pBuffHeader, CameraFrame::FrameType typeOfFrame)
{
    status_t ret = NO_ERROR;
    int currentBufferIdx, nextBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        currentBufferIdx = ( unsigned int ) pBuffHeader->pAppPrivate;

        if ( currentBufferIdx >= imgCaptureData->mNumBufs)
            {
            CAMHAL_LOGEB("Invalid bracketing buffer index 0x%x", currentBufferIdx);
            ret = -EINVAL;
            }
        }

    if ( NO_ERROR == ret )
        {
        mBracketingBuffersQueued[currentBufferIdx] = false;
        mBracketingBuffersQueuedCount--;

        if ( 0 >= mBracketingBuffersQueuedCount )
            {
            nextBufferIdx = ( currentBufferIdx + 1 ) % imgCaptureData->mNumBufs;
            mBracketingBuffersQueued[nextBufferIdx] = true;
            mBracketingBuffersQueuedCount++;
            mLastBracetingBufferIdx = nextBufferIdx;
            setFrameRefCount(imgCaptureData->mBufferHeader[nextBufferIdx]->pBuffer, typeOfFrame, 1);
            returnFrame(imgCaptureData->mBufferHeader[nextBufferIdx]->pBuffer, typeOfFrame);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::sendBracketFrames()
{
    status_t ret = NO_ERROR;
    int currentBufferIdx;
    OMXCameraPortParameters * imgCaptureData = NULL;

    LOG_FUNCTION_NAME

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if ( OMX_StateExecuting != mComponentState )
        {
        CAMHAL_LOGEA("OMX component is not in executing state");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {

        currentBufferIdx = mLastBracetingBufferIdx;
        do
            {
            currentBufferIdx++;
            currentBufferIdx %= imgCaptureData->mNumBufs;
            if (!mBracketingBuffersQueued[currentBufferIdx] )
                {
                CameraFrame cameraFrame;
                initCameraFrame(cameraFrame,
						imgCaptureData->mBufferHeader[currentBufferIdx],
						imgCaptureData->mImageType,
						imgCaptureData);
                sendFrame(imgCaptureData->mBufferHeader[currentBufferIdx], cameraFrame);
                }
            } while ( currentBufferIdx != mLastBracetingBufferIdx );

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::stopBracketing()
{
  status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mBracketingLock);

    if ( mBracketingEnabled )
        {

        if ( NULL != mBracketingBuffersQueued )
        {
            delete [] mBracketingBuffersQueued;
        }

        ret = stopImageCapture();
        mBracketingBuffersQueued = NULL;
        mBracketingEnabled = false;
        mBracketingBuffersQueuedCount = 0;
        mLastBracetingBufferIdx = 0;

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::takePicture()
{
    status_t ret = NO_ERROR;
    Message msg = {0,0,0,0,0,0};

    LOG_FUNCTION_NAME

    msg.command = CommandHandler::CAMERA_START_IMAGE_CAPTURE;
    msg.arg1 = mErrorNotifier;
    ret = mCommandHandler->put(&msg);

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::startImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * capData = NULL;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mLock);

    if(!mCaptureConfigured)
        {
        ///Image capture was cancelled before we could start
        return NO_ERROR;
        }

    //During bracketing image capture is already active
    {
    Mutex::Autolock lock(mBracketingLock);
    if ( mBracketingEnabled )
        {
        //Stop bracketing, activate normal burst for the remaining images
        mBracketingEnabled = false;
        mCapturedFrames = mBracketingRange;
        ret = sendBracketFrames();
        goto EXIT;
        }
    }

    if ( NO_ERROR == ret )
        {
        ret = setPictureRotation(mPictureRotation);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("Error configuring image rotation %x", ret);
            }
        }

    //OMX shutter callback events are only available in hq mode
    if (( HIGH_QUALITY == mCapMode ) ||
        ( HIGH_QUALITY_NOTSET== mCapMode ) ||
        ( HIGH_QUALITY_NONZSL== mCapMode ))
        {
        if ( NO_ERROR == ret )
            {
            ret = mStartCaptureSem.Create(0);
            }

        if ( NO_ERROR == ret )
            {
            ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                        (OMX_EVENTTYPE) OMX_EventIndexSettingChanged,
                                        OMX_ALL,
                                        OMX_TI_IndexConfigShutterCallback,
                                        mStartCaptureSem);
            }

        if ( NO_ERROR == ret )
            {
            ret = setShutterCallback(true);
            }

        }

    if ( NO_ERROR == ret )
        {
        capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        ///Queue all the buffers on capture port
        for ( int index = 0 ; index < capData->mNumBufs ; index++ )
            {
            CAMHAL_LOGDB("Queuing buffer on Capture port - 0x%x", ( unsigned int ) capData->mBufferHeader[index]->pBuffer);
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                        (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
            }

        mWaitingForSnapshot = true;
        mCapturing = true;
        mCaptureSignalled = false;

        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
        bOMX.bEnabled = OMX_TRUE;

        /// sending Capturing Command to the component
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCapturing, &bOMX);

        CAMHAL_LOGDB("Capture set - 0x%x", eError);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        }

    //OMX shutter callback events are only available in hq mode
    if (( HIGH_QUALITY == mCapMode )||
        ( HIGH_QUALITY_NOTSET== mCapMode ) ||
        ( HIGH_QUALITY_NONZSL== mCapMode ))
        {

        if ( NO_ERROR == ret )
            {
            ret = mStartCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);
            }

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDA("Shutter callback received");
            ret = notifyShutterSubscribers();
            }
        else
            {
            CAMHAL_LOGEA("Timeout expired on shutter callback");
            goto EXIT;
            }

        }

    EXIT:

    if ( eError != OMX_ErrorNone )
        {
        ret = -1;
        mWaitingForSnapshot = false;
        mCapturing = false;

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError;
    OMX_CONFIG_BOOLEANTYPE bOMX;
    OMXCameraPortParameters *imgCaptureData = NULL;

    LOG_FUNCTION_NAME

    {
    Mutex::Autolock lock(mLock);

    if(!mCaptureConfigured)
        {
        //Capture is not ongoing, return from here
        return NO_ERROR;
        }

    ret = mStopCaptureSem.Create(0);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEA("Semaphore initialization failed");
        goto EXIT;
        }

    if ( mCapturing )
        {

        mWaitingForSnapshot = false;
        mSnapshotCount = 0;
        mCapturing = false;

        //Disable the callback first
        ret = setShutterCallback(false);

        //Wait here for the capture to be done, in worst case timeout and proceed with cleanup
        mCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);

        //Disable image capture
        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_BOOLEANTYPE);
        bOMX.bEnabled = OMX_FALSE;
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCapturing, &bOMX);

        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGDB("Error during SetConfig- 0x%x", eError);
            ret = -1;
            }
        }

        mCaptureSignalled = true; //set this to true if we exited because of timeout
        mCaptureConfigured = false;
    }

    CAMHAL_LOGDB("Capture set - 0x%x", eError);

    ///Register for Image port Disable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mImagePortIndex,
                                mStopCaptureSem);

    ///Disable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                OMX_CommandPortDisable,
                                mCameraAdapterParameters.mImagePortIndex,
                                NULL);

    ///Free all the buffers on capture port
    CAMHAL_LOGDB("Freeing buffer on Capture port - %d", imgCaptureData->mNumBufs);
    for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++)
        {
        CAMHAL_LOGDB("Freeing buffer on Capture port - 0x%x", ( unsigned int ) imgCaptureData->mBufferHeader[index]->pBuffer);
        eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                        mCameraAdapterParameters.mImagePortIndex,
                        (OMX_BUFFERHEADERTYPE*)imgCaptureData->mBufferHeader[index]);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

    CAMHAL_LOGDA("Waiting for port disable");
    //Wait for the image port enable event
    ret = mStopCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
        {
        CAMHAL_LOGDA("Port disabled");
        }
    else
        {
        CAMHAL_LOGDA("Timeout expired on port disable");
        goto EXIT;
        }

    EXIT:

    //Signal end of image capture
    if ( NULL != mEndImageCaptureCallback)
        {
        mEndImageCaptureCallback(mEndCaptureData);
        }

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

status_t OMXCameraAdapter::startVideoCapture()
{
    return BaseCameraAdapter::startVideoCapture();
}

status_t OMXCameraAdapter::stopVideoCapture()
{
    return BaseCameraAdapter::stopVideoCapture();
}

//API to get the frame size required to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
void OMXCameraAdapter::getFrameSize(int &width, int &height)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_RECTTYPE tFrameDim;

    LOG_FUNCTION_NAME

    OMX_INIT_STRUCT_PTR (&tFrameDim, OMX_CONFIG_RECTTYPE);
    tFrameDim.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("Calling queryBufferPreviewResolution() when not in LOADED state");
        width = -1;
        height = -1;
        goto exit;
        }

    ret = setLDC(mIPP);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("setLDC() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        goto exit;
        }

    ret = setNSF(mIPP);
    if ( NO_ERROR != ret )
        {
        CAMHAL_LOGEB("setNSF() failed %d", ret);
        LOG_FUNCTION_NAME_EXIT
        goto exit;
        }

    if ( NO_ERROR == ret )
        {
        ret = setCaptureMode(mCapMode);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setCaptureMode() failed %d", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        ret = setFormat (mCameraAdapterParameters.mPrevPortIndex, mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex]);
        if ( NO_ERROR != ret )
            {
            CAMHAL_LOGEB("setFormat() failed %d", ret);
            }
        }

    if(mCapMode == OMXCameraAdapter::VIDEO_MODE)
        {
        if ( NO_ERROR == ret )
            {
            ///Enable/Disable Video Noise Filter
            ret = enableVideoNoiseFilter(mVnfEnabled);
            }

        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VNF %x", ret);
            }

        if ( NO_ERROR == ret )
            {
            ///Enable/Disable Video Stabilization
            ret = enableVideoStabilization(mVstabEnabled);
            }

        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
            }
         }
    else
        {
        if ( NO_ERROR == ret )
            {
            mVnfEnabled = false;
            ///Enable/Disable Video Noise Filter
            ret = enableVideoNoiseFilter(mVnfEnabled);
            }

        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VNF %x", ret);
            }

        if ( NO_ERROR == ret )
            {
            mVstabEnabled = false;
            ///Enable/Disable Video Stabilization
            ret = enableVideoStabilization(mVstabEnabled);
            }

        if ( NO_ERROR != ret)
            {
            CAMHAL_LOGEB("Error configuring VSTAB %x", ret);
            }
        }

    if ( NO_ERROR == ret )
        {
        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_TI_IndexParam2DBufferAllocDimension, &tFrameDim);
        if ( OMX_ErrorNone == eError)
            {
            width = tFrameDim.nWidth;
            height = tFrameDim.nHeight;
            }
       else
            {
            width = -1;
            height = -1;
            }
        }
    else
        {
        width = -1;
        height = -1;
        }
exit:

    CAMHAL_LOGDB("Required frame size %dx%d", width, height);

    LOG_FUNCTION_NAME_EXIT
}

status_t OMXCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME

    if ( OMX_StateLoaded != mComponentState )
        {
        CAMHAL_LOGEA("Calling getFrameDataSize() when not in LOADED state");
        dataFrameSize = 0;
        ret = -1;
        }

    if ( NO_ERROR == ret  )
        {
        OMX_INIT_STRUCT_PTR(&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);
        portCheck.nPortIndex = mCameraAdapterParameters.mMeasurementPortIndex;

        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamPortDefinition, &portCheck);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_GetParameter on OMX_IndexParamPortDefinition returned: 0x%x", eError);
            dataFrameSize = 0;
            ret = -1;
            }
        }

    if ( NO_ERROR == ret )
        {
        portCheck.nBufferCountActual = bufferCount;
        eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamPortDefinition, &portCheck);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_SetParameter on OMX_IndexParamPortDefinition returned: 0x%x", eError);
            dataFrameSize = 0;
            ret = -1;
            }
        }

    if ( NO_ERROR == ret  )
        {
        eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp, OMX_IndexParamPortDefinition, &portCheck);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMX_GetParameter on OMX_IndexParamPortDefinition returned: 0x%x", eError);
            ret = -1;
            }
        else
            {
            mCameraAdapterParameters.mCameraPortParams[portCheck.nPortIndex].mBufSize = portCheck.nBufferSize;
            dataFrameSize = portCheck.nBufferSize;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME

    if ( mCapturing )
        {
        CAMHAL_LOGEA("getPictureBufferSize() called during image capture");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        imgCaptureData->mNumBufs = bufferCount;
        ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        if ( ret == NO_ERROR )
            {
            length = imgCaptureData->mBufSize;
            }
        else
            {
            CAMHAL_LOGEB("setFormat() failed 0x%x", ret);
            length = 0;
            }
        }

    CAMHAL_LOGDB("getPictureBufferSize %d", length);

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_PTR pAppData,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN OMX_PTR pEventData)
{
    LOG_FUNCTION_NAME

    CAMHAL_LOGDB("Event %d", eEvent);

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    ret = oca->OMXCameraAdapterEventHandler(hComponent, eEvent, nData1, nData2, pEventData);

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN OMX_PTR pEventData)
{

    LOG_FUNCTION_NAME

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CAMHAL_LOGDB("+OMX_Event %x, %d %d", eEvent, (int)nData1, (int)nData2);

    switch (eEvent) {
        case OMX_EventCmdComplete:
            CAMHAL_LOGDB("+OMX_EventCmdComplete %d %d", (int)nData1, (int)nData2);

            if (OMX_CommandStateSet == nData1) {
                mCameraAdapterParameters.mState = (OMX_STATETYPE) nData2;

            } else if (OMX_CommandFlush == nData1) {
                CAMHAL_LOGDB("OMX_CommandFlush received for port %d", (int)nData2);

            } else if (OMX_CommandPortDisable == nData1) {
                CAMHAL_LOGDB("OMX_CommandPortDisable received for port %d", (int)nData2);

            } else if (OMX_CommandPortEnable == nData1) {
                CAMHAL_LOGDB("OMX_CommandPortEnable received for port %d", (int)nData2);

            } else if (OMX_CommandMarkBuffer == nData1) {
                ///This is not used currently
            }

            CAMHAL_LOGDA("-OMX_EventCmdComplete");
        break;

        case OMX_EventIndexSettingChanged:
            CAMHAL_LOGDB("OMX_EventIndexSettingChanged event received data1 0x%x, data2 0x%x",
                            ( unsigned int ) nData1, ( unsigned int ) nData2);
            break;

        case OMX_EventError:
            CAMHAL_LOGDB("OMX interface failed to execute OMX command %d", (int)nData1);
            CAMHAL_LOGDA("See OMX_INDEXTYPE for reference");
            if ( NULL != mErrorNotifier && ( ( OMX_U32 ) OMX_ErrorHardware == nData1 ) && mComponentState != OMX_StateInvalid)
              {
                LOGD("***Got MMU FAULT***\n");
                mComponentState = OMX_StateInvalid;
                ///Report Error to App
                mErrorNotifier->errorNotify(CAMERA_ERROR_UKNOWN);
              }
            break;

        case OMX_EventMark:
        break;

        case OMX_EventPortSettingsChanged:
        break;

        case OMX_EventBufferFlag:
        break;

        case OMX_EventResourcesAcquired:
        break;

        case OMX_EventComponentResumed:
        break;

        case OMX_EventDynamicResourcesAvailable:
        break;

        case OMX_EventPortFormatDetected:
        break;

        default:
        break;
    }

    ///Signal to the thread(s) waiting that the event has occured
    SignalEvent(hComponent, eEvent, nData1, nData2, pEventData);

   LOG_FUNCTION_NAME_EXIT
   return eError;

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError=%x", __FUNCTION__, eError);
    LOG_FUNCTION_NAME_EXIT
    return eError;
}

OMX_ERRORTYPE OMXCameraAdapter::SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN OMX_PTR pEventData)
{
    Mutex::Autolock lock(mEventLock);
    Message *msg;

    LOG_FUNCTION_NAME

    if ( !mEventSignalQ.isEmpty() )
        {
        CAMHAL_LOGDA("Event queue not empty");

        for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
            {
            msg = mEventSignalQ.itemAt(i);
            if ( NULL != msg )
                {
                if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                    && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                    && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                    && msg->arg3)
                    {
                    Semaphore *sem  = (Semaphore*) msg->arg3;
                    CAMHAL_LOGDA("Event matched, signalling sem");
                    mEventSignalQ.removeAt(i);
                    //Signal the semaphore provided
                    sem->Signal();
		    if(sem == &mUseCaptureSemPortEnable)
			LOGE("mUseCaptureSemPortEnable - trigger");
		    if(sem == &mUseCaptureSemPortDisable)
			LOGE("mUseCaptureSemPortDisable - trigger");
                    free(msg);
                    break;
                    }
                }
            }
        }
    else
        {
        CAMHAL_LOGEA("Event queue empty!!!");
        }

    LOG_FUNCTION_NAME_EXIT

    return OMX_ErrorNone;
}

status_t OMXCameraAdapter::RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN Semaphore &semaphore)
{
    status_t ret = NO_ERROR;
    ssize_t res;
    Mutex::Autolock lock(mEventLock);

    LOG_FUNCTION_NAME

    Message * msg = ( struct Message * ) malloc(sizeof(struct Message));
    if ( NULL != msg )
        {
        msg->command = ( unsigned int ) eEvent;
        msg->arg1 = ( void * ) nData1;
        msg->arg2 = ( void * ) nData2;
        msg->arg3 = ( void * ) &semaphore;
        msg->arg4 =  ( void * ) hComponent;
        res = mEventSignalQ.add(msg);
        if ( NO_MEMORY == res )
            {
            CAMHAL_LOGEA("No ressources for inserting OMX events");
            ret = -ENOMEM;
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_PTR pAppData,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    LOG_FUNCTION_NAME

    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    eError = oca->OMXCameraAdapterEmptyBufferDone(hComponent, pBuffHeader);

    LOG_FUNCTION_NAME_EXIT
    return eError;
}


/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{

   LOG_FUNCTION_NAME

   LOG_FUNCTION_NAME_EXIT

   return OMX_ErrorNone;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        LOGD("Camera %d Frames, %f FPS", mFrameCount, mFps);
    }
    // XXX: mFPS has the value we want
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::  Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_PTR pAppData,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (UNLIKELY(mDebugFps)) {
        debugShowFPS();
    }

    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    eError = oca->OMXCameraAdapterFillBufferDone(hComponent, pBuffHeader);

    return eError;
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::  Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{

    status_t  stat = NO_ERROR;
    status_t  res1, res2;
    OMXCameraPortParameters  *pPortParam;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CameraFrame::FrameType typeOfFrame = CameraFrame::ALL_FRAMES;
    unsigned int refCount = 0;
    static int mCafStatus = 0;

    if (  mCafStatus == 0)
        {
	mCafNotifier->cafNotify(AppCallbackNotifier::CAF_MSG_FOCUS_REQ);
        mCafStatus = 1;
        }

    res1 = res2 = -1;
    pPortParam = &(mCameraAdapterParameters.mCameraPortParams[pBuffHeader->nOutputPortIndex]);
    if (pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
        {

        recalculateFPS();

            {
            Mutex::Autolock lock(mFaceDetectionLock);
            if ( mFaceDetectionRunning )
                {
                detectFaces(pBuffHeader);
                CAMHAL_LOGVB("Faces detected: %s", mFaceDectionResult);
                }
            }

        stat |= advanceZoom();

        ///On the fly update to 3A settings not working
        if( mPending3Asettings )
            {
            apply3Asettings(mParameters3A);
            }

        ///Prepare the frames to be sent - initialize CameraFrame object and reference count
        CameraFrame cameraFrameVideo, cameraFramePreview;
        if ( mRecording )
            {
            res1 = initCameraFrame(cameraFrameVideo,
							pBuffHeader,
							CameraFrame::VIDEO_FRAME_SYNC,
							pPortParam);
            }

        if( mWaitingForSnapshot )
            {
            mWaitingForSnapshot = false;
            mSnapshotCount++;
            typeOfFrame = CameraFrame::SNAPSHOT_FRAME;
            }
        else
            {
            typeOfFrame = CameraFrame::PREVIEW_FRAME_SYNC;
            }

        
        res2 = initCameraFrame(cameraFramePreview,
						pBuffHeader,
						typeOfFrame,
						pPortParam);
        
        stat |= ( ( NO_ERROR == res1 ) || ( NO_ERROR == res2 ) ) ? ( ( int ) NO_ERROR ) : ( -1 );

        if ( mRecording )
            {
            res1  = sendFrame(pBuffHeader, cameraFrameVideo);

            }

        if (  ( mSnapshotCount == 1 ) &&
            ( HIGH_SPEED == mCapMode ) )
            {
            mSnapshotCount = 0;
            notifyShutterSubscribers();
            }

        res2 = sendFrame(pBuffHeader, cameraFramePreview);

        stat |= ( ( NO_ERROR == res1 ) || ( NO_ERROR == res2 ) ) ? ( ( int ) NO_ERROR ) : ( -1 );

        }
    else if( pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_VIDEO_OUT_MEASUREMENT )
        {
        typeOfFrame = CameraFrame::FRAME_DATA_SYNC;
        CameraFrame cameraFrame;
        stat |= initCameraFrame(cameraFrame,
						pBuffHeader,
						typeOfFrame,
						pPortParam);
        stat |= sendFrame(pBuffHeader, cameraFrame);
       }
    else if( pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_IMAGE_OUT_IMAGE )
        {
        CameraHal::mCameraKPI.stopKPITimer(CAMKPI_JPG);
        CameraHal::mCameraKPI.startKPITimer(CAMKPI_JPGToPreview);

        if ( OMX_COLOR_FormatUnused == mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mColorFormat )
            {
            typeOfFrame = CameraFrame::IMAGE_FRAME;
            }
        else
            {
            typeOfFrame = CameraFrame::RAW_FRAME;
            }

            pPortParam->mImageType = typeOfFrame;

            if((mCapturedFrames>0) && !mCaptureSignalled)
                {
                mCaptureSignalled = true;
                mCaptureSem.Signal();
                }

            if(!mCapturing)
                {
                goto EXIT;
                }

            {
            Mutex::Autolock lock(mBracketingLock);
            if ( mBracketingEnabled )
                {
                doBracketing(pBuffHeader, typeOfFrame);
                return eError;
                }
            }

            if ( mHDRCaptureEnabled )
               {
                   doHDRProcessing(pBuffHeader, pBuffHeader->nOutputPortIndex);
               }

        if ( 1 > mCapturedFrames )
            {
            goto EXIT;
            }

        CAMHAL_LOGDB("Captured Frames: %d", mCapturedFrames);

        mCapturedFrames--;

        //The usual jpeg capture does not include raw data.
        if ( ( CodingNone == mCodingMode ) &&
             ( typeOfFrame == CameraFrame::IMAGE_FRAME ) )
            {
            sendEmptyRawFrame();
            }

        CameraFrame cameraFrame;
        stat |= initCameraFrame(cameraFrame,
							pBuffHeader,
							typeOfFrame,
							pPortParam);
        stat |= sendFrame(pBuffHeader, cameraFrame);

        }
    else
        {
        CAMHAL_LOGEA("Frame received for non-(preview/capture/measure) port. This is yet to be supported");
        goto EXIT;
        }

    if ( NO_ERROR != stat )
        {
        CAMHAL_LOGDB("sendFrameToSubscribers error: %d", stat);
         returnFrame(pBuffHeader->pBuffer, typeOfFrame);
        }

    return eError;

    EXIT:

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, stat, eError);

    if ( NO_ERROR != stat )
        {
        if ( NULL != mErrorNotifier )
            {
            mErrorNotifier->errorNotify(CAMERA_ERROR_UKNOWN);
            }
        }

    return eError;
}

status_t OMXCameraAdapter::advanceZoom()
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mZoomLock);

    if ( mReturnZoomStatus )
        {
        mTargetZoomIdx = mCurrentZoomIdx;
        mReturnZoomStatus = false;
        mSmoothZoomEnabled = false;
        ret = doZoom(mCurrentZoomIdx);
        notifyZoomSubscribers(mCurrentZoomIdx, true);
        }
    else if ( mCurrentZoomIdx != mTargetZoomIdx )
        {
        if ( mSmoothZoomEnabled )
            {
            if ( mCurrentZoomIdx < mTargetZoomIdx )
                {
                mZoomInc = 1;
                }
            else
                {
                mZoomInc = -1;
                }

            mCurrentZoomIdx += mZoomInc;
            }
        else
            {
            mCurrentZoomIdx = mTargetZoomIdx;
            }

        ret = doZoom(mCurrentZoomIdx);

        if ( mSmoothZoomEnabled )
            {
            if ( mCurrentZoomIdx == mTargetZoomIdx )
                {
                CAMHAL_LOGDB("[Goal Reached] Smooth Zoom notify currentIdx = %d, targetIdx = %d", mCurrentZoomIdx, mTargetZoomIdx);
                mSmoothZoomEnabled = false;
                notifyZoomSubscribers(mCurrentZoomIdx, true);
                }
            else
                {
                CAMHAL_LOGDB("[Advancing] Smooth Zoom notify currentIdx = %d, targetIdx = %d", mCurrentZoomIdx, mTargetZoomIdx);
                notifyZoomSubscribers(mCurrentZoomIdx, false);
                }
            }
        }
    else if ( mSmoothZoomEnabled )
        {
        mSmoothZoomEnabled = false;
        }

    return ret;
}

status_t OMXCameraAdapter::recalculateFPS()
{
    float currentFPS;

    mFrameCount++;

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
        {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
            {
            mFPS = currentFPS;
            }
        else
            {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
            }

        mLastFPS = mFPS;
        mIter++;
        }

    return NO_ERROR;
}

status_t OMXCameraAdapter::sendEmptyRawFrame()
{
    status_t ret = NO_ERROR;
    CameraFrame frame;

    LOG_FUNCTION_NAME

    if ( NO_ERROR == ret )
        {
        memset(&frame, 0, sizeof(CameraFrame));
        frame.mFrameType = CameraFrame::RAW_FRAME;
        ret = sendFrameToSubscribers(&frame);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::sendFrame(OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, CameraFrame &frame)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if ( NO_ERROR == ret )
        {
        if( mHDRCaptureEnabled && (pBuffHeader->nOutputPortIndex == OMX_CAMERA_PORT_IMAGE_OUT_IMAGE))
            {
            //Don't send the callback as HDR Processing needs to be done
            CAMHAL_LOGDA("don't sendFrame, HDR Capture in progress");
            }
        else
            {
            ret = sendFrameToSubscribers(&frame);
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXCameraAdapter::initCameraFrame(CameraFrame &frame,
                                                                                          OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader,
                                                                                          int typeOfFrame,
                                                                                          OMXCameraPortParameters *port)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if ( NULL == port)
        {
        CAMHAL_LOGEA("Invalid portParam");
        ret = -EINVAL;
        }

    if ( NULL == pBuffHeader )
        {
        CAMHAL_LOGEA("Invalid Buffer header");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        frame.mFrameType = typeOfFrame;
        frame.mBuffer = pBuffHeader->pBuffer;
        frame.mLength = pBuffHeader->nFilledLen;
        frame.mAlignment =port->mStride;
        frame.mOffset = pBuffHeader->nOffset;
        frame.mWidth = port->mWidth;
        frame.mHeight = port->mHeight;

        // Calculating the time source delta of Ducati & system time only once at the start of camera.
        // It's seen that there is a one-time constant diff between the ducati source clock &
        // System monotonic timer, although both derived from the same 32KHz clock.
        // This delta is offsetted to/from ducati timestamp to match with system time so that
        // video timestamps are aligned with Audio with a periodic timestamp intervals.
        if ( mOnlyOnce )
            {
            mTimeSourceDelta = (pBuffHeader->nTimeStamp * 1000) - systemTime(SYSTEM_TIME_MONOTONIC);
            mOnlyOnce = false;
            }

        // Calculating the new video timestamp based on offset from ducati source.
        frame.mTimestamp = (pBuffHeader->nTimeStamp * 1000) - mTimeSourceDelta;
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

OMX_ERRORTYPE OMXCameraAdapter::apply3Asettings( Gen3A_settings& Gen3A )
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    unsigned int currSett; // 32 bit
    int portIndex;

    /*
     * Scenes have a priority during the process
     * of applying 3A related parameters.
     * They can override pretty much all other 3A
     * settings and similarly get overridden when
     * for instance the focus mode gets switched.
     * There is only one exception to this rule,
     * the manual a.k.a. auto scene.
     */
    if ( ( SetSceneMode & mPending3Asettings ) )
        {
        mPending3Asettings &= ~SetSceneMode;
        return setScene(Gen3A);
        }

    for( currSett = 1; currSett < E3aSettingMax; currSett <<= 1)
        {
        if( currSett & mPending3Asettings )
            {
            switch( currSett )
                {
                case SetEVCompensation:
                    {
                    OMX_CONFIG_EXPOSUREVALUETYPE expValues;
                    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
                    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

                    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,OMX_IndexConfigCommonExposureValue, &expValues);
                    CAMHAL_LOGDB("old EV Compensation for OMX = 0x%x", (int)expValues.xEVCompensation);
                    CAMHAL_LOGDB("EV Compensation for HAL = %d", Gen3A.EVCompensation);

                    expValues.xEVCompensation = ( Gen3A.EVCompensation * ( 1 << Q16_OFFSET ) )  / 10;
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,OMX_IndexConfigCommonExposureValue, &expValues);
                    CAMHAL_LOGDB("new EV Compensation for OMX = 0x%x", (int)expValues.xEVCompensation);
                    break;
                    }

                case SetWhiteBallance:
                    {


                    if ( WB_FACE_PRIORITY == Gen3A.WhiteBallance )
                        {
                        //Disable Region priority and enable Face priority
                        setAlgoPriority(REGION_PRIORITY, WHITE_BALANCE_ALGO, false);
                        setAlgoPriority(FACE_PRIORITY, WHITE_BALANCE_ALGO, true);

                        //Then set the mode to auto
                        Gen3A.WhiteBallance = OMX_WhiteBalControlAuto;
                        }
                    else
                        {
                        //Disable Face and Region priority
                        setAlgoPriority(FACE_PRIORITY, WHITE_BALANCE_ALGO, false);
                        setAlgoPriority(REGION_PRIORITY, WHITE_BALANCE_ALGO, false);
                        }

                    OMX_CONFIG_WHITEBALCONTROLTYPE wb;
                    OMX_INIT_STRUCT_PTR (&wb, OMX_CONFIG_WHITEBALCONTROLTYPE);
                    wb.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    wb.eWhiteBalControl = (OMX_WHITEBALCONTROLTYPE)Gen3A.WhiteBallance;

                    CAMHAL_LOGDB("White Ballance for Hal = %d", Gen3A.WhiteBallance);
                    CAMHAL_LOGDB("White Ballance for OMX = %d", (int)wb.eWhiteBalControl);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonWhiteBalance, &wb);
                    break;
                    }

                case SetFlicker:
                    {
                    OMX_CONFIG_FLICKERCANCELTYPE flicker;
                    OMX_INIT_STRUCT_PTR (&flicker, OMX_CONFIG_FLICKERCANCELTYPE);
                    flicker.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    flicker.eFlickerCancel = (OMX_COMMONFLICKERCANCELTYPE)Gen3A.Flicker;

                    CAMHAL_LOGDB("Flicker for Hal = %d", Gen3A.Flicker);
                    CAMHAL_LOGDB("Flicker for  OMX= %d", (int)flicker.eFlickerCancel);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexConfigFlickerCancel, &flicker );
                    break;
                    }

                case SetBrightness:
                    {
                    OMX_CONFIG_BRIGHTNESSTYPE brightness;
                    OMX_INIT_STRUCT_PTR (&brightness, OMX_CONFIG_BRIGHTNESSTYPE);
                    brightness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    brightness.nBrightness = Gen3A.Brightness;

                    CAMHAL_LOGDB("Brightness for Hal and OMX= %d", (int)Gen3A.Brightness);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonBrightness, &brightness);
                    break;
                    }

                case SetContrast:
                    {
                    OMX_CONFIG_CONTRASTTYPE contrast;
                    OMX_INIT_STRUCT_PTR (&contrast, OMX_CONFIG_CONTRASTTYPE);
                    contrast.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    contrast.nContrast = Gen3A.Contrast;

                    CAMHAL_LOGDB("Contrast for Hal and OMX= %d", (int)Gen3A.Contrast);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonContrast, &contrast);
                    break;
                    }

                case SetSharpness:
                    {
                    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE procSharpness;
                    OMX_INIT_STRUCT_PTR (&procSharpness, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
                    procSharpness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    procSharpness.nLevel = Gen3A.Sharpness;

                    if( procSharpness.nLevel == 0 )
                        {
                        procSharpness.bAuto = OMX_TRUE;
                        }
                    else
                        {
                    procSharpness.bAuto = OMX_FALSE;
                        }

                    CAMHAL_LOGDB("Sharpness for Hal and OMX= %d", (int)Gen3A.Sharpness);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_IndexConfigSharpeningLevel, &procSharpness);
                    break;
                    }

                case SetSaturation:
                    {
                    OMX_CONFIG_SATURATIONTYPE saturation;
                    OMX_INIT_STRUCT_PTR (&saturation, OMX_CONFIG_SATURATIONTYPE);
                    saturation.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    saturation.nSaturation = Gen3A.Saturation;

                    CAMHAL_LOGDB("Saturation for Hal and OMX= %d", (int)Gen3A.Saturation);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonSaturation, &saturation);
                    break;
                    }

                case SetISO:
                    {
                    OMX_CONFIG_EXPOSUREVALUETYPE expValues;
                    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
                    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

                    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,OMX_IndexConfigCommonExposureValue, &expValues);
                    if( 0 == Gen3A.ISO )
                        {
                        expValues.bAutoSensitivity = OMX_TRUE;
                        }
                    else
                        {
                        expValues.bAutoSensitivity = OMX_FALSE;
                        expValues.nSensitivity = Gen3A.ISO;
                        }
                    CAMHAL_LOGDB("ISO for Hal and OMX= %d", (int)Gen3A.ISO);
                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,OMX_IndexConfigCommonExposureValue, &expValues);
                    }
                    break;

                case SetEffect:
                    {
                    OMX_CONFIG_IMAGEFILTERTYPE effect;
                    OMX_INIT_STRUCT_PTR (&effect, OMX_CONFIG_IMAGEFILTERTYPE);
                    effect.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
                    effect.eImageFilter = (OMX_IMAGEFILTERTYPE)Gen3A.Effect;

                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonImageFilter, &effect);
                    CAMHAL_LOGDB("effect for OMX = 0x%x", (int)effect.eImageFilter);
                    CAMHAL_LOGDB("effect for Hal = %d", Gen3A.Effect);
                    break;
                    }

                case SetFocus:
                    {

                    //First Disable Face and Region priority
                    setAlgoPriority(FACE_PRIORITY, FOCUS_ALGO, false);
                    setAlgoPriority(REGION_PRIORITY, FOCUS_ALGO, false);

                    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;
                    OMX_INIT_STRUCT_PTR (&focus, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
                    focus.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

                    focus.eFocusControl = (OMX_IMAGE_FOCUSCONTROLTYPE)Gen3A.Focus;

                    ret = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);
                    CAMHAL_LOGDB("Focus type in hal , OMX : %d , 0x%x", Gen3A.Focus, focus.eFocusControl );

                    break;
                    }

                case SetExpMode:
                    {
                    ret = setExposureMode(Gen3A);

                    break;
                    }

                case SetFlash:
                    {
                    ret = setFlashMode(mParameters3A);
                    break;
                    }

                default:
                    CAMHAL_LOGEB("this setting (0x%x) is still not supported in CameraAdapter ", currSett);
                    break;
                }
                mPending3Asettings &= ~currSett;
            }
        }

        return ret;
}

int OMXCameraAdapter::getLUTvalue_HALtoOMX(const char * HalValue, LUTtype LUT)
{
    int LUTsize = LUT.size;
    if( HalValue )
        for(int i = 0; i < LUTsize; i++)
            if( 0 == strcmp(LUT.Table[i].userDefinition, HalValue) )
                return LUT.Table[i].omxDefinition;

    return -1;
}

const char* OMXCameraAdapter::getLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT)
{
    int LUTsize = LUT.size;
    for(int i = 0; i < LUTsize; i++)
        if( LUT.Table[i].omxDefinition == OMXValue )
            return LUT.Table[i].userDefinition;

    return NULL;
}

// Set AutoConvergence
status_t OMXCameraAdapter::setAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE pACMode, OMX_S32 pManualConverence)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_CONVERGENCETYPE ACParams;

    LOG_FUNCTION_NAME

    ACParams.nSize = sizeof(OMX_TI_CONFIG_CONVERGENCETYPE);
    ACParams.nVersion = mLocalVersionParam;
    ACParams.nPortIndex = OMX_ALL;
    ACParams.nManualConverence = pManualConverence;
    ACParams.eACMode = pACMode;
    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_TI_IndexConfigAutoConvergence, &ACParams);
    if ( eError != OMX_ErrorNone )
        {
        CAMHAL_LOGEB("Error while setting AutoConvergence 0x%x", eError);
        ret = -EINVAL;
        }
    else
        {
        CAMHAL_LOGDA("AutoConvergence applied successfully");
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// Get AutoConvergence
status_t OMXCameraAdapter::getAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE *pACMode, OMX_S32 *pManualConverence)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_TI_CONFIG_CONVERGENCETYPE ACParams;

    ACParams.nSize = sizeof(OMX_TI_CONFIG_CONVERGENCETYPE);
    ACParams.nVersion = mLocalVersionParam;
    ACParams.nPortIndex = OMX_ALL;

    LOG_FUNCTION_NAME

    eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE)OMX_TI_IndexConfigAutoConvergence, &ACParams);
    if ( eError != OMX_ErrorNone )
        {
        CAMHAL_LOGEB("Error while getting AutoConvergence 0x%x", eError);
        ret = -EINVAL;
        }
    else
        {
        *pManualConverence = ACParams.nManualConverence;
        *pACMode = ACParams.eACMode;
        CAMHAL_LOGDA("AutoConvergence got successfully");
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}



// Motorola specific - begin

// This function is used when setting bit TestPattern1_EnManualExposure
status_t OMXCameraAdapter::setEnManualExposure(bool bEnable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE exposure;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&exposure, OMX_CONFIG_EXPOSUREVALUETYPE);
        exposure.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDB("old bAutoSensitivity is: %d ", exposure.bAutoSensitivity);

            exposure.bAutoSensitivity = (OMX_BOOL)(!bEnable);
            CAMHAL_LOGDB("new bAutoSensitivity is: %d ", exposure.bAutoSensitivity);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring EnManualExposure"
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting attribute DebugAttrib_ExposureTime
status_t OMXCameraAdapter::setExposureTime(unsigned int expTimeMicroSec)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE exposure;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&exposure, OMX_CONFIG_EXPOSUREVALUETYPE);
        exposure.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if ( NO_ERROR == ret )
                {
            CAMHAL_LOGDB("old exposure time is: nShutterSpeedMsec = %ld miliseconds", exposure.nShutterSpeedMsec);

            exposure.nShutterSpeedMsec = expTimeMicroSec/1000; // converts micro- to mili- seconds
            CAMHAL_LOGDB("new exposure time set is: nShutterSpeedMsec = %ld miliseconds = (%d [us]/1000)",
                         exposure.nShutterSpeedMsec, expTimeMicroSec);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring ExposureTime"
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting attribute DebugAttrib_ExposureGain
status_t OMXCameraAdapter::setExposureGain(int expGain100thdB)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE exposure;
    double dB;

    LOG_FUNCTION_NAME
    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&exposure, OMX_CONFIG_EXPOSUREVALUETYPE);
        exposure.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDB("old exposure gain is: nShutterSpeedMsec = %ld [0.01 dB]", exposure.nSensitivity);

            dB = expGain100thdB/100;
            exposure.nSensitivity = (OMX_U32)( pow(10, dB/10) * ISO100 );
            CAMHAL_LOGDB("new exposure gain is: nShutterSpeedMsec = %ld [0.01 dB]", exposure.nSensitivity);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring ExposureGain"
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting bit TestPattern1_TargetedExposure
status_t OMXCameraAdapter::setEnableTargetedExposure(bool bEnable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_TARGETEXPOSURE targetedExposure;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&targetedExposure, OMX_CONFIG_TARGETEXPOSURE);
        targetedExposure.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigTargetExposure, &targetedExposure);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDB("old bUseTargetExposure is: %d ", targetedExposure.bUseTargetExposure);

            targetedExposure.bUseTargetExposure = (OMX_BOOL)bEnable;
            CAMHAL_LOGDB("new bUseTargetExposure is: %d ", targetedExposure.bUseTargetExposure);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigTargetExposure, &targetedExposure);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring TargetedExposure bit (enabled/disabled)"
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting attribute DebugAttrib_TargetExpValue [0..255], ex. 64
status_t OMXCameraAdapter::setTargetExpValue(unsigned char expTimeMicroSec)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_TARGETEXPOSURE targetedExposure;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&targetedExposure, OMX_CONFIG_TARGETEXPOSURE);
        targetedExposure.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigTargetExposure, &targetedExposure);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }

        if (NO_ERROR == ret)
            {
            CAMHAL_LOGDB("old nTargetExposure is: %d ", targetedExposure.nTargetExposure);

            targetedExposure.nTargetExposure = (OMX_U8)expTimeMicroSec;
            CAMHAL_LOGDB("new nTargetExposure is:  %d ", targetedExposure.nTargetExposure);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigTargetExposure, &targetedExposure);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring nTargetExposure 0..255 value. "
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }

        }
    LOG_FUNCTION_NAME_EXIT

    return ret;
}


// This function is used when setting bit TestPattern1_TargetedExposure
status_t OMXCameraAdapter::setEnableColorBars(bool bEnable)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BOOLEANTYPE colorBars;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&colorBars, OMX_CONFIG_BOOLEANTYPE);

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigColorBars, &colorBars);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if (NO_ERROR == ret)
            {
            CAMHAL_LOGDB("old colorBars.bEnabled is: %d ", colorBars.bEnabled);

            colorBars.bEnabled = (OMX_BOOL)bEnable;
            CAMHAL_LOGDB("new colorBars.bEnabled is: %d ", colorBars.bEnabled);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigColorBars, &colorBars);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring ColorBars bit (enabled/disabled). "
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting attribute DebugAttrib_EnLensPosGetSet
status_t OMXCameraAdapter::setEnLensPosGetSet(int val)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;

    LOG_FUNCTION_NAME
    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ((0 == val) || (1 == val))
        {
        mEnLensPos = val;
        }
    else
        {
        CAMHAL_LOGEB("Warning: DebugAtrib_EnLensPosGetSet accepts values: 0 or 1.\n"
                     "You are trying to set it to %d. Focus mode will be unchanged.", val);
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting attribute DebugAttrib_LensPosition
status_t OMXCameraAdapter::setLensPosition(int positionPercent)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;

    LOG_FUNCTION_NAME
    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if (( NO_ERROR == ret ) &&
        (1 == mEnLensPos) &&
        (OMX_IMAGE_FocusControlOff == mParameters3A.Focus)) // Focus mode "fixed"
        {
        // Set manual focus position,
        // only if KEY_DEBUGATTRIB_ENLENSPOSGETSET was set to 1.

        OMX_INIT_STRUCT_PTR (&focus, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
        focus.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDB("old nFocusSteps value is %d", focus.nFocusSteps);

            // set autofocus to manual control
            focus.eFocusControl = OMX_IMAGE_FocusControlOn;
            focus.nFocusSteps = positionPercent;

            CAMHAL_LOGDB("new nFocusSteps value is %d", focus.nFocusSteps);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring LensPosition"
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}


status_t OMXCameraAdapter::setMIPIReset(void)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_MIPICOUNTERS mipiCounters;


    LOG_FUNCTION_NAME
    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&mipiCounters, OMX_CONFIG_MIPICOUNTERS);
        mipiCounters.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               (OMX_INDEXTYPE) OMX_IndexConfigMipiCounters, &mipiCounters);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x when getting OMX_IndexConfigMipiCounters", eError);
            ret = -1;
            }

        if ( NO_ERROR == ret )
            {
            CAMHAL_LOGDB("old bResetMIPICounter value is %d", mipiCounters.bResetMIPICounter);

            mipiCounters.bResetMIPICounter = OMX_TRUE;

            CAMHAL_LOGDB("new bResetMIPICounter value is %d", mipiCounters.bResetMIPICounter);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                   (OMX_INDEXTYPE) OMX_IndexConfigMipiCounters, &mipiCounters);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while setting bResetMIPICounter = true for OMX_IndexConfigMipiCounters."
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;

}

// This function is used when setting LedFlash Intensity
status_t OMXCameraAdapter::setLedFlash(int nLedFlashIntensP)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_LEDINTESITY LedIntensity;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&LedIntensity, OMX_CONFIG_LEDINTESITY);

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigLedIntensity, &LedIntensity);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if (NO_ERROR == ret)
            {
            CAMHAL_LOGDB("old LedIntensity.nLedFlashIntens is: %d ", LedIntensity.nLedFlashIntens);

            LedIntensity.nLedFlashIntens = (OMX_U32)nLedFlashIntensP;
            CAMHAL_LOGDB("new LedIntensity.nLedFlashIntens is: %d ", LedIntensity.nLedFlashIntens);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigLedIntensity, &LedIntensity);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring LedFlash Intensity. "
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used when setting LedTorch Intensity
status_t OMXCameraAdapter::setLedTorch(int nLedTorchIntensP)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_LEDINTESITY LedIntensity;
    char value[PROPERTY_VALUE_MAX];
    unsigned int torchIntensity = DEFAULT_INTENSITY;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&LedIntensity, OMX_CONFIG_LEDINTESITY);

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigLedIntensity, &LedIntensity);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if (NO_ERROR == ret)
            {
            CAMHAL_LOGDB("old LedIntensity.nLedTorchIntens is: %d ", LedIntensity.nLedTorchIntens);

            // read product specific torvh value
            if (property_get("ro.media.capture.torchIntensity", value, 0) > 0)
                {
                torchIntensity = atoi(value);
                if ((torchIntensity < 0) || (torchIntensity > DEFAULT_INTENSITY))
                    {
                    torchIntensity = DEFAULT_INTENSITY;
                    }
                }
            else
                {
                torchIntensity = nLedTorchIntensP;
                }

            LedIntensity.nLedTorchIntens = (OMX_U32)torchIntensity;
            CAMHAL_LOGDB("new LedIntensity.nLedTorchIntens is: %d ", LedIntensity.nLedTorchIntens);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigLedIntensity, &LedIntensity);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring LedTorch Intensity. "
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}


// This function is used when setting ManulaExposure Time, ms, used by camera_test
status_t OMXCameraAdapter::setManualExposureTimeMs(unsigned int expTimeMilliSec)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE exposure;

    LOG_FUNCTION_NAME

    if ( OMX_StateInvalid == mComponentState )
        {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
        }

    if ( NO_ERROR == ret )
        {
        OMX_INIT_STRUCT_PTR (&exposure, OMX_CONFIG_EXPOSUREVALUETYPE);
        exposure.nPortIndex = OMX_ALL;

        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);
        if ( OMX_ErrorNone != eError )
            {
            CAMHAL_LOGEB("OMXGetConfig() returned error 0x%x", eError);
            ret = -1;
            }
        if ( NO_ERROR == ret )
                {
            CAMHAL_LOGDB("old manual exposure time is: nShutterSpeedMsec = %ld miliseconds", exposure.nShutterSpeedMsec);
            if ( expTimeMilliSec > 0 )
                {
                exposure.bAutoShutterSpeed = OMX_FALSE;
                exposure.nShutterSpeedMsec = expTimeMilliSec; // Note: this is in milliseconds.
                }
            else
                {
                exposure.bAutoShutterSpeed = OMX_TRUE;
                }
            CAMHAL_LOGDB("new manual exposure time set is: nShutterSpeedMsec = %ld miliseconds = (%d [us]/1000)",
                         exposure.nShutterSpeedMsec, expTimeMilliSec);
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp, OMX_IndexConfigCommonExposureValue, &exposure);

            if ( OMX_ErrorNone != eError )
                {
                CAMHAL_LOGEB("Error while configuring manual exposure time "
                             "OMXSetConfig() returned error 0x%x", eError);
                ret = -1;
                }
            }
        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

// This function is used by CameraCalibration to read the sensor EEPROM data
status_t OMXCameraAdapter::getOTPEeprom(unsigned char * pData, unsigned long nSize)
{
    OMX_CONFIG_OTPEEPROM OTPEeprom;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    OMX_INIT_STRUCT_PTR (&OTPEeprom, OMX_CONFIG_OTPEEPROM);
    OTPEeprom.nPortIndex = OMX_ALL;

    OMX_GetConfig(mCameraAdapterParameters.mHandleComp, (OMX_INDEXTYPE) OMX_IndexConfigOTPEeprom, &OTPEeprom);
    if (OMX_ErrorNone != eError)
        {
        CAMHAL_LOGEB("OMXGetConfig(OMX_IndexConfigOTPEeprom) returned error 0x%x", eError);
        ret = -EINVAL;
        }
    else if (NULL == pData)
        {
        CAMHAL_LOGEA("Destination pData is NULL.");
        ret = -EINVAL;
        }
    else if (nSize != OTPEeprom.nDataSize)
        {
        CAMHAL_LOGEB("Data size requested does not match the EEPROM size. size is %d", OTPEeprom.nDataSize);
        ret = -EINVAL;
        }
    else if (OMX_FALSE == OTPEeprom.bValid)
        {
        CAMHAL_LOGEA("EEPROM data was not read correctly by the driver.");
        ret = -EINVAL;
        }
    else
        {
        memcpy(pData, OTPEeprom.pData, OTPEeprom.nDataSize);
        }

    return ret;
}

int OMXCameraAdapter::getCameraCalStatus()
{
    int calStatus = CAL_STATUS_NOT_SUPPORTED;
    uint8_t  pOTPEepromData[SENSOR_EEPROM_SIZE];
    uint32_t CameraID_read_from_file;
    uint32_t CameraID;
    const char calFile[] = "/pds/camera/module1/cameracal.bin" ;
    int fd = -1;
    int ret = NO_ERROR;

    // Read CameraID from EEPROM
    memset(pOTPEepromData, 0x0, SENSOR_EEPROM_SIZE);
    ret = getOTPEeprom(pOTPEepromData, SENSOR_EEPROM_SIZE);
    if (NO_ERROR != ret)
        {
        CAMHAL_LOGEA("OTP_ERROR: OMXCameraAdapter::getOTPEeprom() failed");
        }
    else
        {
        CameraID = ((pOTPEepromData[OTP_BYTE_CAMERA_ID]<<24)|
                    (pOTPEepromData[OTP_BYTE_CAMERA_ID+1]<<16)|
                    (pOTPEepromData[OTP_BYTE_CAMERA_ID+2]<<8)|
                    (pOTPEepromData[OTP_BYTE_CAMERA_ID+3]));
        CAMHAL_LOGDB("CameraID read from OTP EEPROM is: 0x%08x", CameraID);

        fd = open(calFile, O_APPEND| O_RDWR | O_SYNC , 0777);

        if (fd < 0)
            {
            /* The file doesn't exist. */
            calStatus = CAL_STATUS_DATA_MISSING;
            }
        else
            {
            /* Verifying CameraID from output file */
            lseek(fd, DCC_HEADER_SIZE, SEEK_SET);
            read(fd, &CameraID_read_from_file,sizeof(CameraID_read_from_file));
            if (CameraID != CameraID_read_from_file)
                {
                /* CameraID does not match. Leaving output file unchanged. */
                calStatus = CAL_STATUS_DATA_MISMATCH;
                }
           else
                {
                calStatus = CAL_STATUS_SUCCESS;
                }
           close(fd);
           }
        }

    CAMHAL_LOGDB("Camera calibration status = %d", calStatus);
    return calStatus;
}

bool OMXCameraAdapter::getCameraModuleQueryString( char *str, unsigned long length)
{

    uint8_t otpData[SENSOR_EEPROM_SIZE];
    char currentString[SENSOR_EEPROM_SIZE];
    int dataStart;
    int offset =0;
    bool ret = false;

    memset(otpData, 0x0, SENSOR_EEPROM_SIZE);

    if ( getOTPEeprom(otpData, SENSOR_EEPROM_SIZE) != NO_ERROR)
        {
        CAMHAL_LOGEA("OTP_ERROR: OMXCameraAdapter::getOTPEeprom() failed");
        }
    else
        {
        for ( int i = 0 ; i<sizeof(moduleQueryFilter)/sizeof(ModuleQueryFilter) ; i++)
            {
            if (offset>=length) break;

            dataStart = moduleQueryFilter[i].offset ;

            switch(moduleQueryFilter[i].elemType) {

                case modInfo_character:
                     strlcpy(currentString,
                     (const char *)&otpData[moduleQueryFilter[i].offset],
                               sizeof(currentString)) ;
                               offset += snprintf(str+offset,
                               length-offset,
                               "%s=%s",
                               moduleQueryFilter[i].name,
                               currentString);
                     break;

                case modInfo_uint16:
                     offset += snprintf(str +offset,
                               length-offset,
                               "%s=%04x",
                               moduleQueryFilter[i].name,
                               otpData[dataStart]<<8|otpData[dataStart+1]);
                     break;

                case modInfo_uint32:
                     offset += snprintf(str +offset,
                               length-offset,
                               "%s=%08x",moduleQueryFilter[i].name,
                               otpData[dataStart]<<24|otpData[dataStart+1]<<16|
                               otpData[dataStart+2]<<8|otpData[dataStart+3]);
                     break;

                case modInfo_byte:
                     offset += snprintf(str +offset,
                               length-offset,
                                "%s=",
                               moduleQueryFilter[i].name);
                     for (int i2=0;i2<moduleQueryFilter[i].length;i2++)
                         {
                         offset += snprintf(str +offset, length-offset,
                                   "%02x",otpData[dataStart+i2]);
                         }
                     break;

                default:
                     break;
                }

                if (i<sizeof(moduleQueryFilter)/sizeof(ModuleQueryFilter)-1)
                   offset += snprintf(str+offset,length-offset,",");
            }
         snprintf(str+offset,2,"\0");
         CAMHAL_LOGDB("%s", str);
         ret = true;

       }
    return ret;
}

// Motorola specific - end


bool OMXCameraAdapter::CommandHandler::Handler()
{
    Message msg = {0,0,0,0,0,0};
    volatile int forever = 1;
    status_t stat;
    ErrorNotifier *errorNotify = NULL;

    LOG_FUNCTION_NAME

    while(forever){
        stat = NO_ERROR;
        CAMHAL_LOGDA("waiting for messsage...");
        MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        mCommandMsgQ.get(&msg);
        CAMHAL_LOGDB("msg.command = %d", msg.command);
        switch ( msg.command ) {
            case CommandHandler::CAMERA_START_IMAGE_CAPTURE:
            {
                stat = mCameraAdapter->startImageCapture();
                break;
            }
            case CommandHandler::CAMERA_PERFORM_AUTOFOCUS:
            {
                stat = mCameraAdapter->doAutoFocus();
                break;
            }
            case CommandHandler::COMMAND_EXIT:
            {
                CAMHAL_LOGEA("Exiting command handler");
                forever = 0;
                break;
            }
        }

        if ( NO_ERROR != stat )
            {
            errorNotify = ( ErrorNotifier * ) msg.arg1;
            if ( NULL != errorNotify )
                {
                errorNotify->errorNotify(CAMERA_ERROR_UKNOWN);
                }
            }
    }

    LOG_FUNCTION_NAME_EXIT
    return false;
}


OMXCameraAdapter::OMXCameraAdapter():mComponentState (OMX_StateInvalid)
{
    LOG_FUNCTION_NAME

    mHDRCaptureEnabled = false;
    mFocusStarted = false;
    mPictureRotation = 0;
    // Initial values
    mTimeSourceDelta = 0;
    mOnlyOnce = true;

    mCameraAdapterParameters.mHandleComp = 0;
    signal(SIGTERM, SigHandler);
    signal(SIGALRM, SigHandler);

    LOG_FUNCTION_NAME_EXIT
}

OMXCameraAdapter::~OMXCameraAdapter()
{
    LOG_FUNCTION_NAME
    LOGD("\n\n**** ~OMXCameraAdapter called() ! ****\n\n");

    if (mHDRCaptureEnabled)
        {
        if(mHDRInterface != NULL)
            {
                delete mHDRInterface;
                mHDRInterface = NULL;
            }
        }

   ///Free the handle for the Camera component
    if(mCameraAdapterParameters.mHandleComp)
        {
        OMX_FreeHandle(mCameraAdapterParameters.mHandleComp);
        }

    ///De-init the OMX
    if( (mComponentState==OMX_StateLoaded) || (mComponentState==OMX_StateInvalid))
        {
        OMX_Deinit();
        }

    //Exit and free ref to command handling thread
    if ( NULL != mCommandHandler.get() )
    {
        Message msg = {0,0,0,0,0,0};
        msg.command = CommandHandler::COMMAND_EXIT;
        msg.arg1 = mErrorNotifier;
        mCommandHandler->put(&msg);
        mCommandHandler->requestExitAndWait();
        mCommandHandler.clear();
    }

    LOG_FUNCTION_NAME_EXIT
}

extern "C" CameraAdapter* CameraAdapter_Factory()
{
    Mutex::Autolock lock(gAdapterLock);

    LOG_FUNCTION_NAME

    if ( NULL == gCameraAdapter )
        {
        CAMHAL_LOGDA("Creating new Camera adapter instance");
        gCameraAdapter= new OMXCameraAdapter();
        }
    else
        {
        CAMHAL_LOGDA("Reusing existing Camera adapter instance");
        }


    LOG_FUNCTION_NAME_EXIT

    return gCameraAdapter;
}

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

