
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* =============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
* @file AacDecTest.c
*
* This file contains the test application code that invokes the component.
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\aac_dec\tests
*
* @rev  1.0
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! ===================================
*! 21-sept-2006 bk: updated some review findings for alpha release
*! 24-Aug-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests some more
*! 18-July-2006 bk: Khronos OpenMAX (TM) 1.0 Conformance tests validated for few cases
*! This is newest file
* =========================================================================== */
/* ------compilation control switches -------------------------*/
/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/


#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/vt.h>
#include <signal.h>
#include <sys/stat.h>
#include <OMX_Index.h>
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>
#include <TIDspOmx.h>

#include <pthread.h>
#include <stdio.h>

#include <linux/soundcard.h>
#include <OMX_AacDec_Utils.h>
#ifndef ANDROID
#include <AudioManagerAPI.h>
#endif

#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

FILE *fpRes = NULL;

#undef APP_DEBUG

#undef USE_BUFFER

/*For timestamp and tickcount*/
#undef APP_TIME_TIC_DEBUG

#ifdef APP_DEBUG
#define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_DPRINT(...)
#endif

#ifdef APP_MEMCHECK
#define APP_MEMPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_MEMPRINT(...)
#endif

#ifdef APP_TIME_TIC_DEBUG
#define TIME_PRINT(...)     fprintf(stderr,__VA_ARGS__)
#define TICK_PRINT(...)     fprintf(stderr,__VA_ARGS__)
#else
#define TIME_PRINT(...)
#define TICK_PRINT(...)
#endif

#define APP_OUTPUT_FILE "output.pcm"
#define SLEEP_TIME 5
#define AACDEC_APP_ID  100
#define INPUT_PORT  0
#define OUTPUT_PORT 1
#undef AACDEC_DEBUGMEM

/* Using DASF */
#ifdef DSP_RENDERING_ON
/* AM_APP - Request AM interface through APP */
/*      Undefine it to force OMX to interface directly with AM */
#undef AM_APP
#ifdef AM_APP
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#endif
#endif

#ifdef AACDEC_DEBUGMEM
#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif

#ifdef OMX_GETTIME
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif
int nIpBufSize = 2000;
int nOpBufSize = 8192;

int gDasfMode = 0;
int MimoOutput = -1;
int gStream = 0;
int gBitOutput = 0;
int frame_no = 0;
int lastinputbuffersent = 0;
int playcompleted = 0;
int framemode = 0;
int stress = 0;
int conceal = 0;
int ringCount = 0;

int num_flush = 0;
int nNextFlushFrame = 100;

OMX_STATETYPE gState = OMX_StateExecuting;

int preempted = 0;

#undef  WAITFORRESOURCES

static OMX_BOOL bInvalidState = OMX_FALSE;
void* ArrayOfPointers[7] = {NULL};

#ifdef AACDEC_DEBUGMEM
void *arr[500] = {NULL};
int lines[500] = {0};
int bytes[500] = {0};
char file[500][50] = {""};
int r = 0;
#endif

/* inline int maxint(int a, int b); */
int maxint(int a, int b);
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);
void ConfigureAudio(int SampleRate);

#ifdef AACDEC_DEBUGMEM
void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);
#endif

OMX_STRING strAacDecoder = "OMX.TI.AAC.decode";

int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};


pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag = 0;
OMX_U8      TargetedState = 0;

pthread_mutex_t WaitForOUTFlush_mutex;
pthread_cond_t  WaitForOUTFlush_threshold;
pthread_mutex_t WaitForINFlush_mutex;
pthread_cond_t  WaitForINFlush_threshold;

enum {
    MONO,
    STEREO
};

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer,
                                 FILE *fIn);

fd_set rfds;

int FreeAllResources( OMX_HANDLETYPE pHandle,
                      OMX_BUFFERHEADERTYPE* pBufferIn,
                      OMX_BUFFERHEADERTYPE* pBufferOut,
                      int NIB, int NOB,
                      FILE* fIn, FILE* fOut);

#ifdef USE_BUFFER
int  FreeAllUseResources(OMX_HANDLETYPE pHandle,
                         OMX_U8* UseInpBuf[],
                         OMX_U8* UseOutBuf[],
                         int NIB, int NOB,
                         FILE* fIn, FILE* fOut);

#endif

/* safe routine to get the maximum of 2 integers */
/* inline int maxint(int a, int b) */

int maxint(int a, int b) {
    return (a > b) ? a : b;
}

/* This method will wait for the component to get to the state
 * specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle, OMX_STATETYPE DesiredState) {
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (bInvalidState == OMX_TRUE) {
        eError = OMX_ErrorInvalidState;
        return eError;
    }

    eError = OMX_GetState(pHandle, &CurState);

    if (CurState != DesiredState) {
        WaitForState_flag = 1;
        TargetedState = DesiredState;
        pthread_mutex_lock(&WaitForState_mutex);
        pthread_cond_wait(&WaitForState_threshold, &WaitForState_mutex);/*Going to sleep till signal arrives*/
        pthread_mutex_unlock(&WaitForState_mutex);
    }

    if ( eError != OMX_ErrorNone ) return eError;

    return OMX_ErrorNone;
}


OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData) {

    OMX_U8 writeValue = 0;
    OMX_S32 nData1Temp = (OMX_S32)nData1;

    AACDEC_OMX_CONF_CHECK_CMD(hComponent, pAppData, 1);
    switch (eEvent) {

        case OMX_EventCmdComplete:
            gState = (OMX_STATETYPE)nData2;
            APP_DPRINT ("%d: APP: Component state = %d \n", __LINE__, gState);

            if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) &&
                    (WaitForState_flag)) {
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                pthread_mutex_unlock(&WaitForState_mutex);
            }

            if (nData1 == OMX_CommandFlush) {
                if (nData2 == 0) {
                    pthread_mutex_lock(&WaitForINFlush_mutex);
                    pthread_cond_signal(&WaitForINFlush_threshold);
                    pthread_mutex_unlock(&WaitForINFlush_mutex);
                }

                if (nData2 == 1) {
                    pthread_mutex_lock(&WaitForOUTFlush_mutex);
                    pthread_cond_signal(&WaitForOUTFlush_threshold);
                    pthread_mutex_unlock(&WaitForOUTFlush_mutex);
                }
            }

            break;

        case OMX_EventError:

            if (nData1 != OMX_ErrorNone && (bInvalidState == OMX_FALSE)) {
                APP_DPRINT ("%d: App: ErrorNotification received: Error Num %d:\
                            String :%s\n", __LINE__, (int)nData1, (OMX_STRING)pEventData);
            }

            if (nData1Temp== OMX_ErrorInvalidState && (bInvalidState == OMX_FALSE)) {
                APP_DPRINT("EventHandler: Invalid State!!!!\n");
                bInvalidState = OMX_TRUE;
                if (WaitForState_flag == 1){
                    WaitForState_flag = 0;
                    pthread_mutex_lock(&WaitForState_mutex);
                    pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                    pthread_mutex_unlock(&WaitForState_mutex);
                }
            } else if (nData1Temp== OMX_ErrorResourcesPreempted) {
                preempted = 1;
                writeValue = 0;
                write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            } else if (nData1Temp== OMX_ErrorResourcesLost) {
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                pthread_mutex_unlock(&WaitForState_mutex);
            }

            break;

        case OMX_EventMax:
            APP_DPRINT ("%d: APP: OMX_EventMax has been reported \n", __LINE__);
            break;

        case OMX_EventMark:
            APP_DPRINT ("%d: APP: OMX_EventMark has been reported \n", __LINE__);
            break;

        case OMX_EventPortSettingsChanged:

            if (nData2 == OMX_AUDIO_AACObjectHE) {
                printf("File is OMX_AUDIO_AACObjectHE (%ld), port setting changed...\n", nData2);
            }

            if (nData2 == OMX_AUDIO_AACObjectHE_PS) {
                printf("File is OMX_AUDIO_AACObjectHE_PS (%ld), port setting changed...\n", nData2);
            }

            APP_DPRINT ("%d: APP: OMX_EventPortSettingsChanged has been reported \n", __LINE__);

            break;

        case OMX_EventBufferFlag:

            if ((nData2 == (OMX_U32)OMX_BUFFERFLAG_EOS) && (nData1 == (OMX_U32)NULL)) {
                playcompleted = 1;
                lastinputbuffersent = 0;
            }

#ifdef WAITFORRESOURCES
            writeValue = 2;

            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));

#endif
            break;

        case OMX_EventResourcesAcquired:
            APP_DPRINT ("%d: APP: OMX_EventResourcesAcquired has been reported \n", __LINE__);

            writeValue = 1;

            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));

            preempted = 0;

            break;

        case OMX_EventComponentResumed:
            APP_DPRINT ("%d: APP: OMX_EventComponentResumed has been reported \n", __LINE__);

            break;

        case OMX_EventDynamicResourcesAvailable:
            APP_DPRINT ("%d: APP: OMX_EventDynamicResourcesAvailable has been reported \n", __LINE__);

            break;

        case OMX_EventPortFormatDetected:
            APP_DPRINT ("%d: APP: OMX_EventPortFormatDetected has been reported \n", __LINE__);

            break;
        default:
            APP_DPRINT ("%d: APP: EventHandler: eEvent: \
                        nothing to do for this case::\n", __LINE__);
            break;
    }

    return OMX_ErrorNone;
}

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer) {
    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    /*add on: TimeStamp & TickCount FillBufferDone*/
    TIME_PRINT("TimeStamp Output: %lld\n", pBuffer->nTimeStamp);
    TICK_PRINT("TickCount Output: %ld\n\n", pBuffer->nTickCount);
#ifdef OMX_GETTIME

    if (GT_FlagF == 1 ) { /* First Buffer Reply*/ /* 1 = First Buffer,  0 = Not First Buffer  */

        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }

#endif
}


void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer) {
    if (!preempted)
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));

#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) { /* First Buffer Reply*/ /* 1 = First Buffer,  0 = Not First Buffer  */

        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }

#endif
}

int main(int argc, char* argv[]) {
    OMX_CALLBACKTYPE AacCaBa = {(void *)EventHandler,
                                (void*)EmptyBufferDone,
                                (void*)FillBufferDone
                               };
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = AACDEC_APP_ID ;
    TI_OMX_DSP_DEFINITION audioinfo;
#ifdef AM_APP
    AM_COMMANDDATATYPE cmd_data;
#else
/*    TI_OMX_STREAM_INFO am_streaminfo; */
#endif
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_AUDIO_PARAM_AACPROFILETYPE *pAacParam = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *oAacParam = NULL;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[MAX_NUM_OF_BUFS_AACDEC] = { NULL };
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[MAX_NUM_OF_BUFS_AACDEC] = { NULL };
    OMX_S16 numOfInputBuffer = 0;
    OMX_S16 numOfOutputBuffer = 0;
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[10] = {NULL};
    OMX_U8* pOutputBuffer[10] = {NULL};
#endif

    struct timeval tv;
    int retval = 0, j = 0, nProfile = 0, i = 0, naacformat = 0;
    int nFrameCount = 1;
    int frmCount = 0;
    int frmCnt = 1, k;
    int testcnt = 1;
    int testcnt1 = 1;
    int SampleRate = 0;
    char fname[100] = APP_OUTPUT_FILE;
    FILE* fIn = NULL;
    FILE* fOut = NULL;
    int tcID = 0;
    int ConfigChanged = 0;

    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;

    pthread_mutex_init(&WaitForOUTFlush_mutex, NULL);
    pthread_cond_init (&WaitForOUTFlush_threshold, NULL);
    pthread_mutex_init(&WaitForINFlush_mutex, NULL);
    pthread_cond_init (&WaitForINFlush_threshold, NULL);


    bInvalidState = OMX_FALSE;
#ifndef ANDROID
    OMX_INDEXTYPE index;
    TI_OMX_DATAPATH dataPath;
#endif
#ifdef AM_APP
    int aacdecfdwrite = 0;
    int aacdecfdread = 0;
#endif
    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In AAC DECODER Test Application:\n");
    APP_DPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_DPRINT("------------------------------------------------------\n");

#ifdef OMX_GETTIME
    printf("Line %d\n", __LINE__);
    GTeError = OMX_ListCreate(&pListHead);
    printf("Line %d\n", __LINE__);
    printf("eError = %d\n", GTeError);
    GT_START();
    printf("Line %d\n", __LINE__);
#endif
    /* check the input parameters */

    if (argc < 15 || argc > 17) {
        printf("Usage: Wrong Arguments: See Example Below\n\n");
        printf("./AacDecoder_Test patterns/ps_mj_44khz_32000.aac STEREO TC_1 22050 DM PS \
               8192 EProfileLC SBR NON_RAW 2000 1 1 NON_FRAME\n");
        goto EXIT;
    }

    /* check to see that the input file exists */

    struct stat sb = {
        0
    };

    int status = stat(argv[1], &sb);

    if ( status != 0 ) {
        printf( "Cannot find file %s. (%u)\n", argv[1], errno);
        goto EXIT;
    }

    /* Create a pipe used to queue data from the callback. */
    retval = pipe(IpBuf_Pipe);

    if ( retval != 0) {
        printf( "Error:Fill Data Pipe failed to open\n");
        goto EXIT;
    }

    retval = pipe(OpBuf_Pipe);

    if ( retval != 0) {
        printf( "Error:Empty Data Pipe failed to open\n");
        goto EXIT;
    }

    retval = pipe(Event_Pipe);

    if (retval != 0) {
        printf( "Error:Empty Event Pipe failed to open\n");
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
    int fdmax = maxint(IpBuf_Pipe[0], OpBuf_Pipe[0]);

    fdmax = maxint(fdmax, Event_Pipe[0]);

    error = TIOMX_Init();

    if (error != OMX_ErrorNone) {
        APP_DPRINT("%d :: Error returned by OMX_Init()\n", __LINE__);
        goto EXIT;
    }

    if (!strcmp(argv[5], "DM")) {
        gDasfMode = 1;

        if (argv[15] != 0)
            stress = 1;
    } else if (!strcmp(argv[5], "M0")) {
        /* Multiple output implementation
         * it implies DASF and should set the dataPath
         * in different way, that will be passed to the omx component
         * There are 3 different outputs to choose from */
        gDasfMode = 1;
        MimoOutput = 0;
    } else if (!strcmp(argv[5], "M1")) {
        gDasfMode = 1;
        MimoOutput = 1;
    } else if (!strcmp(argv[5], "M2")) {
        gDasfMode = 1;
        MimoOutput = 2;
    } else if (!strcmp(argv[5], "FM")) {
        gDasfMode = 0;

        if (argv[16] != 0)
            stress = 1;
    } else {
        puts("Missing or invalid operation mode choose DM or FM");
        goto EXIT;
    }

    audioinfo.dasfMode = gDasfMode;

    if (gDasfMode) {
        if (!(strcmp(argv[8], "EProfileMain"))) {
            nProfile = OMX_AUDIO_AACObjectMain;
            printf("EProfileMain\n");
        } else if (!(strcmp(argv[8], "EProfileLC"))) {
            nProfile = OMX_AUDIO_AACObjectLC;
            printf("EProfileLC\n");
        } else if (!(strcmp(argv[8], "EProfileSSR"))) {
            nProfile = OMX_AUDIO_AACObjectSSR;
            printf("EProfileSSR\n");
        } else if (!(strcmp(argv[8], "EProfileLTP"))) {
            nProfile = OMX_AUDIO_AACObjectLTP;
            printf("EProfileLTP\n");
        } else {
            printf("Invalid Profile\n");
            goto EXIT;
        }

        if (!(strcmp(argv[9], "SBR"))) {
            nProfile = OMX_AUDIO_AACObjectHE;
            printf("SBR\n");
        }

        if (!(strcmp(argv[10], "RAW"))) {
            naacformat = 1;
            printf("RAW\n");
        }

        nIpBufSize = AACD_INPUT_BUFFER_SIZE;//atoi(argv[11]);

        if (nIpBufSize == 0) {
            printf("Please use a valid size, (e.g. 2500)\n");
            goto EXIT;
        } else {
            printf("Input Buffer size:%d\n", nIpBufSize);
        }

        numOfInputBuffer = AACD_NUM_INPUT_BUFFERS;//atoi(argv[12]);

        APP_DPRINT("Number of Input Buffers:%d\n", numOfInputBuffer);


        numOfOutputBuffer = AACD_NUM_OUTPUT_BUFFERS;//atoi(argv[13]);
        APP_DPRINT("Number of Output Buffers:%d\n", numOfOutputBuffer);


        if (!(strcmp(argv[14], "FRAME_MODE"))) {
            audioinfo.framemode = 1;
            framemode = audioinfo.framemode;
            printf("FRAME_MODE\n");
        } else if (!(strcmp(argv[14], "NON_FRAME"))) {
            audioinfo.framemode = 0;
            framemode = audioinfo.framemode;
        } else {
            puts("Missing or invalid frame mode operation choose: FRAME_MODE or NON_FRAME");
            goto EXIT;
        }
    }

    if (gDasfMode == 0) {
        if (17 < argc || 16 > argc) {
            printf("Usage: Wrong Arguments: See Example Below\n\n");
            printf("./AacDecoder_Test patterns/ps_mj_44khz_32000.aac STEREO TC_1 22050 \
                   FM PS 8192 B16 EProfileLC SBR RAW 2000 1 1 NON_FRAME\n");
            goto EXIT;
        }

        if (!strcmp(argv[8], "B24")) {
            gBitOutput = 24;
        } else if (!strcmp(argv[8], "B16")) {
            gBitOutput = 16;
        } else {
            puts("Missing or invalid bit per sample mode, choose: B16 or B24\n");
            goto EXIT;
        }

        if (!(strcmp(argv[9], "EProfileMain"))) {
            nProfile = OMX_AUDIO_AACObjectMain;
            printf("EProfileMain\n");
        } else if (!(strcmp(argv[9], "EProfileLC"))) {
            nProfile = OMX_AUDIO_AACObjectLC;
            printf("EProfileLC\n");
        } else if (!(strcmp(argv[9], "EProfileSSR"))) {
            nProfile = OMX_AUDIO_AACObjectSSR;
            printf("EProfileSSR\n");
        } else if (!(strcmp(argv[9], "EProfileLTP"))) {
            nProfile = OMX_AUDIO_AACObjectLTP;
            printf("EProfileLTP\n");
        } else {
            printf("Invalid Profile\n");
            goto EXIT;
        }

        if (!(strcmp(argv[10], "SBR"))) {
            nProfile = OMX_AUDIO_AACObjectHE;
            printf("SBR\n");
        }

        if (!(strcmp(argv[11], "RAW"))) {
            naacformat = 1;
            printf("RAW\n");
        }

        nIpBufSize = AACD_INPUT_BUFFER_SIZE;//atoi(argv[12]);

        if (nIpBufSize == 0) {
            printf("Please use a valid size, (e.g. 2000)\n");
            goto EXIT;
        } else {
            printf("Input Buffer size:%d\n", nIpBufSize);
        }

        numOfInputBuffer = AACD_NUM_INPUT_BUFFERS;//atoi(argv[13]);

        APP_DPRINT("Number of Input Buffers:%d\n", numOfInputBuffer);


        numOfOutputBuffer = AACD_NUM_INPUT_BUFFERS;//atoi(argv[14]);
        APP_DPRINT("Number of Output Buffers:%d\n", numOfOutputBuffer);


        if (!(strcmp(argv[15], "FRAME_MODE"))) {
            audioinfo.framemode = 1;
            framemode = audioinfo.framemode;
            printf("FRAME_MODE\n");
        } else if (!(strcmp(argv[15], "NON_FRAME"))) {
            audioinfo.framemode = 0;
            framemode = audioinfo.framemode;
        } else {
            puts("Missing or invalid frame mode operation choose: FRAME_MODE or NON_FRAME");
            goto EXIT;
        }
    }

    if (!strcmp(argv[3], "TC_1")) {
        tcID = 1;
    } else if (!strcmp(argv[3], "TC_2")) {
        tcID = 2;
    } else if (!strcmp(argv[3], "TC_3")) {
        tcID = 3;
    } else if (!strcmp(argv[3], "TC_4")) {
        tcID = 4;
    } else if (!strcmp(argv[3], "TC_5")) {
        tcID = 5;
    } else if (!strcmp(argv[3], "TC_6")) {
        tcID = 6;
    }  else if (!strcmp(argv[3], "TC_7")) {
        if (!(audioinfo.framemode)) {
            printf("Test can only be used in FRAME_MODE\n");
            goto EXIT;
        } else {
            tcID = 7;
        }
    } else if (!strcmp(argv[3], "TC_8")) {
        tcID = 8;
    } else if (!strcmp(argv[3], "TC_13")) {
        tcID = 13;
    } else {
        printf("Invalid Test Case ID: exiting..\n");
        goto EXIT;
    }

    switch (tcID) {

        case 1:
            printf ("-------------------------------------\n");
            printf ("Testing Simple PLAY till EOF \n");
            printf ("-------------------------------------\n");
            break;

        case 2:
            printf ("-------------------------------------\n");
            printf ("Testing Basic Stop \n");
            printf ("-------------------------------------\n");
            break;

        case 3:
            printf ("-------------------------------------\n");
            printf ("Testing PAUSE & RESUME Command\n");
            printf ("-------------------------------------\n");
            break;

        case 4:
            printf ("---------------------------------------------\n");
            printf ("Testing STOP Command by Stopping In-Between\n");
            printf ("---------------------------------------------\n");
            strcat(fname, "_tc4.pcm");
            testcnt = 2;
            break;

        case 5:
            printf ("-------------------------------------------------\n");
            printf ("Testing Repeated PLAY without Deleting Component\n");
            printf ("-------------------------------------------------\n");
            strcat(fname, "_tc5.pcm");

            if (stress)
                testcnt = 100;
            else
                testcnt = 20;

            break;

        case 6:
            printf ("------------------------------------------------\n");

            printf ("Testing Repeated PLAY with Deleting Component\n");

            printf ("------------------------------------------------\n");

            strcat(fname, "_tc6.pcm");

            if (stress)
                testcnt1 = 100;
            else
                testcnt1 = 20;

            break;

        case 7:
            printf ("------------------------------------------------\n");

            printf ("Testing Frame Concealment (Frame Mode only)\n");

            printf ("------------------------------------------------\n");

            strcat(fname, "_tc7.pcm");

            break;

        case 8:
            printf ("-------------------------------------\n");

            printf ("Testing Middle of frame Test \n");

            printf ("-------------------------------------\n");

            break;

        case 13:
            printf ("------------------------------------------------\n");

            printf ("Testing Ringtone test\n");

            printf ("------------------------------------------------\n");

            strcat(fname, "_tc13.pcm");

            /*ringCount = 5;*/
            testcnt = 10;

            break;
    }

    for (j = 0; j < testcnt1; j++) {
        printf("TC-6 counter = %d\n", j);
        lastinputbuffersent=0;
        if (j == 1) {
            fIn = fopen(argv[1], "rb");

            if ( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\access\n", argv[1]);
                goto EXIT;
            }


            if (0 == audioinfo.dasfMode) {
                fOut = fopen(fname, "w");

                if ( fOut == NULL ) {
                    printf("Error:  failed to create the output file \n");
                    goto EXIT;
                }

                printf("%d :: Op File has created\n", __LINE__);
            }
        }

#ifdef AM_APP
        if ((aacdecfdwrite = open(FIFO1, O_WRONLY)) < 0) {
            printf("[%s] - failure to open WRITE pipe\n", __FILE__);
        } else {
            printf("[%s] - opened WRITE pipe\n", __FILE__);
        }

        if ((aacdecfdread = open(FIFO2, O_RDONLY)) < 0) {
            printf("[%s] - failure to open READ pipe\n", __FILE__);
            goto EXIT;
        } else {
            printf("[%s] - opened READ pipe\n", __FILE__);
        }

#endif
        if (tcID == 6) {
            printf("*********************************************************\n");
            printf ("%d :: App: Outer %d time Sending OMX_StateExecuting Command: TC6\n", __LINE__, j);
            printf("*********************************************************\n");
        }


#ifdef OMX_GETTIME
        GT_START();

        error = TIOMX_GetHandle(&pHandle, strAacDecoder, &AppData, &AacCaBa);

        GT_END("Call to GetHandle");

#else
        error = TIOMX_GetHandle(&pHandle, strAacDecoder, &AppData, &AacCaBa);

#endif
        if ((error != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT ("Error in Get Handle function\n");
            goto EXIT;
        }

        if (!strcmp(argv[6], "PS")) {
            nProfile = OMX_AUDIO_AACObjectHE_PS;
        }

        nOpBufSize = AACD_OUTPUT_BUFFER_SIZE;//atoi(argv[7]);/*Out put buffer size*/

#ifndef USE_BUFFER

        for (i = 0; i < numOfInputBuffer; i++) {
            pInputBufferHeader[i] = newmalloc(1);
            error = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize);
            APP_DPRINT("%d :: called OMX_AllocateBuffer\n", __LINE__);

            if (error != OMX_ErrorNone) {
                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n", __LINE__);
                goto EXIT;
            }

            APP_DPRINT("%d :: pInputBufferHeader[%d] = %p\n", __LINE__, i, pInputBufferHeader[i]);
        }

        if (audioinfo.dasfMode == 0) {
            for (i = 0; i < numOfOutputBuffer; i++) {
                pOutputBufferHeader[i] = newmalloc(1);
                error = OMX_AllocateBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, nOpBufSize);

                if (error != OMX_ErrorNone) {
                    APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n", __LINE__);
                    goto EXIT;
                }

                APP_DPRINT("%d :: pOutputBufferHeader[%d] = %p\n", __LINE__, i, pOutputBufferHeader[i]);
            }
        }

#else
        for (i = 0; i < numOfInputBuffer; i++) {
            OMX_MALLOC_SIZE_DSPALIGN(pInputBuffer[i], nIpBufSize, OMX_U8);
            printf("%d :: pInputBufferHeader[%d] = %p\n", __LINE__, i, pInputBufferHeader[i]);
            error = OMX_UseBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize, pInputBuffer[i]);

            if (error != OMX_ErrorNone) {
                printf("%d :: APP: Error returned by OMX_UseBuffer()\n", __LINE__);
                goto EXIT;
            }
        }

        if (audioinfo.dasfMode == 0) {
            for (i = 0; i < numOfOutputBuffer; i++) {
                OMX_MALLOC_SIZE_DSPALIGN(pOutputBuffer[i], nOpBufSize, OMX_U8);
                printf("%d :: pOutputBufferHeader[%d] = %p\n", __LINE__, i, pOutputBufferHeader[i]);
                error = OMX_UseBuffer(pHandle, &pOutputBufferHeader[i], 1, NULL, nOpBufSize, pOutputBuffer[i]);

                if (error != OMX_ErrorNone) {
                    APP_DPRINT("%d :: APP: Error returned by OMX_UseBuffer()\n", __LINE__);
                    goto EXIT;
                }
            }
        }

#endif

        pCompPrivateStruct = newmalloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));

        if (NULL == pCompPrivateStruct) {
            printf("%d :: App: Malloc Failed\n", __LINE__);
            goto EXIT;
        }

        ArrayOfPointers[0] = (OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;

        pCompPrivateStruct->nPortIndex = INPUT_PORT;
        error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        pCompPrivateStruct->eDir = OMX_DirInput;

        pCompPrivateStruct->nBufferCountActual = numOfInputBuffer;
        pCompPrivateStruct->nBufferSize = nIpBufSize;
        pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        pCompPrivateStruct->nPortIndex = OUTPUT_PORT;

#ifdef OMX_GETTIME
        GT_START();
        error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_GetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        pCompPrivateStruct->eDir = OMX_DirOutput;

        pCompPrivateStruct->nBufferCountActual = numOfOutputBuffer;
        pCompPrivateStruct->nBufferSize = nOpBufSize;
#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        pAacParam = newmalloc (sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

        if (NULL == pAacParam) {
            printf("%d :: App: Malloc Failed\n", __LINE__);
            goto EXIT;
        }

        ArrayOfPointers[1] = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pAacParam;

        pAacParam->nPortIndex = INPUT_PORT;
        error = OMX_GetParameter (pHandle, OMX_IndexParamAudioAac, pAacParam);

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        if (!strcmp("MONO", argv[2])) {
            APP_DPRINT(" ------------- AAC: MONO Mode ---------------\n");
            pAacParam->nChannels = OMX_AUDIO_ChannelModeMono;
            gStream = MONO;
        } else if (!strcmp("STEREO", argv[2])) {
            APP_DPRINT(" ------------- AAC: STEREO Mode ---------------\n");
            gStream = STEREO;
            pAacParam->nChannels = OMX_AUDIO_ChannelModeStereo;
        } else {
            printf("%d :: Error: Invalid Audio Format..exiting.. \n", __LINE__);
            goto EXIT;
        }


        if (atoi(argv[4]) == 8000) {
            pAacParam->nSampleRate = 8000;
        } else if (atoi(argv[4]) == 11025) {
            pAacParam->nSampleRate = 11025;
        } else if (atoi(argv[4]) == 12000) {
            pAacParam->nSampleRate = 12000;
        } else if (atoi(argv[4]) == 16000) {
            pAacParam->nSampleRate = 16000;
        } else if (atoi(argv[4]) == 22050) {
            pAacParam->nSampleRate = 22050;
        } else if (atoi(argv[4]) == 24000) {
            pAacParam->nSampleRate = 24000;
        } else if (atoi(argv[4]) == 32000) {
            pAacParam->nSampleRate = 32000;
        } else if (atoi(argv[4]) == 44100) {
            pAacParam->nSampleRate = 44100;
        } else if (atoi(argv[4]) == 48000) {
            pAacParam->nSampleRate = 48000;
        } else if (atoi(argv[4]) == 64000) {
            pAacParam->nSampleRate = 64000;
        } else if (atoi(argv[4]) == 88200) {
            pAacParam->nSampleRate = 88200;
        } else if (atoi(argv[4]) == 96000) {
            pAacParam->nSampleRate = 96000;
        } else {
            printf("%d :: Sample Frequency Not Supported\n", __LINE__);
            printf("%d :: Exiting...........\n", __LINE__);
            goto EXIT;
        }

        APP_DPRINT("%d:pAacParam->nSampleRate:%lu\n", __LINE__, pAacParam->nSampleRate);

        pAacParam->eAACProfile = nProfile;

        if (naacformat) {
            pAacParam->eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
        } else {
            pAacParam->eAACStreamFormat = OMX_AUDIO_AACStreamFormatMax;
        }

        APP_DPRINT("%d :: pAacParam->eAACStreamFormat = %d\n", __LINE__, pAacParam->eAACStreamFormat);

#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioAac, pAacParam);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioAac, pAacParam);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            goto EXIT;
        }

        /* second instance of this structure to get SBR, PS params while runnig */
        OMX_AUDIO_PARAM_AACPROFILETYPE* pAacParam_SBR_PS;

        pAacParam_SBR_PS = newmalloc(sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

        ArrayOfPointers[2] = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pAacParam_SBR_PS;


        oAacParam = newmalloc (sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));

        if (NULL == oAacParam) {
            APP_DPRINT("%d :: APP: Malloc Failed\n", __LINE__);
            error = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        ArrayOfPointers[3] = (OMX_AUDIO_PARAM_PCMMODETYPE*)oAacParam;

        oAacParam->nPortIndex = OUTPUT_PORT;
        error = OMX_GetParameter (pHandle, OMX_IndexParamAudioPcm, oAacParam);

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: APP: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        oAacParam->nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);

        oAacParam->nVersion.s.nVersionMajor = 0xF1;
        oAacParam->nVersion.s.nVersionMinor = 0xF2;
        oAacParam->nBitPerSample = gBitOutput;
        oAacParam->nSamplingRate = pAacParam->nSampleRate;
        oAacParam->nChannels = pAacParam->nChannels;
        oAacParam->bInterleaved = 1;

#ifdef OMX_GETTIME
        GT_START();
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioPcm, oAacParam);
        GT_END("Set Parameter Test-SetParameter");
#else
        error = OMX_SetParameter (pHandle, OMX_IndexParamAudioPcm, oAacParam);
#endif

        if (error != OMX_ErrorNone) {
            error = OMX_ErrorBadParameter;
            APP_DPRINT("%d :: APP: OMX_ErrorBadParameter\n", __LINE__);
            goto EXIT;
        }

        if (audioinfo.dasfMode) {
            SampleRate = pAacParam->nSampleRate;
            printf("%d :: AAC DECODER RUNNING UNDER DASF MODE\n", __LINE__);
        } else if (audioinfo.dasfMode == 0) {
            printf("%d :: AAC DECODER RUNNING UNDER FILE MODE\n", __LINE__);
        } else {
            printf("%d :: IMPROPER SETTING OF DASF/FILE MODE DASF-1 FILE MODE-0 \n", __LINE__);
            goto EXIT;
        }

        /*Calling setconfig*/
        if (j != 0) {
            audioinfo.framemode = framemode;
        }

#ifdef AM_APP
        /* Request the AM stream ID by yourself */
        cmd_data.hComponent = pHandle;

        cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;

        cmd_data.param1 = 0;

        if ((write(aacdecfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
            printf("%d [%s]- send command to audio manager\n", __LINE__, __FILE__);
            goto EXIT;
        }

        if ((read(aacdecfdread, &cmd_data, sizeof(cmd_data))) < 0) {
            printf("%d [%s]- failure to get data from the audio manager\n", __LINE__, __FILE__);
            goto EXIT;
        }

        audioinfo.streamId = cmd_data.streamID;

#else
        /* Or use this to request OMX component to get the AM stream ID for you*/
        /*
        printf("[%s]-Request OMX to get the AM stream ID\n",__FILE__);
        error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.aacdecstreamIDinfo",&index);
        if (error != OMX_ErrorNone) {
        printf("Error getting extension index\n");
        goto EXIT;
        }
        error = OMX_GetConfig(pHandle,index,&am_streaminfo);
        if (error != OMX_ErrorNone) {
        printf("Error in SetConfig\n");
        goto EXIT;
        }
        audioinfo.streamId = am_streaminfo.streamId;
        */
#endif
        /*
                error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.aacdecHeaderInfo",&index);
                if (error != OMX_ErrorNone) {
                    printf("Error getting extension index\n");
                    goto EXIT;
                }

                error = OMX_SetConfig(pHandle,index,&audioinfo);
                if (error != OMX_ErrorNone) {
                    printf("Error in SetConfig\n");
                    goto EXIT;
                }
        */
#ifndef ANDROID

        if (audioinfo.dasfMode) {
            printf("***************StreamId=%d******************\n", (int)audioinfo.streamId);

            switch (MimoOutput) {

                case 0:
                    dataPath = DATAPATH_MIMO_0;
                    break;

                case 1:
                    dataPath = DATAPATH_MIMO_1;
                    break;

                case 2:
                    dataPath = DATAPATH_MIMO_2;
                    break;

                case - 1:
#ifdef RTM_PATH
                    dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
                    dataPath = DATAPATH_APPLICATION;
#endif
                    break;

                default:
                    fprintf (stderr, "APP: Bad Dasf Mode Selected\n");
                    goto EXIT;
                    break;
            }

            error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.aacdec.datapath", &index);

            if (error != OMX_ErrorNone) {
                printf("Error getting extension index\n");
                goto EXIT;
            }

            error = OMX_SetConfig (pHandle, index, &dataPath);

            if (error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                APP_DPRINT("%d :: AacDecTest.c :: Error from OMX_SetConfig() function\n", __LINE__);
                goto EXIT;
            }
        }

#endif
#ifdef OMX_GETTIME
        GT_START();

#endif
        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

        if (error != OMX_ErrorNone) {
            APP_DPRINT ("Error from SendCommand-Idle(Init) State function - error = %d\n", error);
            goto EXIT;
        }

        error = WaitForState(pHandle, OMX_StateIdle);

#ifdef OMX_GETTIME
        GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

        if (error != OMX_ErrorNone) {
            APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an error %X\n", error);
            goto EXIT;
        }

        for (i = 0; i < testcnt; i++) {

            printf("TC-5 counter = %d\n", i);
            lastinputbuffersent=0;
            close(IpBuf_Pipe[0]);
            close(IpBuf_Pipe[1]);
            close(OpBuf_Pipe[0]);
            close(OpBuf_Pipe[1]);
            close(Event_Pipe[0]);
            close(Event_Pipe[1]);

            /* Create a pipe used to queue data from the callback. */
            retval = pipe(IpBuf_Pipe);

            if ( retval != 0) {
                APP_DPRINT( "Error:Fill Data Pipe failed to open\n");
                goto EXIT;
            }

            retval = pipe(OpBuf_Pipe);

            if ( retval != 0) {
                APP_DPRINT( "Error:Empty Data Pipe failed to open\n");
                goto EXIT;
            }

            retval = pipe(Event_Pipe);

            if ( retval != 0) {
                printf( "Error:Empty Data Pipe failed to open\n");
                goto EXIT;
            }

            nFrameCount = 1;

            fIn = fopen(argv[1], "rb");

            if (fIn == NULL) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                goto EXIT;
            }

            if (0 == audioinfo.dasfMode) {
                fOut = fopen(fname, "w");

                if (fOut == NULL) {
                    fprintf(stderr, "Error:  failed to create the output file \n");
                    goto EXIT;
                }

                APP_DPRINT("%d :: Op File has created\n", __LINE__);
            }

            if ((tcID == 5) | (tcID == 4)) {
                printf("*********************************************************\n");
                printf ("App: %d time Sending OMX_StateExecuting Command: TC 5\n", i);
                printf("*********************************************************\n");
            }

            if (tcID == 13) {
                if (i == 0) {
#ifdef OMX_GETTIME
                    GT_START();
#endif
                    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

                    if (error != OMX_ErrorNone) {
                        APP_DPRINT ("Error from SendCommand-Executing State function\n");
                        goto EXIT;
                    }

                    error = WaitForState(pHandle, OMX_StateExecuting);

#ifdef OMX_GETTIME
                    GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

                    if (error != OMX_ErrorNone) {
                        APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an error %X\n", error);
                        goto EXIT;
                    }
                }
            } else {
#ifdef OMX_GETTIME
                GT_START();
#endif
                error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

                if (error != OMX_ErrorNone) {
                    APP_DPRINT ("Error from SendCommand-Executing State function\n");
                    goto EXIT;
                }

                error = WaitForState(pHandle, OMX_StateExecuting);

#ifdef OMX_GETTIME
                GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

                if (error != OMX_ErrorNone) {
                    APP_DPRINT( "Error:  hAmrEncoder->WaitForState reports an error %X\n", error);
                    goto EXIT;
                }

            }

#ifndef ANDROID
            if (dataPath > DATAPATH_MIMO_0 ) {

                cmd_data.hComponent = pHandle;
                cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;
                cmd_data.param1 = dataPath == DATAPATH_MIMO_1 ? 8000 : 16000;
                cmd_data.param2 = 1;

                if ((write(aacdecfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
                    printf("send command to audio manager\n");
                }
            }

#endif
            for (k = 0; k < numOfInputBuffer; k++) {
#ifdef OMX_GETTIME

                if (k == 0) {
                    GT_FlagE = 1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                    GT_START(); /* Empty Bufffer */
                }

#endif
                error = send_input_buffer (pHandle, pInputBufferHeader[k], fIn);
            }

            if (audioinfo.dasfMode == 0) {
                for (k = 0; k < numOfOutputBuffer; k++) {
#ifdef OMX_GETTIME

                    if (k == 0) {
                        GT_FlagF = 1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                        GT_START(); /* Fill Buffer */
                    }

#endif
                    OMX_FillThisBuffer(pHandle,  pOutputBufferHeader[k]);
                }
            }

#ifdef WAITFORRESOURCES
            /*while( 1 ){
              if(( (error == OMX_ErrorNone) && (gState != OMX_StateIdle)) && (gState != OMX_StateInvalid)&& (timeToExit!=1)){*/
#else
            while ((error == OMX_ErrorNone) && (gState != OMX_StateIdle) && (gState != OMX_StateInvalid)) {
                if (1) {
#endif
            FD_ZERO(&rfds);
            FD_SET(IpBuf_Pipe[0], &rfds);
            FD_SET(OpBuf_Pipe[0], &rfds);
            FD_SET(Event_Pipe[0], &rfds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;
            frmCount++;

            retval = select(fdmax + 1, &rfds, NULL, NULL, &tv);

            if (retval == -1) {
                perror("select()");
                APP_DPRINT ( " : Error \n");
                break;
            }

            if (retval == 0) {
                APP_DPRINT ("%d :: BasicFn App Timeout !!!!!!!!!!! \n", __LINE__);
            }

            switch (tcID) {

                case 1:

                case 5:

                case 6:

                case 13:

                    if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                        /*
                        if ((frmCount >= nNextFlushFrame) && (num_flush < 5000)){
                            printf("Flush #%d\n", num_flush++);

                            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                            if(error != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Pasue State function\n");
                                goto EXIT;
                            }
                            error = WaitForState(pHandle, OMX_StatePause);
                            if(error != OMX_ErrorNone) {
                                fprintf(stderr, "Error:  WaitForState reports an error %X\n", error);
                                goto EXIT;
                            }
                            error = OMX_SendCommand(pHandle, OMX_CommandFlush, 0, NULL);
                            pthread_mutex_lock(&WaitForINFlush_mutex);
                            pthread_cond_wait(&WaitForINFlush_threshold,
                                              &WaitForINFlush_mutex);
                            pthread_mutex_unlock(&WaitForINFlush_mutex);

                            error = OMX_SendCommand(pHandle, OMX_CommandFlush, 1, NULL);
                            pthread_mutex_lock(&WaitForOUTFlush_mutex);
                            pthread_cond_wait(&WaitForOUTFlush_threshold,
                                              &WaitForOUTFlush_mutex);
                            pthread_mutex_unlock(&WaitForOUTFlush_mutex);

                            error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                            if(error != OMX_ErrorNone) {
                                fprintf (stderr,"Error from SendCommand-Executing State function\n");
                                goto EXIT;
                            }
                            error = WaitForState(pHandle, OMX_StateExecuting);
                            if(error != OMX_ErrorNone) {
                                fprintf(stderr, "Error:  WaitForState reports an error %X\n", error);
                                goto EXIT;
                            }
                            
                            fseek(fIn, 0, SEEK_SET);
                            frmCount = 0;
                            nNextFlushFrame = 5 + 50 * ((rand() * 1.0) / RAND_MAX);
                            printf("nNextFlushFrame d= %d\n", nNextFlushFrame);
                        }
                        */
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                        pBuffer->nFlags = 0;
                        error = send_input_buffer (pHandle, pBuffer, fIn);

                        if (error != OMX_ErrorNone) {
                            goto EXIT;
                        }

                        frmCnt++;
                    }

                    break;

                case 2:

                case 4:

                    if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));

                        if (frmCnt == 10) {
                            APP_DPRINT("Shutting down ---------- \n");
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                            if (error != OMX_ErrorNone) {
                                fprintf (stderr, "Error from SendCommand-Idle(Stop) State function\n");
                                goto EXIT;
                            }

                            error = WaitForState(pHandle, OMX_StateIdle);

#ifdef OMX_GETTIME
                            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

                            if (error != OMX_ErrorNone) {
                                fprintf(stderr, "Error:  hAacDecoder->OMX_StateIdle reports an error %X\n", error);
                                goto EXIT;
                            }

                        } else {
                            APP_DPRINT("%d APP: Before send_input_buffer \n", __LINE__);

                            /* Checking if SBR or SBR+PS was detected*/

                            if (frmCnt == 3 ) {
                                pAacParam_SBR_PS->nPortIndex = INPUT_PORT;
                                error = OMX_GetConfig (pHandle, OMX_IndexParamAudioAac, pAacParam_SBR_PS);

                                if (pAacParam_SBR_PS->eAACProfile == OMX_AUDIO_AACObjectHE_PS) {
                                    printf("%d :: [APP] : FILE is e-AAC+  (SBR + PS ) \n", __LINE__);
                                    ConfigChanged = 1;
                                } else if (pAacParam_SBR_PS->eAACProfile == OMX_AUDIO_AACObjectHE) {
                                    printf("%d :: [APP] : FILE is AAC+  (SBR) \n", __LINE__);
                                    ConfigChanged = 1;
                                } else
                                    printf("%d :: [APP] : No SBR or PS Detected \n", __LINE__);

                                if (pAacParam->eAACProfile != pAacParam_SBR_PS->eAACProfile) {
                                    if (ConfigChanged )
                                        printf("%d :: [APP] : File is SBR or PS and the parameter is not set \n", __LINE__);
                                }
                            }

                            error = send_input_buffer (pHandle, pBuffer, fIn);

                            if (error != OMX_ErrorNone) {
                                printf ("Error While reading input pipe\n");
                                goto EXIT;
                            }
                        }

                        if (frmCnt == 10 && tcID == 4) {
                            printf ("*********** Waiting for state to change to Idle ************\n\n");
                            error = WaitForState(pHandle, OMX_StateIdle);

                            if (error != OMX_ErrorNone) {
                                fprintf(stderr, "Error:  hAacDecoder->WaitForState reports an error %X\n", error);
                                goto EXIT;
                            }

                            printf ("*********** State Changed to Idle ************\n\n");

                            printf("Component Has Stopped here and waiting for %d seconds before it plays further\n", SLEEP_TIME);
                            sleep(SLEEP_TIME);
                        }

                        frmCnt++;
                    }

                    break;

                case 3:
                    /* Checking if SBR or SBR+PS was detected*/

                    if (frmCount == 3 ) {
                        pAacParam_SBR_PS->nPortIndex = INPUT_PORT;
                        error = OMX_GetConfig (pHandle, OMX_IndexParamAudioAac, pAacParam_SBR_PS);

                        if (pAacParam_SBR_PS->eAACProfile == OMX_AUDIO_AACObjectHE_PS) {
                            printf("%d :: [APP] : FILE is e-AAC+  (SBR + PS ) \n", __LINE__);
                            ConfigChanged = 1;
                        } else if (pAacParam_SBR_PS->eAACProfile == OMX_AUDIO_AACObjectHE) {
                            printf("%d :: [APP] : FILE is AAC+  (SBR) \n", __LINE__);
                            ConfigChanged = 1;
                        } else
                            printf("%d :: [APP] : No SBR or PS Detected \n", __LINE__);

                        if (pAacParam->eAACProfile != pAacParam_SBR_PS->eAACProfile) {
                            if (ConfigChanged )
                                printf("%d :: [APP] : File is SBR or PS and the parameter is not set \n", __LINE__);
                        }
                    }

                    if (frmCount == 25 || frmCount == 35) {
                        printf ("\n\n*************** Pause command to Component *******************\n");
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);

                        if (error != OMX_ErrorNone) {
                            fprintf (stderr, "Error from SendCommand-Pasue State function\n");
                            goto EXIT;
                        }

                        printf ("*********** Waiting for state to change to Pause ************\n");

                        error = WaitForState(pHandle, OMX_StatePause);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StatePause>");
#endif

                        if (error != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hAacDecoder->WaitForState reports an error %X\n", error);
                            goto EXIT;
                        }

                        printf ("*********** State Changed to Pause ************\n\n");


                        printf("Sleeping for %d secs....\n\n", SLEEP_TIME);
                        sleep(SLEEP_TIME);


                        printf ("*************** Resume command to Component *******************\n");
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

                        if (error != OMX_ErrorNone) {
                            fprintf (stderr, "Error from SendCommand-Executing State function\n");
                            goto EXIT;
                        }

                        printf ("******** Waiting for state to change to Resume ************\n");

                        error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
                        GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

                        if (error != OMX_ErrorNone) {
                            fprintf(stderr, "Error:  hAacDecoder->WaitForState reports an error %X\n", error);
                            goto EXIT;
                        }

                        printf ("*********** State Changed to Resume ************\n\n");
                    }

                    if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                        error = send_input_buffer (pHandle, pBuffer, fIn);

                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }

                        frmCnt++;
                    }

                    break;

                case 7:

                    if (frmCount >= 25 && frmCount <= 75) {
                        conceal = 1;
                        printf("Corrupted data, concealing buffer\n");
                    }  else {
                        conceal = 0;
                    }

                    if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                        error = send_input_buffer (pHandle, pBuffer, fIn);

                        if (error != OMX_ErrorNone) {
                            printf ("Error While reading input pipe\n");
                            goto EXIT;
                        }

                        frmCnt++;
                    }

                    break;

                case 8:

                    if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                        OMX_BUFFERHEADERTYPE* pBuffer = NULL;

                        read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                        pBuffer->nFlags = 0;
                        /* Checking if SBR or SBR+PS was detected*/

                        if (frmCnt == 3 ) {
                            pAacParam_SBR_PS->nPortIndex = INPUT_PORT;
                            error = OMX_GetConfig (pHandle, OMX_IndexParamAudioAac, pAacParam_SBR_PS);

                            if (pAacParam_SBR_PS->eAACProfile == OMX_AUDIO_AACObjectHE_PS) {
                                printf("%d :: [APP] : FILE is e-AAC+  (SBR + PS ) \n", __LINE__);
                                ConfigChanged = 1;
                            } else if (pAacParam_SBR_PS->eAACProfile == OMX_AUDIO_AACObjectHE) {
                                printf("%d :: [APP] : FILE is AAC+  (SBR) \n", __LINE__);
                                ConfigChanged = 1;
                            } else
                                printf("%d :: [APP] : No SBR or PS Detected \n", __LINE__);

                            if (pAacParam->eAACProfile != pAacParam_SBR_PS->eAACProfile) {
                                if (ConfigChanged )
                                    printf("%d :: [APP] : File is SBR or PS and the parameter is not set \n", __LINE__);
                            }
                        }

                        if (frmCnt == 30 || frmCnt == 60 || frmCnt == 90 || frmCnt == 120) {
                            printf("Middle of frame %d\n", frmCnt);
                            pBuffer->pBuffer += rand() % 20;
                            error = send_input_buffer (pHandle, pBuffer, fIn);

                            if (error != OMX_ErrorNone) {
                                goto EXIT;
                            }
                        } else {
                            error = send_input_buffer (pHandle, pBuffer, fIn);

                            if (error != OMX_ErrorNone) {
                                goto EXIT;
                            }
                        }

                        frmCnt++;
                    }

                    break;

                default:
                    APP_DPRINT("%d :: ### Running Simple DEFAULT Case Here ###\n", __LINE__);
            }

            if ( FD_ISSET(OpBuf_Pipe[0], &rfds) ) {
                OMX_BUFFERHEADERTYPE* pBuf = NULL;
                read(OpBuf_Pipe[0], &pBuf, sizeof(pBuf));
                fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                APP_DPRINT ("%d :: Output File has been written\n", __LINE__);

                fflush(fOut);
                OMX_FillThisBuffer(pHandle, pBuf);
            }

            APP_DPRINT ("%d: APP: Component state = %d \n", __LINE__, gState);

            if (playcompleted) {
                if (tcID == 13 && (i != 9)) {
                    puts("RING_TONE: Lets play again!");
                    playcompleted = 0;
                    goto my_exit;

                } else {
#ifdef OMX_GETTIME
                    GT_START();
#endif
                    printf("Sending component to idle \n");
                    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                    if (error != OMX_ErrorNone) {
                        fprintf (stderr, "\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
                        goto EXIT;
                    }

                    error = WaitForState(pHandle, OMX_StateIdle);

#ifdef OMX_GETTIME
                    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

                    if (error != OMX_ErrorNone) {
                        fprintf(stderr, "\nError:  hAACDecoder->WaitForState reports an error %X!!!!!!!\n", error);
                        goto EXIT;
                    }

                    playcompleted = 0;
                }
            }

            /*-------*/
            if ( FD_ISSET(Event_Pipe[0], &rfds) ) {
                OMX_U8 pipeContents = 0;
                read(Event_Pipe[0], &pipeContents, sizeof(OMX_U8));
                APP_DPRINT("%d :: received RM event: %d\n", __LINE__, pipeContents);

                if (pipeContents == 0) {

                    printf("Test app received OMX_ErrorResourcesPreempted\n");
                    WaitForState(pHandle, OMX_StateIdle);

                    for (i = 0; i < numOfInputBuffer; i++) {
                        APP_DPRINT("%d :: App: Freeing %p IP BufHeader\n", __LINE__, pInputBufferHeader[i]);
                        error = OMX_FreeBuffer(pHandle, INPUT_PORT, pInputBufferHeader[i]);

                        if ((error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                            goto EXIT;
                        }
                    }

                    if (audioinfo.dasfMode == 0) {
                        for (i = 0; i < numOfOutputBuffer; i++) {
                            APP_DPRINT("%d :: App: Freeing %p OP BufHeader\n", __LINE__, pOutputBufferHeader[i]);
                            error = OMX_FreeBuffer(pHandle, OUTPUT_PORT, pOutputBufferHeader[i]);

                            if ((error != OMX_ErrorNone)) {
                                APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                                goto EXIT;
                            }
                        }
                    }

#ifdef USE_BUFFER
                    /* free the UseBuffers */
                    for (i = 0; i < numOfInputBuffer; i++) {
                        OMX_MEMFREE_STRUCT_DSPALIGN(pInputBuffer[i], OMX_U8);
                    }

                    if (audioinfo.dasfMode == 0) {
                        for (i = 0; i < numOfOutputBuffer; i++) {
                            OMX_MEMFREE_STRUCT_DSPALIGN(pOutputBuffer[i], OMX_U8);
                        }
                    }

#endif

                    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

                    WaitForState(pHandle, OMX_StateLoaded);

                    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateWaitForResources, NULL);

                    WaitForState(pHandle, OMX_StateWaitForResources);
                } else if (pipeContents == 1) {
                    printf("Test app received OMX_ErrorResourcesAcquired\n");

                    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                    for (i = 0; i < numOfInputBuffer; i++) {
                        pInputBufferHeader[i] = newmalloc(1);
                        /* allocate input buffer */
                        error = OMX_AllocateBuffer(pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize); /*To have enought space for    */

                        if (error != OMX_ErrorNone) {
                            APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n", __LINE__);
                        }
                    }

                    WaitForState(pHandle, OMX_StateIdle);

                    OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                    WaitForState(pHandle, OMX_StateExecuting);
                    rewind(fIn);

                    for (i = 0; i < numOfInputBuffer;i++) {
                        send_input_buffer(pHandle, pInputBufferHeader[i], fIn);
                    }
                }

                if (pipeContents == 2) {
#ifdef OMX_GETTIME
                    GT_START();
#endif
                    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                    if (error != OMX_ErrorNone) {
                        fprintf (stderr, "Error from SendCommand-Idle(Stop) State function\n");
                        goto EXIT;
                    }

                    error = WaitForState(pHandle, OMX_StateIdle);

#ifdef OMX_GETTIME
                    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

                    if (error != OMX_ErrorNone) {
                        fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                        goto EXIT;
                    }

                    for (i = 0; i < numOfInputBuffer; i++) {
                        APP_DPRINT("%d :: App: Freeing %p IP BufHeader\n", __LINE__, pInputBufferHeader[i]);
                        error = OMX_FreeBuffer(pHandle, INPUT_PORT, pInputBufferHeader[i]);

                        if ((error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                            goto EXIT;
                        }
                    }

                    if (audioinfo.dasfMode == 0) {
                        for (i = 0; i < numOfOutputBuffer; i++) {
                            APP_DPRINT("%d :: App: Freeing %p OP BufHeader\n", __LINE__, pOutputBufferHeader[i]);
                            error = OMX_FreeBuffer(pHandle, OUTPUT_PORT, pOutputBufferHeader[i]);

                            if ((error != OMX_ErrorNone)) {
                                APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                                goto EXIT;
                            }
                        }
                    }

#if 1
                    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

                    if (error != OMX_ErrorNone) {
                        APP_DPRINT ("%d :: Error from SendCommand-Idle State function\n", __LINE__);
                        printf("goto EXIT %d\n", __LINE__);
                        goto EXIT;
                    }

                    error = WaitForState(pHandle, OMX_StateLoaded);

#ifdef OMX_GETTIME
                    GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif

                    if (error != OMX_ErrorNone) {
                        APP_DPRINT( "%d :: Error:  WaitForState reports an eError %X\n", __LINE__, error);
                        printf("goto EXIT %d\n", __LINE__);
                        goto EXIT;
                    }

#endif
                    goto SHUTDOWN;
                }
            }

            /*-------*/
        } else if (!preempted) {
            sched_yield();
        } else {
            goto SHUTDOWN;
        }
    } /* While Loop Ending Here */

my_exit:
    if (0 == audioinfo.dasfMode) {
        fclose(fOut);
    }

    APP_DPRINT("%d :: Closing input file\n", __LINE__);

    fclose(fIn);

    if (i != (testcnt - 1)) {
        if ((tcID == 5) || (tcID == 2)) {
            printf("%d :: sleeping here for %d secs\n", __LINE__, SLEEP_TIME);
            sleep (SLEEP_TIME);
        } else {
            sleep (0);
        }
    }
}

SHUTDOWN:

#ifdef AM_APP
cmd_data.hComponent = pHandle;
cmd_data.AM_Cmd = AM_Exit;

if ((write(aacdecfdwrite, &cmd_data, sizeof(cmd_data))) < 0)
    printf("%s(%s)-Fail to send command to audio manager\n", __FILE__, __func__);

close(aacdecfdwrite);

close(aacdecfdread);

#endif
/* free the Allocate Buffers */
for (i = 0; i < numOfInputBuffer; i++) {
    APP_DPRINT("%d :: App: Freeing %p IP BufHeader\n", __LINE__, pInputBufferHeader[i]);
    error = OMX_FreeBuffer(pHandle, INPUT_PORT, pInputBufferHeader[i]);

    if ((error != OMX_ErrorNone)) {
        APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
        goto EXIT;
    }
}

if (audioinfo.dasfMode == 0) {
    for (i = 0; i < numOfOutputBuffer; i++) {
        APP_DPRINT("%d :: App: Freeing %p OP BufHeader\n", __LINE__, pOutputBufferHeader[i]);
        error = OMX_FreeBuffer(pHandle, OUTPUT_PORT, pOutputBufferHeader[i]);

        if ((error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
            goto EXIT;
        }
    }
}

#ifdef USE_BUFFER
/* free the UseBuffers */
for (i = 0; i < numOfInputBuffer; i++) {
    OMX_MEMFREE_STRUCT_DSPALIGN(pInputBuffer[i], OMX_U8);
}

if (audioinfo.dasfMode == 0) {
    for (i = 0; i < numOfOutputBuffer; i++) {
        OMX_MEMFREE_STRUCT_DSPALIGN(pOutputBuffer[i], OMX_U8);
    }
}

#endif

#ifdef OMX_GETTIME
GT_START();

#endif
error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

error = WaitForState(pHandle, OMX_StateLoaded);

#ifdef OMX_GETTIME
GT_END("Call to SendCommand <OMX_StateLoaded>");

#endif
if (error != OMX_ErrorNone) {
    APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n", __LINE__);
    goto EXIT;
}

#ifdef WAITFORRESOURCES
error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateWaitForResources, NULL);

if (error != OMX_ErrorNone) {
    APP_DPRINT ("%d :: Error from SendCommand-Idle State function\n", __LINE__);
    printf("goto EXIT %d\n", __LINE__);

    goto EXIT;
}

error = WaitForState(pHandle, OMX_StateWaitForResources);

/* temporarily put this here until I figure out what should really happen here */
sleep(10);
/* temporarily put this here until I figure out what should really happen here */
#endif

APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n", __LINE__, pCompPrivateStruct);

if (pCompPrivateStruct != NULL) {
    newfree(pCompPrivateStruct);
    pCompPrivateStruct = NULL;
}

APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n", __LINE__, pAacParam);

if (pAacParam != NULL) {
    newfree(pAacParam);
    pAacParam = NULL;
}

APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n", __LINE__, pAacParam_SBR_PS);

if (pAacParam_SBR_PS != NULL) {
    newfree(pAacParam_SBR_PS);
    pAacParam_SBR_PS = NULL;
}


APP_MEMPRINT("%d :: [TESTAPPFREE] %p\n", __LINE__, oAacParam);

if (oAacParam != NULL) {
    newfree(oAacParam);
    oAacParam = NULL;
}

error = TIOMX_FreeHandle(pHandle);

if ( (error != OMX_ErrorNone)) {
    APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
    goto EXIT;
}

APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n", __LINE__);
}

pthread_mutex_destroy(&WaitForState_mutex);

pthread_cond_destroy(&WaitForState_threshold);

pthread_mutex_destroy(&WaitForOUTFlush_mutex);
pthread_cond_destroy(&WaitForOUTFlush_threshold);
pthread_mutex_destroy(&WaitForINFlush_mutex);
pthread_cond_destroy(&WaitForINFlush_threshold);

if (audioinfo.dasfMode == 0) {
    printf("**********************************************************\n");
    printf("NOTE: An output file %s has been created in file system\n", APP_OUTPUT_FILE);
    printf("**********************************************************\n");
}

#ifdef AACDEC_DEBUGMEM
printf("\n-Printing memory not deleted-\n");

for (r = 0;r < 500;r++) {
    if (lines[r] != 0) {
        printf(" --->%d Bytes allocated on %p File:%s Line: %d\n", bytes[r], arr[r], file[r], lines[r]);
    }
}

#endif

EXIT:
if (bInvalidState == OMX_TRUE) {
#ifndef USE_BUFFER
    error = FreeAllResources(pHandle,
                             pInputBufferHeader[0],
                             pOutputBufferHeader[0],
                             numOfInputBuffer,
                             numOfOutputBuffer,
                             fIn, fOut);
#else
    error = FreeAllUseResources(pHandle,
                                pInputBuffer,
                                pOutputBuffer,
                                numOfInputBuffer,
                                numOfOutputBuffer,
                                fIn, fOut);
#endif
}

#ifdef OMX_GETTIME
GT_END("AAC_DEC test <End>");

OMX_ListDestroy(pListHead);

#endif
return error;
       }


       OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle,
                                       OMX_BUFFERHEADERTYPE* pBuffer,
FILE *fIn) {
    OMX_ERRORTYPE error = OMX_ErrorNone;
    int nRead = 0;
    int framelen[2] = {0};
    OMX_S32 nAllocLenTemp = (OMX_S32)pBuffer->nAllocLen;

    if (!framemode) {
        nRead = fill_data (pBuffer, fIn);
        /*Don't send more buffers after OMX_BUFFERFLAG_EOS*/

        if (lastinputbuffersent) {
            pBuffer->nFlags = 0;
            return error;
        }

        if (nRead < nAllocLenTemp && !lastinputbuffersent ) {
            if (ringCount > 0) {
                printf("Rings left: %d\n", ringCount);
                fseek(fIn, 0L, SEEK_SET);
                ringCount--;
            } else {
                pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                lastinputbuffersent = 1;
            }
        } else {
            pBuffer->nFlags = 0;
        }

        pBuffer->nTimeStamp = rand() % 100;

        pBuffer->nTickCount = rand() % 70;
        TIME_PRINT("TimeStamp Input: %lld\n", pBuffer->nTimeStamp);
        TICK_PRINT("TickCount Input: %ld\n", pBuffer->nTickCount);
        pBuffer->nFilledLen = nRead;

        if (!preempted) {
            error = OMX_EmptyThisBuffer(pHandle, pBuffer);

            if (error == OMX_ErrorIncorrectStateOperation)
                error = 0;
        }

        return error;
    } else {

        /* Read the frame size */
        framelen[0] = fgetc(fIn);
        framelen[1] = fgetc(fIn);
        nRead = framelen[1];
        nRead <<= 8;
        nRead |= framelen[0];
        APP_DPRINT("length: %d\n", nRead);

        if (nRead > 0) {
            fread(pBuffer->pBuffer, 1, nRead, fIn);

            /*For error concealment*/

            if (conceal) {
                pBuffer->nFlags = OMX_BUFFERFLAG_DATACORRUPT;
            } else {
                pBuffer->nFlags = 0;
            }

            pBuffer->nTimeStamp = rand() % 100;

            pBuffer->nTickCount = rand() % 70;
            TIME_PRINT("TimeStamp Input: %lld\n", pBuffer->nTimeStamp);
            TICK_PRINT("TickCount Input: %ld\n", pBuffer->nTickCount);
            pBuffer->nFilledLen = nRead;

            if (!preempted) {
                error = OMX_EmptyThisBuffer(pHandle, pBuffer);

                if (error == OMX_ErrorIncorrectStateOperation)
                    error = 0;
            }

        } else {
            if (!lastinputbuffersent) {
                pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                pBuffer->nFilledLen = 0;

                pBuffer->nTimeStamp = rand() % 100;
                pBuffer->nTickCount = rand() % 70;
                TIME_PRINT("TimeStamp Input: %lld\n", pBuffer->nTimeStamp);
                TICK_PRINT("TickCount Input: %ld\n", pBuffer->nTickCount);

                if (!preempted) {
                    error = OMX_EmptyThisBuffer(pHandle, pBuffer);

                    if (error == OMX_ErrorIncorrectStateOperation)
                        error = 0;
                }

                lastinputbuffersent = 1;
            } else {
                lastinputbuffersent = 0;
                return error;
            }
        }
    }

    return error;
}

int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn) {
    int nRead = 0;
    static int totalRead = 0;
    static int fileHdrReadFlag = 0;

    if (!fileHdrReadFlag) {
        fileHdrReadFlag = 1;
    }

    nRead = fread(pBuf->pBuffer, 1, pBuf->nAllocLen , fIn);

    APP_DPRINT ("\n*****************************************************\n");
    APP_DPRINT ("%d :: APP:: Buffer No : %d  pBuf->pBuffer = %p pBuf->nAllocLen = * %ld, nRead = %d\n",
                __LINE__, ++frame_no, pBuf->pBuffer, pBuf->nAllocLen, nRead);
    APP_DPRINT ("\n*****************************************************\n");

    totalRead += nRead;
    pBuf->nFilledLen = nRead;
    return nRead;
}

#ifdef AACDEC_DEBUGMEM
/* ========================================================================== */
/**
* @mymalloc() This function is used to debug memory leaks
*
* @param
*
* @pre None
*
* @post None
*
* @return None
*/
/* ========================================================================== */
void * mymalloc(int line, char *s, int size) {
    void *p;
    int e = 0;
    p = malloc(size);

    if (p == NULL) {
        APP_DPRINT("Memory not available\n");
        exit(1);
    } else {
        while ((lines[e] != 0) && (e < 500) ) {
            e++;
        }

        arr[e] = p;

        lines[e] = line;
        bytes[e] = size;
        strcpy(file[e], s);
        APP_DPRINT("Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
        return p;
    }
}

/* ========================================================================== */
/**
* @myfree() This function is used to debug memory leaks
*
* @param
*
* @pre None
*
* @post None
*
* @return None
*/
/* ========================================================================== */
int myfree(void *dp, int line, char *s) {
    int q = 0;

    for (q = 0;q < 500;q++) {
        if (arr[q] == dp) {
            APP_DPRINT("Deleting %d bytes on address %p, line %d file %s\n", bytes[q], dp, line, s);
            free(dp);
            dp = NULL;
            lines[q] = 0;
            strcpy(file[q], "");
            break;
        }
    }

    if (500 == q)
        APP_DPRINT("\n\nPointer not found. Line:%d    File%s!!\n\n", line, s);
}

#endif
/*=================================================================
    Freeing All allocated resources
==================================================================*/
int FreeAllResources( OMX_HANDLETYPE pHandle,
                      OMX_BUFFERHEADERTYPE* pBufferIn,
                      OMX_BUFFERHEADERTYPE* pBufferOut,
                      int NIB, int NOB,
FILE* fIn, FILE* fOut) {
    printf("%d::Freeing all resources by state invalid \n", __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U16 i = 0;

    for (i = 0; i < NIB; i++) {
        printf("%d :: APP: About to free pInputBufferHeader[%d]\n", __LINE__, i);
        eError = OMX_FreeBuffer(pHandle, INPUT_PORT, pBufferIn + i);
    }

    for (i = 0; i < NOB; i++) {
        printf("%d :: APP: About to free pOutputBufferHeader[%d]\n", __LINE__, i);
        eError = OMX_FreeBuffer(pHandle, OUTPUT_PORT, pBufferOut + i);
    }

    /*i value is fixed by the number calls to malloc in the App */
    for (i = 0; i < 6;i++) {
        if (ArrayOfPointers[i] != NULL)
            free(ArrayOfPointers[i]);
    }

    eError = close (IpBuf_Pipe[0]);

    eError = close (IpBuf_Pipe[1]);
    eError = close (OpBuf_Pipe[0]);
    eError = close (OpBuf_Pipe[1]);
    eError = close (Event_Pipe[0]);
    eError = close (Event_Pipe[1]);

    if (fOut != NULL) {
        fclose(fOut);
        fOut = NULL;
    }

    if (fIn != NULL) {
        fclose(fIn);
        fIn = NULL;
    }

    TIOMX_FreeHandle(pHandle);

    return eError;
}

/*=================================================================
    Freeing the resources with USE_BUFFER define
==================================================================*/
#ifdef USE_BUFFER
int FreeAllUseResources(OMX_HANDLETYPE pHandle,
                        OMX_U8* UseInpBuf[],
                        OMX_U8* UseOutBuf[],
                        int NIB, int NOB,
FILE* fIn, FILE* fOut) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U16 i;
    printf("%d::Freeing all resources by state invalid \n", __LINE__);
    /* free the UseBuffers */

    for (i = 0; i < NIB; i++) {
        OMX_MEMFREE_STRUCT_DSPALIGN(UseInpBuf[i],OMX_U8);
    }

    for (i = 0; i < NOB; i++) {
        OMX_MEMFREE_STRUCT_DSPALIGN(UseOutBuf[i],OMX_U8);
    }

    /*i value is fixed by the number calls to malloc in the App */
    for (i = 0; i < 6;i++) {
        if (ArrayOfPointers[i] != NULL)
            free(ArrayOfPointers[i]);
    }

    eError = close (IpBuf_Pipe[0]);

    eError = close (IpBuf_Pipe[1]);
    eError = close (OpBuf_Pipe[0]);
    eError = close (OpBuf_Pipe[1]);
    eError = close (Event_Pipe[0]);
    eError = close (Event_Pipe[1]);

    if (fOut != NULL) {
        fclose(fOut);
        fOut = NULL;
    }

    if (fIn != NULL) {
        fclose(fIn);
        fIn = NULL;
    }

    TIOMX_FreeHandle(pHandle);

    return eError;
}

#endif
