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

/**
 *
 * This File contains testcase table for Jpeg Decoder Proxy.
 *
 * @path
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------
 *!
 *! Revision History
 *! =============================================================
 *! Initial Version
 * ============================================================== */

/****************************************************************
 *  INCLUDE FILES
 ****************************************************************/
/* ----- system and platform files ----------------------------*/

/*-------program files ----------------------------------------*/
#include "omx_jpegd_test.h"
#include "omx_jpegd.h"
#include "omx_jpegd_util.h"

/****************************************************************
 *  EXTERNAL REFERENCES NOTE : only use if not found in header file
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/
/****************************************************************
 *  PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

OMX_TEST_CASE_ENTRY OMX_JPEGDTestCaseTable[] = {

  /************************ TEST CASES FROM TEST SPEC ************************/
	/* id                                    Description                               Src                                          Dst                               OW         OH      SH                    ICF                                          OCF                             Output Mode              X  Y  X  Y   Input Buf  Output Buf */
	{"JPEGDecode_OMX_JPEGD_0001", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0001_320x240_420.jpg                 JPEGD_0001_320x240_420.yuv                  320        240     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0002", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0002_80x60_420.jpg                   JPEGD_0002_80x64_420.yuv                     80         64     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0003", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0003_eagle_1600x1200_420.jpg         JPEGD_0003_eagle_1600x1200_420.yuv         1600       1200     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_FALSE  OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0004", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0004_market_5120x3200_16MP_420.jpg   JPEGD_0004_market_5120x3200_16MP_420.yuv   5120       3200     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_FALSE  OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0005", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0005_5120x3200_16MP_420.jpg          JPEGD_0005_5120x3200_16MP_420.yuv          5120       3200     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0006", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0006_chile_3072x2048_420.jpg         JPEGD_0006_chile_3072x2048_422.yuv         3072       2048     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_FALSE  OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0007", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0007_ametlles_1280x1024_420.jpg      JPEGD_0007_ametlles_1280x1024_argb.yuv     1280       1024     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_Format32bitARGB8888             OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0008", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0008_flowers_5120x3200_16MP_420.jpg  JPEGD_0008_flowers_5120x3200_16MP_565.yuv  5120       3200     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_Format16bitRGB565               OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0009", "YUV422 image Decoding in FRAME Mode",
		    "JPEGD_0009_3072x2304_7MP_422.jpg           JPEGD_0009_3072x2304_7MP_420.yuv           3072       2304     0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_FALSE  OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0010", "YUV422 image Decoding in FRAME Mode",
		    "JPEGD_0010_5088x3392_16MP_422.jpg          JPEGD_0010_2544x1696_16MP_422.yuv          2544       1696     0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0011", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0011_5120x3200_color_16MP_420.jpg    JPEGD_0011_1280x800_color_16MP_565.yuv     1280        800     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_Format16bitRGB565               OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0012", "YUV422 image Decoding in FRAME Mode",
		    "JPEGD_0012_5088x3392_16MP_422.jpg          JPEGD_0012_640x480_16MP_420.yuv             640        480     0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0013", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0013_villa_4096x3072_12MP_420.jpg    JPEGD_0013_villa_4096x3072_12MP_420.yuv    4096       3072     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0014", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0014_shredder_3072x3072_9MP_420.jpg  JPEGD_0014_shredder_3072x3072_9MP_565.yuv  3072       3072     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_Format16bitRGB565               OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0015", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0015_3072x2048_6MP_420.jpg           JPEGD_0015_3072x2048_6MP_420.yuv           3072       2048     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0016", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0016_park_3072x2304_7MP_420.jpg      JPEGD_0016_park_3072x2304_7MP_565.yuv      3072       2304     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_Format16bitRGB565               OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0017", "YUV422 image Decoding in FRAME Mode",
		    "JPEGD_0017_lily_2048x1536_422.jpg          JPEGD_0017_lily_2048x1536_420.yuv          2048       1536     0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0018", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0018_352x288_420.jpg                 JPEGD_0018_352x288_420.yuv                  352        288     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0019", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0019_market_640x480_420.jpg          JPEGD_0019_market_640x480_420.yuv           640        480     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0020", "Decoding a Bitstream having Error",
		    "JPEGD_0020_hdr_error_image.jpg             JPEGD_0020_hdr_error_image.yuv              100        100     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0021", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0021_eagle_800x480_420.jpg           JPEGD_0021_eagle_800x480_420.yuv            800        480     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_FALSE  OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,

  /************************ FRAME MODE TEST CASES ************************/
	/* Y420 NV12 Format */
	/* id                                    Description                               Src                                          Dst                               OW         OH      SH                    ICF                                          OCF                             Output Mode              X  Y  X  Y   Input Buf  Output Buf */
	{"JPEGDecode_OMX_JPEGD_0022", "YUV420 image Decoding in FRAME Mode",
		    "1_128x48_nv12.jpg                          1_128x48_nv12.yuv                           128        48      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0023", "YUV420 image Decoding in FRAME Mode",
		    "2_128x128_nv12.jpg                         2_128x128_nv12.yuv                          128       128      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0024", "YUV420 image Decoding in FRAME Mode",
		    "3_176x144_qf-50_ROT-0_nv12.jpg             3_176x144_qf-50_ROT-0_nv12.yuv              176       144      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0025", "YUV420 image Decoding in FRAME Mode",
		    "4_640x480_nv12.jpg                         4_640x480_nv12.yuv                          640       480      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0026", "YUV420 image Decoding in FRAME Mode",
		    "5_640x480_Market_vga_nv12.jpg              5_640x480_Market_vga_nv12.yuv               640       480      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0027", "YUV420 image Decoding in FRAME Mode",
		    "6_640x640_panda_nv12.jpg                   6_640x640_panda_nv12.yuv                    640       640      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0028", "YUV420 image Decoding in FRAME Mode",
		    "7_1024x768_flower_nv12.jpg                 7_1024x768_flower_nv12.yuv                 1024       768      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0029", "YUV420 image Decoding in FRAME Mode",
		    "8_960x1280_foreman_qf-75_nv12.jpg          8_960x1280_foreman_qf-75_nv12.yuv           960      1280      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0030", "YUV420 image Decoding in FRAME Mode",
		    "9_1280x1024_Ametlles_sxga_nv12.jpg         9_1280x1024_Ametlles_sxga_nv12.yuv         1280      1024      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0031", "YUV420 image Decoding in FRAME Mode",
		    "10_5088x3392_qf-75_rot-90_nv12.jpg         10_5088x3392_qf-75_rot-90_nv12.yuv         3392      5088      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,

	/* Y422 Interleaved Format */
	{"JPEGDecode_OMX_JPEGD_0032", "YUV422 image Decoding in FRAME Mode",
		    "11_128x128_uyvy.jpg                        11_128x128_uyvy.yuv                         128       128      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0033", "YUV422 image Decoding in FRAME Mode",
		    "12_512x512_uyvy.jpg                        12_512x512_uyvy.yuv                         512       512      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0034", "YUV422 image Decoding in FRAME Mode",
		    "13_2560x2560_lena_qf-75_rot-0_uyvy.jpg     13_2560x2560_lena_qf-75_rot-0_uyvy.yuv     2560      2560      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0035", "YUV422 image Decoding in FRAME Mode",
		    "14_5088x3392_qf-75_rot-180_uyvy.jpg        14_5088x3392_qf-75_rot-180_uyvy.yuv        5088      3392      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,

	/* Planar or Non-interleaved Formats */
	{"JPEGDecode_OMX_JPEGD_0036", "YUV444 image Decoding in FRAME Mode",
		    "15_176x144_444_ni.jpg                      15_176x144_444_ni.yuv                       176       144      0     OMX_COLOR_FormatYUV444Interleaved         OMX_COLOR_FormatYUV444Interleaved         OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0037", "YUV422 image Decoding in FRAME Mode",
		    "16_176x144_422_ni.jpg                      16_176x144_422_ni.yuv                       176       144      0     OMX_COLOR_FormatYUV422Planar              OMX_COLOR_FormatYUV422Planar              OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0038", "YUV420 image Decoding in FRAME Mode",
		    "17_176x144_420_ni.jpg                      17_176x144_420_ni.yuv                       176       144      0     OMX_COLOR_FormatYUV420Planar              OMX_COLOR_FormatYUV420Planar              OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,

/************************ SLICE MODE TEST CASES ************************/
	/* id                                    Description                               Src                                          Dst                               OW         OH      SH                    ICF                                          OCF                             Output Mode              X  Y  X  Y   Input Buf  Output Buf */
	{"JPEGDecode_OMX_JPEGD_0039", "YUV420 image Decoding in SLICE Mode",
		    "6_640x640_panda_nv12.jpg                   6_640X640_panda_nv12.yuv                    640       640     16     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeSlice    0  0  0  0   OMX_FALSE  OMX_TRUE   ",
		    "", 1, JPEGD_TestSliceMode}
	,

/* Least dimension images and Generic image size decode */
	{"JPEGDecode_OMX_JPEGD_0040", "YUV420 image Decoding in FRAME Mode",
		    "18_16x16_foreman_nv12.jpg                  18_16x16_foreman_nv12.yuv                    16        16      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0041", "YUV420 image Decoding in FRAME Mode",
		    "ss_16x8_420.jpg                            ss_16x8_420.yuv                              16        16      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0042", "YUV420 image Decoding in FRAME Mode",
		    "19_357x292_nv12.jpg                        19_357x292_nv12.yuv                         368       304      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0043", "YUV420 image Decoding in FRAME Mode",
		    "20_177x141_nv12.jpg                        20_177x141_nv12.yuv                         192       144      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0044", "YUV422 image Decoding in FRAME Mode",
		    "21_177x144_uyvy.jpg                        21_177x144_uyvy.yuv                         192       144      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,

/* Test Resize and Color conversion */
	{"JPEGDecode_OMX_JPEGD_0045", "YUV422 image Decoding in FRAME Mode",
		    "12_512x512_uyvy.jpg                        12_128x128_rgb565.yuv                       128       128      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_Format16bitRGB565               OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0046", "YUV422 image Decoding in FRAME Mode",
		    "12_512x512_uyvy.jpg                        12_64x64_argb.yuv                            64        64      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_Format32bitARGB8888             OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0047", "YUV422 image Decoding in FRAME Mode",
		    "12_512x512_uyvy.jpg                        12_512x512_nv12.yuv                         512       512      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0048", "YUV422 image Decoding in FRAME Mode",
		    "12_512x512_uyvy.jpg                        12_256x256_nv12.yuv                         256       256      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0049", "YUV422 image Decoding in FRAME Mode",
		    "12_512x512_uyvy.jpg                        12_256x256_uyvy.yuv                         256       256      0     OMX_COLOR_FormatCbYCrY                    OMX_COLOR_FormatCbYCrY                    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,

/************************ STANDARD TEST IMAGES ************************/
	{"JPEGDecode_OMX_JPEGD_0050", "YUV420 image Decoding in FRAME Mode",
		    "lenna_512x512_nv12.jpg                     lenna_512x512_nv12.yuv                      512       512      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0051", "YUV420 image Decoding in FRAME Mode",
		    "mandril_256x256_nv12.jpg                   mandril_256x256_nv12.yuv                    256       256      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,

/* Old Test cases */
	{"JPEGDecode_OMX_JPEGD_0002", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0002_64x64_420.jpg                   JPEGD_0002_64x64_420.yuv                     64         64     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0006", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0006_waste_3072x2048_420.jpg         JPEGD_0006_waste_3072x2048_420.yuv         3072       2048     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_FALSE  OMX_TRUE   ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0009", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0009_3072x2304_7MP_420.jpg           JPEGD_0009_3072x2304_7MP_420.yuv           3072       2304     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0016", "YUV420 image Decoding in FRAME Mode",
		    "JPEGD_0016_fall_3072x2304_7MP_420.jpg      JPEGD_0016_fall_3072x2304_7MP_420.yuv      3072       2304     0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
	{"JPEGDecode_OMX_JPEGD_0039", "YUV420 image Decoding in FRAME Mode",
		    "18_16x16_nv12.jpg                          18_16x16_nv12.yuv                            16        16      0     OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_COLOR_FormatYUV420PackedSemiPlanar    OMX_JPEG_UncompressedModeFrame    0  0  0  0   OMX_TRUE   OMX_FALSE  ",
		    "", 1, JPEGD_TestFrameMode}
	,
};
