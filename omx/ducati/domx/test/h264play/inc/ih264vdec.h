/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== ih264vdec.h ========
 *  IH264VDEC Interface Header
 */
#ifndef IH264VDEC_
#define IH264VDEC_

#include <ti/xdais/ialg.h>

#include <ti/xdais/dm/ividdec3.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*
 *  ======== IH264VDEC_Handle ========
 *  This handle is used to reference all H264VDEC instance objects
 */
	typedef struct IH264VDEC_Obj *IH264VDEC_Handle;

/*
 *  ======== IH264VDEC_Obj ========
 *  This structure must be the first field of all H264VDEC instance objects
 */
	typedef struct IH264VDEC_Obj
	{
		struct IH264VDEC_Fxns *fxns;
	} IH264VDEC_Obj;

/*
 *  ======== IH264VDEC_Status ========
 *  Status structure defines the parameters that can be changed or read
 *  during real-time operation of the alogrithm.
 */
	typedef struct IH264VDEC_Status
	{
		IVIDDEC3_Status viddec3Status;
	} IH264VDEC_Status;


/*
 *  ======== IH264VDEC_Params ========
 *  This structure defines the creation parameters for all H264VDEC objects
 */
	typedef struct IH264VDEC_Params
	{
		IVIDDEC3_Params viddec3Params;
		XDAS_UInt32 seiDataWriteMode;	/* Specify SEI data Mode (0: Disable)
						   see enum IH264VDEC_seiDataMode below
						 */
		XDAS_UInt32 vuiDataWriteMode;	/* Specify VUI data Mode (0: Disable)
						   see enum IH264VDEC_vuiDataMode below
						 */
		XDAS_UInt32 maxNumRefFrames;
	} IH264VDEC_Params;

/*
 *  ======== enum: SEI Data write Modes ========
 */
	typedef enum
	{
		IH264VDEC_SEI_DATA_NONE = 0,
					   /**< Do not write any SEI data */
		IH264VDEC_WRITE_PARSED_SEI_DATA = 1,
					   /**< Write out Parsed SEI data */
		IH264VDEC_WRITE_ENCODED_SEI_DATA = 2
					   /**< Write out Compressed/Encoded
                                                SEI data */
	} IH264VDEC_seiDataMode;

/*
 *  ======== enum: VUI Data write Modes ========
 */
	typedef enum
	{
		IH264VDEC_VUI_DATA_NONE = 0,
					   /**< Do not write any SEI data */
		IH264VDEC_WRITE_PARSED_VUI_DATA = 1,
					   /**< Write out Parsed SEI data */
		IH264VDEC_WRITE_ENCODED_VUI_DATA = 2
					   /**< Write out Compressed/Encoded
                                                VUI data */
	} IH264VDEC_vuiDataMode;

/*
 *  ======== IH264VDEC_Cmd ========
 *  This structure defines the control commands for the IMP4VENC module.
 */
	typedef IVIDDEC3_Cmd IH264VDEC_Cmd;

/*
 *  ======== IH264VDEC_PARAMS ========
 *  Default parameter values for H264VDEC instance objects
 */
	extern IH264VDEC_Params IH264VDEC_PARAMS;


/*
 *  ======== IH264VDEC_DynamicParams ========
 *  This structure defines the dynamic parameters for all H264VDEC objects
 */
	typedef struct IH264VDEC_DynamicParams
	{
		IVIDDEC3_DynamicParams viddec3DynamicParams;
	} IH264VDEC_DynamicParams;


/*
 *  ======== IH264VDEC_InArgs ========
 *  This structure defines the runtime input arguments for IH264VDEC::process
 */

	typedef struct IH264VDEC_InArgs
	{

		IVIDDEC3_InArgs viddec3InArgs;

	} IH264VDEC_InArgs;

/*
 *  ======== IH264VDEC_OutArgs ========
 *  This structure defines the run time output arguments for IH264VDEC::process
 *  function.
 */

	typedef struct IH264VDEC_OutArgs
	{

		/* OLD Extended class. Just retained for reference */
		/*
		   IVIDDEC1_OutArgs viddec1OutArgs;
		   XDAS_Bool hrd;
		   XDAS_Int16 picStruct;
		   // These later parameters probably should not be here... only
		   // inserted for quick port to xDM
		   XDAS_Int16 xOffset;
		   XDAS_Int16 yOffset;
		   XDAS_Int16 refHeight;
		   XDAS_Int16 refStorageType; //Field or frame storage
		   XDAS_Int16 fieldPicFlag;   // Current decoded picture is Field or frame
		   XDAS_Int16 bottomFieldFlag;// Current decoded picture is Bot field/top Fld
		 */

		IVIDDEC3_OutArgs viddec3OutArgs;

	} IH264VDEC_OutArgs;

/*
 *  ======== IH264VDEC_DYNAMICPARAMS ========
 *  Default dynamic parameter values for H264VDEC instance objects
 */
	extern IH264VDEC_DynamicParams IH264VDEC_TI_DYNAMICPARAMS;

/*
 *  ======== IH264VDEC_Fxns ========
 *  This structure defines all of the operations on H264VDEC objects
 */
	typedef struct IH264VDEC_Fxns
	{
		IVIDDEC3_Fxns ividdec3;	/* IH264VDEC extends IVIDDEC */
	} IH264VDEC_Fxns;


/**********************************************************************************
* \enum   H.264 Error Codes
* \brief Delaration of Internal Error Check Codes and return values for H.264 Decoder
*********************************************************************************/
	typedef enum
	{
		H264D_ERR_NOSLICE = 0,
		H264D_ERR_NALUNITTYPE,	// not used now
		H264D_ERR_SPS,
		H264D_ERR_PPS,
		H264D_ERR_SLICEHDR,
		H264D_ERR_MBDATA,
		H264D_ERR_UNAVAILABLESPS,
		H264D_ERR_UNAVAILABLEPPS,
		H264D_ERR_INVALIDPARAM_IGNORE = 16,
		H264D_ERR_UNSUPPFEATURE,	// not used now
		//H264D_ERR_MBSMISSED,
		//H264D_ERR_FRAMEMISSED
		H264D_ERR_SEIBUFOVERFLOW,
		H264D_ERR_VUIBUFOVERFLOW,
		H264D_ERR_STREAM_END,
		H264D_ERR_NO_FREEBUF,
		H264D_ERR_PICSIZECHANGE,
		H264D_ERR_BITSBUF_UNDERFLOW,
		H264D_ERR_UNSUPPRESOLUTION,
		H264D_ERR_NUMREF_FRAMES
	} IH264VDEC_ErrorBit;

  /*--------------------------------------------------
   Constant definitions required for SEI support
  --------------------------------------------------*/
#define H264VDEC_MAXCPBCNT        32	/*!< HRD sequence parameter set   */

 /*-------------------------------------------------------------
   sBufferingPeriod
   This structure contains the buffering period SEI msg elements
  ----------------------------------------------------------------*/
	typedef struct buffering_period_SEI
	{
		XDAS_UInt32 parsed_flag;
		XDAS_UInt32 seq_parameter_set_id;	//!< ue(v)
		XDAS_UInt32 nal_cpb_removal_delay[H264VDEC_MAXCPBCNT];	//!< u(v)
		XDAS_UInt32 nal_cpb_removal_delay_offset[H264VDEC_MAXCPBCNT];	//!< u(v)
		XDAS_UInt32 vcl_cpb_removal_delay[H264VDEC_MAXCPBCNT];	//!< u(v)
		XDAS_UInt32 vcl_cpb_removal_delay_offset[H264VDEC_MAXCPBCNT];	//!< u(v)
	} sBufferingPeriod;

  /*------------------------------------------------------------
    sPictureTiming
    This structure contains the picture timing SEI msg elements
   ------------------------------------------------------------*/
	typedef struct picture_timing_SEI
	{
		XDAS_UInt32 parsed_flag;
		XDAS_UInt32 NumClockTs;
		XDAS_UInt32 cpb_removal_delay;	//!< u(v)
		XDAS_UInt32 dpb_output_delay;	//!< u(v)
		XDAS_UInt32 pic_struct;	//!< u(4)

		XDAS_UInt32 clock_time_stamp_flag[4];	//!< u(1)
		XDAS_UInt32 ct_type[4];	//!< u(2)
		XDAS_UInt32 nuit_field_based_flag[4];	//!< u(1)
		XDAS_UInt32 counting_type[4];	//!< u(5)
		XDAS_UInt32 full_timestamp_flag[4];	//!< u(1)
		XDAS_UInt32 discontinuity_flag[4];	//!< u(1)
		XDAS_UInt32 cnt_dropped_flag[4];	//!< u(1)
		XDAS_UInt32 n_frames[4];	//!< u(8)
		XDAS_UInt32 seconds_flag[4];	//!< u(1)
		XDAS_UInt32 minutes_flag[4];	//!< u(1)
		XDAS_UInt32 hours_flag[4];	//!< u(1)
		XDAS_UInt32 seconds_value[4];	//!< u(6)
		XDAS_UInt32 minutes_value[4];	//!< u(6)
		XDAS_UInt32 hours_value[4];	//!< u(5)
		XDAS_UInt32 time_offset[4];	//!< i(v)

	} sPictureTiming;

 /*--------------------------------------------------------
   sUserDataUnregistered
   This structure contains the user data SEI msg elements
   -------------------------------------------------------*/
	typedef struct user_data_unregistered_SEI
	{
		XDAS_UInt8 parsed_flag;
		XDAS_UInt32 num_payload_bytes;
		XDAS_UInt8 uuid_iso_iec_11578[16];	//!< u(128)
		XDAS_UInt8 user_data_payload_byte[128];	//!< b(8)
	} sUserDataUnregistered;

  /*----------------------------------------------------------
    sSeiMessages
    This structure contains all the supported SEI msg objects
   ----------------------------------------------------------*/
	typedef struct sei_messages
	{
		XDAS_UInt32 parsed_flag;
		sUserDataUnregistered user_data_unregistered;
		sBufferingPeriod buffering_period_info;
		sPictureTiming pic_timing;
	} sSeiMessages;

  /*----------------------------------------------------------
    sProfileInfo
    This structure contains elements related to profiling
    information
   ----------------------------------------------------------*/
	typedef struct profiling_struct
	{
		XDAS_UInt32 preMbLoop;
		XDAS_UInt32 inMbLoop;
		XDAS_UInt32 postMbLoop;
		XDAS_UInt32 ivahdTotalCycles;
		XDAS_UInt32 sliceTask[128];
		XDAS_UInt32 noOfSlices;
	} sProfileInfo;

	typedef enum
	{
		IH264VDEC_NUM_REFFRAMES_AUTO = 0,
		IH264VDEC_NUM_REFFRAMES_1 = 1,
		IH264VDEC_NUM_REFFRAMES_2 = 2,
		IH264VDEC_NUM_REFFRAMES_3 = 3,
		IH264VDEC_NUM_REFFRAMES_4 = 4,
		IH264VDEC_NUM_REFFRAMES_5 = 5,
		IH264VDEC_NUM_REFFRAMES_6 = 6,
		IH264VDEC_NUM_REFFRAMES_7 = 7,
		IH264VDEC_NUM_REFFRAMES_8 = 8,
		IH264VDEC_NUM_REFFRAMES_9 = 9,
		IH264VDEC_NUM_REFFRAMES_10 = 10,
		IH264VDEC_NUM_REFFRAMES_11 = 11,
		IH264VDEC_NUM_REFFRAMES_12 = 12,
		IH264VDEC_NUM_REFFRAMES_13 = 13,
		IH264VDEC_NUM_REFFRAMES_14 = 14,
		IH264VDEC_NUM_REFFRAMES_15 = 15,
		IH264VDEC_NUM_REFFRAMES_16 = 16,
		IH264VDEC_NUM_REFFRAMES_DEFAULT = IH264VDEC_NUM_REFFRAMES_AUTO
	} IH264VDEC_numRefFrames;



#ifdef __cplusplus
}


#endif				/* extern "C" */
#endif				/* IH264VDEC_ */
