#ifndef MOTO_DEBUG_H
#define MOTO_DEBUG_H

#ifndef LOG_TAG
#define LOG_TAG "MotHDRInterface"
#endif

#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <android/log.h>

#include <cutils/properties.h>

//LOG_LEVEL 1, DMOTLOGV enabled
//LOG_LEVEL 0, DMOTLOGV disabled
#define LOG_LEVEL 1

#if LOG_LEVEL == 1
#define DMOTLOGV(FMT, ARGS...)   __android_log_print(ANDROID_LOG_VERBOSE, "LOG_TAG" , "%d: %s: " FMT, __LINE__, __func__, ##ARGS)
#else
#define DMOTLOGV(FMT, ARGS...) void(0)
#endif

#define DMOTLOGE(FMT, ARGS...)   __android_log_print(ANDROID_LOG_ERROR, "LOG_TAG" , "%d: %s: " FMT, __LINE__, __func__, ##ARGS)
#define DMOTPRINT DMOTLOGV(" called")

#endif
