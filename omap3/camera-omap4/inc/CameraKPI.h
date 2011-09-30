#ifndef __CAMERA_KPI_H__
#define __CAMERA_KPI_H__

#include <utils/Timers.h>
#include <utils/Log.h>

//#define CAM_PERF

namespace android {

typedef enum {
     CAMKPI_AF
    ,CAMKPI_Shutter
    ,CAMKPI_Raw
    ,CAMKPI_JPG
    ,CAMKPI_Postview
    ,CAMKPI_Preview
    ,CAMKPI_JPGToPreview
    ,CAMKPI_RestartPreview
    ,CAMKPI_Construct
    }CameraKPIType;

typedef enum {
     CAMKPI_StatusNone
    ,CAMKPI_StatusFailure
    ,CAMKPI_StatusSuccess
    }CameraKPIStatusType;


class CameraKPI
{
public:
    CameraKPI();
    ~CameraKPI();

    /* duration related */
    void startKPITimer(CameraKPIType type);
    void stopKPITimer(CameraKPIType type);
    void stopKPITimer(CameraKPIType type,
                      CameraKPIStatusType status);


private:
    bool bBlurLoggingEnabled;

    /* Performance measurement */
    DurationTimer       mPrvTimer;
    DurationTimer       mAfTimer;
    DurationTimer       mShutterTimer;
    DurationTimer       mRawTimer;
    DurationTimer       mJpgTimer;
    DurationTimer       mPVTimer;
    DurationTimer       mJPGtoPrvTimer;
    DurationTimer       mRestartPrvTimer;
    DurationTimer       mConstructTimer;

    /* Blur logging */
    EXTRALOGS_T *mBlurLog;

    /* status related */
    void logKPIstatus(CameraKPIType type, CameraKPIStatusType status);

    void logDurationToBlur(long long value, const char* key);
    void updateStatusEntry(const char* status,
                           const char* value,
                           const char* key);

    bool isBlurLoggingEnabled();

};

}
#endif // __CAMERA_KPI_H__
