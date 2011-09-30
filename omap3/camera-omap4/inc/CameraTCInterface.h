#ifndef __CAMERA_TC_INTERFACE_H__
#define __CAMERA_TC_INTERFACE_H__

#define DEFAULT_VENDOR_STRING "Unknown,DefaultCamera,Rev1"
#define MAX_VENDOR_STRING_SIZE 128

class CameraTCInterface
{
public:
   typedef enum
   {
     CameraTest_DLI,
     CameraTest_MIPICounter
   } CameraTest;

   typedef struct
   {
     unsigned char Threshold;  // Not used with walking ones DLI
   } DLITestInputs;

   typedef enum
   {
     RESET_MIPI_COUNTER = 0x00,
     GET_MIPI_COUNTER   = 0x01
   } MIPICounterOperation;

   typedef struct
   {
     union
     {
       DLITestInputs DLI;
       MIPICounterOperation MIPICounterOp;
     } test;
   } CameraTestInputs;

   typedef struct
   {
     unsigned short HighLines;    // Denoting data lines shorted high
     unsigned short LowLines;     // Denoting data lines shorted low
     unsigned short ShortedLines; // Denoting data lines shorted to other data lines
     // Note: Each of the above values is a bit mask in which each bit corresponds to
     // a physical sensor data line.  A resulting value of 1 implies proper operation.
     // A resulting value of 0 implies a short.  Unused bits should be 1's.
   } DLITestOutputs;

   typedef struct
   {
     unsigned long FrameCount;    // total frames after last reset
     unsigned long ECCErrors;     // ECC errors after last reset
     unsigned long CRCErrors;     // CRC errors after last reset
   } MIPICounterOutputs;

   typedef struct
   {
     union
     {
       DLITestOutputs DLI;
       MIPICounterOutputs MIPICounter;
     } test;
   } CameraTestOutputs;

   // Test commands must inform HAL when it's using the test APIs
   virtual void EnableTestMode( bool enable ) = 0;

   virtual bool PerformCameraTest( CameraTest         test
                                 , CameraTestInputs  *inputs
                                 , CameraTestOutputs *outputs
                                 ) = 0;

   // Defined as [Module Manufacturer],[Sensor Model],[Sensor Revision], plus optionally [Module ID],[Sensor ID],[Lens ID]
   // Example: "Semco,Aptina5130,Rev8"
   virtual bool GetCameraVendorString( char *str, unsigned char length ) = 0;
   virtual bool GetCameraModuleQueryString( char *str, unsigned char length ) = 0;

   typedef enum
   { TestPattern1_None                  = 0x00000000
   , TestPattern1_ColorBars             = (1 << 0)  // Mutually-Exclusive with WalkingOnes
   , TestPattern1_WalkingOnes           = (1 << 1)  // Mutually-Exclusive with ColorBars
   , TestPattern1_Bayer10AsRaw          = (1 << 2)  // Forces Bayer10 to be the RAW format
   , TestPattern1_DisableSharpening     = (1 << 3)
   , TestPattern1_UnityGamma            = (1 << 4)
   , TestPattern1_EnManualExposure      = (1 << 5)  // Enables use of ExpGain & ExpTime attribs
   , TestPattern1_DisableLensCorrection = (1 << 6)
   , TestPattern1_TargetedExposure      = (1 << 7)  // Enables targeted auto exposure (see TargetExpValue)
   , TestPattern1_DisableWatermark      = (1 << 8)  // Disable the JPG watermark
   } TestPattern;

   typedef enum
     // Get/Set TestPattern1 value
     // Default: None
   { DebugAttrib_TestPattern1     = 1

     // Get/Set bool value (0=disabled/1=enabled)
     // Default: disabled
   , DebugAttrib_EnLensPosGetSet  = 10

     // Get/Set unsigned 8 bit value (position percentage, 0=infinity/100=macro)
     // Default: 0
     // A value of 255 is returned for a 'Get' if reading/writing the lens position is not supported
   , DebugAttrib_LensPosition

     // Get unsigned 32 bit value (as much granularity as possible)
     // Default: 0
   , DebugAttrib_AFSharpnessScore

     // Get/Set signed 32 bit value (range of -600 to 2000 [-6dB to 20dB])
     // Enabled with the EnManualExposure test pattern bit
     // The range can be truncated if the full range is not supported in both directions
     // A read will return the truncated value, if truncation occurs
     // Default: 0
   , DebugAttrib_ExposureGain

     // Get/Set exposure time in microseconds
     // Enabled with the EnManualExposure test pattern bit
     // Default: 0 (meaning auto selected)
   , DebugAttrib_ExposureTime

     // Get unsigned 32 bit value (50 or 60)
     // Default: 60
   , DebugAttrib_FlickerDefault

     // Get/Set power maximum level for strobe for testing (percentage, 1-100)
     // Default: 0 (meaning auto selected)
   , DebugAttrib_StrobePower

     // Get/Set unsigned 8 bit value (range of 0 - 255)
     // Default: 128 (mid-range)
   , DebugAttrib_TargetExpValue

     // Get status value (0 = Not Supported; 1 = FirstTime; 2 = Success; 3 = Failure)
   , DebugAttrib_CalibrationStatus

     // Set ME(mechanical) shutter value (0 = close 1 = open and 2 = auto)
   , DebugAttrib_MEShutterControl

     //Set Strobe Settings (0 = Not Supported; 1 = Quench level; 2 = Charge level;
     //3 = Charge enabler; 4 = Is Charge complete; 5 = Strobe Enable
   , DebugAttrib_StrobeSettings

     //Get brightness value
   , DebugAttrib_LuminanceMeasure

     // Get/Set MIPI frame counter
   , DebugAttrib_MipiFrameCount

     // Get/Set MIPI ECC error counter
   , DebugAttrib_MipiEccErrors

     // Get/Set MIPI CRC error counter
   , DebugAttrib_MipiCrcErrors

     // Num of elements, must be the last
   , DebugAttrib_NUM
   } DebugAttrib;

   // Returns 0 for invalid attribute
   virtual unsigned long GetDebugAttrib( DebugAttrib attrib ) = 0;
   virtual bool          SetDebugAttrib( DebugAttrib attrib, unsigned long value ) = 0;
   virtual bool          SetFlashLedTorch( unsigned intensity ) = 0;

   virtual ~CameraTCInterface() {};
};

#endif // __CAMERA_TC_INTERFACE_H__
