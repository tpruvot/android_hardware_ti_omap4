
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
* @file OMX_Mp3DecTest.c
*
* This file contains the test application code that invokes the component.
*
* @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\mp3_dec\tests
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
#include <alloca.h>
#include <OMX_TI_Common.h>
#ifdef AM_APP
#include <AudioManagerAPI.h>
#endif
#include <linux/soundcard.h>
#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

#ifdef ALSA
#include <alsa/asoundlib.h>
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
static char *device = "default";            /* playback device */

snd_pcm_t *playback_handle = NULL;
snd_pcm_hw_params_t *hw_params = NULL;

snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE ;   /* sample format */
snd_pcm_sframes_t frames = 0;
unsigned int rate = 0;

int err;
#endif
FILE *fpRes = NULL;

/* ======================================================================= */
/**
 * @def    APP_MEMDEBUG    This Macro turns On the logic to detec memory
 *                         leaks on the App. To debug the component,
 *                         MP3DEC_MEMDEBUG must be defined.
 */
/* ======================================================================= */
#undef APP_DEBUG
/*#define APP_MEMDEBUG*/
/*for Timestamp and Tickcount*/
#undef APP_TIME_TIC_DEBUG


void *arr[500];
int lines[500];
int bytes[500];
char file[500][50]; /*int ind=0;*/
#ifdef APP_MEMDEBUG
int r = 0;
#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)
void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);
#else
#define newmalloc(x) malloc(x)
#define newfree(z) free(z)
#endif

#undef USE_BUFFER

#ifdef APP_DEBUG
#define APP_DPRINT(...)    fprintf(stderr,__VA_ARGS__)
#else
#define APP_DPRINT(...)
#endif

#ifdef APP_TIME_TIC_DEBUG
#define TIME_PRINT(...)        fprintf(stderr,__VA_ARGS__)
#define TICK_PRINT(...)        fprintf(stderr,__VA_ARGS__)
#else
#define TIME_PRINT(...)
#define TICK_PRINT(...)
#endif

#define APP_OUTPUT_FILE "output.pcm"
#define SLEEP_TIME 2

#define MP3_APP_ID  100 /* Defines MP3 Dec App ID, App must use this value */
#define MP3D_MAX_NUM_OF_BUFS 10 /* Max number of buffers used */

#define MP3D_MONO_STREAM  1 /* Mono stream index */
#define MP3D_STEREO_STREAM  2
#ifdef AM_APP
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"
#define GRFIFO1 "/dev/grfifo.1"
#define GRFIFO2 "/dev/grfifo.2"
#endif
#define PERMS 0666

#ifdef OMX_GETTIME
OMX_ERRORTYPE eError = OMX_ErrorNone;
int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
static OMX_NODE* pListHead = NULL;
#endif

typedef enum MP3D_COMP_PORT_TYPE {
    MP3D_INPUT_PORT = 0,
    MP3D_OUTPUT_PORT
}MP3D_COMP_PORT_TYPE;


int nIpBuffs = 4;
int nOpBuffs = 4;
#ifdef ALSA
#define PERIODS    32 /* 32 periods with 576 frames each */
#define FRAMES    576    /* 576 frames per period */
int nIpBufSize = 2 * 1152;
int nOpBufSize = PERIODS * FRAMES * 2;
#else
int nIpBufSize = 8000;
int nOpBufSize = 8192 * 10;
#endif

int gDasfMode = 0;
int MimoOutput = -1;
int gStream = 0;
int gBitOutput = 0;
int sampleRateChange = 0;
int playcompleted = 0;
int sendlastbuffer = 0;

static OMX_BOOL bInvalidState;
void* ArrayOfPointers[6] = {NULL};
int gainRamp = 0;
int fdgrwrite = 0;
int fdgrread = 0;
#ifdef AM_APP
MDN_GAIN_ENVELOPE *ptGainEnvelope = NULL;
unsigned char *pDctnParamBuffer = NULL;
#endif
int stress = 0;
int framemode = 0;
int lastinputbuffersent = 0;

int num_flush = 0;
int nFlushDone = 0;
int nNextFlushFrame = 100;
int iFlushDone = 0;

OMX_STATETYPE gState = OMX_StateExecuting;

int preempted = 0;

#undef  WAITFORRESOURCES

int mp3decfdwrite = 0;
int mp3decfdread = 0;

/* inline int maxint(int a, int b); */

int maxint(int a, int b);

int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn);

/*void ConfigureAudio(int SampleRate);*/

OMX_STRING strMp3Decoder = "OMX.TI.MP3.decode";

int IpBuf_Pipe[2] = {0};
int OpBuf_Pipe[2] = {0};
int Event_Pipe[2] = {0};

static OMX_BOOL bInvalidState;

pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag = 0;
OMX_U8        TargetedState = 0;

pthread_mutex_t WaitForOUTFlush_mutex;
pthread_cond_t  WaitForOUTFlush_threshold;
pthread_mutex_t WaitForINFlush_mutex;
pthread_cond_t  WaitForINFlush_threshold;

enum {
    MONO,
    STEREO
};

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);

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
/* ================================================================================= * */
/**
* @fn setAudioParams() Set audio hw via ALSA APIs
*
* @param         None
*
* @param         None
*
* @pre          None
*
* @post         None
*
*  @return      Nothing
*
*  @see         None
*/
/* ================================================================================ * */
#ifdef ALSA
void setAudioParams(int channels) {
    int dir = 0;

    if ((err = snd_pcm_open(&playback_handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(1);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&hw_params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(playback_handle, hw_params);

    /* Set the desired hardware parameters. */
    /* Interleaved mode */
    snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);

    /* 1 channel for mono, 2 channels for stereo */
    snd_pcm_hw_params_set_channels(playback_handle, hw_params, channels);

    printf("setAudioParams - rate: %d\n", rate);

    /* Sampling rate in bits/second */
    snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &rate, 0);

    /* Set period size to x frames. */
    frames = nOpBufSize / (PERIODS * channels * 2);

    /* frames = FRAMES;     period size in frames */
    snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &frames, &dir);

    printf("setAudioParams - period: %d frames; output buffer size: %d\n", frames, nOpBufSize);

    /* Write the parameters to the driver */
    if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
        exit (1);
    }
}

#endif

/* ================================================================================= * */
/**
* @fn maxint() gives the maximum of two integers.
*
* @param a intetger a
*
* @param b integer b
*
* @pre          None
*
* @post         None
*
*  @return      bigger number
*
*  @see         None
*/
/* ================================================================================ * */
int maxint(int a, int b) {
    return (a > b) ? a : b;
}

/* ================================================================================= * */
/**
* @fn WaitForState() Waits for the state to change.
*
* @param pHandle This is component handle allocated by the OMX core.
*
* @param DesiredState This is state which the app is waiting for.
*
* @pre          None
*
* @post         None
*
*  @return      Appropriate OMX Error.
*
*  @see         None
*/
/* ================================================================================ * */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle, OMX_STATETYPE DesiredState) {
    OMX_STATETYPE CurState = OMX_StateInvalid;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (bInvalidState == OMX_TRUE) {
        eError = OMX_ErrorInvalidState;
        return eError;
    }

    eError = OMX_GetState(*pHandle, &CurState);

    if (CurState != DesiredState) {
        WaitForState_flag = 1;
        TargetedState = DesiredState;
        pthread_mutex_lock(&WaitForState_mutex);
        pthread_cond_wait(&WaitForState_threshold,
                          &WaitForState_mutex);/*Going to sleep till signal arrives*/
        pthread_mutex_unlock(&WaitForState_mutex);
    }

    if ( eError != OMX_ErrorNone ) return eError;

    return OMX_ErrorNone;
}


/* ================================================================================= * */
/**
* @fn EventHandler() This is event handle which is called by the component on
* any event change.
*
* @param hComponent This is component handle allocated by the OMX core.
*
* @param pAppData This is application private data.
*
* @param eEvent  This is event generated from the component.
*
* @param nData1  Data1 associated with the event.
*
* @param nData2  Data2 associated with the event.
*
* @param pEventData Any other string event data from component.
*
* @pre          None
*
* @post         None
*
*  @return      Appropriate OMX error.
*
*  @see         None
*/
/* ================================================================================ * */
OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData) {
    OMX_U8 writeValue = 0;
    int error = 0;
    switch (eEvent) {

        case OMX_EventCmdComplete:
            gState = (OMX_STATETYPE)nData2;

            if (nData1 == OMX_CommandFlush) {
                nFlushDone++;
                iFlushDone = 1;

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

            if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) && (WaitForState_flag)) {
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                pthread_mutex_unlock(&WaitForState_mutex);
            }
            break;

        case OMX_EventError:

            if (nData1 != OMX_ErrorNone && (bInvalidState == OMX_FALSE)) {
                APP_DPRINT ("%d: App: ErrorNotification received: Error Num %d: String :%s\n", __LINE__, (int)nData1, (OMX_STRING)pEventData);
            }

            if (nData1 == OMX_ErrorInvalidState && (bInvalidState == OMX_FALSE)) {
                APP_DPRINT("EventHandler: Invalid State!!!!\n");
                bInvalidState = OMX_TRUE;
                if (WaitForState_flag == 1){
                    WaitForState_flag = 0;
                    pthread_mutex_lock(&WaitForState_mutex);
                    pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                    pthread_mutex_unlock(&WaitForState_mutex);
                }
            } else if (nData1 == OMX_ErrorResourcesPreempted) {
                preempted = 1;
                writeValue = 0;
                write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));
            } else if (nData1 == OMX_ErrorResourcesLost) {
                WaitForState_flag = 0;
                pthread_mutex_lock(&WaitForState_mutex);
                pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                pthread_mutex_unlock(&WaitForState_mutex);
            } else if (nData1 == OMX_ErrorInsufficientResources) {
                printf("Insufficient Resources\n\n");
            }
            break;

        case OMX_EventMax:

        case OMX_EventMark:

        case OMX_EventPortSettingsChanged:
            error = OMX_SendCommand(hComponent, OMX_CommandPortEnable, 1,NULL);
            if (error){
                printf("There was an error on port enable \n");
            }
            break;

        case OMX_EventBufferFlag:
            if ((nData1 == 0x01) && (nData2 == (OMX_U32)OMX_BUFFERFLAG_EOS)) {
                playcompleted = 1;
            }

#ifdef WAITFORRESOURCES
            writeValue = 2;

            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));

#endif

            break;

        case OMX_EventComponentResumed:

        case OMX_EventDynamicResourcesAvailable:

        case OMX_EventPortFormatDetected:

        case OMX_EventResourcesAcquired:
            writeValue = 1;

            write(Event_Pipe[1], &writeValue, sizeof(OMX_U8));

            preempted = 0;

            break;
    }

    return OMX_ErrorNone;
}

/* ================================================================================= * */
/**
* @fn FillBufferDone() Component sens the output buffer to app using this
* callback.
*
* @param hComponent This is component handle allocated by the OMX core.
*
* @param ptr This is another pointer.
*
* @param pBuffer This is output buffer.
*
* @pre          None
*
* @post         None
*
*  @return      None
*
*  @see         None
*/
/* ================================================================================ * */
void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer) {
    write(OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME

    if (GT_FlagF == 1 ) /* First Buffer Reply*/ { /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }

#endif
}


/* ================================================================================= * */
/**
* @fn EmptyBufferDone() Component sends the input buffer to app using this
* callback.
*
* @param hComponent This is component handle allocated by the OMX core.
*
* @param ptr This is another pointer.
*
* @param pBuffer This is input buffer.
*
* @pre          None
*
* @post         None
*
*  @return      None
*
*  @see         None
*/
/* ================================================================================ * */
void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer) {
    if (!preempted)
        write(IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));

    /*add on: timestamp & tickcount Output Buffer print*/
    TIME_PRINT("TimeStamp Output: %lld\n", pBuffer->nTimeStamp);

    TICK_PRINT("TickCount Output: %lu\n\n", pBuffer->nTickCount);

#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) /* First Buffer Reply*/ { /* 1 = First Buffer,  0 = Not First Buffer  */
        GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
        GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }

#endif
}

/* ================================================================================= * */
/**
* @fn main() This is the main function of application which gets called.
*
* @param argc This is the number of commandline arguments..
*
* @param argv[] This is an array of pointers to command line arguments..
*
* @pre          None
*
* @post         None
*
*  @return      An integer value.
*
*  @see         None
*/
/* ================================================================================ * */
int main(int argc, char* argv[]) {
    OMX_CALLBACKTYPE Mp3CaBa = {(void *)EventHandler,
                                (void*)EmptyBufferDone,
                                (void*)FillBufferDone
                               };
    OMX_HANDLETYPE *pHandle = NULL;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = MP3_APP_ID ;
    TI_OMX_DSP_DEFINITION* audioinfo = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL;
    OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute = NULL;
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume = NULL;
    OMX_INDEXTYPE index = 0;
#ifdef AM_APP
    AM_COMMANDDATATYPE cmd_data;
#endif
    TI_OMX_DATAPATH dataPath;
    OMX_AUDIO_PARAM_PCMMODETYPE *pPcmParam = NULL;
    OMX_AUDIO_PARAM_MP3TYPE *pMp3Param = NULL;
    OMX_COMPONENTTYPE *pComponent = NULL;
    OMX_BUFFERHEADERTYPE* pInputBufferHeader[MP3D_MAX_NUM_OF_BUFS] = {NULL};
    OMX_BUFFERHEADERTYPE* pOutputBufferHeader[MP3D_MAX_NUM_OF_BUFS] = {NULL};

#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[MP3D_MAX_NUM_OF_BUFS] = {NULL};
    OMX_U8* pOutputBuffer[MP3D_MAX_NUM_OF_BUFS] = {NULL};
#endif

    struct timeval tv;
    fd_set rfds;

    int retval, i, l, j = 0;
    int nFrameCount = 1;
    int frmCount = 0;
    int frmCnt = 1, k;
    int testcnt = 1;
    int testcnt1 = 1;
    int testcnt2 = 1;
    int SampleRate = 0;
    char fname[100] = APP_OUTPUT_FILE;
    FILE* fIn = NULL;
    FILE* fOut = NULL;
    int tcID = 0;
    bInvalidState = OMX_FALSE;
    int mpeg1layer2 = 0;

    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);

    pthread_mutex_init(&WaitForOUTFlush_mutex, NULL);
    pthread_cond_init (&WaitForOUTFlush_threshold, NULL);
    pthread_mutex_init(&WaitForINFlush_mutex, NULL);
    pthread_cond_init (&WaitForINFlush_threshold, NULL);

    WaitForState_flag = 0;

#ifdef ALSA
    int underrunCount = 0;
    int shortWrites = 0;
    int totalWrites = 0;
#endif

    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In MP3 DECODER Test Application:\n");
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

    if (argc < 6 ) {
        printf("Usage: Wrong Arguments: See Examples Below\n\n");
        printf("./Mp3DecoderTest ./patterns/hecommon_Stereo_44Khz.mp3 STEREO TC_1 44100 DM 0\n");
        printf("./Mp3DecoderTest ./patterns/hecommon_Stereo_44Khz.mp3 STEREO TC_1 44100 FM B16 0\n");
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

    gDasfMode = atoi(argv[5]);

    if (!strcmp(argv[5], "DM")) {
        gDasfMode = 1;
        /* check the input parameters */

        if (argc > 12) {
            printf("Usage: Wrong Arguments: See Examples Below\n\n");
            printf("./Mp3DecoderTest ./patterns/hecommon_Stereo_44Khz.mp3 STEREO TC_1 44100 DM 0\n");
            printf("./Mp3DecoderTest ./patterns/hecommon_Stereo_44Khz.mp3 STEREO TC_1 44100 FM B16 0\n");
            goto EXIT;
        }

        if (argv[6] != 0) {
            mpeg1layer2 = atoi(argv[6]);

            if (argv[7]  != 0) {
                if (atoi(argv[7]) == 0 || atoi(argv[7]) == 1 ) {
                    stress = atoi(argv[7]);

                    if (argv[8] != 0) {
                        nIpBuffs = atoi(argv[8]);

                        if (argv[9]  != 0) {
                            nIpBufSize = atoi(argv[9]);

                            if (argv[10]  != 0) {
                                nOpBuffs = atoi(argv[10]);

                                if (argv[11]  != 0) {
                                    nOpBufSize = atoi(argv[11]);
                                }
                            }
                        }
                    }
                } else {
                    printf("Invalid stress flag (must be 1 for active or 0 for inactive)\n");
                    goto EXIT;
                }
            }
        } else {
            mpeg1layer2 = 0;
        }
    } else if (!strcmp(argv[5], "FM")) {
        /*changed*/
        if ((argc < 7) || (argc > 13) ) {
            printf("Usage: Wrong Arguments: See Example Below\n\n");
            printf("./Mp3DecoderTest ./patterns/MJ08khz32Kbps_Stereo.mp3 STEREO TC_1 8000 FM B16 0\n");
            printf("Robustnes: ./Mp3DecoderTest ./patterns/MJ08khz32Kbps_Stereo.mp3 STEREO TC_1 8000 FM B16 0 1\n");
            goto EXIT;
        }

        gDasfMode = 0;

        if (argv[7] != 0) {
            mpeg1layer2 = atoi(argv[7]);

            if (argv[8]  != 0) {
                if (atoi(argv[8]) == 0 || atoi(argv[8]) == 1 ) {
                    stress = atoi(argv[8]);

                    if (argv[9] != 0) {
                        nIpBuffs = atoi(argv[9]);

                        if (argv[10]  != 0) {
                            nIpBufSize = atoi(argv[10]);

                            if (argv[11]  != 0) {
                                nOpBuffs = atoi(argv[11]);

                                if (argv[12]  != 0) {
                                    nOpBufSize = atoi(argv[12]);
                                }
                            }
                        }
                    }
                } else {
                    printf("Invalid stress flag (must be 1 for active or 0 for inactive)\n");
                    goto EXIT;
                }
            }
        } else {
            mpeg1layer2 = 0;
        }

        if (!strcmp(argv[6], "B24")) {
            gBitOutput = 24;
        } else if (!strcmp(argv[6], "B16")) {
            gBitOutput = 16;
        } else {
            printf("Missing or invalid bits per sample. Use B16 or B24\n");
            goto EXIT;
        }
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
    } else {
        printf("%d :: IMPROPER SETTING OF DASF/FILE MODE DASF-1 FILE MODE-0 \n", __LINE__);
        goto EXIT;
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
    } else if (!strcmp(argv[3], "TC_7")) {
        tcID = 7;
    } else if (!strcmp(argv[3], "TC_8")) {
        tcID = 8;
    } else if (!strcmp(argv[3], "TC_9")) {
        tcID = 1;
        sampleRateChange = 1;
    } else if (!strcmp(argv[3], "TC_10")) {
        tcID = 1;
        gainRamp = 1;
    } else if (!strcmp(argv[3], "TC_11")) {
        tcID = 11;
    } else if (!strcmp(argv[3], "TC_12")) {
        tcID = 12;
    } else if (!strcmp(argv[3], "TC_13")) {
        tcID = 13;
    } else {
        printf("Invalid Test Case ID: exiting..\n");
        goto EXIT;
    }

    printf("nIpBuffs = %d \n", nIpBuffs);
    printf("nIpBufSize = %d \n", nIpBufSize);
    printf("nOpBuffs = %d \n", nOpBuffs);
    printf("nOpBufSize = %d \n", nOpBufSize);

    switch (tcID) {

        case 1:

        case 7:

        case 8:
            printf ("-------------------------------------\n");
            printf ("Testing Simple PLAY till EOF \n");
            printf ("-------------------------------------\n");
            break;

        case 2:
            printf ("-------------------------------------\n");
            printf ("Testing Basic Stop \n");
            printf ("-------------------------------------\n");
            strcat(fname, "_tc2.pcm");
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

        case 12:
            printf ("------------------------------------------------\n");
            printf ("Testing List play\n");
            printf ("------------------------------------------------\n");
            strcat(fname, "_tc12.pcm");
            testcnt2 = 8;
            break;

        case 13:
            printf ("------------------------------------------------\n");
            printf ("Testing PALM requirement play\n");
            printf ("------------------------------------------------\n");
            strcat(fname, "_tc13.pcm");
            testcnt = 10;
            break;
    }

    for (j = 0; j < testcnt1; j++) {

        if (j == 1) {
            newfree(pHandle);
            fIn = fopen(argv[1], "r");

            if ( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\
                        access\n", argv[1]);
                goto EXIT;
            }

#ifndef ALSA
            if (0 == gDasfMode) {
                fOut = fopen(fname, "w");

                if ( fOut == NULL ) {
                    printf("Error:  failed to create the output file \n");
                    goto EXIT;
                }
                printf("%d :: Op File has created\n", __LINE__);
            }

#endif
        }

        if (tcID == 6) {
            printf("*********************************************************\n");
            printf ("%d :: App: Outer %d time Sending OMX_StateExecuting Command: TC6\n", __LINE__, j);
            printf("*********************************************************\n");
        }

        pHandle = newmalloc(sizeof(OMX_HANDLETYPE));

        if (NULL == pHandle) {
            printf("%d :: App: Malloc Failed\n", __LINE__);
            goto EXIT;
        }

#ifdef OMX_GETTIME
        GT_START();

        error = TIOMX_GetHandle(pHandle, strMp3Decoder, &AppData, &Mp3CaBa);

        GT_END("Call to GetHandle");

#else
        error = TIOMX_GetHandle(pHandle, strMp3Decoder, &AppData, &Mp3CaBa);

#endif
        if ((error != OMX_ErrorNone) || (*pHandle == NULL)) {
            APP_DPRINT ("Error in Get Handle function\n");
            goto EXIT;
        }

        /*add on*/
        for (l = 0; l < testcnt2; l++) {
#ifdef DSP_RENDERING_ON

            if ((mp3decfdwrite = open(FIFO1, O_WRONLY)) < 0) {
                printf("[MP3TEST] - failure to open WRITE pipe\n");
            } else {
                printf("[MP3TEST] - opened WRITE pipe\n");
            }

            if ((mp3decfdread = open(FIFO2, O_RDONLY)) < 0) {
                printf("[MP3TEST] - failure to open READ pipe\n");
                goto EXIT;
            } else {
                printf("[MP3TEST] - opened READ pipe\n");
            }

#endif
            audioinfo = newmalloc (sizeof (TI_OMX_DSP_DEFINITION));

            if (NULL == audioinfo) {
                printf("%d :: Malloc Failed\n", __LINE__);
                error = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            ArrayOfPointers[0] = (TI_OMX_DSP_DEFINITION*)audioinfo;

            audioinfo->dasfMode = gDasfMode;

            pComponent = (OMX_COMPONENTTYPE *)(*pHandle);

            pCompPrivateStruct = newmalloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));

            if (NULL == pCompPrivateStruct) {
                printf("%d :: App: Malloc Failed\n", __LINE__);
                goto EXIT;
            }

            ArrayOfPointers[1] = (OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;

            /* set playback stream mute/unmute */
            pCompPrivateStructMute = newmalloc (sizeof(OMX_AUDIO_CONFIG_MUTETYPE));

            if (pCompPrivateStructMute == NULL) {
                printf("%d :: App: Malloc Failed\n", __LINE__);
                goto EXIT;
            }

            ArrayOfPointers[2] = (OMX_AUDIO_CONFIG_MUTETYPE*)pCompPrivateStructMute;

            /* set playback stream volume */
            pCompPrivateStructVolume = newmalloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));

            if (pCompPrivateStructVolume == NULL) {
                printf("%d :: App: Malloc Failed\n", __LINE__);
                goto EXIT;
            }

            ArrayOfPointers[3] = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pCompPrivateStructVolume;

#ifndef USE_BUFFER

            for (i = 0; i < nIpBuffs; i++) {
                pInputBufferHeader[i] = newmalloc(1);
                APP_DPRINT("%d :: calling OMX_AllocateBuffer\n", __LINE__);
                error = OMX_AllocateBuffer(*pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize);

                if (error != OMX_ErrorNone) {
                    APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n", __LINE__);
                    goto EXIT;
                }
            }

            if (audioinfo->dasfMode == 0) {
                for (i = 0; i < nOpBuffs; i++) {
                    pOutputBufferHeader[i] = newmalloc(1);
                    error = OMX_AllocateBuffer(*pHandle, &pOutputBufferHeader[i], 1, NULL, nOpBufSize);

                    if (error != OMX_ErrorNone) {
                        APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n", __LINE__);
                        goto EXIT;
                    }
                }
            }

#else
            for (i = 0; i < nIpBuffs; i++) {
                OMX_MALLOC_SIZE_DSPALIGN(pInputBuffer[i], nIpBufSize, OMX_U8);

                /* allocate input buffer */
                error = OMX_UseBuffer(*pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize, pInputBuffer[i]);

                if (error != OMX_ErrorNone) {
                    APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n", __LINE__);
                    goto EXIT;
                }
            }

            if (audioinfo->dasfMode == 0) {
                for (i = 0; i < nOpBuffs; i++) {
                    OMX_MALLOC_SIZE_DSPALIGN(pOutputBuffer[i], nOpBufSize, OMX_U8);

                    /* allocate output buffer */
                    error = OMX_UseBuffer(*pHandle, &pOutputBufferHeader[i], 1, NULL, nOpBufSize, pOutputBuffer[i]);

                    if (error != OMX_ErrorNone) {
                        APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n", __LINE__);
                        goto EXIT;
                    }

                    pOutputBufferHeader[i]->nFilledLen = 0;
                }
            }

#endif

            pCompPrivateStruct->nPortIndex = 0;

            error = OMX_GetParameter (*pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);

            if (error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                printf ("%d :: Error in GetParameter\n", __LINE__);
            }

            pCompPrivateStruct->eDir = OMX_DirInput;

            pCompPrivateStruct->nBufferCountActual = nIpBuffs;
            pCompPrivateStruct->nBufferSize = nIpBufSize;
            pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingMP3;
#ifdef OMX_GETTIME
            GT_START();
            error = OMX_SetParameter (*pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
            GT_END("Set Parameter Test-SetParameter");
#else
            error = OMX_SetParameter (*pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);
#endif

            if (error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
                goto EXIT;
            }

            pCompPrivateStruct->nPortIndex = MP3D_OUTPUT_PORT;

            error = OMX_GetParameter (*pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);

            if (error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                printf ("%d :: Error in GetParameter\n", __LINE__);
            }

            pCompPrivateStruct->eDir = OMX_DirOutput;

            pCompPrivateStruct->nBufferCountActual = nOpBuffs;
            pCompPrivateStruct->nBufferSize = nOpBufSize;

            error = OMX_SetParameter (*pHandle, OMX_IndexParamPortDefinition, pCompPrivateStruct);

            if (error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
                goto EXIT;
            }

            pPcmParam = newmalloc (sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

            if (NULL == pPcmParam) {
                printf("%d :: App: Malloc Failed\n", __LINE__);
            }

            ArrayOfPointers[4] = (OMX_AUDIO_PARAM_PCMMODETYPE*)pPcmParam;

            pPcmParam->nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
            pPcmParam->nPortIndex = MP3D_OUTPUT_PORT;
            pPcmParam->nBitPerSample = gBitOutput;
            pPcmParam->bInterleaved = OMX_TRUE;

            if (!strcmp("MONO", argv[2])) {
                APP_DPRINT(" ------------- MP3: MONO Mode ---------------\n");
                pPcmParam->nChannels = MP3D_MONO_STREAM;
                gStream = MONO;
            } else if (!strcmp("STEREO", argv[2])) {
                APP_DPRINT(" ------------- MP3: STEREO Mode ---------------\n");
                gStream = STEREO;
                pPcmParam->nChannels = MP3D_STEREO_STREAM;
            } else {
                printf("%d :: Error: Invalid Audio Format..exiting.. \n", __LINE__);
                goto EXIT;
            }

            if (atoi(argv[4]) == 8000) {
                pPcmParam->nSamplingRate = 8000;
            } else if (atoi(argv[4]) == 11025) {
                pPcmParam->nSamplingRate = 11025;
            } else if (atoi(argv[4]) == 12000) {
                pPcmParam->nSamplingRate = 12000;
            } else if (atoi(argv[4]) == 16000) {
                pPcmParam->nSamplingRate = 16000;
            } else if (atoi(argv[4]) == 22050) {
                pPcmParam->nSamplingRate = 22050;
            } else if (atoi(argv[4]) == 24000) {
                pPcmParam->nSamplingRate = 24000;
            } else if (atoi(argv[4]) == 32000) {
                pPcmParam->nSamplingRate = 32000;
            } else if (atoi(argv[4]) == 44100) {
                pPcmParam->nSamplingRate = 44100;
            } else if (atoi(argv[4]) == 48000) {
                pPcmParam->nSamplingRate = 48000;
            } else if (atoi(argv[4]) == 64000) {
                pPcmParam->nSamplingRate = 64000;
            } else if (atoi(argv[4]) == 88200) {
                pPcmParam->nSamplingRate = 88200;
            } else if (atoi(argv[4]) == 96000) {
                pPcmParam->nSamplingRate = 96000;
            } else {
                printf("%d :: Sample Frequency Not Supported\n", __LINE__);
                printf("%d :: Exiting...........\n", __LINE__);
                goto EXIT;
            }

            APP_DPRINT("%d:pPcmParam->nSamplingRate:%ld\n", __LINE__, pPcmParam->nSamplingRate);

            /*For list tests*/

            if (l == 1 && tcID == 12) {
                printf("Now playing: 44_Mono_128_16.mp3\n");
                pPcmParam->nChannels = MP3D_MONO_STREAM;
                gStream = MONO;
                pPcmParam->nSamplingRate = 44100;
            } else if (l == 2 && tcID == 12) {
                printf("Now playing: 12_DC_56_16.mp3\n");
                pPcmParam->nChannels = MP3D_STEREO_STREAM;
                gStream = STEREO;
                pPcmParam->nSamplingRate = 12000;
            } else if (l == 3 && tcID == 12) {
                printf("Now playing: err_file_emp_44KHz_Stereo.mp3\n");
                pPcmParam->nChannels = MP3D_STEREO_STREAM;
                gStream = STEREO;
                pPcmParam->nSamplingRate = 44100;
            } else if (l == 4 && tcID == 12) {
                printf("Now playing: 16_DC_64_16.mp3\n");
                pPcmParam->nChannels = MP3D_STEREO_STREAM;
                gStream = STEREO;
                pPcmParam->nSamplingRate = 16000;
            } else if (l == 5 && tcID == 12) {
                printf("Now playing: 32_Mono_96_16.mp3\n");
                pPcmParam->nChannels = MP3D_MONO_STREAM;
                gStream = MONO;
                pPcmParam->nSamplingRate = 32000;
            } else if (l == 6 && tcID == 12) {
                printf("Now playing: MJ22khz32Kbps_Stereo.mp3\n");
                pPcmParam->nChannels = MP3D_STEREO_STREAM;
                gStream = STEREO;
                pPcmParam->nSamplingRate = 22050;
            } else if (l == 7 && tcID == 12) {
                printf("Now playing: 8_Mono_32_16.mp3\n");
                pPcmParam->nChannels = MP3D_MONO_STREAM;
                gStream = MONO;
                pPcmParam->nSamplingRate = 8000;
            }

            error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioPcm, pPcmParam);

            if (error != OMX_ErrorNone) {
                printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
                error = OMX_ErrorBadParameter;
                goto EXIT;
            }

            if (audioinfo->dasfMode) {
                /* Set parameters for input port (dasf) */
                pMp3Param = newmalloc (sizeof(OMX_AUDIO_PARAM_MP3TYPE));

                if (NULL == pMp3Param) {
                    printf("%d :: App: Malloc Failed\n", __LINE__);
                }

                ArrayOfPointers[5] = (OMX_AUDIO_PARAM_MP3TYPE*)pPcmParam;

                pMp3Param->nPortIndex = MP3D_INPUT_PORT;
                pMp3Param->nChannels = pPcmParam->nChannels;
                pMp3Param->nSampleRate = pPcmParam->nSamplingRate;
                error = OMX_SetParameter (*pHandle, OMX_IndexParamAudioMp3, pMp3Param);

                if (error != OMX_ErrorNone) {
                    printf ("%d :: OMX_ErrorBadParameter\n", __LINE__);
                    error = OMX_ErrorBadParameter;
                    goto EXIT;
                }
            }

#ifdef ALSA
            /* MI - end ALSA start */
            rate = atoi(argv[4]);

            setAudioParams(pPcmParam->nChannels);

            /* MI - end ALSA add */
#endif
            /* default setting for Mute/Unmute */
            pCompPrivateStructMute->nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE);

            pCompPrivateStructMute->nVersion.s.nVersionMajor    = 0xF1;

            pCompPrivateStructMute->nVersion.s.nVersionMinor    = 0xF2;

            pCompPrivateStructMute->nPortIndex                  = MP3D_INPUT_PORT;

            pCompPrivateStructMute->bMute                       = OMX_FALSE;

            /* default setting for volume */
            pCompPrivateStructVolume->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE);

            pCompPrivateStructVolume->nVersion.s.nVersionMajor  = 0xF1;

            pCompPrivateStructVolume->nVersion.s.nVersionMinor  = 0xF2;

            pCompPrivateStructVolume->nPortIndex                = MP3D_INPUT_PORT;

            pCompPrivateStructVolume->bLinear                   = OMX_FALSE;

            pCompPrivateStructVolume->sVolume.nValue            = 50;               /* actual volume */

            pCompPrivateStructVolume->sVolume.nMin              = 0;                /* min volume */

            pCompPrivateStructVolume->sVolume.nMax              = 100;              /* max volume */

            if (audioinfo->dasfMode) {
                SampleRate = pMp3Param->nSampleRate;
                APP_DPRINT("%d :: MP3 DECODER RUNNING UNDER DASF MODE\n", __LINE__);
            } else if (audioinfo->dasfMode == 0) {
                APP_DPRINT("%d :: MP3 DECODER RUNNING UNDER FILE MODE\n", __LINE__);
            }

            error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.mp3headerinfo", &index);

            if (error != OMX_ErrorNone) {
                printf("Error getting extension index\n");
                goto EXIT;
            }

#ifdef AM_APP
            cmd_data.hComponent = *pHandle;

            cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;

            cmd_data.streamID = 0;

            /* for decoder, using AM_CommandIsInputStreamAvailable */
            cmd_data.param1 = 0;

            /*add on*/
#ifdef DSP_RENDERING_ON
            if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
                printf("%d ::Mp3DecTest.c ::[MP3 Dec Component] - send command to audio manager\n", __LINE__);
            }

            if ((read(mp3decfdread, &cmd_data, sizeof(cmd_data))) < 0) {
                printf("%d ::Mp3DecTest.c ::[MP3 Dec Component] - failure to get data from the audio manager\n", __LINE__);
                goto EXIT;
            }

#endif
            audioinfo->streamId = cmd_data.streamID;

            /* Set frame mode operation */
            audioinfo->framemode = framemode;

            /*framemode = audioinfo->framemode*/;

            audioinfo->mpeg1_layer2 = mpeg1layer2;

            error = OMX_SetConfig (*pHandle, index, audioinfo);

            if (error != OMX_ErrorNone) {
                error = OMX_ErrorBadParameter;
                APP_DPRINT("%d :: Error from OMX_SetConfig() function\n", __LINE__);
                goto EXIT;
            }

            if (audioinfo->dasfMode) {
                printf("***************StreamId=%d******************\n", (int)audioinfo->streamId);

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

                error = OMX_GetExtensionIndex(*pHandle, "OMX.TI.index.config.mp3.datapath", &index);

                if (error != OMX_ErrorNone) {
                    printf("Error getting extension index\n");
                    goto EXIT;
                }

                error = OMX_SetConfig (*pHandle, index, &dataPath);

                if (error != OMX_ErrorNone) {
                    error = OMX_ErrorBadParameter;
                    APP_DPRINT("%d :: Mp3DecTest.c :: Error from OMX_SetConfig() function\n", __LINE__);
                    goto EXIT;
                }
            }

#endif
#ifdef OMX_GETTIME
            GT_START();

#endif
            error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

            if (error != OMX_ErrorNone) {
                APP_DPRINT ("Error from SendCommand-Idle(Init) State function - error = %d\n", error);
                goto EXIT;
            }

            APP_DPRINT("%d :: About to call WaitForState to change to idle\n", __LINE__);

            error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

            if (error != OMX_ErrorNone) {
                APP_DPRINT( "Error:  Mp3->WaitForState reports an error %X\n", error);
                goto EXIT;
            }


            for (i = 0; i < testcnt; i++) {
                close(IpBuf_Pipe[0]);
                close(IpBuf_Pipe[1]);
                close(OpBuf_Pipe[0]);
                close(OpBuf_Pipe[1]);
                close(Event_Pipe[0]);
                close(Event_Pipe[1]);

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

                if ( retval != 0) {
                    printf( "Error:Empty Data Pipe failed to open\n");
                    goto EXIT;
                }

                nFrameCount = 1;

                /*for list test*/

                if (l == 1 && tcID == 12) {
                    fIn = fopen("./patterns/44_Mono_128_16.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else if (l == 2 && tcID == 12) {
                    fIn = fopen("./patterns/12_DC_56_16.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else if (l == 3 && tcID == 12) {
                    fIn = fopen("./patterns/err_file_emp_44KHz_Stereo.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else if (l == 4 && tcID == 12) {
                    fIn = fopen("./patterns/16_DC_64_16.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else if (l == 5 && tcID == 12) {
                    fIn = fopen("./patterns/32_Mono_96_16.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else if (l == 6 && tcID == 12) {
                    fIn = fopen("./patterns/MJ22khz32Kbps_Stereo.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else if (l == 7 && tcID == 12) {
                    fIn = fopen("./patterns/8_Mono_32_16.mp3", "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                } else {
                    fIn = fopen(argv[1], "r");

                    if (fIn == NULL) {
                        fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                        goto EXIT;
                    }
                }

#ifndef ALSA
                if (0 == audioinfo->dasfMode) {

                    if (tcID == 13) {
                        fOut = fopen(fname, "a");
                    } else {
                        fOut = fopen(fname, "w");
                    }

                    if (fOut == NULL) {
                        fprintf(stderr, "Error:  failed to create the output file \n");
                        goto EXIT;
                    }

                    APP_DPRINT("%d :: Op File has created\n", __LINE__);
                }

#endif
                if (tcID == 5 || tcID == 13) {
                    printf("*********************************************************\n");
                    printf ("App: %d time Sending OMX_StateExecuting Command: TC 5\n", i);
                    printf("*********************************************************\n");
                }

                if (tcID == 13) {
                    if (i == 0) {
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

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
                    error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

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

#ifdef AM_APP
                if (sampleRateChange) {
                    cmd_data.hComponent = *pHandle;
                    cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;
                    cmd_data.param1 = 44100;
                    /*add on*/
#ifdef DSP_RENDERING_ON

                    if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
                        printf("send command to audio manager\n");
                    }

#endif
                } else if (dataPath > DATAPATH_MIMO_0 ) {

                    cmd_data.hComponent = *pHandle;
                    cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;
                    cmd_data.param1 = dataPath == DATAPATH_MIMO_1 ? 8000 : 16000;
                    cmd_data.param2 = 1;

                    if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
                        printf("send command to audio manager\n");
                    }
                }

#endif
                for (k = 0; k < nIpBuffs; k++) {
                    pInputBufferHeader[k]->nFlags = 0;
#ifdef OMX_GETTIME

                    if (k == 0) {
                        GT_FlagE = 1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                        GT_START(); /* Empty Bufffer */
                    }

#endif
                    error = send_input_buffer (*pHandle, pInputBufferHeader[k], fIn);
                }

                if (audioinfo->dasfMode == 0) {
                    for (k = 0; k < nOpBuffs; k++) {
#ifdef OMX_GETTIME

                        if (k == 0) {
                            GT_FlagF = 1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                            GT_START(); /* Fill Buffer */
                        }

#endif
                        APP_DPRINT("%d :: App: Calling FillThisBuffer\n", __LINE__);

                        OMX_FillThisBuffer(*pHandle, pOutputBufferHeader[k]);
                    }
                }

#ifdef WAITFORRESOURCES
                /*while( 1 ){
                  if(( (error == OMX_ErrorNone) && (gState != OMX_StateIdle)) && (gState != OMX_StateInvalid)&& (timeToExit!=1)){*/
#else
                while (( (error == OMX_ErrorNone) && (gState != OMX_StateIdle)) && (gState != OMX_StateInvalid) ) {
                    if (1) {
#endif
                FD_ZERO(&rfds);
                FD_SET(IpBuf_Pipe[0], &rfds);
                FD_SET(OpBuf_Pipe[0], &rfds);
                FD_SET(Event_Pipe[0], &rfds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                frmCount++;

                if (gainRamp) {
                    if (frmCount == 2) {
#ifdef AM_APP
                        cmd_data.hComponent = *pHandle;

                        ptGainEnvelope = malloc(sizeof(MDN_GAIN_ENVELOPE));
                        ptGainEnvelope->ulSize = sizeof(MDN_GAIN_ENVELOPE);
                        ptGainEnvelope->bDspReleased = 0;
                        ptGainEnvelope->ulNumGainSlopes_Left = 40;
                        ptGainEnvelope->ulNumGainSlopes_Right = 0;

                        for (i = 0; i < 40; i++) {
                            ptGainEnvelope->tGainSlopes[i].ulTargetGain =  (0x8000 * (39 - i)) / 40;
                            ptGainEnvelope->tGainSlopes[i].ulDurationMS = 100;
                        }

                        printf("Sending Gain Ramp\n");

                        cmd_data.AM_Cmd = AM_CommandGainRamp;
                        cmd_data.param1 = (unsigned long)(ptGainEnvelope);
                        /*add on*/
#ifdef DSP_RENDERING_ON

                        if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
                            printf("%d ::[Modem App] - send command to audio manager\n", __LINE__);
                        }

#endif
                        if ((fdgrwrite = open(GRFIFO1, O_WRONLY)) < 0) {
                            printf("[Modem App] - failure to open GRWRITE pipe\n");
                        } else {
                            printf("[Modem App] - opened GRWRITE pipe\n");
                        }

                        if ((fdgrread = open(GRFIFO2, O_RDONLY)) < 0) {
                            printf("[Modem App] - failure to open GRREAD pipe\n");
                        } else {
                            printf("[Modem App] - opened GRREAD pipe\n");
                        }

                        if ((write(fdgrwrite, ptGainEnvelope, sizeof(MDN_GAIN_ENVELOPE))) < 0) {
                            printf("%d ::[Modem App] - send command to audio manager\n", __LINE__);
                        }

#endif
                    }
                }

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

                    case 12:

                    case 13:

                        if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                            /*if ((frmCount >= nNextFlushFrame) && (num_flush < 3000)){
                                printf("Flush #%d\n", num_flush++);
                                nFlushDone = 0;

                                error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
                                if(error != OMX_ErrorNone) {
                                    fprintf (stderr,"Error from SendCommand-Pasue State function\n");
                                    goto EXIT;
                                }
                                error = WaitForState(pHandle, OMX_StatePause);
                                if(error != OMX_ErrorNone) {
                                    fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                }
                                error = OMX_SendCommand(*pHandle, OMX_CommandFlush, 0, NULL);
                                pthread_mutex_lock(&WaitForINFlush_mutex);
                                pthread_cond_wait(&WaitForINFlush_threshold,
                                                  &WaitForINFlush_mutex);
                                pthread_mutex_unlock(&WaitForINFlush_mutex);
                                nFlushDone = 0;

                                error = OMX_SendCommand(*pHandle, OMX_CommandFlush, 1, NULL);
                                pthread_mutex_lock(&WaitForOUTFlush_mutex);
                                pthread_cond_wait(&WaitForOUTFlush_threshold,
                                                  &WaitForOUTFlush_mutex);
                                pthread_mutex_unlock(&WaitForOUTFlush_mutex);
                                nFlushDone = 0;

                                error = OMX_SendCommand(*pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
                                if(error != OMX_ErrorNone) {
                                    fprintf (stderr,"Error from SendCommand-Executing State function\n");
                                    goto EXIT;
                                }
                                error = WaitForState(pHandle, OMX_StateExecuting);
                                if(error != OMX_ErrorNone) {
                                    fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                }
                                
                                fseek(fIn, 0, SEEK_SET);
                                frmCount = 0;
                                nNextFlushFrame = 5 + 50 * ((rand() * 1.0) / RAND_MAX);
                                printf("nNextFlushFrame d= %d\n", nNextFlushFrame);
                            }*/
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            error = send_input_buffer (*pHandle, pBuffer, fIn);

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

                            if (frmCnt == 2) {
                                APP_DPRINT("Shutting down ---------- \n");
#ifdef OMX_GETTIME
                                GT_START();
#endif
                                error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                                if (error != OMX_ErrorNone) {
                                    fprintf (stderr, "Error from SendCommand-Idle(Stop) State function\n");
                                    goto EXIT;
                                }
                            } else {
                                error = send_input_buffer (*pHandle, pBuffer, fIn);

                                if (error != OMX_ErrorNone) {
                                    printf ("Error While reading input pipe\n");
                                    goto EXIT;
                                }
                            }

                            if (frmCnt == 2 && tcID == 4) {
                                printf ("*********** Waiting for state to change to Idle ************\n\n");
                                error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
                                GT_END("Call to SendCommand <OMX_StateIdle>");
#endif

                                if (error != OMX_ErrorNone) {
                                    fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                }

                                printf ("*********** State Changed to Idle ************\n\n");

                                printf("Component Has Stopped here and waiting for %d seconds before it plays further\n", SLEEP_TIME);
                                sleep(SLEEP_TIME);

                                printf ("*************** Execute command to Component *******************\n");
#ifdef OMX_GETTIME
                                GT_START();
#endif
                                error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

                                if (error != OMX_ErrorNone) {
                                    fprintf (stderr, "Error from SendCommand-Executing State function\n");
                                    goto EXIT;
                                }

                                printf ("*********** Waiting for state to change to Executing ************\n\n");

                                error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
                                GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif

                                if (error != OMX_ErrorNone) {
                                    fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                                    goto EXIT;
                                }

                                printf ("*********** State Changed to Executing ************\n\n");

                                if (gDasfMode == 0) {
                                    /*rewind input and output files*/
                                    APP_DPRINT("%d :: Rewinding Input and Output files\n", __LINE__);
                                    fseek(fIn, 0L, SEEK_SET);
#ifndef ASLA
                                    fseek(fOut, 0L, SEEK_SET);
#endif
                                }


                                for (k = 0; k < nIpBuffs; k++) {
                                    /* memset(pInputBufferHeader[k],0,nIpBufSize); */
                                    pInputBufferHeader[k]->nFlags = 0;
                                    //error = send_input_buffer (*pHandle, pInputBufferHeader[k], fIn);
                                }
                            }

                            frmCnt++;
                        }
                        break;

                    case 3:

                        if (frmCount == 10 || frmCount == 20) {
                            printf ("\n\n*************** Pause command to Component *******************\n");
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);

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
                                fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                                goto EXIT;
                            }

                            printf ("*********** State Changed to Pause ************\n\n");

                            printf("Sleeping for %d secs....\n\n", SLEEP_TIME);
                            sleep(SLEEP_TIME);

                            printf ("*************** Resume command to Component *******************\n");
#ifdef OMX_GETTIME
                            GT_START();
#endif
                            error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

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
                                fprintf(stderr, "Error:  hMp3Decoder->WaitForState reports an error %X\n", error);
                                goto EXIT;
                            }

                            printf ("*********** State Changed to Resume ************\n\n");
                        }

                        if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            error = send_input_buffer (*pHandle, pBuffer, fIn);

                            if (error != OMX_ErrorNone) {
                                printf ("Error While reading input pipe\n");
                                goto EXIT;
                            }

                            frmCnt++;
                        }
                        break;

                    case 7:
                        /* test mute/unmute playback stream */

                        if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;

                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            pBuffer->nFlags = 0;
                            error = send_input_buffer (*pHandle, pBuffer, fIn);

                            if (error != OMX_ErrorNone) {
                                goto EXIT;
                            }

                            frmCnt++;
                        }

                        if (frmCnt == 2) {
                            printf("************Mute the playback stream*****************\n");
                            pCompPrivateStructMute->bMute = OMX_TRUE;
                            error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute);

                            if (error != OMX_ErrorNone) {
                                error = OMX_ErrorBadParameter;
                                goto EXIT;
                            }
                        }

                        if (frmCnt == 3) {
                            printf("************Unmute the playback stream*****************\n");
                            pCompPrivateStructMute->bMute = OMX_FALSE;
                            error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute);

                            if (error != OMX_ErrorNone) {
                                error = OMX_ErrorBadParameter;
                                goto EXIT;
                            }
                        }
                        break;

                    case 8:
                        /* test set volume for playback stream */

                        if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;

                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                            pBuffer->nFlags = 0;
                            error = send_input_buffer (*pHandle, pBuffer, fIn);

                            if (error != OMX_ErrorNone) {
                                goto EXIT;
                            }

                            frmCnt++;
                        }

                        if (frmCnt == 2) {
                            printf("************Set stream volume to high*****************\n");
                            pCompPrivateStructVolume->sVolume.nValue = 0x8000;
                            error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume);

                            if (error != OMX_ErrorNone) {
                                error = OMX_ErrorBadParameter;
                                goto EXIT;
                            }
                        }


                        if (frmCnt == 4) {
                            printf("************Set stream volume to low*****************\n");
                            pCompPrivateStructVolume->sVolume.nValue = 0x1000;
                            error = OMX_SetConfig(*pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume);

                            if (error != OMX_ErrorNone) {
                                error = OMX_ErrorBadParameter;
                                goto EXIT;
                            }
                        }
                        break;

                    case 11:

                        if (FD_ISSET(IpBuf_Pipe[0], &rfds)) {
                            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
                            read(IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));

                            if (frmCnt == 1 || frmCnt == 2 || frmCnt == 3 || frmCnt == 4) {
                                printf("Middle of frame %d\n", frmCnt);
                                pBuffer->pBuffer += 20;
                                error = send_input_buffer (*pHandle, pBuffer, fIn);

                                if (error != OMX_ErrorNone) {
                                    goto EXIT;
                                }
                            } else {
                                error = send_input_buffer (*pHandle, pBuffer, fIn);

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

#if 0
                    fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
                    printf("%d :: App: Filled Len = %ld\n", __LINE__, pBuf->nFilledLen);
#endif

#if 1

                    if (pBuf->nFilledLen == 0) {
                        APP_DPRINT("%d :: 0 FilledLen Output Buffer\n", __LINE__);
                    } else {
#ifdef ALSA
                        err = snd_pcm_writei(playback_handle, pBuf->pBuffer, pBuf->nFilledLen / (2 * pPcmParam->nChannels));

                        if (err == -EPIPE) {    /* EPIPE means underrun */
                            underrunCount++;
                            printf("Underrun occurred\n");
                            snd_pcm_prepare(playback_handle);
                            break;
                        } else if (err < 0) {
                            printf("Error from writei: %s\n", snd_strerror(err));
                            break;
                        }  else if (err != (int)(pBuf->nFilledLen / (2*pPcmParam->nChannels))) {
                            shortWrites++;
                            printf("Short write, write %d frames\n", err);
                        }

                        totalWrites++;

#else
                        fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
#endif
                        APP_DPRINT("%d :: wrote %p buffer, len %d  to file =  %ld\n", __LINE__, pBuf, pBuf->nFilledLen);
                    }

#endif
#ifndef ALSA
                    fflush(fOut);

#endif
                    OMX_FillThisBuffer(*pHandle, pBuf);
                }

                if (playcompleted) {
                    if (tcID == 13 && (i != 9)) {
                        APP_DPRINT("RING_TONE: Lets play again!");
                        playcompleted = 0;
                        goto my_exit;
                    } else {
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

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

                        for (i = 0; i < nIpBuffs; i++) {
                            error = OMX_FreeBuffer(*pHandle, MP3D_INPUT_PORT, pInputBufferHeader[i]);

                            if ( (error != OMX_ErrorNone)) {
                                APP_DPRINT ("Error in Free Handle function\n");
                            }
                        }

                        for (i = 0; i < nOpBuffs; i++) {
                            error = OMX_FreeBuffer(*pHandle, MP3D_OUTPUT_PORT, pOutputBufferHeader[i]);

                            if ( (error != OMX_ErrorNone)) {
                                APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                            }
                        }

#ifdef USE_BUFFER
                        /* newfree the App Allocated Buffers */
                        APP_DPRINT("%d :: Freeing the App Allocated Buffers in TestApp\n", __LINE__);
                        for (i = 0; i < nIpBuffs; i++) {
                            APP_DPRINT("%d::: [TESTAPPFREE] pInputBuffer[%d] = %p\n", __LINE__, i, pInputBuffer[i]);

                            OMX_MEMFREE_STRUCT_DSPALIGN(pInputBuffer[i], OMX_U8);
                        }
                        for (i = 0; i < nOpBuffs; i++) {

                            APP_DPRINT("%d::: [TESTAPPFREE] pOutputBuffer[%d] = %p\n", __LINE__, i, pOutputBuffer[i]);

                            OMX_MEMFREE_STRUCT_DSPALIGN(pOutputBuffer[i], OMX_U8);
                        }
#endif

                        OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

                        WaitForState(pHandle, OMX_StateLoaded);

                        OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateWaitForResources, NULL);

                        WaitForState(pHandle, OMX_StateWaitForResources);
                    } else if (pipeContents == 1) {
                        printf("Test app received OMX_ErrorResourcesAcquired\n");

                        OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

                        for (i = 0; i < nIpBuffs; i++) {
                            pInputBufferHeader[i] = newmalloc(1);
                            /* allocate input buffer */
                            error = OMX_AllocateBuffer(*pHandle, &pInputBufferHeader[i], 0, NULL, nIpBufSize); /*To have enought space for    */

                            if (error != OMX_ErrorNone) {
                                APP_DPRINT("%d :: Error returned by OMX_AllocateBuffer()\n", __LINE__);
                            }
                        }

                        WaitForState(pHandle, OMX_StateIdle);

                        OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                        WaitForState(pHandle, OMX_StateExecuting);
                        rewind(fIn);

                        for (i = 0; i < nIpBuffs;i++) {
                            send_input_buffer(*pHandle, pInputBufferHeader[i], fIn);
                        }
                    }

                    if (pipeContents == 2) {
#ifdef OMX_GETTIME
                        GT_START();
#endif
                        error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);

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

                        for (i = 0; i < nIpBuffs; i++) {
                            error = OMX_FreeBuffer(*pHandle, MP3D_INPUT_PORT, pInputBufferHeader[i]);

                            if ( (error != OMX_ErrorNone)) {
                                APP_DPRINT ("%d :: Mp3DecTest.c :: Error in Free Handle function\n", __LINE__);
                            }
                        }

                        for (i = 0; i < nOpBuffs; i++) {
                            error = OMX_FreeBuffer(*pHandle, MP3D_OUTPUT_PORT, pOutputBufferHeader[i]);

                            if ( (error != OMX_ErrorNone)) {
                                APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                            }
                        }

#if 1
                        error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);

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
        lastinputbuffersent = 0;

        if (0 == audioinfo->dasfMode) {
#ifndef ALSA
            fclose(fOut);
#endif
        }

        fclose(fIn);

        if (i != (testcnt - 1)) {
            if ((tcID == 5) || (tcID == 2)) {
                printf("%d :: sleeping here for %d secs\n", __LINE__, SLEEP_TIME);
                sleep (SLEEP_TIME);
            } else {
                sleep (0);
            }
        }
    } /*inner loop ends here*/

SHUTDOWN:
#ifdef AM_APP
    cmd_data.hComponent = *pHandle;

    cmd_data.AM_Cmd = AM_Exit;

    /*add on*/
#endif
#ifdef DSP_RENDERING_ON
    if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0)
        printf("%d[MP3 Dec Component] - send command to audio manager\n", __LINE__);

    close(mp3decfdwrite);

    close(mp3decfdread);

#ifdef ALSA
    printf("\nTotalWrites: %d\n", totalWrites);

    printf("Underruns: %d\n", underrunCount);

    printf("Short Writes: %d\n", shortWrites);

    /* MI: start ALSA add */
    snd_pcm_drain(playback_handle);

    snd_pcm_close(playback_handle);

    /* MI: end ALSA add */
#endif
#endif
    for (i = 0; i < nIpBuffs; i++) {
        APP_DPRINT("%d :: App: Freeing %p IP BufHeader\n", __LINE__, pInputBufferHeader[i]);
        error = OMX_FreeBuffer(*pHandle, MP3D_INPUT_PORT, pInputBufferHeader[i]);

        if ((error != OMX_ErrorNone)) {
            APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
            goto EXIT;
        }
    }

    if (audioinfo->dasfMode == 0) {
        for (i = 0; i < nOpBuffs; i++) {
            APP_DPRINT("%d :: App: Freeing %p OP BufHeader\n", __LINE__, pOutputBufferHeader[i]);
            error = OMX_FreeBuffer(*pHandle, MP3D_OUTPUT_PORT, pOutputBufferHeader[i]);

            if ((error != OMX_ErrorNone)) {
                APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
                goto EXIT;
            }
        }
    }

    APP_DPRINT ("Sending the StateLoaded Command\n");

#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif

    if (error != OMX_ErrorNone) {
        APP_DPRINT ("%d:: Error from SendCommand-Idle State function\n", __LINE__);
        goto EXIT;
    }
}

#ifdef WAITFORRESOURCES
error = OMX_SendCommand(*pHandle, OMX_CommandStateSet, OMX_StateWaitForResources, NULL);

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

#ifndef ALSA

if (audioinfo->dasfMode == 0 && j == 0) {
    printf("**********************************************************\n");
    printf("NOTE: An output file %s has been created in file system\n", APP_OUTPUT_FILE);
    printf("**********************************************************\n");
}

#endif

if (NULL != audioinfo) {
    newfree(audioinfo);
}

if (NULL != pCompPrivateStruct) {
    newfree(pCompPrivateStruct);
}

if (NULL != pPcmParam) {
    newfree(pPcmParam);
}

if (NULL != pMp3Param) {
    newfree(pMp3Param);
}

error = TIOMX_FreeHandle(*pHandle);

if ( (error != OMX_ErrorNone)) {
    APP_DPRINT ("%d:: Error in Free Handle function\n", __LINE__);
    goto EXIT;
}

APP_DPRINT ("%d:: Free Handle returned Successfully \n\n\n\n", __LINE__);
}/*outhern loop ends here*/

pthread_mutex_destroy(&WaitForState_mutex);

pthread_cond_destroy(&WaitForState_threshold);

pthread_mutex_destroy(&WaitForOUTFlush_mutex);

pthread_cond_destroy(&WaitForOUTFlush_threshold);

pthread_mutex_destroy(&WaitForINFlush_mutex);

pthread_cond_destroy(&WaitForINFlush_threshold);

EXIT:
#if AM_APP
if (sampleRateChange) {
    cmd_data.hComponent = *pHandle;
    cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;
    cmd_data.param1 = 48000;
    /*add on*/

#ifdef DSP_RENDERING_ON

    if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
        printf("send command to audio manager\n");
    }

#endif
}

if (ptGainEnvelope) {
    free(ptGainEnvelope);
    ptGainEnvelope = NULL;
}

#endif
if (bInvalidState == OMX_TRUE) {
#ifndef USE_BUFFER
    error = FreeAllResources(*pHandle,
                             pInputBufferHeader[0],
                             pOutputBufferHeader[0],
                             nIpBuffs,
                             nOpBuffs,
                             fIn, fOut);
#else
    error = FreeAllUseResources(*pHandle,
                                pInputBuffer,
                                pOutputBuffer,
                                nIpBuffs,
                                nOpBuffs,
                                fIn, fOut);
#endif
} else {

    if (NULL != pHandle)
        newfree(pHandle);

    if (NULL != pCompPrivateStructMute)
        newfree(pCompPrivateStructMute);

    if (NULL != pCompPrivateStructVolume)
        newfree(pCompPrivateStructVolume);


#ifdef USE_BUFFER
    for (i = 0; i < nIpBuffs; i++)
        OMX_MEMFREE_STRUCT_DSPALIGN(pInputBuffer[i], OMX_U8);
    if (!gDasfMode) {
        for (i = 0; i < nOpBuffs; i++)
            OMX_MEMFREE_STRUCT_DSPALIGN(pOutputBuffer[i], OMX_U8);
    }

#endif
}

#ifdef APP_MEMDEBUG
printf("\n-Printing memory not deleted-\n");

for (r = 0;r < 500;r++) {
    if (lines[r] != 0) {
        printf(" --->%d Bytes allocated on %p File:%s Line: %d\n", bytes[r], arr[r], file[r], lines[r]);
    }
}

#endif

#ifdef OMX_GETTIME
GT_END("MP3_DEC test <End>");

OMX_ListDestroy(pListHead);

#endif
return error;
       }


       /* ================================================================================= * */
       /**
       * @fn send_input_buffer() Sends the input buffer to the component.
       *
       * @param pHandle This is component handle allocated by the OMX core.
       *
       * @param pBuffer This is the buffer pointer.
       *
       * @fIn This is input file handle.
       *
       * @pre          None
       *
       * @post         None
       *
       *  @return      Appropriate OMX error.
       *
       *  @see         None
       */
       /* ================================================================================ * */
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn) {
    OMX_ERRORTYPE error = OMX_ErrorNone;
    int nRead = 0;
    int framelen[4] = {0};
    unsigned long frameLength = 0;

    if (!framemode) {
        nRead = fill_data (pBuffer, fIn);
        APP_DPRINT("%d : APP:: Entering send_input_buffer \n", __LINE__);
        /*Don't send more buffers after OMX_BUFFERFLAG_EOS*/

        if (lastinputbuffersent) {
            /*lastinputbuffersent = 0;*/
            pBuffer->nFlags = 0;

            return error;
        }

        if (nRead < pBuffer->nAllocLen) {
            pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
            lastinputbuffersent = 1;
        } else {
            pBuffer->nFlags = 0;
        }

        /*time stamp & tick count value*/
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
        nRead = fread(framelen, 1, 4, fIn);
        frameLength = framelen[0] +
                      ((long) (framelen[1]) << 8) +
                      ((long) (framelen[2]) << 16) +
                      ((long) (framelen[3]) << 24);
        APP_DPRINT("length: %d\n", nRead);

        if (nRead > 0) {
            /* Copy frame to buffer */
            fread(pBuffer->pBuffer, 1, frameLength, fIn);
            pBuffer->nFlags = 0;
            /*add on: Tickcount value & prints*/
            pBuffer->nFilledLen = frameLength;
            pBuffer->nTimeStamp = rand() % 100;
            pBuffer->nTickCount = rand() % 70;
            TIME_PRINT("TimeStamp Input: %lld\n", pBuffer->nTimeStamp);
            TICK_PRINT("TickCount Input: %ld\n", pBuffer->nTickCount);

            if (!preempted) {
                error = OMX_EmptyThisBuffer(pHandle, pBuffer);

                if (error == OMX_ErrorIncorrectStateOperation)
                    error = 0;
            }

            return error;
        } else {
            if (!lastinputbuffersent) {
                pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
                pBuffer->nFilledLen = 0;
                /*add on: Tickcount value & prints*/
                printf("Marking eos \n");
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
                /*lastinputbuffersent = 0;*/
                return error;
            }
        }
    }

    return error;
}


/* ================================================================================= * */
/**
* @fn fill_data() Reads the data from the input file.
*
* @param pBuffer This is the buffer pointer.
*
* @fIn This is input file handle.
*
* @pre          None
*
* @post         None
*
*  @return      Number of bytes that have been read.
*
*  @see         None
*/
/* ================================================================================ * */
int fill_data (OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn) {
    int nRead = 0;
    static int totalRead = 0;
    static int fileHdrReadFlag = 0;
    static int ccnt = 0;

    if (!fileHdrReadFlag) {
        fileHdrReadFlag = 1;
    }

    nRead = fread(pBuf->pBuffer, 1, pBuf->nAllocLen , fIn);

    APP_DPRINT("\n*****************************************************\n");
    APP_DPRINT ("%d :: App:: pBuf : %p pBuf->pBuffer = %p pBuf->nAllocLen = * %ld, nRead = %ld\n",
                __LINE__, pBuf, pBuf->pBuffer, pBuf->nAllocLen, nRead);
    APP_DPRINT("\n*****************************************************\n");

    totalRead += nRead;
    pBuf->nFilledLen = nRead;
    ccnt++;
    return nRead;
}

#ifdef AM_APP
void SendGainRamp() {
    int i = 0;
    ADN_DCTN_PARAM* pDctnParam = NULL;
    AM_COMMANDDATATYPE cmd_data;

    OMX_MALLOC_SIZE_DSPALIGN(pDctnParamBuffer, 4096*sizeof(unsigned char), (unsigned char));
    pDctnParam = (ADN_DCTN_PARAM*)(pDctnParamBuffer);

    pDctnParam->ulInPortMask  = 0x0;
    pDctnParam->ulOutPortMask = 0x2;
    ptGainEnvelope = (MDN_GAIN_ENVELOPE *)(pDctnParam->data);
    ptGainEnvelope->ulSize = sizeof(MDN_GAIN_ENVELOPE);
    ptGainEnvelope->bDspReleased = 0;
    ptGainEnvelope->ulNumGainSlopes_Left = 40;
    ptGainEnvelope->ulNumGainSlopes_Right = 0;

    for (i = 0; i < 40; i++) {
        ptGainEnvelope->tGainSlopes[i].ulTargetGain =  (0x8000 * (39 - i)) / 40;
        ptGainEnvelope->tGainSlopes[i].ulDurationMS = 100;
    }

    printf("Sending Gain Ramp\n");

    cmd_data.AM_Cmd = AM_CommandRTGainRamp;
    cmd_data.param1 = (unsigned long)(pDctnParam);
#ifdef DSP_RENDERING_ON

    if ((write(mp3decfdwrite, &cmd_data, sizeof(cmd_data))) < 0) {
        printf("%d ::[Modem App] - send command to audio manager\n", __LINE__);
    }

#endif
    if ((fdgrwrite = open(GRFIFO1, O_WRONLY)) < 0) {
        printf("[Modem App] - failure to open GRWRITE pipe\n");
    } else {
        printf("[Modem App] - opened GRWRITE pipe\n");
    }

    if ((fdgrread = open(GRFIFO2, O_RDONLY)) < 0) {
        printf("[Modem App] - failure to open GRREAD pipe\n");
    } else {
        printf("[Modem App] - opened GRREAD pipe\n");
    }

    if ((write(fdgrwrite, pDctnParam, sizeof(ADN_DCTN_PARAM))) < 0) {
        printf("%d ::[Modem App] - send command to audio manager\n", __LINE__);
    }

}

#endif

#ifdef APP_MEMDEBUG
void * mymalloc(int line, char *s, int size) {
    void *p = NULL;
    int e = 0;
    p = malloc(size);

    if (p == NULL) {
        printf("Memory not available\n");
        exit(1);
    } else {
        while ((lines[e] != 0) && (e < 500) ) {
            e++;
        }

        arr[e] = p;

        lines[e] = line;
        bytes[e] = size;
        strcpy(file[e], s);
        printf("Allocating %d bytes on address %p, line %d file %s pos %d\n", size, p, line, s, e);
        return p;
    }
}

int myfree(void *dp, int line, char *s) {
    int q = 0;

    for (q = 0;q < 500;q++) {
        if (arr[q] == dp) {
            printf("Deleting %d bytes on address %p, line %d file %s\n", bytes[q], dp, line, s);
            free(dp);
            dp = NULL;
            lines[q] = 0;
            strcpy(file[q], "");
            break;
        }
    }

    if (500 == q)
        printf("\n\nPointer not found. Line:%d    File%s!!\n\n", line, s);

    return 0;
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
    OMX_U16 i;

    for (i = 0; i < NIB; i++) {
        printf("%d :: APP: About to free pInputBufferHeader[%d]\n", __LINE__, i);
        eError = OMX_FreeBuffer(pHandle, 0, pBufferIn + i);
    }

    for (i = 0; i < NOB; i++) {
        printf("%d :: APP: About to free pOutputBufferHeader[%d]\n", __LINE__, i);
        eError = OMX_FreeBuffer(pHandle, 1, pBufferOut + i);
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
#ifndef ALSA

    if (fOut != NULL) {
        fclose(fOut);
        fOut = NULL;
    }

#endif
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
    OMX_U16 i = 0;
    printf("%d::Freeing all resources by state invalid \n", __LINE__);
    /* free the UseBuffers */

    for (i = 0; i < NIB; i++) {
        OMX_MEMFREE_STRUCT_DSPALIGN(UseInpBuf[i], OMX_U8);
    }

    for (i = 0; i < NOB; i++) {
        OMX_MEMFREE_STRUCT_DSPALIGN(UseOutBuf[i], OMX_U8);
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
#ifndef ALSA

    if (fOut != NULL) {
        fclose(fOut);
        fOut = NULL;
    }

#endif
    if (fIn != NULL) {
        fclose(fIn);
        fIn = NULL;
    }

    TIOMX_FreeHandle(pHandle);

    return eError;

}

#endif
