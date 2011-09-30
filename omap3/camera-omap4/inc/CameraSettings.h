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

// Map settings string to an integer for indexing

#ifndef __CAMERA_SETTINGS_H__
#define __CAMERA_SETTINGS_H__

typedef enum
{ ECSType_PreviewFormat
, ECSType_PreviewSize
, ECSType_PreviewRate
, ECSType_PictureFormat
, ECSType_PictureSize
, ECSType_PictureRotation
, ECSType_ThumbnailWidth
, ECSType_ThumbnailHeight
, ECSType_ThumbnailSize
, ECSType_ImageQuality
, ECSType_PostviewMode
, ECSType_VideoSize
, ECSType_FlipMode
, ECSType_FlashMode
, ECSType_SceneMode
, ECSType_ColorEffect
, ECSType_ExposureMode
, ECSType_ExposureOffset
, ECSType_FocusMode
, ECSType_FocusedAreas
, ECSType_AreasToFocus
, ECSType_ImageStabEn
, ECSType_VideoStabEn
, ECSType_TimestampEn
, ECSType_LightFreqMode
, ECSType_ZoomValue
, ECSType_ZoomStep
, ECSType_ZoomSpeed
, ECSType_BurstCount
, ECSType_FaceDetectEn
, ECSType_FaceDetectNumFaces
, ECSType_FaceDetectionAreas
, ECSType_FaceTrackEn
, ECSType_FaceTrackFrmSkip
, ECSType_SmileDetectEn
, ECSType_SmileDetectionAreas
, ECSType_UserComment
, ECSType_GpsLatitude
, ECSType_GpsLongitude
, ECSType_GpsAltitude
, ECSType_GpsTimestamp
, ECSType_GpsProcMethod
, ECSType_GpsMapDatum
, ECSType_VideoMode
, ECSType_PictureIso
, ECSType_Contrast
, ECSType_Max
, ECSType_Invalid
} ECSType;

// Access rights for parameters.  Not very well enforced right now.
typedef enum
{ ECSAccess_NS  // Not Supported
, ECSAccess_RO  // Read-Only
, ECSAccess_WO  // Write-Only
, ECSAccess_RW  // Read/Write
} ECSAccess;

// Hints on how to parse and validate strings coming from CameraParameters.
typedef enum
{ ECSInvalid
, ECSNone
, ECSSubstring
, ECSPercent
, ECSRectangles
, ECSNumberMin
, ECSNumberMax
, ECSNumberRange
, ECSString
, ECSFloat
} ECSParseType;

class CameraSettings
{
public:
   enum { SIZE_WIDTH = 0, SIZE_HEIGHT };                              // w/h sizes
   enum { ZOOM_STOP = 0, ZOOM_MOVE_CONTINUOUS, ZOOM_MOVE_IMMEDIATE }; // zoom_state
   enum { USER_COMMENT_LENGTH = 256 };                                // user_comment
   enum { MAX_FOCUS_WINDOWS = 1 };                                    // focus_windows
   enum { COLOR_FORMAT_YUYV422I = 0, COLOR_FORMAT_YUV420SP };         // YUV formats
   enum { MAX_DATE_LENGTH = 12 };                                     // date string
   enum { GPS_PROC_METHOD_LENGTH = 32 };                              // process method
   enum { GPS_MAP_DATUM_LENGTH = 32 };                                // gps map datum

   // Some well-known resolution names.  Not sure why this is so important.
   typedef enum
   { RESOLUTION_QCIF = 0,
     RESOLUTION_QVGA,
     RESOLUTION_CIF,
     RESOLUTION_HVGA,
     RESOLUTION_VGA,
     RESOLUTION_WVGA,
     RESOLUTION_1MP,
     RESOLUTION_2MP,
     RESOLUTION_3MP,
     RESOLUTION_4_9MP,
     RESOLUTION_5W,
     RESOLUTION_5MP,
     RESOLUTION_8W,
     RESOLUTION_8MP,
     RESOLUTION_MAX
   } Resolution_T;

   typedef enum
   { CAPTURE_FORMAT_RAW = 0,
     CAPTURE_FORMAT_JPEG,
     CAPTURE_FORMAT_MAX
   } CaptureFormat_T;

   typedef enum
   { JPEG_HEADER_JFIF = 0,
     JPEG_HEADER_EXIF,
     JPEG_HEADER_MAX
   } JpegHeader_T;



   // These objects make up the parse table itself.  "key" is mapped to "type",
   // which is then parsed based on the "parseType" and then verified against
   // "access" and "caps" (capabilities).

   // Note that the capabilities is the string used by an Android application to
   // query for the capabilities of the Camera. The capability default value is
   // a list of the supported vaoues for the key.

   typedef struct
   { const char   *key;
     const char   *defaultValue;
     ECSType       type;
     ECSAccess     access;
     ECSParseType  parseType;
     const char   *capsKey;
     const char   *capsDefaultValue;
   } ParamParseTable_T;

   // Control which focus and flash modes are supported by each scene mode.
   typedef struct
   { const char *scene_mode;
     const char *focus_mode;
     const char *flash_mode;
   } SceneModeParamTable_T;


};

//=========================
// Camera Settings mappings
//=========================

// Every scene mode has its own focus mode and flash mode.
CameraSettings::SceneModeParamTable_T sceneModeParmTable[] =
{ { "action",         "infinity", "off" },
  { "portrait",       "auto",     "auto"},
  { "landscape",      "infinity", "off" },
  { "night",          "infinity", "off" },
  { "night-portrait", "auto",     "on"  },
  { "theatre",        "auto",     "off" },
  { "beach",          "auto",     "auto"},
  { "snow",           "auto",     "auto"},
  { "sunset",         "auto",     "off" },
  { "steadyphoto",    "auto",     "auto"},
  { NULL,             NULL,       NULL}};

// Help with readability of the parse table.
#define NO_PARAM_KEY    NULL
#define NO_DEFAULT      NULL
#define NO_VALUES_KEY   NULL
#define NO_CAPABILITIES NULL
#define NO_HIDDEN_CAPS  NULL

// Help avoid copy-paste errors.
const char FOCUS_MODE_VALUES[] = "off,auto,infinity,macro";
const char FLASH_MODE_VALUES[] = "off,on,auto,torch";
const char VIDEO_MODE_VALUES[] = "auto,fast,slow";
const char WHITE_BALANCE_VALUES[] = "auto,daylight,fluorescent,cloudy,incandescent,warm-fluorescent";

// WARNING: these parameters are tightly controlled by Google as they are part
// of the camera API.  Any new parameters that have been created internally
// must be prefixed with "mot-".


CameraSettings::ParamParseTable_T initialParseTable[] =
{ { "preview-format",               "yuv420sp"
  , ECSType_PreviewFormat,          ECSAccess_RW, ECSSubstring
  , "preview-format-values",       "yuv422i-yuyv,yuv420sp" }
, { "preview-size",                 "640x480"
  , ECSType_PreviewSize,            ECSAccess_RW, ECSNone
  , "preview-size-values",          "176x144,320x240,352x288,640x480,720x480,720x576,800x448,1280x720" }
, { "preview-frame-rate",           "30"
  , ECSType_PreviewRate,            ECSAccess_RW, ECSSubstring
  , "preview-frame-rate-values",    "5,10,15,20,24,25,28,30" }
, { "picture-format",               "jpeg"
  , ECSType_PictureFormat,          ECSAccess_RW, ECSSubstring
  , "picture-format-values",        "jpeg,jfif,exif" }
, { "picture-size",                 "2048x1536"
  , ECSType_PictureSize,            ECSAccess_RW, ECSNone
  , "picture-size-values",          "640x480,1280x960,1600x1200,2048x1536,2592x1936,3264x1840,3264x2448" }

, { "rotation",                     NO_DEFAULT
  , ECSType_PictureRotation,        ECSAccess_RW, ECSSubstring
  , "rotation-values",              "0,90,180,270" }
, { "mot-flip-mode",                "off"
  , ECSType_FlipMode,               ECSAccess_RW, ECSSubstring
  , "mot-flip-mode-values",         "off,vertical,horizontal,both" }

, { "jpeg-thumbnail-width",         "320"
  , ECSType_ThumbnailWidth,         ECSAccess_RW, ECSNone
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "jpeg-thumbnail-height",        "240"
  , ECSType_ThumbnailHeight,        ECSAccess_RW, ECSNone
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "jpeg-quality",                 "95"
  , ECSType_ImageQuality,           ECSAccess_RW, ECSPercent
  , NO_VALUES_KEY,                  NO_CAPABILITIES }

, { "mot-postview-mode",            "on"
  , ECSType_PostviewMode,           ECSAccess_RW, ECSSubstring
  , "mot-postview-modes",           "off,on" }

, { "mot-video-size",               "320x240"
  , ECSType_VideoSize,              ECSAccess_RW, ECSSubstring
  , "mot-video-size-values",        "176x144,320x240,352x288,640x480,720x480,720x576,1280x720" }
, { "scene-mode",                   "auto"
  , ECSType_SceneMode,              ECSAccess_RW, ECSSubstring
  , "scene-mode-values",            "auto,action,portrait,landscape,night,night-portrait,theatre,beach,snow,sunset,steadyphoto" }
, { "flash-mode",                   "off"
  , ECSType_FlashMode,              ECSAccess_RW, ECSSubstring
  , "flash-mode-values",            FLASH_MODE_VALUES }

, { "effect",                       "none"
  , ECSType_ColorEffect,            ECSAccess_RW, ECSSubstring
  , "effect-values",                "none,mono,sepia,negative,solarize,red-tint,blue-tint,green-tint" }
, { "whitebalance",                 "auto"
  , ECSType_ExposureMode,           ECSAccess_RW, ECSSubstring
  , "whitebalance-values",          WHITE_BALANCE_VALUES }
, { "mot-exposure-offset",          "0"
  , ECSType_ExposureOffset,         ECSAccess_RW, ECSSubstring
  , "mot-exposure-offset-values",   "-3,-2.67,-2.33,-2,-1.67,-1.33,-1,-0.67,-0.33,0,0.33,0.67,1,1.33,1.67,2,2.33,2.67,3" }

, { "focus-mode",                   "auto"
  , ECSType_FocusMode,              ECSAccess_RW, ECSSubstring
  , "focus-mode-values",            FOCUS_MODE_VALUES }
, { NULL,                           NULL
  , ECSType_Invalid,                ECSAccess_NS, ECSNone
  , "focal-length",                 "5" }
, { NULL,                           NULL
  , ECSType_Invalid,                ECSAccess_NS, ECSNone
  , "horizontal-view-angle",        "53" }
, { NULL,                           NULL
  , ECSType_Invalid,                ECSAccess_NS, ECSNone
  , "vertical-view-angle",          "40" }
, { "exposure-compensation",        "0"
  ,ECSType_ExposureOffset,          ECSAccess_RW, ECSSubstring
  , NULL,                           "-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6" }
, { NULL,                           NULL
  ,ECSType_Invalid,                 ECSAccess_NS, ECSNone
  , "max-exposure-compensation",    "6"}
, { NULL,                           NULL
  ,ECSType_Invalid,                 ECSAccess_NS, ECSNone
  , "min-exposure-compensation",    "-6"}
, { NULL,                           NULL
  ,ECSType_Invalid,                 ECSAccess_NS, ECSNone
  , "exposure-compensation-step", "0.3333333333"}

, { "mot-areas-to-focus",           "0"
  , ECSType_AreasToFocus,           ECSAccess_RW, ECSRectangles
  , "mot-max-areas-to-focus",       "1" }

, { "mot-image-stabilization",      "off"
  , ECSType_ImageStabEn,            ECSAccess_RW, ECSSubstring
  , "mot-image-stabilization-values", "off,on" }
, { "mot-video-stabilization",      "off"
  , ECSType_VideoStabEn,            ECSAccess_RW, ECSSubstring
  , "mot-video-stabilization-values", "off,on" }
, { "mot-timestamp-mode",           "off"
  , ECSType_TimestampEn,            ECSAccess_RW, ECSSubstring
  , "mot-timestamp-mode-values",    "off,on" }

, { "antibanding",                  "auto"
  , ECSType_LightFreqMode,          ECSAccess_RW, ECSSubstring
  , "antibanding-values",           "auto,50hz,60hz" }

, { "zoom",                         "0"
  , ECSType_ZoomValue,              ECSAccess_RW, ECSNumberMax
  , "max-zoom",                     "6" }
, { NO_PARAM_KEY,                   NO_DEFAULT
  , ECSType_Invalid,                ECSAccess_NS, ECSSubstring
  , "zoom-supported",               "true" }
, { NO_PARAM_KEY,                   NO_DEFAULT
  , ECSType_Invalid,                ECSAccess_NS, ECSSubstring
  , "smooth-zoom-supported",        "true" }
, { "mot-zoom-step",                "0.5"
  , ECSType_ZoomStep,               ECSAccess_RW, ECSSubstring
  , "mot-continuous-zoom-step-values", "1,0.5,0.25" }
, { "mot-zoom-speed",               "99"
  , ECSType_ZoomSpeed,              ECSAccess_RW, ECSPercent
  , NO_VALUES_KEY,                  NO_CAPABILITIES }

, { "mot-burst-picture-count",      "0"
  , ECSType_BurstCount,             ECSAccess_RW, ECSSubstring
  , "mot-burst-picture-count-values", "0,3,6,9" }
, { "mot-face-detect-mode",         "off"
  , ECSType_FaceDetectEn,           ECSAccess_NS, ECSSubstring
  , "mot-face-detect-mode-values",  "off,on" }
, { "mot-face-detect-num-faces",    "4"
  , ECSType_FaceDetectNumFaces,     ECSAccess_RW, ECSNumberMax
  , "mot-max-face-detect-num-faces", "9" }
, { "mot-face-detect-areas",        NO_DEFAULT
  , ECSType_FaceDetectionAreas,     ECSAccess_RO, ECSInvalid
  , "mot-max-face-detect-num-faces", "9" }
, { "mot-face-track-mode",          "off"
  , ECSType_FaceTrackEn,            ECSAccess_NS, ECSSubstring
  , "mot-face-track-mode-values",   "off,on" }
, { "mot-face-track-frame-skip",    "15"
  , ECSType_FaceTrackFrmSkip,       ECSAccess_RW, ECSNone
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "mot-smile-detect-mode",       "off"
  , ECSType_SmileDetectEn,          ECSAccess_NS, ECSSubstring
  , "mot-smile-detect-mode-values", "off,on" }
, { "mot-smile-detect-areas",       NO_DEFAULT
  , ECSType_SmileDetectionAreas,    ECSAccess_RO, ECSInvalid
  , "mot-max-smile-detect-num-faces", "9" }

, { "mot-user-comment",             NO_DEFAULT
  , ECSType_UserComment,            ECSAccess_RW, ECSString
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "gps-latitude",                 NO_DEFAULT
  , ECSType_GpsLatitude,            ECSAccess_RW, ECSFloat
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "gps-longitude",                NO_DEFAULT
  , ECSType_GpsLongitude,           ECSAccess_RW, ECSFloat
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "gps-altitude",                 NO_DEFAULT
  , ECSType_GpsAltitude,            ECSAccess_RW, ECSFloat
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "gps-timestamp",                NO_DEFAULT
  , ECSType_GpsTimestamp,           ECSAccess_RW, ECSFloat
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "gps-processing-method",        NO_DEFAULT
  , ECSType_GpsProcMethod,          ECSAccess_RW, ECSString
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "mot-gps-map-datum",                NO_DEFAULT
  , ECSType_GpsMapDatum,            ECSAccess_RW, ECSString
  , NO_VALUES_KEY,                  NO_CAPABILITIES }
, { "video-mode",                   "auto"
  , ECSType_VideoMode,              ECSAccess_RW, ECSSubstring
  , "video-mode-values",            VIDEO_MODE_VALUES }  //To be replaced later with a new parameter

, { NO_PARAM_KEY,                   NO_DEFAULT
  , ECSType_Invalid,                ECSAccess_NS, ECSNumberMax
  , "mot-max-picture-continuous-zoom", "4" }
, { "mot-picture-iso",              "auto"
  , ECSType_PictureIso,             ECSAccess_RW, ECSSubstring
  , "mot-picture-iso-values",       "auto,100,200,400,800" }
, { "mot-contrast",                 "normal"
  , ECSType_Contrast,               ECSAccess_RW, ECSSubstring
  , "mot-contrast-values",         "lowest,low,normal,high,highest" }

// List terminator
, { NULL, NULL, ECSType_Invalid, ECSAccess_NS, ECSInvalid, NULL, NULL }  // Must be last
};



#endif // __CAMERA_SETTINGS_H__

