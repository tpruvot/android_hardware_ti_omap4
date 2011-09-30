//
// This file contains Motorola extensions to the TI camera HAL.
//

#define LOG_TAG "CameraHalM"

#include <CameraHal.h>
#include <TICameraParameters.h>
#include <string.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <ISyslinkIpcListener.h>

#include <cutils/properties.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


#define WATCHDOG_SECONDS 15

#define EXT_MODULE_VENDOR_STRING "SHARP,OV8820"
#define INT_MODULE_VENDOR_STRING "SEMCO,OV7739"

#define DCM_CAM_CONFIG_FILE  "/data/system/dcm_camera.config"
#define CAMERA_ENABLED  0x73
#define CAMERA_DISABLED 0x45

#define MOT_BURST_COUNT        "mot-burst-picture-count"
#define MOT_BURST_COUNT_VALUES "mot-burst-picture-count-values"
#define MOT_MAX_BURST_SIZE     "mot-max-burst-size"
//Wide screen aspect ratio (16:9)
#define WIDE_AR 1.7

// TI supports arbitrary burst limits, so we
// go with what we've used on other platforms
#define SUPPORTED_BURSTS       "0,1,3,6,9"

#define TORCH_DRIVER_DEVICE "/dev/i2c-3"
#define TORCH_DEVICE_SLAVE_ADDR 0x53
#define TORCH_DEVICE_CONTROL_REG 0x10
#define TORCH_BRIGHTNESS_CONTROL_REG 0xA0
#define TORCH_DEFAULT_BRIGHTNESS 0x02
#define TORCH_ENABLE 0x0A
#define TORCH_DISABLE 0x0
#define MAX_EXPOSURE_COMPENSATION 30

namespace android {


bool CameraHal::i2cRW(int read_size, int write_size, unsigned char *read_data, unsigned char *write_data)
{

    int i2cfd = -1;
    int i,j;
    int ioctl_ret;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data i2c_trans;
    bool retL = 0;

    enum { I2C_WRITE, I2C_READ };
    enum { ARG_BUS = 1, ARG_ADDR, ARG_READCNT, ARG_DATA };

    msgs[0].addr = TORCH_DEVICE_SLAVE_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = write_size;
    msgs[0].buf = write_data;

    msgs[1].addr = TORCH_DEVICE_SLAVE_ADDR;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = read_size;
    msgs[1].buf = read_data;

    i2c_trans.msgs = NULL;
    i2c_trans.nmsgs = 0;

    if ((i2cfd = open(TORCH_DRIVER_DEVICE, O_RDWR)) < 0) {
       LOGD("failed to open %s", TORCH_DRIVER_DEVICE);
        return 1;
    }

    if ( write_size && read_size ) {
        i2c_trans.msgs = &msgs[0];
        i2c_trans.nmsgs = 2;
    }
    else if (write_size) {
        i2c_trans.msgs = &msgs[0];
        i2c_trans.nmsgs = 1;
    }
    else if (read_size) {
        i2c_trans.msgs = &msgs[1];
        i2c_trans.nmsgs = 1;
    }
    else {
        goto done;
    }

    if ( write_size > 0 ) {
       LOGD("I2C_WRITE");
        for( i = 0; i < write_size; i++ )
        {
            LOGD( "%02X ", write_data[i] );
        }
    }

    if( (ioctl_ret = ioctl( i2cfd, I2C_RDWR, &(i2c_trans))) < 0 )
    {
        LOGD("* failed driver call: ioctl returned %d", ioctl_ret);
        retL = 1;
        goto done;
    }


    if ( read_size > 0 ) {
       LOGD("I2C_READ");
        for( i = 0; i < read_size; i++ )
        {
            LOGD( "%02X ", read_data[i] );
        }
    }

done:
    /* close the hardware */
    close(i2cfd);

    return retL;

}

unsigned int CameraHal::getFlashIntensity()
{
    unsigned int flashIntensity = DEFAULT_INTENSITY;
    char value[PROPERTY_VALUE_MAX];
    unsigned long minFlashThreshold;

    if (property_get("ro.media.capture.flashIntensity", value, 0) > 0)
        {
        flashIntensity = atoi(value);
        }
    if (property_get("ro.media.capture.flashMinV", value, 0) > 0)
        {
        minFlashThreshold = atoi(value);
        }
    else
        {
        minFlashThreshold = FLASH_VOLTAGE_THRESHOLD2;
        }
    if ( minFlashThreshold != 0)
       {
           int fd = open("/sys/class/power_supply/battery/voltage_now", O_RDONLY);
           if (fd < 0)
               {
               LOGE("Battery status is unavailable, defaulting Flash intensity to MAX");
               }
           else
               {
               char buf[8];
               if ( read(fd, buf, 8) < 0 )
                   {
                   LOGE("Unable to read battery status, defaulting flash intensity to MAX\n");
                   }
               else
                   {
                   unsigned long batteryLevel;
                   buf[7] = '\0';
                   batteryLevel = atol(buf);
	           LOGD("Current battery Level, %d uv", batteryLevel);
                   if (batteryLevel < minFlashThreshold)
                       {
                       LOGW("Disabling flash due to low Battery");
                       flashIntensity = 0;
                       }
                   else if (batteryLevel < FLASH_VOLTAGE_THRESHOLD1)
                       {
                       flashIntensity = flashIntensity >> 1 ;
                       }
                   }
               close(fd);
               }
       }
    LOGD("flashIntensity = %d", flashIntensity);
    return flashIntensity;
}

bool CameraHal::GetCameraModuleQueryString( char *str, unsigned char length)
{
    return mCameraAdapter->getCameraModuleQueryString(str,length);
}

// This function processes our Motorola specific parameters.  Right now these are transformed
// into the appropriate TI parameters.  This makes merges easier, and allows TI to use their
// apps for testing.  In the future, we may want to take ownership of the TI Camera HAL and
// implement this inline.
status_t CameraHal::setMotParameters(CameraParameters &params)
{
    const char *valstr = NULL;

    if ((valstr = params.get(MOT_BURST_COUNT)) != NULL)
        {
	
// Commented out in order to turn off support for multi-shot/burst capture.
// Once the exposure is fixed by MMS, this can be turned back on
//            params.set(TICameraParameters::KEY_BURST, valstr);





// FIXME
// This code sets up burst to use the HIGH PERFORMANCE pipe instead of the HQ pipe.  It
// works, but there are some lingering issue to resolve before enabling it.
#if 0
            if (atoi(valstr) > 1)
                {
                    mBurstEnabled = true;
                    if ((valstr = params.get(TICameraParameters::KEY_CAP_MODE)) != NULL)
                        {
                            strncpy(mLastCapMode, valstr, sizeof(mLastCapMode));
                        }
                    // We use the HIGH_PERFORMANCE Ducati pipe for burst
                    params.set(TICameraParameters::KEY_CAP_MODE,
                               TICameraParameters::HIGH_PERFORMANCE_MODE);
                }
            else if (mBurstEnabled)
                {
                    // Burst was enabled, now it's getting turned off,
                    // need to restore previous capture mode
                    mBurstEnabled = false;
                    params.set(TICameraParameters::KEY_CAP_MODE,
                               mLastCapMode);
                }
#endif
        }

    return NO_ERROR;
}

void CameraHal::initDefaultMotParameters(CameraParameters &params)
{
    Vector<Size> captureSizes;
    int maxwidth = 0, maxheight = 0;
    int ws_maxwidth = 0, ws_maxheight = 0;
    char burstsize[16];

    params.getSupportedPictureSizes(captureSizes);

    /* Find the largerst available wide screen capture size resolution 
       to use for burst mode */
    for(int i = 0; i < captureSizes.size(); i++)
        {
            if(captureSizes[i].width > maxwidth ||
               captureSizes[i].height > maxheight)
                {
                    maxwidth = captureSizes[i].width;
                    maxheight = captureSizes[i].height;

		    /* If the current capture size aspect ratio matches the
                       wide screen aspect ratio (16:9) and it's the largest
                       we've found so far - select it */
		    if(((float)maxwidth/maxheight > WIDE_AR) &&
                       (maxwidth > ws_maxwidth || maxheight > ws_maxheight))
                    {
			ws_maxwidth = maxwidth;
                        ws_maxheight = maxheight;
                    }
                }
        }

    /* If we didn't find wide screen capture size resolution
       for burst capture, use the largest regular one (4:3) */
    if(ws_maxwidth > 0 && ws_maxheight > 0)
        snprintf(burstsize, sizeof(burstsize), "%dx%d", ws_maxwidth, ws_maxheight);
    else
        snprintf(burstsize, sizeof(burstsize), "%dx%d", maxwidth, maxheight);

// Commented out in order to turn off support for multi-shot/burst capture.
// Once the exposure is fixed by MMS, this can be turned back on
//    params.set(MOT_MAX_BURST_SIZE, burstsize);
//    params.set(MOT_BURST_COUNT_VALUES, SUPPORTED_BURSTS);

}

bool CameraHal::PerformCameraTest( CameraTest         test
                                    , CameraTestInputs  *inputs
                                    , CameraTestOutputs *outputs
                                    )
{
    CameraParameters params;
    int valueL;
    bool retL = false;

    // for OMAP4 only MIPICounter is applicable
    if (CameraTest_MIPICounter == test)
    {
        if (RESET_MIPI_COUNTER == inputs->test.MIPICounterOp)
        {
            // TODO: reset MIPI counter
            params = getParameters();
            LOGD("Setting MIPIReset to TRUE");
            params.set(TICameraParameters::KEY_DEBUGATTRIB_MIPIRESET, 1);
            setParameters(params);

            retL = true;
        }
        if (GET_MIPI_COUNTER == inputs->test.MIPICounterOp)
        {
            // TODO: set returned output values here:
            LOGD("Getting Mipi Counters: ");
            params = getParameters();

            valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_MIPIFRAMECOUNT);
            outputs->test.MIPICounter.FrameCount = (unsigned long)valueL;
            LOGD("DebugAttrib_MipiFrameCount value is: %d, GetAttrib will return: %ld", valueL,
                  outputs->test.MIPICounter.FrameCount);

            valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_MIPIECCERRORS);
            outputs->test.MIPICounter.ECCErrors = (unsigned long)valueL;
            LOGD("DebugAttrib_MipiEccErrors value is: %d, GetAttrib will return: %ld", valueL,
                  outputs->test.MIPICounter.ECCErrors);

            valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_MIPICRCERRORS);
            outputs->test.MIPICounter.CRCErrors = (unsigned long)valueL;;
            LOGD("DebugAttrib_MipiCrcErrors value is: %d, GetAttrib will return: %ld", valueL,
                  outputs->test.MIPICounter.CRCErrors);

            retL = true;

        }
    }
    return retL;
}

// =============================================================================
//
// Description: Returns vendor string
//
// Defined as [Module Manufacturer],[Sensor Model],[Sensor Revision], plus optionally [Module ID],[Sensor ID],[Lens ID]
// Example: "Semco,Aptina5130,Rev8"
// =============================================================================
bool CameraHal::GetCameraVendorString( char *str, unsigned char length )
{
    bool retL;
    int vendorStrLenL;
    if (str != NULL)
    {
        if (!mCameraIndex)
        {
            vendorStrLenL = strlen(EXT_MODULE_VENDOR_STRING);
            if ( vendorStrLenL >= length)
            {
                LOGD("Vendor String is %d characters long. \n"
                "This is more than length = %d. \n"
                "Vendor string will be printed but truncated to length = %d",
                vendorStrLenL, length, length);
            }
            snprintf(str, length, EXT_MODULE_VENDOR_STRING);
            retL = true;
        }
        else
        {
            vendorStrLenL = strlen(INT_MODULE_VENDOR_STRING);
            if ( vendorStrLenL >= length)
            {
                LOGD("Vendor String is %d characters long. \n"
                "This is more than length = %d. \n"
                "Vendor string will be printed but truncated to length = %d",
                vendorStrLenL, length, length);
            }
            snprintf(str, length, INT_MODULE_VENDOR_STRING);
            retL = true;
        }
    }
    else
    {
        LOGE("str parameter is NULL - can't print vendor thring there");
        retL = false;
    }
    return retL;
}

// =============================================================================
//
// Description: Gets attributes during production test
//
// returns: Returns 0 for invalid attribute
//
// =============================================================================

unsigned long CameraHal::GetDebugAttrib( DebugAttrib attrib )
{
    unsigned long retL;
    CameraParameters params;
    int valueL;

    if (attrib < DebugAttrib_NUM)
    {
        //retL = arrDebugAttribs[attrib];
        retL = 0;
        switch (attrib)
        {
        case DebugAttrib_TestPattern1:
            LOGD("Getting DebugAttrib_TestPattern1: ");
            params = getParameters();
            //   , TestPattern1_ColorBars             = (1 << 0)
            if (params.get(TICameraParameters::KEY_TESTPATTERN1_COLORBARS) != NULL)
            {
                if (0 == strcmp(params.get(TICameraParameters::KEY_TESTPATTERN1_COLORBARS),
                            TICameraParameters::TESTPATTERN1_ENABLE))
                {
                    retL |= TestPattern1_ColorBars;
                    LOGD("TestPattern1_ColorBars             (1 << 0) bit is 1.");
                }
                else
                {
                    retL &= ~TestPattern1_ColorBars;
                    LOGD("TestPattern1_ColorBars             (1 << 0) bit is 0.");
                }
            }
            else
            {
                LOGE("TestPattern1_ColorBars is not supported");
            }
            //   , TestPattern1_WalkingOnes           = (1 << 1)  not done

            //   , TestPattern1_Bayer10AsRaw          = (1 << 2)
            if (params.get(TICameraParameters::PIXEL_FORMAT_RAW) != NULL)
            {
                if (0 == strcmp(TICameraParameters::PIXEL_FORMAT_RAW,
                                params.getPictureFormat()))
                {
                    retL |= TestPattern1_Bayer10AsRaw;
                    LOGD("TestPattern1_Bayer10AsRaw          (1 << 2) bit is 1. (PIXEL_FORMAT_RAW)");
                }
                else
                {
                    retL &= ~TestPattern1_Bayer10AsRaw;
                    LOGD("TestPattern1_Bayer10AsRaw          (1 << 2) bit is 0. (not PIXEL_FORMAT_RAW)");
                }
            }
            else
            {
                LOGE("TestPattern1_Bayer10AsRaw is not supported");
            }


            //   , TestPattern1_DisableSharpening     = (1 << 3)

            //   , TestPattern1_UnityGamma            = (1 << 4)

            //   , TestPattern1_EnManualExposure      = (1 << 5)
            if (params.get(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE) != NULL)
            {
                if (0==strcmp(params.get(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE),
                              TICameraParameters::TESTPATTERN1_ENABLE))
                {
                    retL |= TestPattern1_EnManualExposure;
                    LOGD("TestPattern1_EnManualExposure      (1 << 5) bit is 1.");
                }
                else
                {
                    retL &= ~TestPattern1_EnManualExposure;
                    LOGD("TestPattern1_EnManualExposure      (1 << 5) bit is 0.");
                }
            }
            else
            {
                LOGE("TestPattern1_EnManualExposure is not supported");
            }

            //   , TestPattern1_DisableLensCorrection = (1 << 6)
            if (params.get(TICameraParameters::KEY_IPP) != NULL)
            {
                if (0==strcmp(params.get(TICameraParameters::KEY_IPP),
                              TICameraParameters::IPP_NONE))
                {
                    retL |= TestPattern1_DisableLensCorrection; // true, LDC is disabled
                    LOGD("TestPattern1_DisableLensCorrection (1 << 6) bit is 1.");
                }
                else
                {
                    retL &= ~TestPattern1_DisableLensCorrection; // false, LDC is not disabled
                    LOGD("TestPattern1_DisableLensCorrection (1 << 6) bit is 0.");
                }
            }
            else
            {
                LOGE("TestPattern1_DisableLensCorrection is not supported");
            }

            //   , TestPattern1_TargetedExposure      = (1 << 7)
            if (params.get(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE) != NULL)
            {
                if (0 == strcmp(params.get(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE),
                            TICameraParameters::TESTPATTERN1_ENABLE))
                {
                    retL |= TestPattern1_TargetedExposure;
                    LOGD("TestPattern1_TargetedExposure      (1 << 7) bit is 1.");
                }
                else
                {
                    retL &= ~TestPattern1_TargetedExposure;
                    LOGD("TestPattern1_TargetedExposure      (1 << 7) bit is 0.");
                }
            }
            else
            {
                LOGE("TestPattern1_TargetedExposure is not supported");
            }
//   , TestPattern1_DisableWatermark      = (1 << 8)


            break;
        case DebugAttrib_EnLensPosGetSet:
            params = getParameters();
            if (params.get(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET);
                retL = (unsigned long)valueL;
                LOGD("DebugAttrib_EnLensPosGetSet value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
                LOGE("DebugAttrib_EnLensPosGetSet is not supported");
            }
            break;
        case DebugAttrib_LensPosition:
            params = getParameters();
            if (params.get(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION);
                retL = (unsigned long)valueL;
                LOGD("DebugAttrib_LensPosition value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
                LOGE("DebugAttrib_LensPosition is not supported");
            }
            break;
        case DebugAttrib_AFSharpnessScore:
            params = getParameters();
            if (params.get(TICameraParameters::KEY_DEBUGATTRIB_AFSHARPNESSSCORE) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_AFSHARPNESSSCORE);
                retL = (unsigned long)(valueL/50.0);
                LOGD("DebugAttrib_AFSharpnessScore value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
                LOGE("DebugAttrib_AFSharpnessScore is not supported.");
            }
            break;
        case DebugAttrib_ExposureGain:
            params = getParameters();
            if (params.get(TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN);
                retL = (unsigned long)valueL;
                LOGD("DebugAttrib_ExposureGain value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
                LOGE("DebugAttrib_ExposureGain is not supported");
            }
            break;
        case DebugAttrib_ExposureTime:
            params = getParameters();
            if (params.get(TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME);
                retL = (unsigned long)valueL;
                LOGD("DebugAttrib_ExposureTime value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
                LOGE("DebugAttrib_ExposureTime is not supported");
            }
            break;
        case DebugAttrib_FlickerDefault:
            break;
        case DebugAttrib_StrobePower:
            params = getParameters();
	    if (params.get(TICameraParameters::KEY_MOT_LEDFLASH) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_MOT_LEDFLASH);
                retL = (unsigned long)valueL;
                LOGD("DebugAttrib_StrobePower value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
               LOGE("DebugAttrib_StrobePower is not supported");
            }
            break;
        case DebugAttrib_TargetExpValue:
            params = getParameters();
            if (params.get(TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE) != NULL)
            {
                valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_TARGETEXPVALUE);
                retL = (unsigned long)valueL;
                LOGD("DebugAttrib_TargetExpValue value is: %d, GetAttrib will return: %ld", valueL, retL);
            }
            else
            {
                LOGE("DebugAttrib_TargetExpValue is not supported");
            }
            break;
        case DebugAttrib_CalibrationStatus:
           retL = mCameraCalStatus = mCameraAdapter->getCameraCalStatus();
           LOGD("DebugAttrib_CalibrationStatus will return: %ld", retL);
           break;
        case DebugAttrib_MEShutterControl:
            break;
        case DebugAttrib_StrobeSettings:
            break;
        case DebugAttrib_LuminanceMeasure:
            break;
        case DebugAttrib_MipiFrameCount:
            LOGD("Getting DebugAttrib_MipiFrameCount: ");
            params = getParameters();
            valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_MIPIFRAMECOUNT);
            retL = (unsigned long)valueL;
            LOGD("DebugAttrib_MipiFrameCount value is: %d, GetAttrib will return: %ld", valueL, retL);
            break;
        case DebugAttrib_MipiEccErrors:
            LOGD("Getting DebugAttrib_MipiEccErrors: ");
            params = getParameters();
            valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_MIPIECCERRORS);
            retL = (unsigned long)valueL;
            LOGD("DebugAttrib_MipiEccErrors value is: %d, GetAttrib will return: %ld", valueL, retL);
            break;
        case DebugAttrib_MipiCrcErrors:
            LOGD("Getting DebugAttrib_MipiCrcErrors: ");
            params = getParameters();
            valueL = params.getInt(TICameraParameters::KEY_DEBUGATTRIB_MIPICRCERRORS);
            retL = (unsigned long)valueL;
            LOGD("DebugAttrib_MipiCrcErrors value is: %d, GetAttrib will return: %ld", valueL, retL);
            break;

        default:
            retL = 0;
        }
    }
    else
    {
        LOGE("Wrong DebugAttrib index: %d", attrib);
        retL = 0;
    }
    return retL;
}

// =============================================================================
//
// Description: Sets attributes during production test
//
// returns: true - if OK, false - if not OK.
//
// =============================================================================
bool CameraHal::SetDebugAttrib( DebugAttrib attrib, unsigned long value )
{
    bool retL;
    CameraParameters params;

    if (attrib < DebugAttrib_NUM)
    {
        arrDebugAttribs[attrib] = value;
        retL = true;

        switch (attrib)
        {
        case DebugAttrib_TestPattern1:
            // set the flags, given by the value parameter.
            // It should be TestPattern type, defined by CameraTCInterfce.h
            arrDebugAttribs[DebugAttrib_TestPattern1] |= value;

            //   , TestPattern1_ColorBars             = (1 << 0)
            if (value & TestPattern1_ColorBars){
                params = getParameters();
                LOGD("Setting ColorBars to enabled");
                params.set(TICameraParameters::KEY_TESTPATTERN1_COLORBARS, TICameraParameters::TESTPATTERN1_ENABLE);
                setParameters(params);
            }else{
                params = getParameters();
                LOGD("Setting ColorBars to disabled");
                params.set(TICameraParameters::KEY_TESTPATTERN1_COLORBARS, TICameraParameters::TESTPATTERN1_DISABLE);
                setParameters(params);
            }
            //   , TestPattern1_WalkingOnes           = (1 << 1)  not done
            if (value & TestPattern1_WalkingOnes){
            }else{
            }
            //   , TestPattern1_Bayer10AsRaw          = (1 << 2)
            if (value & TestPattern1_Bayer10AsRaw){
                // enable callback for Bayer10 RAW image
                disableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
                enableMsgType(CAMERA_MSG_RAW_IMAGE);
                // set parameters
                params = getParameters();
                LOGD("Setting CaptureFormat to Bayer10 : PIXEL_FORMAT_YUV422I");
                params.setPictureFormat(TICameraParameters::PIXEL_FORMAT_RAW);
                setParameters(params);
            }else{
                // disable Bayer10 - enable JPEG
                disableMsgType(CAMERA_MSG_RAW_IMAGE);
                enableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
                // set parameters
                params = getParameters();
                LOGD("Setting CaptureFormat to Bayer10 : PIXEL_FORMAT_YUV422I");
                params.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
                setParameters(params);
            }

            //   , TestPattern1_DisableSharpening     = (1 << 3)
            if (value & TestPattern1_DisableSharpening){
            }else{
            }
            //   , TestPattern1_UnityGamma            = (1 << 4)
            if (value & TestPattern1_UnityGamma){
            }else{
            }
            //   , TestPattern1_EnManualExposure      = (1 << 5)
            if (value & TestPattern1_EnManualExposure){
                params = getParameters();
                LOGD("Setting EnManualExposure to enabled");
                params.set(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE, TICameraParameters::TESTPATTERN1_ENABLE);
                setParameters(params);
            }else{
                params = getParameters();
                LOGD("Setting EnManualExposure to disabled");
                params.set(TICameraParameters::KEY_TESTPATTERN1_ENMANUALEXPOSURE, TICameraParameters::TESTPATTERN1_DISABLE);
                setParameters(params);
            }
            //   , TestPattern1_DisableLensCorrection = (1 << 6)
            if (value & TestPattern1_DisableLensCorrection){
                // set parameters
                params = getParameters();
                LOGD("Disabling Lens Distortion Correction (LDC)");
                params.set(TICameraParameters::KEY_IPP, TICameraParameters::IPP_NONE);
                setParameters(params);
            }else{
                // set parameters
                params = getParameters();
                LOGD("Enabling Lens Distortion Correction (LDCNSF)");
                params.set(TICameraParameters::KEY_IPP, TICameraParameters::IPP_LDCNSF);
                setParameters(params);
            }
            //   , TestPattern1_TargetedExposure      = (1 << 7)
            if (value & TestPattern1_TargetedExposure){
                params = getParameters();
                LOGD("Setting TargetedExposure to enabled");
                params.set(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE, TICameraParameters::TESTPATTERN1_ENABLE);
                setParameters(params);
            }else{
                params = getParameters();
                LOGD("Setting TargetedExposure to disabled");
                params.set(TICameraParameters::KEY_TESTPATTERN1_TARGETEDEXPOSURE, TICameraParameters::TESTPATTERN1_DISABLE);
                setParameters(params);
            }
            //   , TestPattern1_DisableWatermark      = (1 << 8)
            if (value & TestPattern1_DisableWatermark){
                // enable watermark
            }else{
                // disable watermark
            }

            break;

        case DebugAttrib_EnLensPosGetSet:
            params = getParameters();
            LOGD("Setting EnLensPosGetSet to %ld", value);
            params.set(TICameraParameters::KEY_DEBUGATTRIB_ENLENSPOSGETSET, (int)value);
            setParameters(params);
            break;
        case DebugAttrib_LensPosition:
            params = getParameters();
            LOGD("Setting LensPosition to %ld", value);
            params.set(TICameraParameters::KEY_DEBUGATTRIB_LENSPOSITION, (int)value);
            setParameters(params);
            break;
        case DebugAttrib_AFSharpnessScore:
            break;
        case DebugAttrib_ExposureTime:
            params = getParameters();
            LOGD("Setting ExposureTime to %ld", value);
            params.set(TICameraParameters::KEY_DEBUGATTRIB_EXPOSURETIME, (int)value);
            setParameters(params);
            break;

        case DebugAttrib_ExposureGain:
            params = getParameters();
            LOGD("Setting ExposureGime to %ld", value);
            params.set(TICameraParameters::KEY_DEBUGATTRIB_EXPOSUREGAIN, (int)value);
            setParameters(params);
            break;

        case DebugAttrib_FlickerDefault:
            break;
        case DebugAttrib_StrobePower:
            params = getParameters();
            LOGD("Setting StrobePower to %ld", value);
            params.set(TICameraParameters::KEY_MOT_LEDFLASH, (int)value);
            params.set(TICameraParameters::KEY_MOT_LEDTORCH, (int)value);
            setParameters(params);
            break;

        case DebugAttrib_TargetExpValue:
            {
            long exposureComp = 0;
            LOGD("TargetExpValue from Config file %ld", value);
            if (value >= (2 * MAX_EXPOSURE_COMPENSATION))
                {
                exposureComp = MAX_EXPOSURE_COMPENSATION;
                }
            else
                {
                exposureComp = (long)value - MAX_EXPOSURE_COMPENSATION;
                }
            LOGD("Setting TargetExpValue to %ld", exposureComp);
            params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, exposureComp);
            setParameters(params);
            }
            break;
        case DebugAttrib_CalibrationStatus:
            break;
        case DebugAttrib_MEShutterControl:
            break;
        case DebugAttrib_StrobeSettings:
            break;
        case DebugAttrib_LuminanceMeasure:
            break;
        case DebugAttrib_MipiFrameCount:
            // Nothing to set. This is "get-only" parameter.
            break;
        case DebugAttrib_MipiEccErrors:
            // Nothing to set. This is "get-only" parameter.
            break;
        case DebugAttrib_MipiCrcErrors:
            // Nothing to set. This is "get-only" parameter.
            break;

        default:
            // in case not listed here, i.e. not supported, return false
            retL = false;
        }


    }
    else
    {
        LOGE("Wrong DebugAttrib index to set: %d", attrib);
        retL = false;
    }

    return retL;
}

//=========================================================
// setSdmStatus - check if camera should be disabled via
//                subscriber device management
//

bool CameraHal::setSdmStatus()
{
    char buf[2];
    bool sdmDisabled = false;

    FILE *pfile = fopen(DCM_CAM_CONFIG_FILE, "r");
    if(pfile != NULL)
    {
        // read the first byte from the file
        if( fread(buf, 1, 1, pfile) > 0 )
        {
            int firstByte = (int)buf[0];
            if(firstByte == CAMERA_DISABLED)
            {
                CAMHAL_LOGDA("SDM preview STOPPED");
                sdmDisabled = true;
            }
        }
        fclose(pfile);
    }
    else // if file handle is null, default to enabled
        CAMHAL_LOGDA("SDM file does not exist, default to SDM enabled");

    mSdmDisabled = sdmDisabled;

    return sdmDisabled;
}

bool CameraHal::SetFlashLedTorch( unsigned intensity )
{
    CameraParameters params = getParameters();
    LOGD("CameraHalTI::SetFlashLedTorch ");
    unsigned char write_data[2];

    if (intensity == 0)
    {
        write_data[0] = TORCH_DEVICE_CONTROL_REG;
        write_data[1] = TORCH_DISABLE;
        i2cRW(0, 2, NULL, write_data);
    }
    else
    {
        write_data[0] = TORCH_BRIGHTNESS_CONTROL_REG;
        write_data[1] = TORCH_DEFAULT_BRIGHTNESS;
        i2cRW(0, 2, NULL, write_data);

        write_data[0] = TORCH_DEVICE_CONTROL_REG;
        write_data[1] = TORCH_ENABLE;
        i2cRW(0, 2, NULL, write_data);
    }

    return true;
}

bool CameraHalMotBase::WatchDogThread::threadLoop()
{
    Mutex::Autolock lock(mSuccessLock);
    status_t status = NO_ERROR;
    while (!success && status == NO_ERROR)
    {
        status = mSuccessCond.waitRelative(mSuccessLock,
                seconds(WATCHDOG_SECONDS));
    }
    if (status == TIMED_OUT)
    {
        LOGE("Detected hang in camera library, signaling syslink daemon");

        sp<IServiceManager> sm = defaultServiceManager();
        sp<ISyslinkIpcListener> syslink_recovery =
            interface_cast<ISyslinkIpcListener>(
                    sm->getService(String16("omap4.syslink_recovery")));
        if (syslink_recovery != NULL && syslink_recovery.get() != NULL)
        {
            LOGD("syslink_recovery signaling restart from camera");
            syslink_recovery->sendSyslinkMessage(543210);
            sleep(2); //allow syslink daemon to restart
        }
        else
        {
            LOGE("Unable to get syslink_recovery service");
        }
        LOGE("Forcing restart of mediaserver");
        raise(SIGTERM);
    }
    return false;
}

void CameraHalMotBase::AutoWatchDog::startWatchDog()
{
    mWatchDog = new WatchDogThread();
}

void CameraHalMotBase::AutoWatchDog::stopWatchDog()
{
    mWatchDog->signalSuccess();
    mWatchDog->requestExitAndWait();
    mWatchDog.clear();
}

} // namespace android

