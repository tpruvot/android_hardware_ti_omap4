//=========================================================
// Camera KPI logging implementation
//=========================================================

#include <cutils/properties.h>
#include "CameraKPI.h"

#define BLUR_COMP_NAME       "CAM"

#define TIME_TO_FOCUS_KEY    "AF"
#define TIME_TO_SHUTTER_KEY  "SHTLG"
#define TIME_TO_POSTVIEW_KEY "PSTVW"
#define TIME_TO_RAW_KEY      "RAW"
#define TIME_TO_JPEG_KEY     "JPG"
#define TIME_TO_PREVIEW_KEY  "PRVHAL"
#define TIME_FROM_JPG_TO_PRV_KEY "PRVAPP"
#define TIME_TO_RESTART_PRV_KEY  "PRVRST"
#define TIME_TO_CONSTRUCT_KEY "CNSTR"

#define STATUS_FOCUS_KEY     "afstat"

#define STATUS_SUCCESS       "1"
#define STATUS_FAILURE       "0"

#define MIN_EXTRALOG_SIZE     1
#define MAX_EXTRALOG_SIZE     2
#define DUR_INDEX             0
#define STATUS_INDEX          1

#define MAX_STR_SIZE 12

namespace android {

//=========================================================
// Public Interface
//=========================================================

//=========================================================
// CameraKPI - Constructor for the CameraKPI class
//=========================================================
CameraKPI::CameraKPI()
:mBlurLog(NULL)
{
    // check if blurLogging is enabled
    bBlurLoggingEnabled = isBlurLoggingEnabled();

}

//=========================================================
// ~CameraKPI - Destructor for the CameraKPI class
//=========================================================
CameraKPI::~CameraKPI()
{

}

//=========================================================
// startKPITimer - Start timer for specific event
//
// Arguments:
//    CameraKPIType tmtype - KPI event type
//=========================================================
void CameraKPI::startKPITimer(CameraKPIType tmtype)
{
#ifdef CAM_PERF

    switch (tmtype)
    {
        case CAMKPI_AF:
            mAfTimer.start();
        break;

        case CAMKPI_Shutter:
            mShutterTimer.start();
        break;

        case CAMKPI_Raw:
            mRawTimer.start();
        break;

        case CAMKPI_JPG:
            mJpgTimer.start();
        break;

        case CAMKPI_Postview:
            mPVTimer.start();
        break;

        case CAMKPI_Preview:
            mPrvTimer.start();
        break;

        case CAMKPI_JPGToPreview:
            mJPGtoPrvTimer.start();
        break;

        case CAMKPI_RestartPreview:
            mRestartPrvTimer.start();
        break;

        case CAMKPI_Construct:
            mConstructTimer.start();
        break;

        default:
            LOGE("Unknown KPI timer start request");
    }

#endif //CAM_PERF

}

//=========================================================
// stopKPITimer - Stop timer for specific event
//
// Arguments:
//    CameraKPIType tmtype - KPI event type
//    CameraKPIStatusType status - KPI status
//
// Notes: Should be preceeded by call to startKPITimer
//=========================================================
void CameraKPI::stopKPITimer(CameraKPIType tmtype)
{
    stopKPITimer(tmtype, CAMKPI_StatusNone);
}

//=========================================================
// stopKPITimer - Stop timer for specific event
//
// Arguments:
//    CameraKPIType tmtype - KPI event type
//    CameraKPIStatusType status - KPI status
//
// Notes: Should be preceeded by call to startKPITimer
//=========================================================
void CameraKPI::stopKPITimer(CameraKPIType tmtype,
                             CameraKPIStatusType status)
{
#ifdef CAM_PERF

    long long time_elapsed = 0;
    const char* log_key;

    switch (tmtype)
    {
        case CAMKPI_AF:
            mAfTimer.stop();
            time_elapsed = mAfTimer.durationUsecs()/1000;
            log_key = TIME_TO_FOCUS_KEY;
            LOGD("CAM_PERF: Time to focus %lld ms\n",time_elapsed);
        break;

        case CAMKPI_Shutter:
            mShutterTimer.stop();
            time_elapsed = mShutterTimer.durationUsecs()/1000;
            log_key = TIME_TO_SHUTTER_KEY;
            LOGD("CAM_PERF: Time to shutter received %lld ms\n",time_elapsed);
        break;

        case CAMKPI_Raw:
            mRawTimer.stop();
            time_elapsed = mRawTimer.durationUsecs()/1000;
            log_key = TIME_TO_RAW_KEY;
            LOGD("CAM_PERF: Time to raw %lld ms\n",time_elapsed);
        break;

        case CAMKPI_JPG:
            mJpgTimer.stop();
            time_elapsed = mJpgTimer.durationUsecs()/1000;
            log_key = TIME_TO_JPEG_KEY;
            LOGD("CAM_PERF: Time to jpg %lld ms\n",time_elapsed);
        break;

        case CAMKPI_Postview:
            mPVTimer.stop();
            time_elapsed = mPVTimer.durationUsecs()/1000;
            log_key = TIME_TO_POSTVIEW_KEY;
            LOGD("CAM_PERF: Time to postview %lld ms\n",time_elapsed);
        break;

        case CAMKPI_Preview:
            mPrvTimer.stop();
            time_elapsed = mPrvTimer.durationUsecs()/1000;
            log_key = TIME_TO_PREVIEW_KEY;
            LOGD("CAM_PERF: Time to start preview %lld ms\n",time_elapsed);
        break;

        case CAMKPI_JPGToPreview:
            mJPGtoPrvTimer.stop();
            time_elapsed = mJPGtoPrvTimer.durationUsecs()/1000;
            log_key = TIME_FROM_JPG_TO_PRV_KEY;
            LOGD("CAM_PERF: Delta between JPG and preview restarted by app %lld ms\n",
                 time_elapsed);
        break;

        case CAMKPI_RestartPreview:
            mRestartPrvTimer.stop();
            time_elapsed = mRestartPrvTimer.durationUsecs()/1000;
            log_key = TIME_TO_RESTART_PRV_KEY;
            LOGD("CAM_PERF: Time to restart preview after capture %lld ms\n",
                 time_elapsed);
        break;

        case CAMKPI_Construct:
            mConstructTimer.stop();
            time_elapsed = mConstructTimer.durationUsecs()/1000;
            log_key = TIME_TO_CONSTRUCT_KEY;
            LOGD("CAM_PERF: Time to construct %lld ms\n",
                 time_elapsed);
        break;

        default:
            LOGE("Unknown KPI timer stop request");
            return;
    }

    if(bBlurLoggingEnabled)
    {
        if (mBlurLog != NULL)
        {
            FREE_EXTRALOGS(mBlurLog);
            mBlurLog = NULL;
        }
        if(status != CAMKPI_StatusNone)
        {
            ALLOC_EXTRALOGS(mBlurLog,MAX_EXTRALOG_SIZE)
            logKPIstatus(tmtype, status);
            //log to blur
            logDurationToBlur(time_elapsed, log_key);

        }
        else
        {
            ALLOC_EXTRALOGS(mBlurLog,MIN_EXTRALOG_SIZE)
            logDurationToBlur(time_elapsed, log_key);
        }
        if (mBlurLog != NULL)
        {
            FREE_EXTRALOGS(mBlurLog);
            mBlurLog = NULL;
        }
    }

#endif //CAM_PERF
}


//=========================================================
// Private Interface
//=========================================================

//=========================================================
// logKPIstatus - Log status of a specific event
//
// Arguments:
//    CameraKPIType tmtype - KPI event type
//    CameraKPIStatusType status - status of event
//
//=========================================================
void CameraKPI::logKPIstatus(CameraKPIType type,
                             CameraKPIStatusType status )
{
#ifdef CAM_PERF

    const char* log_key;
    const char* status_key;
    const char* value;

    switch (type)
    {
        case CAMKPI_AF:
            status_key = STATUS_FOCUS_KEY;
            log_key = TIME_TO_FOCUS_KEY;
            value = (status == CAMKPI_StatusSuccess) ?
                        STATUS_SUCCESS : STATUS_FAILURE;
            LOGD("CAM_PERF: AF status %d",status);
        break;

        default:
            LOGE("Unknown KPI status request");
            return;
    }

    if(bBlurLoggingEnabled)
        updateStatusEntry(status_key, value, log_key);

#endif //CAM_PERF
}

//=========================================================
// logDurationToBlur - Log event data to blur cloud
//
// Arguments:
//    long long value - event data value
//    const char* key - blur portal event key
//
//=========================================================
void CameraKPI::logDurationToBlur(long long value, const char* key)
{
    char str[MAX_STR_SIZE];

    if(bBlurLoggingEnabled && mBlurLog)
    {
        snprintf(str, MAX_STR_SIZE, "%lld", value);
        SET_KEYVALUE(mBlurLog->keyvalues[DUR_INDEX],
                           "dur", (const char *)&str);
        LOG_KPI(KPI_E, LEVEL_3, BLUR_COMP_NAME, key, mBlurLog);
    }
}

//=========================================================
// updateStatusEntry - update event status
//
// Arguments:
//    const char* status - event status
//    const char* value  - event status value
//    const char* key    - blur portal event key
//
//=========================================================
void CameraKPI::updateStatusEntry(const char* status,
                                  const char* value,
                                  const char* key)
{
    if(bBlurLoggingEnabled && mBlurLog)
    {
        SET_KEYVALUE(mBlurLog->keyvalues[STATUS_INDEX],
                                          status, value);
    }
}

//=========================================================
// isBlurLoggingEnabled - Check system properties to determine
//                        logging to blur cloud
//
// Arguments:
//    None
//
// Return: bool - True (if system property is enabled)
//
//=========================================================
bool CameraKPI::isBlurLoggingEnabled()
{
    char value[PROPERTY_VALUE_MAX];
    bool status = true; //always enable by default !!!!

    if (property_get("ro.media.kpi.blurlog", value, 0) > 0)
    {
        if(strcmp(value, "1") == 0)
        {
            status = true;
        }
    }

    if(status)
        LOGV("KPI: Blur logging enabled");

    return status;
}

} //namespace android
