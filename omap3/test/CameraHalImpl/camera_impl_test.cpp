#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <CameraHal.h>
#include <TICameraParameters.h>

#define LOG_TAG "camera_impl_test"
#define LOG_FUNCTION_NAME         LOGD("%s %d: %s() ENTER", __FILE__, __LINE__, __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT    LOGD("%s %d: %s() EXIT", __FILE__, __LINE__, __FUNCTION__);

#define LEN_OUTPUT_DIR 256

using namespace android;

// Global variables
long int data_cbPreviewFramesCounter = 0;
long int data_cbCaptureFramesCounter = 0;
int previewMaxCaptures;
int previewPeriodInFrames;
int captureMaxCaptures;

char output_dir[LEN_OUTPUT_DIR];

// used to print in file name
int filenamePreviewWidth;
int filenamePreviewHeight;

class ImplTest{

private:
    sp<CameraHal> instCamera;
    CameraParameters params;
    int width, height;
    void* user;

public:
    ImplTest();
    ~ImplTest();

    void printSupportedParams();

    void TestStartPreviewSaveFrames(long int previewNumFramesP,
                                int previewPeriodInFramesP,
                                int previewMaxCapturesP);

    void TestCaptureSaveFrames(int captureMaxCapturesP,
                               char * par_PrvFormat_P,
                               int par_PrvW_P,
                               int par_PrvH_P,
                               const char * par_CapFormat_P,
                               int par_CapW_P,
                               int par_CapH_P,
                               bool bDisableLDC,
                               int sensor_id_P);

    void RunMipiMenuTest(void);


};

// =============================================================================
// Description: Saves a preview buffer to file. Counts files internally.
//              This function is used by the data_cb callback to
//              to save buffers.
//
// mem - a memory buffer;
// file_name_to_concat - file name with directory, that will be used to
//              concatenate the automatic numbeing;
// file_extension - a string for the file name extension "jpg" or yuv";
//
//              E.g. file_name_to_concat = "/images/preview"
//                   file_extension = "yuv"
//
//              will generate file names like: "/images/preview001.yuv"
// =============================================================================
void saveFile(const sp<IMemory>& mem,
              const char * output_dir,
              const char * file_name_to_concat,
              const char * file_extension)
{
    static int      counter = 1;
    unsigned char   *buff = NULL;
    int             size;
    int             fd = -1;
    char            fn[300];

    LOG_FUNCTION_NAME

    if (mem == NULL)
        goto out;

    fn[0] = 0;
    // example /images/preview001.yuv
    // create output dir, if it does not exist
    sprintf(fn, "mkdir %s", output_dir);
    system(fn);
    // open file name
    sprintf(fn, "%s/%s%03d.%s", output_dir, file_name_to_concat, 
                                counter, file_extension);
    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    if(fd < 0) {
        LOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        LOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        LOGE("Buffer pointer is invalid");
        goto out;
    }

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d, file: %s\n",
           __FUNCTION__, (int)buff, size, fn);

out:

    if (fd >= 0)
    {
        close(fd);
        LOGD("File %s written.", fn);
        printf("File %s written.\n", fn);
    }

    LOG_FUNCTION_NAME_EXIT
}

// Set the three callback bunctions:
//    notify_callback notify_cb;
//    data_callback data_cb;
//    data_callback_timestamp data_cb_timestamp;


// =============================================================================
//
// Description: The callback function of type  notify_callback.
//
// =============================================================================
void impltest_notify_cb(int32_t msgType,
                        int32_t ext1,
                        int32_t ext2,
                        void* user)
{
    LOGD("impltest_notify_cb() called\n");
    // This function is not used for this test.
    // This is to remove warnings of unused variables.
    msgType = msgType;
    ext1 = ext1;
    ext2 = ext2;
    user = user;
}

// =============================================================================
// Description: The callback function of type data_callback.
//
// Notes:
//       similar to: void CameraHandler::
//                        postData(int32_t msgType, const sp<IMemory>& dataPtr)
//       but plus a cookie parameter.
// =============================================================================
void impltest_data_cb (int32_t msgType,
                   const sp<IMemory>& dataPtr,
                   void *cookie)
{
    char str[256];
    if ( msgType & CAMERA_MSG_PREVIEW_FRAME )
    {
        data_cbPreviewFramesCounter++;
        LOGD("impltest_data_cb() preview frame # %ld, Period = %d, maxCaptures = %d \n",
             data_cbPreviewFramesCounter, previewPeriodInFrames, previewMaxCaptures);

        // save one given frame
        if (previewPeriodInFrames != 0)
        {
            if ((data_cbPreviewFramesCounter%previewPeriodInFrames == 0) &&
                (data_cbPreviewFramesCounter/previewPeriodInFrames <= previewMaxCaptures))
            {
                sprintf(str, "preview_%dx%d_", filenamePreviewWidth, filenamePreviewHeight);
                saveFile(dataPtr,"/sdcard/images", str, "yuv");
            }
        }
    }
    else if ( msgType & CAMERA_MSG_COMPRESSED_IMAGE )
    {
        // save captured image whenever it is received:
        saveFile(dataPtr, "/sdcard/images/","image", "jpg");
    }else if ( msgType & CAMERA_MSG_RAW_IMAGE)
    {
        saveFile(dataPtr, "/sdcard/images/","image", "raw");
    }
    else
    {
        LOGD("data callback with unknown msgType");
    }

}

// =============================================================================
//
// Description: The callback function of type data_callback_timestamp.
//
// =============================================================================
void testimpl_data_cb_timestamp(nsecs_t timestamp,
                                int32_t msgType,
                                const sp<IMemory>& dataPtr,
                                void* user,
                                uint32_t offset, uint32_t stride)
{
    LOGD("testimpl_data_cb_timestamp() called\n");

    // This function is not used for this test.
    // This is to remove warnings of unused variables.
    timestamp = timestamp;
    msgType = msgType;
    user = user;
    offset = offset;
    stride = stride;
}



// callbacks for attrib mipi test

// =============================================================================
//
// Description: The callback function of type  notify_callback.
//
// =============================================================================
void attrib_mipi_impltest_notify_cb(int32_t msgType,
                        int32_t ext1,
                        int32_t ext2,
                        void* user)
{
    LOGD("attrib_mipi_impltest_notify_cb() called\n");
    // This function is not used for this test.
    // This is to remove warnings of unused variables.
    msgType = msgType;
    ext1 = ext1;
    ext2 = ext2;
    user = user;
}

// =============================================================================
// Description: The callback function of type data_callback.
//
// Notes:
//       similar to: void CameraHandler::
//                        postData(int32_t msgType, const sp<IMemory>& dataPtr)
//       but plus a cookie parameter.
// =============================================================================
void attrib_mipi_impltest_data_cb (int32_t msgType,
                   const sp<IMemory>& dataPtr,
                   void *cookie)
{
    LOGD("attrib_mipi_impltest_data_cb() called\n");
    // This function is not used for this test.
    // This is to remove warnings of unused variables.
    msgType = msgType;
    cookie = cookie;
}

// =============================================================================
//
// Description: The callback function of type data_callback_timestamp.
//
// =============================================================================
void attrib_mipi_testimpl_data_cb_timestamp(nsecs_t timestamp,
                                int32_t msgType,
                                const sp<IMemory>& dataPtr,
                                void* user,
                                uint32_t offset, uint32_t stride)
{
    LOGD("attrib_mipi_testimpl_data_cb_timestamp() called\n");
    // This function is not used for this test.
    // This is to remove warnings of unused variables.
    timestamp = timestamp;
    msgType = msgType;
    user = user;
    offset = offset;
    stride = stride;
}


ImplTest::ImplTest()
{
    if (instCamera.get() == NULL)
        LOGE("Before allocating memory, instCamera.get() == NULL");
    else
        LOGE("Before allocating memory, instCamera.get() is not NULL");

    instCamera = new CameraHal();
    if (!instCamera.get())
    {
        LOGE("Could not create instance of CameraHalIml class");
    }
    LOGD("**** instCamera.get() = %x" ,  (unsigned int)(instCamera.get()) );
    instCamera->initialize();
    data_cbPreviewFramesCounter = 0;
    data_cbCaptureFramesCounter = 0;

};

ImplTest::~ImplTest()
{

    LOGD("**** ImpltTest destructor **** \n");
    if (instCamera.get()){
        LOGD("**** instCamera.get() = %x" , (unsigned int)(instCamera.get()) );
        LOGD("**** ImpltTest destructor: now will call instCamera.clear() **** \n");
        instCamera.clear();
    }

};

void ImplTest::printSupportedParams()
{
    printf("\n\r\tSupported Cameras: %s", params.get("camera-indexes"));
    printf("\n\r\tSupported Picture Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES));
    printf("\n\r\tSupported Picture Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS));
    printf("\n\r\tSupported Preview Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
    printf("\n\r\tSupported Preview Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS));
    printf("\n\r\tSupported Preview Frame Rates: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES));
    printf("\n\r\tSupported Jpeg Thumbnail Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES));
    printf("\n\r\tSupported Whitebalance Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE));
    printf("\n\r\tSupported Effects: %s", params.get(CameraParameters::KEY_SUPPORTED_EFFECTS));
    printf("\n\r\tSupported Scene Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES));
    printf("\n\r\tSupported Focus Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
    printf("\n\r\tSupported Antibanding Options: %s", params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING));
    printf("\n\r\tSupported Flash Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES));
}



// =========================================================================
//
// previewNumFramesP  - number of preview frames to receive
// previewPeriodInFramesP - prediod between preview frames to be saved
// previewMaxCapturesP  - number of captured YUV files from preview frames
//
// =========================================================================
void ImplTest::TestStartPreviewSaveFrames(long int previewNumFramesP,
                                int previewPeriodInFramesP,
                                int previewMaxCapturesP)
{
    LOG_FUNCTION_NAME

    if (instCamera.get())
    {
        // Set internal variables
        previewMaxCaptures = previewMaxCapturesP;
        previewPeriodInFrames = previewPeriodInFramesP;

        instCamera->EnableTestMode(true);

        // set callbacks
        instCamera->setCallbacks(impltest_notify_cb,
                                 impltest_data_cb,
                                 testimpl_data_cb_timestamp,
                                user);
        // enable preview callbacks
        instCamera->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);

        // get parameters
        params = instCamera->getParameters();
        printSupportedParams();

        params.getPreviewSize(&width, &height);
        LOGD("PreviewSize: width = %d, height = %d", width, height);
        LOGD("PreviewFormat = %s", params.getPreviewFormat());
        params.getPictureSize(&width, &height);
        LOGD("PictureSize: width = %d, height = %d", width, height);

        // set parameters
        LOGD("Setting PreviewSize, PreviewFormat, PictureSize");
        // The following variables are used to prepare the file name of yuv file
        filenamePreviewWidth = 320;
        filenamePreviewHeight = 240;
        params.setPreviewSize(filenamePreviewWidth, filenamePreviewHeight);
        params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV422I);
        params.setPictureSize(3264, 2448);
        instCamera->setParameters(params);

        // get parameters
        params = instCamera->getParameters();
        params.getPreviewSize(&width, &height);
        LOGD("PreviewSize: width = %d, height = %d", width, height);
        LOGD("PreviewFormat = %s", params.getPreviewFormat());
        params.getPictureSize(&width, &height);
        LOGD("PictureSize: width = %d, height = %d", width, height);

        //start preview
        // passing NULL disables Overlay
//        instCamera->setOverlay(NULL); - moved to CameraHal::startPreview() 
        instCamera->startPreview();


        LOGD("Waiting %ld preview frames to be received "
             "by data_cb callback", previewNumFramesP);
        printf("Running %ld preview frames and saving each %d-th frame, up to max %d frames."
               "\n Preview frames will be saved as .yuv files - format YUV422I\n",
               previewNumFramesP, previewPeriodInFramesP, previewMaxCapturesP);
        while (data_cbPreviewFramesCounter <= previewNumFramesP);

        instCamera->stopPreview();

        instCamera->EnableTestMode(false);

    }
    else
    {
        LOGE("instCamera was not created properly");
    }



    LOG_FUNCTION_NAME_EXIT
}




// =============================================================================
// Test Capturing of specified format
//
// captureNumFramesP  - number of preview frames to receive
// num_capturesP  - number of captured YUV files from preview frames
//
// par_PrvFormat_P -
//
//     Example values:
//          CameraParameters::PIXEL_FORMAT_YUV420SP[] = "yuv420sp"
//          CameraParameters::PIXEL_FORMAT_YUV422I[] = "yuv422i-yuyv"
//
// par_PrvW_P - ex. 320
// par_PrvH_P - ex. 240
//
// par_CapFormat_P - format of the captured image to be requested
//     Example values:
//          CameraParameters::PIXEL_FORMAT_JPEG[] = "jpeg"
//          TICameraParameters::PIXEL_FORMAT_RAW[] = "raw"
//
// par_CapW_P - ex. 320, 3264
// par_CapH_P - ex. 240, 2448
//
// Ref: Preview and Capture formats:
//      frameworks/base/libs/camera/CameraParameters.cpp
//
// bool bDisableLDC - if true, Attrib will be called to disable LDC
//                             (Lens Distortion Correction)
// int sensor_id_P - integet index of the sendor, passed to parameters key 
//     "camera-sensor", defined by Motorola.
//     Note: The default key "camera-index" is not set.
//     The purpose is to test Motorola requireq key "camera-sensor".
// =============================================================================
void ImplTest::TestCaptureSaveFrames(int captureMaxCapturesP,
                                    char * par_PrvFormat_P,
                                    int par_PrvW_P,
                                    int par_PrvH_P,
                                    const char * par_CapFormat_P,
                                    int par_CapW_P,
                                    int par_CapH_P,
                                    bool bDisableLDC,
                                    int sensor_id_P)
{
    LOG_FUNCTION_NAME

    if (instCamera.get())
    {
        // Set internal variables
        // These parameters are used in data callback impltest_data_cb()
        data_cbPreviewFramesCounter = 0;
        data_cbCaptureFramesCounter = 0;
        previewMaxCaptures = 0; // no captures from preview
        captureMaxCaptures = captureMaxCapturesP;



        instCamera->EnableTestMode(true);

        // set callbacks
        instCamera->setCallbacks(impltest_notify_cb,
                                impltest_data_cb,
                                testimpl_data_cb_timestamp,
                                user);
        // enable preview callbacks
        instCamera->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);

        // enable capture callbacks, returned after takePicture calls
        if (0 == strcmp(par_CapFormat_P,CameraParameters::PIXEL_FORMAT_JPEG))
        {
            instCamera->enableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
        }
        else if (0 == strcmp(par_CapFormat_P,TICameraParameters::PIXEL_FORMAT_RAW))
        {
            instCamera->enableMsgType(CAMERA_MSG_RAW_IMAGE);
        }
        else
        {
            LOGE("\nWarning : unknown capture format specified: %s\n", par_CapFormat_P);
            printf("\nWarning : unknown capture format specified: %s\n", par_CapFormat_P);
            printf("Did you mean: %s or %s ?\n\n",
                   CameraParameters::PIXEL_FORMAT_JPEG,
                   TICameraParameters::PIXEL_FORMAT_RAW);
        }

        // get parameters
        params = instCamera->getParameters();
        printSupportedParams();

        params.getPreviewSize(&width, &height);
        LOGD("PreviewSize: width = %d, height = %d", width, height);
        LOGD("PreviewFormat = %s", params.getPreviewFormat());
        params.getPictureSize(&width, &height);
        LOGD("PictureSize: width = %d, height = %d", width, height);
        LOGD("PictureFormat = %s", params.getPictureFormat());

        // set parameters
        LOGD("Setting PreviewSize, PreviewFormat, PictureSize");
        filenamePreviewWidth = par_PrvW_P;  // 320;
        filenamePreviewHeight = par_PrvH_P; // 240;
        params.setPreviewSize(filenamePreviewWidth, filenamePreviewHeight);

        //params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV422I);
        //params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);
        params.setPreviewFormat(par_PrvFormat_P);

        //params.setPictureSize(3264, 2448);
        //params.setPictureSize(4000, 3000);
        //params.setPictureSize(320, 240);
        params.setPictureSize(par_CapW_P, par_CapH_P);
        params.set(TICameraParameters::KEY_CAP_MODE, TICameraParameters::HIGH_QUALITY_MODE);
        // Select Jpeg or Bayer10 format, depending on parameter:
        params.setPictureFormat(par_CapFormat_P);

        LOGD("KEY_CAMERA_MOTO (\"camera-sensor\") old value: %d",
             params.getInt(TICameraParameters::KEY_CAMERA_MOTO));
        params.set(TICameraParameters::KEY_CAMERA_MOTO, sensor_id_P);

/*
        if (0 == strcmp(par_CapFormat_P,CameraParameters::PIXEL_FORMAT_JPEG))
        {
            // Set JPEG quality
            params.set(CameraParameters::KEY_JPEG_QUALITY, 50); // 0..100 %
        }

        // Turn off lens distortion correction (LDC):
        // params.set(TICameraParameters::KEY_IPP, TICameraParameters::IPP_NONE);
        // implemented in TestPattern1_DisableLensCorrection:
        // instCamera.SetDebugAttrib(CameraHal::DebugAttrib_TestPattern1,
        //                           CameraHal::TestPattern1_DisableLensCorrection);
        // */

//        instCamera->SetDebugAttrib(CameraHal::DebugAttrib_TestPattern1,
//                                   CameraHal::TestPattern1_ColorBars);

/**/
        instCamera->setParameters(params);

        // test disabling Lens Distortion Correction
        if (bDisableLDC)
        {
            instCamera->SetDebugAttrib(CameraHal::DebugAttrib_TestPattern1,
                                       CameraHal::TestPattern1_DisableLensCorrection);
        }
        // get parameters
        params = instCamera->getParameters();
        params.getPreviewSize(&width, &height);
        LOGD("PreviewSize: width = %d, height = %d", width, height);
        LOGD("PreviewFormat = %s", params.getPreviewFormat());
        params.getPictureSize(&width, &height);
        LOGD("PictureSize: width = %d, height = %d", width, height);
        LOGD("PictureFormat = %s", params.getPictureFormat());

        LOGD("KEY_CAMERA_MOTO (\"camera-sensor\") new value set: %d",
             params.getInt(TICameraParameters::KEY_CAMERA_MOTO));

        LOGD("Running preview frames and making %d "
               "captures - saved to jpg/raw files.\n", captureMaxCapturesP);
        printf("Running preview frames and making %d "
               "captures - saved to jpg/raw files.\n", captureMaxCapturesP);
        //start preview
        // passing NULL disables Overlay
//        instCamera->setOverlay(NULL); - moved to CameraHal::startPreview() 
        instCamera->startPreview();


        while (data_cbCaptureFramesCounter < captureMaxCapturesP)
        {
            LOGD("data_cbCaptureFramesCounter = %ld of %d total captures\n",
                   data_cbCaptureFramesCounter, captureMaxCapturesP);
            printf("data_cbCaptureFramesCounter = %ld of %d total captures\n",
                   data_cbCaptureFramesCounter, captureMaxCapturesP);

            data_cbCaptureFramesCounter++;

            LOGD("PreviewFramesCounter = %ld of total %d frames., Calling takePicture() ...",
                 data_cbPreviewFramesCounter, captureMaxCapturesP);
            instCamera->takePicture();


            if (0 == strcmp(par_CapFormat_P,CameraParameters::PIXEL_FORMAT_JPEG))
            {
                LOGD("Waiting for some preview to run - 10 s\n");
                printf("Waiting for some preview to run - 10 s\n");
                sleep(10);
                LOGD("10 s passed, continuing with next capture ...\n");
                printf("10 s passed, continuing with next capture ...\n");
            }
            if (0 == strcmp(par_CapFormat_P,TICameraParameters::PIXEL_FORMAT_RAW))
            {
                LOGD("Waiting for some preview to run - 15 s\n");
                printf("Waiting for some preview to run - 15 s\n");
                sleep(15);
                LOGD("15 s passed, continuing with next capture ...\n");
                printf("15 s passed, continuing with next capture ...\n");
            }


        };

        instCamera->stopPreview();

/* This disabling of messages is not needed :
        // disabled ports, the same way as enabling them before preview
        // disable preview callbacks
        instCamera->disableMsgType(CAMERA_MSG_PREVIEW_FRAME);

        // disable capture callbacks, returned after takePicture calls
        if (0 == strcmp(par_CapFormat_P,CameraParameters::PIXEL_FORMAT_JPEG))
        {
            instCamera->disableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
        }
        else if (0 == strcmp(par_CapFormat_P,TICameraParameters::PIXEL_FORMAT_RAW))
        {
            instCamera->disableMsgType(CAMERA_MSG_RAW_IMAGE);
        }
        else
        {
            LOGE("\nWarning : unknown capture format specified: %s\n", par_CapFormat_P);
            printf("\nWarning : unknown capture format specified: %s\n", par_CapFormat_P);
            printf("Did you mean: %s or %s ?\n\n",
                   CameraParameters::PIXEL_FORMAT_JPEG,
                   TICameraParameters::PIXEL_FORMAT_RAW);
        }
        */

        instCamera->EnableTestMode(false);

    }
    else
    {
        LOGE("instCamera was not created properly");
    }



    LOG_FUNCTION_NAME_EXIT
}


void ImplTest::RunMipiMenuTest(void)
{
    char ch = 0;
    bool bPrintMenu = TRUE;
    unsigned long resL;
    CameraHal::CameraTestInputs  inputsL;
    CameraHal::CameraTestOutputs outputsL;

    LOG_FUNCTION_NAME

    if (instCamera.get())
    {
        // Set internal variables


        instCamera->EnableTestMode(true);

        // set callbacks
        instCamera->setCallbacks(attrib_mipi_impltest_notify_cb,
                                 attrib_mipi_impltest_data_cb,
                                 attrib_mipi_testimpl_data_cb_timestamp,
                                user);
        // enable preview callbacks
        instCamera->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);

        // get parameters
        params = instCamera->getParameters();

        // set parameters
        LOGD("Setting PreviewSize, PreviewFormat, PictureSize");
        // The following variables are used to prepare the file name of yuv file
        filenamePreviewWidth = 320;
        filenamePreviewHeight = 240;
        params.setPreviewSize(filenamePreviewWidth, filenamePreviewHeight);
        params.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV422I);
        params.setPictureSize(3264, 2448);
        instCamera->setParameters(params);

        //start preview
        // passing NULL disables Overlay
//        instCamera->setOverlay(NULL); - moved to CameraHal::startPreview()
        instCamera->startPreview();


        // Interactive menu of the test:
        do
        {
            if (TRUE == bPrintMenu)
            {
                printf("\n\n");
                printf("MIPI counter test menu    \n");
                printf("========================= \n");
                printf(" 1. Get Mipi counter      \n");
                printf(" 2. Get ECC counter       \n");
                printf(" 3. Get CRC counter       \n");
                printf(" 4. Reset MIPI counter    \n");
                printf("                          \n");
                printf(" p. Start Preview         \n");
                printf(" s. Stop  Preview         \n");
                printf("                          \n");
                printf(" q. Quit.\n");
                printf("                          \n");
                printf("Choice: \n");
            }
            ch = getchar();

            bPrintMenu = TRUE;
            switch (ch)
            {
            case '1':
                resL = instCamera->GetDebugAttrib(CameraHal::DebugAttrib_MipiFrameCount);
                printf("\n GetDebugAttrib() returned: Mipi counter = %lu \n", resL);

                inputsL.test.MIPICounterOp = CameraHal::GET_MIPI_COUNTER;
                instCamera->PerformCameraTest(CameraHal::CameraTest_MIPICounter, &inputsL, &outputsL);
                printf("\n PerformCameraTest() returned: Mipi counter = %lu \n",
                        outputsL.test.MIPICounter.FrameCount);
                break;

            case '2':
                resL = instCamera->GetDebugAttrib(CameraHal::DebugAttrib_MipiEccErrors);
                printf("\n GetDebugAttrib() returned: ECC counter = %lu \n", resL);

                inputsL.test.MIPICounterOp = CameraHal::GET_MIPI_COUNTER;
                instCamera->PerformCameraTest(CameraHal::CameraTest_MIPICounter, &inputsL, &outputsL);
                printf("\n PerformCameraTest() returned: ECC counter = %lu \n",
                        outputsL.test.MIPICounter.ECCErrors);
                break;

            case '3':
                resL = instCamera->GetDebugAttrib(CameraHal::DebugAttrib_MipiCrcErrors);
                printf("\n GetDebugAttrib() returned: CRC counter = %lu \n", resL);

                inputsL.test.MIPICounterOp = CameraHal::GET_MIPI_COUNTER;
                instCamera->PerformCameraTest(CameraHal::CameraTest_MIPICounter, &inputsL, &outputsL);
                printf("\n PerformCameraTest() returned: CRC counter = %lu \n",
                        outputsL.test.MIPICounter.CRCErrors);
                break;

            case '4':
                printf("\n Reseting Mipi counter.\n");
                inputsL.test.MIPICounterOp = CameraHal::RESET_MIPI_COUNTER;
                instCamera->PerformCameraTest(CameraHal::CameraTest_MIPICounter, &inputsL, &outputsL);
                break;

            case 'p':
                instCamera->startPreview();
                printf("\n Preview started. \n");
                break;

            case 's':
                instCamera->stopPreview();
                printf("\n Preview stopped. \n");
                break;

            case 'q':
                printf("\n Quitting... \n");
                break;

            default:
                bPrintMenu = FALSE;
                break;
            }
        }while (ch != 'q');


        instCamera->stopPreview();

        instCamera->EnableTestMode(false);

    }
    else
    {
        LOGE("instCamera was not created properly");
    }



    LOG_FUNCTION_NAME_EXIT
}


void print_usage(char *argv[])
{
    printf("\n");
    printf("Usage: \n");
    printf("       %s preview <output_dir>\n", argv[0]);
    printf("       %s capture <output_dir> <prv. format> <prv. W> <prv. H>  \n", argv[0]);
    printf("                               <cap. format> <cap. W> <cap. H>                  \n");
    printf("                  <disableLDC> <sensor>\n");
    printf("       %s attrib <attrib_test>\n", argv[0]);
    printf("--------------------------------------------------------------------------------\n");
    printf("preview - Test receiving of preview buffers and saving to yuv files             \n");
    printf("           (not displayed)\n");
    printf("                                                                                \n");
    printf("capture - Test capturing of jpeg or raw images with different resolutions       \n");
    printf("                                                                                \n");
    printf("    <output_dir>  : output directory. ex. /sdcard/images or /images             \n");
    printf("                    Note: if the given directory  does not exist,               \n");
    printf("                          it will be created automatically.                     \n");
    printf("                                                                                \n");
    printf("    <prv. format> : \"yuv420sp\", \"yuv422i-yuyv\"                              \n");
    printf("                                                                                \n");
    printf("    <prv. W>      : 320, 800, etc.                                              \n");
    printf("    <prv. H>      : 240, 480, etc.                                              \n");
    printf("                                                                                \n");
    printf("    <cap. format> : \"jpeg\", \"raw\"                                           \n");
    printf("        jpeg - test capturing of jpeg images                                    \n");
    printf("        raw  - test capturing of Bayer10 raw images.                            \n");
    printf("                                                                                \n");
    printf("    <cap. W>      : 3264, 320, etc.                                             \n");
    printf("    <cap. H>      : 2448, 240, etc.                                             \n");
    printf("                                                                                \n");
    printf("                                                                                \n");
    printf("    <disableLDC>  : \"noLDC\": disables Lens Distortion Correction              \n");
    printf("                        via SetDebugAttrib() interface,                         \n");
    printf("                        with flag TestPattern1_DisableLensCorrection.           \n");
    printf("                    \"-\":  does not disable LDC.                               \n");
    printf("                                                                                \n");
    printf("    <sensor>      : 0 - primary sensor   (max res. 3264x2448)                   \n");
    printf("                    1 - secondary sensor (max res. 230x240)                     \n");
    printf("                    This sets the value of \"camera-sensor\" key.               \n");
    printf("                                                                                \n");
    printf("attrib - Test DebugAttrib interface                                             \n");
    printf("                                                                                \n");
    printf("    <attrib_test> : mipi - test MIPI, ECC, CRC counters withinteractive menu    \n");
    printf("                                                                                \n");
    printf("EXAMPLE TESTS:                                                                  \n");
    printf("                                                                                \n");
    printf("%s preview /sdcard/images \n", argv[0]);
    printf("%s capture /sdcard/images yuv422i-yuyv 320 240 jpeg 320 240 - 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv422i-yuyv 320 240 jpeg 640 480 - 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv422i-yuyv 640 480 jpeg 320 240 - 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv422i-yuyv 640 480 jpeg 640 480 - 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv422i-yuyv 640 480 jpeg 3264 2448 - 0\n", argv[0]);
    printf("                                                                                \n");
    printf("%s capture /sdcard/images yuv420sp     320 240 jpeg 320 240 - 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv420sp     640 480 jpeg 640 480 - 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv420sp     640 480 jpeg 3264 2448 - 0\n", argv[0]);
    printf("                                                                                \n");
    printf("%s capture /sdcard/images yuv420sp     320 240 jpeg 320 240 noLDC 0\n", argv[0]);
    printf("%s capture /sdcard/images yuv420sp     320 240 jpeg 320 240 noLDC 1\n", argv[0]);
    printf("                                                                                \n");
    printf("%s capture /sdcard/images yuv420sp     320 240 raw  320 240 - 0\n", argv[0]);
    printf("                                                                                \n");
    printf("%s attrib mipi\n", argv[0]);
    printf("                                                                                \n");
    printf("\n");
    printf("\n");
}


int main(int argc, char *argv[])
{
    ImplTest * impltest;

    LOG_FUNCTION_NAME

    printf("\nTest Application for the CameraHalImpl class, v04 - (1).\n");

    if (argc <= 1 )
    {
        print_usage(argv);
        return 0;
    }
    else if (0==strcmp(argv[1], "preview") )
    {
        if ((argc > 3 ))
        {
            print_usage(argv);
            printf("\nError: Too many parameters for preview.\n\n");
            return 0;
        }
        // only "preview" parameter is given
    }
    else if (0==strcmp(argv[1], "capture") )
    {
        if (argc > 11 )
        {
            print_usage(argv);
            printf("\nError: Too many parameters.\n\n");
            return 0;
        }
        else
        {
            if ((0!=strcmp(argv[6],"jpeg")) &&
                (0!=strcmp(argv[6],"raw"))
               )
            {
                print_usage(argv);
                printf("\nError: Wrong <cap. format> %s: expected jpeg or raw. \n\n", argv[5]);
                return 0;
            }
        }
    }
    else if (0==strcmp(argv[1], "attrib") )
    {
        if ((argc > 3 ))
        {
            print_usage(argv);
            printf("\nError: Too many parameters for preview.\n\n");
            return 0;
        }
    }
    else
    {
        print_usage(argv);
        return 0;
    }

    impltest = new ImplTest();
    if (!impltest)
    {
        LOGE("Could not create ImplTest class instance!");
        printf("Could not create ImplTest class instance!\n");
    }
    else
    {
        if (0==strcmp(argv[1], "preview"))
        {
            if ( strlen(argv[2]) < LEN_OUTPUT_DIR )
            {
                strcpy(output_dir, argv[2]);
                printf("===== Saving some yuv images from preview ===== \n");
                impltest->TestStartPreviewSaveFrames(330, 30, 10);
                // 11 sec of 30 fps = 330 frames
            }
            else
            {
                LOGE("<output_dir> is too long");
                printf("<output_dir> is too long\n");
            }
        }
        else if (0==strcmp(argv[1], "capture"))
        {
            if ( strlen(argv[2])< LEN_OUTPUT_DIR )
            {
                strcpy(output_dir, argv[2]);
                printf("===== Capturing JPEG or raw image ===== \n");
                // CameraParameters::PIXEL_FORMAT_JPEG
                // TICameraParameters::PIXEL_FORMAT_RAW
                impltest->TestCaptureSaveFrames(1,
                                                argv[3], atoi(argv[4]), atoi(argv[5]),
                                                argv[6], atoi(argv[7]), atoi(argv[8]),
                                                (bool)(0==strcmp(argv[9], "noLDC")), atoi(argv[10]));
            }
            else
            {
                LOGE("Error: <output_dir> is too long");
                printf("\nError: <output_dir> is too long\n");
            }
        }
        else if (0==strcmp(argv[1], "attrib"))
        {
            if  (0==strcmp(argv[2], "mipi"))
            {
                impltest->RunMipiMenuTest();
            }
            else
            {
                LOGE("Error: parameters %s not supported", argv[2]);
                printf("\nError: parameters %s not supported\n", argv[2]);
            }
        }
    }

    if (impltest)
    {
        LOGD("**** now will call \"delete impltest\" **** \n");
        delete impltest;
    }

    printf("End of test.\n");
    LOG_FUNCTION_NAME_EXIT
}


