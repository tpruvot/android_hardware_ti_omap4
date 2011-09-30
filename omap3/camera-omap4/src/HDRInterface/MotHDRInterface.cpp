#define LOG_TAG "MotHDRInterface"

#include <ctype.h>
#include <pthread.h>

#include "MotHDRInterface.h"
#include "MotDebug.h"

#define LINE_BYTES(Width, BitCt) (((long)(Width) * (BitCt) + 31) / 32 * 4)

//Below macros dumps the input buffers in /data/data/
//#define DUMP_INPUT_RAW_IMAGES_AS_JPEG
//#define DUMP_YUV_INPUT_BUFFERS

namespace android {

class MotHDRInterface::Worker {
public:
    Worker(MotHDRInterface* pHDRIntf);
    ~Worker();

    status_t start();
    void stop();

private:
    volatile bool mDone;
    MotHDRInterface* mHDRIntf;
    pthread_t mThread;
    static void *ThreadWrapper(void *me);
    void threadEntry();
    Worker(const Worker &);
    Worker &operator=(const Worker &);
};

/*
 * MotHDRInterface class
 */

MotHDRInterface::MotHDRInterface(CameraAdapter *omxCamAdap)
{
    DMOTLOGV("Constructor\n");
    mImageWidth = 0;
    mImageHeight = 0;
    mImagePitch = 0;
    mImageLength = 0;
    mHdrResultData = NULL;

    mOmxCamAdapter = omxCamAdap;
    mJpegDataBuff = NULL;
}

MotHDRInterface::~MotHDRInterface()
{
    stop();
}

bool MotHDRInterface::init()
{
    DMOTLOGV("MotHDRInterface::init Enter\n");
    mWorker = new Worker(this);
    if (mWorker == NULL)
    {
        return false;
    }
    return mWorker->start();
    DMOTLOGV("MotHDRInterface::init Exit\n");
}

void MotHDRInterface::setImageSize(int width, int height)
{
    DMOTLOGV("MotHDRInterface::setImageSize Enter\n");
    if ((mImageWidth != width) || (mImageHeight != height))
    {
        mImageWidth = width;
        mImageHeight = height;
        RAWPictureHeapForUnderExp.clear();
        RAWPictureMemBaseForUnderExp.clear();
        RAWPictureHeapForNormalExp.clear();
        RAWPictureMemBaseForNormalExp.clear();
        RAWPictureHeapForOverExp.clear();
        RAWPictureMemBaseForOverExp.clear();
    }
    DMOTLOGV("MotHDRInterface::setImageSize Exit\n");
}

int MotHDRInterface::setDataBuffers(MByte* pBuf, unsigned int filledLen, unsigned int offset)
{
    DMOTLOGV("MotHDRInterface::setDataBuffers Enter\n\n");
    static int count = 0;

#ifdef DUMP_YUV_INPUT_BUFFERS
    FILE* fp = NULL;
    char filename[100];
    sprintf(filename, "/data/data/InputBuffer%d.yuv", (count+1));
    DMOTLOGV("Filename = %s\n", filename);
    fp = fopen(filename, "wb");
    if ( fp != NULL)
    {
    fwrite((const uint8_t *)pBuf + offset,
                1,
                filledLen,
                fp);
    fclose(fp);
    }
    else
    {
        DMOTLOGV("ifp is NULL\n");
    }
#endif

    // Since Tiler memory is not shared, don't use it directly
    // Once the tiler memory is shared,
    // below code should be changed to use the buffer pointers directly
    if (count == 0)
    {
        if ((RAWPictureHeapForUnderExp.get() == NULL) && (RAWPictureMemBaseForUnderExp.get() == NULL))
        {
            RAWPictureHeapForUnderExp = new MemoryHeapBase(filledLen);
            RAWPictureMemBaseForUnderExp = new MemoryBase(RAWPictureHeapForUnderExp, 0, filledLen);
        }
        if (RAWPictureMemBaseForUnderExp->pointer() != NULL)
            memcpy(RAWPictureMemBaseForUnderExp->pointer(), ( void * ) ( (unsigned int) pBuf + offset) , filledLen);
    }
    else if (count == 1)
    {
        if ((RAWPictureHeapForNormalExp.get() == NULL) && (RAWPictureMemBaseForNormalExp.get() == NULL))
        {
            RAWPictureHeapForNormalExp = new MemoryHeapBase(filledLen);
            RAWPictureMemBaseForNormalExp = new MemoryBase(RAWPictureHeapForNormalExp, 0, filledLen);
        }
        if (RAWPictureMemBaseForNormalExp->pointer() != NULL)
            memcpy(RAWPictureMemBaseForNormalExp->pointer(), ( void * ) ( (unsigned int) pBuf + offset) , filledLen);
    }
    else
    {
        if ((RAWPictureHeapForOverExp.get() == NULL) && (RAWPictureMemBaseForOverExp.get() == NULL))
        {
            RAWPictureHeapForOverExp = new MemoryHeapBase(filledLen);
            RAWPictureMemBaseForOverExp = new MemoryBase(RAWPictureHeapForOverExp, 0, filledLen);
        }
        if (RAWPictureMemBaseForOverExp->pointer())
            memcpy(RAWPictureMemBaseForOverExp->pointer(), ( void * ) ( (unsigned int) pBuf + offset) , filledLen);
    }

    count++;
    if(count == 3)
    {
        count = 0;
        mCondition.signal();
    }
    DMOTLOGV("MotHDRInterface::setDataBuffers Exit\n\n");
    return 1;
}

void MotHDRInterface::stop()
{
    DMOTLOGV("MotHDRInterface::stop Enter\n\n");
    //TODO : Remove buffer allocation and clean up once CameraHAL
    // can provide 3 shared buffers
    RAWPictureHeapForUnderExp.clear();
    RAWPictureMemBaseForUnderExp.clear();
    RAWPictureHeapForNormalExp.clear();
    RAWPictureMemBaseForNormalExp.clear();
    RAWPictureHeapForOverExp.clear();
    RAWPictureMemBaseForOverExp.clear();

    if (mJpegDataBuff != NULL)
    {
        free(mJpegDataBuff);
        mJpegDataBuff = NULL;
    }
    if (mHdrResultData != NULL)
    {
        MMemFree(MNull, mHdrResultData);
        mHdrResultData = NULL;
    }
    if(mWorker != NULL)
    {
        delete(mWorker);
        mWorker = NULL;
    }
    DMOTLOGV("MotHDRInterface::stop Exit\n\n");
}

/*
 * Worker class
 */

MotHDRInterface::Worker::Worker(MotHDRInterface* pHdrIntf)
    : mDone(false),
      mHDRIntf(pHdrIntf)
{
    DMOTLOGV("MotHDRInterface::Worker::constructor\n");
}

MotHDRInterface::Worker::~Worker()
{
    stop();
}

status_t MotHDRInterface::Worker::start()
{
    DMOTLOGV("MotHDRInterface::Worker::start enter\n");

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    mDone = false;

    DMOTLOGV("MotHDRInterface::Worker::start before pthread_create\n");
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    DMOTLOGV("MotHDRInterface::Worker::start after pthread_create\n");
    pthread_attr_destroy(&attr);

    DMOTLOGV("MotHDRInterface::Worker::start exit\n");
    return OK;
}

void MotHDRInterface::Worker::stop()
{
    DMOTLOGV("MotHDRInterface::Worker::stop enter\n");
    if (mDone)
        return;

    mDone = true;
    mHDRIntf->mCondition.signal();

    void *dummy;
    pthread_join(mThread, &dummy);

    DMOTLOGV("MotHDRInterface::Worker::stop exit\n");
}

void *MotHDRInterface::Worker::ThreadWrapper(void *me)
{
    Worker *worker = static_cast<Worker *>(me);
    worker->threadEntry();
    return NULL;
}

void MotHDRInterface::Worker::threadEntry()
{
    DMOTLOGV("MotHDRInterface::Worker::threadEntry enter\n");
    int32_t numBuffers = 0;

    while (!mDone)
    {
        Mutex::Autolock l(mHDRIntf->mMutex);
        mHDRIntf->mCondition.wait(mHDRIntf->mMutex);

        if ( !mDone )
        {
            mHDRIntf->HDRInterfaceProcess();
        }

    }
    DMOTLOGV("MotHDRInterface::Worker::threadEntry Exit\n");
}

MBool MotHDRInterface::HDRInterfaceProcess()
{
    DMOTLOGV("MotHDRInterface::HDRInterfaceProcess Enter\n");
    MBool ret = MTrue;
    if((RAWPictureMemBaseForNormalExp->pointer() == MNull) || (RAWPictureMemBaseForUnderExp->pointer() == MNull) || (RAWPictureMemBaseForOverExp->pointer() == MNull))
    {
        return MFalse;
    }

    mImagePitch = LINE_BYTES(mImageWidth, 8);
    mImageLength = (mImagePitch * mImageHeight*3)/2;
    if(mHdrResultData!=NULL)
    {
        MMemFree(MNull, mHdrResultData);
        mHdrResultData = NULL;
    }
    mHdrResultData = (MByte*)MMemAlloc(MNull, mImageLength);

    ret = HDRProcess((MByte*)RAWPictureMemBaseForUnderExp->pointer(), (MByte*)RAWPictureMemBaseForNormalExp->pointer(), (MByte*)RAWPictureMemBaseForOverExp->pointer(), mHdrResultData, mImagePitch, mImageWidth, mImageHeight);
    DMOTLOGV("HDRProcess finish, %s.\n", ret?"Success":"Failed");
    DMOTLOGV("MotHDRInterface::HDRInterfaceProcess Exit\n");
    return ret;
}

void MotHDRInterface::provideResultantBuffToCamHAL(ASVLOFFSCREEN* exposureImgOut)
{
    DMOTLOGV("MotHDRInterface::provideResultantBuffToCamHAL Enter\n");
    long buffLength;
    if (mJpegDataBuff != NULL)
    {
        free(mJpegDataBuff);
        mJpegDataBuff = NULL;
    }

    // Arcsoft should provide Init and Uninit fns for jpeg encode
    // the below API allocates memory for encoded buffer but doesn't free it
    // As such it's more likely chances of memleak from client side
    SaveToJpgBuffer(&mJpegDataBuff, &buffLength, exposureImgOut, 80);

    mOmxCamAdapter->sendCommand(CameraAdapter::CAMERA_COMPLETE_HDR_PROCESSING, (int)mJpegDataBuff, (int)buffLength, mImagePitch);
    DMOTLOGV("MotHDRInterface::provideResultantBuffToCamHAL Exit\n");
}

MBool MotHDRInterface::HDRProcess(MByte* pUnderExposure, MByte* pNormalExposure, MByte* pOverExposure, MByte* pHdrResultData, int image_pitch, int image_width, int image_height)
{
    DMOTLOGV("MotHDRInterface::HDRProcess Enter\n");
    MRESULT  ret     = MOK;
    MHandle phExpEffect = MNull;
    ASVLOFFSCREEN mExposureImgIn[3];
    ASVLOFFSCREEN mExposureImgOut;
    AME_PARAM AdjustParam = {0};
    AME_ALIGNSIZE AlignSize = {0};
    MVoid *membuffer=MNull;
    MHandle memHandle=MNull;

    if(pUnderExposure == MNull || pNormalExposure == MNull || pOverExposure == MNull
        || pHdrResultData == MNull || image_pitch <= 0 || image_width <= 0 || image_height <= 0)
    {
        return MFalse;
    }

    //under exposure
    mExposureImgIn[0].i32Width = image_width;
    mExposureImgIn[0].i32Height = image_height;
    mExposureImgIn[0].pi32Pitch[0] = image_pitch;
    mExposureImgIn[0].pi32Pitch[1] = image_pitch;
    mExposureImgIn[0].ppu8Plane[0] = pUnderExposure;
    mExposureImgIn[0].ppu8Plane[1] = pUnderExposure + image_pitch * image_height;
    mExposureImgIn[0].u32PixelArrayFormat = ASVL_PAF_NV12;
#ifdef DUMP_INPUT_RAW_IMAGES_AS_JPEG
   SaveToJpg("/data/data/InImg1.jpeg", &mExposureImgIn[0], 80);
#endif

    //normal exposure
    mExposureImgIn[1].i32Width = image_width;
    mExposureImgIn[1].i32Height = image_height;
    mExposureImgIn[1].pi32Pitch[0] = image_pitch;
    mExposureImgIn[1].pi32Pitch[1] = image_pitch;
    mExposureImgIn[1].ppu8Plane[0] = pNormalExposure;
    mExposureImgIn[1].ppu8Plane[1] = pNormalExposure + image_pitch * image_height;
    mExposureImgIn[1].u32PixelArrayFormat = ASVL_PAF_NV12;
#ifdef DUMP_INPUT_RAW_IMAGES_AS_JPEG
   SaveToJpg("/data/data/InImg2.jpeg", &mExposureImgIn[1], 80);
#endif

    //over exposure
    mExposureImgIn[2].i32Width = image_width;
    mExposureImgIn[2].i32Height = image_height;
    mExposureImgIn[2].pi32Pitch[0] = image_pitch;
    mExposureImgIn[2].pi32Pitch[1] = image_pitch;
    mExposureImgIn[2].ppu8Plane[0] = pOverExposure;
    mExposureImgIn[2].ppu8Plane[1] = pOverExposure + image_pitch * image_height;
    mExposureImgIn[2].u32PixelArrayFormat = ASVL_PAF_NV12;
#ifdef DUMP_INPUT_RAW_IMAGES_AS_JPEG
   SaveToJpg("/data/data/InImg3.jpeg", &mExposureImgIn[2], 80);
#endif

    //hdr result
    mExposureImgOut.i32Width = image_width;
    mExposureImgOut.i32Height = image_height;
    mExposureImgOut.pi32Pitch[0] = image_pitch;
    mExposureImgOut.pi32Pitch[1] = image_pitch;
    mExposureImgOut.ppu8Plane[0] = pHdrResultData;
    mExposureImgOut.ppu8Plane[1] = pHdrResultData + image_pitch * image_height;
    mExposureImgOut.u32PixelArrayFormat = ASVL_PAF_NV12;

    //create memory manager handle
    if (membuffer == NULL)
    {
        membuffer = MMemAlloc(MNull, 2000 *1024);
    }
    memHandle = MMemMgrCreate(membuffer, 2000 * 1024);

    ret = AME_EXP_Init(memHandle, &phExpEffect);
    if (ret != MERR_NONE)
    {
        goto Error;
    }
    else
    {
        DMOTLOGV("HDRProcess.AME_EXP_Init Success.\n");
    }
    AdjustParam.lToneDetail = 50;
    AdjustParam.lToneSmooth = 50;
    DMOTLOGV("Before AME_RunHighDynamicRangeImage start\n");
    ret = AME_RunHighDynamicRangeImage (memHandle, phExpEffect, mExposureImgIn, 3, &AlignSize, &mExposureImgOut, &AdjustParam, MNull, MNull);
    DMOTLOGV("After AME_RunHighDynamicRangeImage completed\n");
    if (ret != MERR_NONE)
    {
        goto Error;
    }
    else
    {
        DMOTLOGV("HDRProcess.AME_RunHighDynamicRangeImage Success.\n");
        provideResultantBuffToCamHAL(&mExposureImgOut);
    }

    ret = AME_EXP_Uninit(memHandle, phExpEffect);
    if (ret != MERR_NONE)
    {
        goto Error;
    }
    else
    {
        DMOTLOGV("HDRProcess.AME_EXP_Uninit Success.\n");
    }
    MMemMgrDestroy(memHandle);
    if (membuffer != NULL)
    {
        MMemFree(MNull, membuffer);
        membuffer = NULL;
    }
    DMOTLOGV("MotHDRInterface::HDRProcess Exit\n");
    return MTrue;

Error:
    DMOTLOGE("HDRProcess Error");
    MMemMgrDestroy(memHandle);
    if (membuffer != NULL)
    {
        MMemFree(MNull, membuffer);
        membuffer = NULL;
    }
    return MFalse;
}

} // namespace android
