
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
#undef  WAITFORRESOURCES


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
/*#include <TIDspOmx.h>*/
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Audio.h>

#include <pthread.h>
#include <stdio.h>
#include <linux/soundcard.h>
/*#include <AudioManagerAPI.h>*/
#include <time.h>
#ifdef OMX_GETTIME
#include <OMX_Common_Utils.h>
#include <OMX_GetTime.h>     /*Headers for Performance & measuremet    */
#endif

OMX_S16 numInputBuffers = 0;
OMX_S16 numOutputBuffers = 0;
#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[10] = {NULL};
    OMX_U8* pOutputBuffer[10] ={NULL};
#endif

OMX_BUFFERHEADERTYPE* pInputBufferHeader[10] = {NULL};
OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10] = {NULL};
OMX_BOOL IsEFRSet = OMX_FALSE;
FILE *fIn=NULL;
int timeToExit=0;
int preempted=0;
/**flush*/
int num_flush = 0;
int nNextFlushFrame = 100;
/***/

FILE *fpRes = NULL;
/* ======================================================================= */
/**
 * @def  OMX_AMRDEC_NonMIME             Define a value for NonMIME processing
 */
/* ======================================================================= */
#define OMX_AMRDEC_NonMIME 1

/* ======================================================================= */
/**
 * @def  MIME_HEADER_LEN                Define a Buffer Header Size
 */
/* ======================================================================= */
#define MIME_HEADER_LEN 6
#define FIFO1 "/dev/fifo.1"
#define FIFO2 "/dev/fifo.2"

#undef APP_DEBUG
/*#define APP_DEBUG*/
#undef APP_MEMCHECK

/* ======================================================================= */
/**
 * @def  DASF                           Define a Value for DASF mode 
 */
/* ======================================================================= */
#define DASF 1

/*#define USE_BUFFER*/
#undef USE_BUFFER

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

/* Borrowed from http://www.dtek.chalmers.se/groups/dvd/dist/oss_audio.c */
/* AFMT_AC3 is really IEC61937 / IEC60958, mpeg/ac3/dts over spdif */
#ifndef AFMT_AC3
/* ======================================================================= */
/**
  * @def    AFMT_AC3                             Dolby Digital AC3
 */
/* ======================================================================= */
#define AFMT_AC3        0x00000400
#endif
#ifndef AFMT_S32_LE
/* ======================================================================= */
/**
  * @def    AFMT_S32_LE                 32/24-bits, in 24bit use the msbs
 */
/* ======================================================================= */
#define AFMT_S32_LE     0x00001000  
#endif
#ifndef AFMT_S32_BE
/* ======================================================================= */
/**
  * @def    AFMT_S32_BE                 32/24-bits, in 24bit use the msbs
 */
/* ======================================================================= */
#define AFMT_S32_BE     0x00002000
#endif

/* ======================================================================= */
/**
  * @def  GAIN                      Define a GAIN value for Configure Audio
 */
/* ======================================================================= */
#define GAIN 95

/* ======================================================================= */
/**
 * @def    STANDARD_INPUT_BUFFER_SIZE             Standart Input Buffer Size
 *                                                (1 frame)
 */
/* ======================================================================= */
#define STD_NBAMRDEC_BUF_SIZE 118
#define INPUT_NBAMRDEC_BUFFER_SIZE ((IsEFRSet) ? 120 : 118)

/* ======================================================================= */
/**
 * @def    OUTPUT_NBAMRDEC_BUFFER_SIZE           Standart Output Buffer Size
 */
/* ======================================================================= */
#define OUTPUT_NBAMRDEC_BUFFER_SIZE 320

/* ======================================================================= */
/**
 * @def    OUTPUT_NBAMRDEC_BUFFER_SIZE           Standart Output Buffer Size
 */
/* ======================================================================= */
#define OUTPUT_NBAMRDEC_BUFFER_SIZE 320

/* ======================================================================= */
/**
 * @def    INPUT_NBAMRDEC_BUFFER_SIZE_MIME       Mime Input Buffer Size
 */
/* ======================================================================= */
#define INPUT_NBAMRDEC_BUFFER_SIZE_MIME 34

/* ======================================================================= */
/**
 * @def    NBAMRDEC_SAMPLING_FREQUENCY          Sampling Frequency   
 */
/* ======================================================================= */
#define NBAMRDEC_SAMPLING_FREQUENCY 48000

/* ======================================================================= */
/**
 * @def    EXTRA_BUFFBYTES                Num of Extra Bytes to be allocated
 */
/* ======================================================================= */
#define EXTRA_BUFFBYTES (256)

/* ======================================================================= */
/**
 * @def    APP_DEBUGMEM    This Macro turns On the logic to detec memory
 *                         leaks on the App. To debug the component, 
 *                         NBAMRDEC_DEBUGMEM must be defined.
 */
/* ======================================================================= */
#undef APP_DEBUGMEM
/*#define APP_DEBUGMEM*/

/* ======================================================================= */
/** NBAMRDEC_BUFDATA       
*
*  @param  nFrames           # of Frames processed on the Output Buffer.
*
*/
/*  ==================================================================== */
typedef struct NBAMRDEC_BUFDATA {
   OMX_U8 nFrames;     
}NBAMRDEC_BUFDATA;

/* ======================================================================= */
/** MIME_Settings       
*
*  @param  MIME_NO_SUPPORT                    MIME mode is not supported
*
*  @param  MIME_SUPPORTED                    MIME mode is supported
*/
/*  ==================================================================== */
typedef enum
{
    MIME_NO_SUPPORT = 0,
    MIME_SUPPORTED
}MIME_Settings;

/* ======================================================================= */
/** NBAMRDEC_MimeMode  Mime Mode
*
*  @param  NBAMRDEC_FORMATCONFORMANCE        Mime Mode and IF2 Off
*
*  @param  NBAMRDEC_MIMEMODE                Mime Mode On
*/
/*  ==================================================================== */
/*enum NBAMRDEC_MimeMode {
    NBAMRDEC_FORMATCONFORMANCE,
    NBAMRDEC_MIMEMODE, 
    NBAMRDEC_IF2
};*/

#ifdef OMX_GETTIME
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  int GT_FlagE = 0;  /* Fill Buffer 1 = First Buffer,  0 = Not First Buffer  */
  int GT_FlagF = 0;  /*Empty Buffer  1 = First Buffer,  0 = Not First Buffer  */
  static OMX_NODE* pListHead = NULL;
#endif

#ifdef APP_DEBUGMEM
void *arr[500] = {NULL};
int lines[500] = {0};
int bytes[500] = {0};
char file[500][50] = {""};
int ind=0;

#define newmalloc(x) mymalloc(__LINE__,__FILE__,x)
#define newfree(z) myfree(z,__LINE__,__FILE__)

void * mymalloc(int line, char *s, int size);
int myfree(void *dp, int line, char *s);

#else

#define newmalloc(x) malloc(x)
#define newfree(z) free(z)

#endif

static OMX_BOOL bInvalidState;
void* ArrayOfPointers[6] = {NULL};

OMX_S16 maxint(OMX_S16 a, OMX_S16 b);

OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle);
OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle);

/*OMX_S16 gMimeFlag = 0;*/ /*Antes*/
OMX_S16 AmrFrameFormat = 0; /*IF2 change*/
OMX_S16 gStateNotifi = 0;
OMX_S16 gState = 0;
OMX_S16 inputPortDisabled = 0;
OMX_S16 outputPortDisabled = 0;
OMX_S16 alternate = 0;
OMX_S16 numRead = 0;

OMX_S16 testCaseNo = 0;

OMX_U8 NextBuffer[STD_NBAMRDEC_BUF_SIZE*3] = {0};
OMX_U8 TempBuffer[STD_NBAMRDEC_BUF_SIZE] = {0};

pthread_mutex_t WaitForState_mutex;
pthread_cond_t  WaitForState_threshold;
OMX_U8          WaitForState_flag = 0;
OMX_U8        TargetedState = 0;
/**flush*/
pthread_mutex_t WaitForOUTFlush_mutex;
pthread_cond_t  WaitForOUTFlush_threshold;
pthread_mutex_t WaitForINFlush_mutex;
pthread_cond_t  WaitForINFlush_threshold;
/*****/
OMX_S16 fileHdrReadFlag = 0;

int FirstTime = 1;
int nRead=0;
/*AM_COMMANDDATATYPE cmd_data;*/
int MimoOutput = -1;
int  teemode=0;

OMX_S16 fill_data (OMX_U8 *pBuf, int mode, FILE *fIn);

#ifndef USE_BUFFER
int FreeAllResources( OMX_HANDLETYPE *pHandle,
                            OMX_BUFFERHEADERTYPE* pBufferIn,
                            OMX_BUFFERHEADERTYPE* pBufferOut,
                            int NIB, int NOB,
                            FILE* fIn, FILE* fOut);
#else
int  FreeAllResources(OMX_HANDLETYPE *pHandle,
                          OMX_U8* UseInpBuf[],
                          OMX_U8* UseOutBuf[],             
                          int NIB, int NOB,
                          FILE* fIn, FILE* fOut);
#endif    

typedef enum COMPONENTS {
    COMP_1,
    COMP_2
}COMPONENTS;

void ConfigureAudio();

OMX_STRING strAmrEncoder = "OMX.TI.AMR.decode";
int nbamrIpBuf_Pipe[2] = {0};
int nbamrOpBuf_Pipe[2] = {0};
int nbamrEvent_Pipe[2] = {0};


fd_set rfds;
OMX_S16 dasfmode = 0;

/* safe routine to get the maximum of 2 integers */
OMX_S16 maxint(OMX_S16 a, OMX_S16 b)
{
         return (a>b) ? a : b;
}

OMX_ERRORTYPE send_input_buffer (OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn);
/* This method will wait for the component to get to the state
 * specified by the DesiredState input. */
static OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE* pHandle,
                                  OMX_STATETYPE DesiredState)
{
     OMX_STATETYPE CurState = OMX_StateInvalid;
     OMX_ERRORTYPE eError = OMX_ErrorNone;
     OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
     if (bInvalidState == OMX_TRUE)
     {
         eError = OMX_ErrorInvalidState;
         return eError;
     }
     eError = pComponent->GetState(pHandle, &CurState);
     if(CurState != DesiredState){
            WaitForState_flag = 1;
            TargetedState = DesiredState;
            pthread_mutex_lock(&WaitForState_mutex); 
            pthread_cond_wait(&WaitForState_threshold, &WaitForState_mutex);/*Going to sleep till signal arrives*/
            pthread_mutex_unlock(&WaitForState_mutex);                 
     }
     return eError;      
}


OMX_ERRORTYPE EventHandler(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData)
{

   OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)hComponent;
   OMX_STATETYPE state = OMX_StateInvalid;
   OMX_ERRORTYPE eError = OMX_ErrorNone;
   OMX_U8 writeValue = 0;
   
#ifdef APP_DEBUG
   int iComp = *((int *)(pAppData));
#endif
   eError = pComponent->GetState (hComponent, &state);
   if(eError != OMX_ErrorNone) {
       APP_DPRINT("%d :: AmrDecTest.c :: Error returned from GetState\n",__LINE__);
   }

   switch (eEvent) {
    case OMX_EventResourcesAcquired:
            writeValue = 1;
            write(nbamrEvent_Pipe[1], &writeValue, sizeof(OMX_U8));
            preempted=0;

       break;

       case OMX_EventCmdComplete:
             /**flush*/
            if (nData1 == OMX_CommandFlush){    
                if(nData2 == 0){
                    pthread_mutex_lock(&WaitForINFlush_mutex);
                    pthread_cond_signal(&WaitForINFlush_threshold);
                    pthread_mutex_unlock(&WaitForINFlush_mutex);
                }
                if(nData2 == 1){
                    pthread_mutex_lock(&WaitForOUTFlush_mutex);
                    pthread_cond_signal(&WaitForOUTFlush_threshold);
                    pthread_mutex_unlock(&WaitForOUTFlush_mutex);
                }
            }
        /****/
           APP_DPRINT ( "%d :: AmrDecTest.c :: Component State Changed To %d\n", __LINE__,state);
        if (nData1 == OMX_CommandPortDisable) {
            if (nData2 == OMX_DirInput) {
                inputPortDisabled = 1;
            }
            if (nData2 == OMX_DirOutput) {
                outputPortDisabled = 1;
            }
        }
        if ((nData1 == OMX_CommandStateSet) && (TargetedState == nData2) && (WaitForState_flag)){
                        WaitForState_flag = 0;
                        pthread_mutex_lock(&WaitForState_mutex);
                        pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                        pthread_mutex_unlock(&WaitForState_mutex);
                }
           break;

    case OMX_EventBufferFlag:
    if((nData1 == (OMX_U32)NULL) && (nData2 == (OMX_U32)OMX_BUFFERFLAG_EOS)){
            writeValue = 2;  
            write(nbamrEvent_Pipe[1], &writeValue, sizeof(OMX_U8));
    }
        break;
       case OMX_EventError:
           if (nData1 == OMX_ErrorInvalidState) {
                   bInvalidState =OMX_TRUE;
                   if (WaitForState_flag == 1){
                       WaitForState_flag = 0;
                       pthread_mutex_lock(&WaitForState_mutex);
                       pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                       pthread_mutex_unlock(&WaitForState_mutex);
                   }
          }            
           else if(nData1 == OMX_ErrorResourcesPreempted) {
              preempted=1;
              writeValue = 0;  
              write(nbamrEvent_Pipe[1], &writeValue, sizeof(OMX_U8));
           }
           else if (nData1 == OMX_ErrorResourcesLost) { 
                    WaitForState_flag = 0;
                    pthread_mutex_lock(&WaitForState_mutex);
                    pthread_cond_signal(&WaitForState_threshold);/*Sending Waking Up Signal*/
                    pthread_mutex_unlock(&WaitForState_mutex);
           }
     
           break;
       case OMX_EventMax:
           break;
       case OMX_EventMark:
           break;
           
        break;

       default:
           break;
   }
    return eError;
}

void FillBufferDone (OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    APP_DPRINT ("%d :: AmrDecTest.c :: OUTPUT BUFFER = %p && %p\n",__LINE__,pBuffer, pBuffer->pBuffer);
    APP_DPRINT ("%d :: AmrDecTest.c :: pBuffer->nFilledLen = %ld\n",__LINE__,pBuffer->nFilledLen);
/*    OutputFrames = pBuffer->pOutputPortPrivate;
    printf("Receiving %d Frames\n",OutputFrames->nFrames);*/

    write(nbamrOpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    if (GT_FlagF == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
        GT_END("Call to FillBufferDone  <First: FillBufferDone>");
        GT_FlagF = 0 ;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}


void EmptyBufferDone(OMX_HANDLETYPE hComponent, OMX_PTR ptr, OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_S16 ret = 0;
     APP_DPRINT ("%d :: AmrDecTest.c :: INPUT BUFFER = %p && %p\n",__LINE__,pBuffer, pBuffer->pBuffer);
    if (!preempted) 
    ret = write(nbamrIpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
#ifdef OMX_GETTIME
    if (GT_FlagE == 1 ) /* First Buffer Reply*/  /* 1 = First Buffer,  0 = Not First Buffer  */
    {
      GT_END("Call to EmptyBufferDone <First: EmptyBufferDone>");
      GT_FlagE = 0;   /* 1 = First Buffer,  0 = Not First Buffer  */
    }
#endif
}


FILE *inputToSN;

int main(int argc, char* argv[])
{
    OMX_CALLBACKTYPE AmrCaBa = {(void *)EventHandler,
                (void*)EmptyBufferDone,
                                (void*)FillBufferDone};
    OMX_HANDLETYPE pHandle;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    OMX_U32 AppData = 100;
    OMX_PARAM_PORTDEFINITIONTYPE* pCompPrivateStruct = NULL; 
    OMX_AUDIO_PARAM_AMRTYPE *pAmrParam = NULL; 
    OMX_COMPONENTTYPE *pComponent= (OMX_COMPONENTTYPE *)pHandle;
    OMX_COMPONENTTYPE *pComponent_dasf = NULL;     
    OMX_STATETYPE state = 0;
    /* TODO: Set a max number of buffers */
    /*OMX_BUFFERHEADERTYPE* pInputBufferHeader[10];*/
    /* TODO: Set a max number of buffers */
    /*OMX_BUFFERHEADERTYPE* pOutputBufferHeader[10];*/
    OMX_BUFFERHEADERTYPE* pBuf = NULL;
    OMX_U8 count=0;
    /*OMX_BOOL IsEFRSet = OMX_FALSE;*/
    bInvalidState=OMX_FALSE;
    /*TI_OMX_DATAPATH dataPath;*/
 
#ifdef DSP_RENDERING_ON    
    int nbamrdecfdwrite = 0;
    int nbamrdecfdread = 0;
#endif

/*#ifdef USE_BUFFER
    OMX_U8* pInputBuffer[10];
    OMX_U8* pOutputBuffer[10];
#endif*/
    

    /*TI_OMX_DSP_DEFINITION *audioinfo = NULL;    

    audioinfo = newmalloc(sizeof(TI_OMX_DSP_DEFINITION));
    TI_OMX_STREAM_INFO *streaminfo = NULL;
    streaminfo = newmalloc(sizeof(TI_OMX_STREAM_INFO));*/
    
    /*OMX_S16 numInputBuffers = 0;*/
    /*OMX_S16 numOutputBuffers = 0;*/
    struct timeval tv;
    OMX_S16 retval = 0, i = 0, j = 0,k = 0;
    OMX_S16 frmCount = 0;
    OMX_S16 testcnt = 1;
    OMX_S16 testcnt1 = 1;
    OMX_INDEXTYPE index = 0;  
    OMX_U32    streamId = 0;
     
    /* Open the file of data to be rendered. */
    FILE *fIn=NULL;
    FILE *fOut=NULL; 
    
    OMX_AUDIO_CONFIG_MUTETYPE* pCompPrivateStructMute = NULL; 
    OMX_AUDIO_CONFIG_VOLUMETYPE* pCompPrivateStructVolume = NULL;     

    /*ArrayOfPointers[0]=(TI_OMX_STREAM_INFO*)streaminfo;
    ArrayOfPointers[1]=(TI_OMX_DSP_DEFINITION*)audioinfo;*/

    pthread_mutex_init(&WaitForState_mutex, NULL);
    pthread_cond_init (&WaitForState_threshold, NULL);
    WaitForState_flag = 0;
    /**flush*/
    pthread_mutex_init(&WaitForOUTFlush_mutex, NULL);
    pthread_cond_init (&WaitForOUTFlush_threshold, NULL);
    pthread_mutex_init(&WaitForINFlush_mutex, NULL);
    pthread_cond_init (&WaitForINFlush_threshold, NULL);
    /*****/

    APP_DPRINT("------------------------------------------------------\n");
    APP_DPRINT("This is Main Thread In NBAMR DECODER Test Application:\n");
    APP_DPRINT("Test Core 1.5 - " __DATE__ ":" __TIME__ "\n");
    APP_DPRINT("------------------------------------------------------\n");



#ifdef OMX_GETTIME
    printf("Line %d\n",__LINE__);
      GTeError = OMX_ListCreate(&pListHead);
        printf("Line %d\n",__LINE__);
      printf("eError = %d\n",GTeError);
      GT_START();
  printf("Line %d\n",__LINE__);
#endif

    /* check the input parameters */
    if((argc < 11) || (argc > 13))
    {
        printf( "Usage:  test infile [outfile]\n");
        goto EXIT;
    }

 APP_DPRINT("\n Argv 0  [TestApp]                  %s \n", argv[0]);
 APP_DPRINT("Argv 1  [IP File]                  %s \n", argv[1]);
 APP_DPRINT("Argv 2  [OP File]                  %s \n", argv[2]);
 APP_DPRINT("Argv 3  [ MIME/NONMINE IF2 ]       %s \n", argv[3]);
 APP_DPRINT("Argv 4  [EFR/DTXOFF]", argv[4]);    
 APP_DPRINT("Argv 5  [AMR Band MOde             %s \n", argv[5]);
 APP_DPRINT("Argv 6  [ TEST CASE]      %s \n", argv[6]);
 APP_DPRINT("Argv 7  [DASF MODE]                %s \n", argv[7]);
 APP_DPRINT("Argv 8  [TEEDN /ACDN]              %s \n", argv[8]);
 APP_DPRINT("Argv 9  [IpBuffers]                %s \n", argv[9]);
 APP_DPRINT("Argv 10 [OpBuffers]                %s \n", argv[10]);
 APP_DPRINT("Argv 11 [Test case Count           %s \n", argv[11]);
 APP_DPRINT("Argv 12 [TEEMODE]           %s \n", argv[12]);
 


    numInputBuffers = atoi(argv[9]);
    numOutputBuffers = atoi(argv[10]);

    if ( (numInputBuffers < 1) || (numInputBuffers >4)){
        APP_DPRINT ("%d :: App: ERROR: No. of input buffers not valid (0-4) \n",__LINE__);
        goto EXIT;
    }

    if ( (numOutputBuffers< 1) || (numOutputBuffers>4)){
        APP_DPRINT ("%d :: App: ERROR: No. of output buffers not valid (0-4) \n",__LINE__);
        goto EXIT;
    }
    

    /* check to see that the input file exists */
    struct stat sb = {0};
    OMX_S16 status = stat(argv[1], &sb);
    if( status != 0 ) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Cannot find file %s. (%u)\n",__LINE__, argv[1], errno);
        goto EXIT;
    }

    fIn = fopen(argv[1], "r");
    if( fIn == NULL ) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Error:  failed to open the file %s for readonly\access\n",__LINE__, argv[1]);
        goto EXIT;
    }
       
    fOut = fopen(argv[2], "w");
    if( fOut == NULL ) {
            APP_DPRINT( "%d :: AmrDecTest.c :: Error:  failed to create the output file %s\n",__LINE__, argv[2]);
            goto EXIT;
        }

    /* Create a pipe used to queue data from the callback. */
     retval = pipe(nbamrIpBuf_Pipe);
    if( retval != 0) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Error:Fill Data Pipe failed to open\n",__LINE__);
        goto EXIT;
    }

    retval = pipe(nbamrOpBuf_Pipe);
    if( retval != 0) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Error:Empty Data Pipe failed to open\n",__LINE__);
        goto EXIT;
    }
    
    retval = pipe(nbamrEvent_Pipe);
    if( retval != 0) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Error:Empty Data Pipe failed to open\n",__LINE__);
        goto EXIT;
    }

    /* save off the "max" of the handles for the selct statement */
     OMX_S16 fdmax = maxint(nbamrIpBuf_Pipe[0], nbamrOpBuf_Pipe[0]);
    fdmax = maxint(fdmax,nbamrEvent_Pipe[0]);

    APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
    error = TIOMX_Init();
    APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
    if(error != OMX_ErrorNone) {
        APP_DPRINT("%d :: AmrDecTest.c :: Error returned by OMX_Init()\n",__LINE__);
        goto EXIT;
    }

    int command = atoi(argv[6]);
    APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
    switch (command ) {
         case 1:
               printf ("-------------------------------------\n");
               printf ("Testing Simple PLAY till EOF \n");
               printf ("-------------------------------------\n");
               break;
         case 2:
               printf ("-------------------------------------\n");
               printf ("Testing Stop and Play \n");
               printf ("-------------------------------------\n");
               testcnt = 2;
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
               if (argc == 12)
               {
                    testcnt = atoi(argv[11]);
               }
               else
               {
                    testcnt = 20;  /*20 cycles by default*/
               }
               break;
         case 6:
               printf ("------------------------------------------------\n");
               printf ("Testing Repeated PLAY with Deleting Component\n");
               printf ("------------------------------------------------\n");
               if (argc == 12)
               {
                   testcnt1 = atoi(argv[11]);
               }
               else
               {
                    testcnt1 = 20;  /*20 cycles by default*/
               }
               break;
         case 7:
               printf ("----------------------------------------------------------\n");
               printf ("Testing Multiframe with each buffer size = 2 x frameLength\n");
               printf ("----------------------------------------------------------\n");
               testCaseNo = 7;
               break;
         case 8:
               printf ("------------------------------------------------------------\n");
               printf ("Testing Multiframe with each buffer size = 1/2 x frameLength\n");
               printf ("------------------------------------------------------------\n");
               testCaseNo = 8;
               break;
         case 9:
               printf ("------------------------------------------------------------\n");
               printf ("Testing Multiframe with alternating buffer sizes\n");
               printf ("------------------------------------------------------------\n");
               testCaseNo = 9;
               break;
         case 10:
               printf ("------------------------------------------------------------\n");
               printf ("Testing Mute/Unmute for Playback Stream\n");
               printf ("------------------------------------------------------------\n");
               break;
         case 11:
               printf ("------------------------------------------------------------\n");
               printf ("Testing Set Volume for Playback Stream\n");
               printf ("------------------------------------------------------------\n");
               break;
         case 12:
                     printf ("------------------------------------------------------------\n");
                     printf ("Testing Frame Lost Suport\n");
                     printf ("------------------------------------------------------------\n");
                     break;
         case 13:
                     printf ("------------------------------------------------------------\n");
                     printf ("Testing Ringtone test\n");
                     printf ("------------------------------------------------------------\n");
               testcnt = 10;
                     break;
    }

    APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
    dasfmode = atoi(argv[7]);
    switch (dasfmode)
    {
        case 2:
            MimoOutput=0;
            dasfmode=1;
            break;
        case 3:
            MimoOutput=1;
            dasfmode=1;
            break;
        case 4:
            MimoOutput=2;
            dasfmode=1;
            break;
    case 5: /**TEE mode and MIMO**/
            MimoOutput=3;
            dasfmode=1;
            break;
    case 6: /**TEE mode and MIMO**/
            MimoOutput=4;
            dasfmode=1;
            break;
        default:
            break;
    }




    APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
    for(j = 0; j < testcnt1; j++) {

    #ifdef DSP_RENDERING_ON

    if((nbamrdecfdwrite=open(FIFO1,O_WRONLY))<0) {
        printf("[AMRTEST] - failure to open WRITE pipe\n");
    }
    else {
        printf("[AMRTEST] - opened WRITE pipe\n");
    }

    if((nbamrdecfdread=open(FIFO2,O_RDONLY))<0) {
        printf("[AMRTEST] - failure to open READ pipe\n");
    }
    else {
        printf("[AMRTEST] - opened READ pipe\n");
    }

    #endif
    
        if(j >= 1) {
            printf ("Decoding the file for %d Time\n",j+1);
            close(nbamrIpBuf_Pipe[0]);
            close(nbamrIpBuf_Pipe[1]);
            close(nbamrOpBuf_Pipe[0]);
            close(nbamrOpBuf_Pipe[1]);
            close(nbamrEvent_Pipe[0]);
            close(nbamrEvent_Pipe[1]);
            /* Create a pipe used to queue data from the callback. */
            retval = pipe( nbamrIpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "%d :: AmrDecTest.c :: Error:Fill Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }

            retval = pipe( nbamrOpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "%d :: AmrDecTest.c ::Error:Empty Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }
            
            retval = pipe( nbamrEvent_Pipe);
            if( retval != 0) {
                APP_DPRINT( "%d :: AmrDecTest.c ::Error:Empty Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }
            fIn = fopen(argv[1], "r");
            if( fIn == NULL ) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly\
                                                                   access\n", argv[1]);
                goto EXIT;
            }

            fOut = fopen(argv[2], "a");
            if( fOut == NULL ) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
            error = TIOMX_Init();
            inputToSN = fopen("outputSecondTime.log","w");

        }
        else {
            inputToSN = fopen("outputFirstTime.log","w");
        }

        /* Load the NBAMR Encoder Component */
        APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
    #ifdef OMX_GETTIME
        GT_START();
        error = OMX_GetHandle(&pHandle, strAmrEncoder, &AppData, &AmrCaBa);
        GT_END("Call to GetHandle");
    #else 
        error = TIOMX_GetHandle(&pHandle, strAmrEncoder, &AppData, &AmrCaBa);
    #endif
        APP_DPRINT("%d :: AmrDecTest.c :: \n",__LINE__);
        if((error != OMX_ErrorNone) || (pHandle == NULL)) {
            APP_DPRINT ("%d :: AmrDecTest.c :: Error in Get Handle function\n",__LINE__);
            goto EXIT;
        }

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    pCompPrivateStruct = newmalloc (sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    if (pCompPrivateStruct == NULL) { 
        printf("Malloc failed\n");
        error = -1;
        goto EXIT;
    }
    
    ArrayOfPointers[2]=(OMX_PARAM_PORTDEFINITIONTYPE*)pCompPrivateStruct;

    /* set playback stream mute/unmute */ 
    pCompPrivateStructMute = newmalloc (sizeof(OMX_AUDIO_CONFIG_MUTETYPE)); 
    if(pCompPrivateStructMute == NULL) { 
         printf("%d :: AmrDecTest.c :: App: Malloc Failed\n",__LINE__);
         goto EXIT; 
     } 

    ArrayOfPointers[3] = (OMX_AUDIO_CONFIG_MUTETYPE*)pCompPrivateStructMute;
    
    pCompPrivateStructVolume = newmalloc (sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE)); 
     if(pCompPrivateStructVolume == NULL) { 
         printf("%d :: AmrDecTest.c :: App: Malloc Failed\n",__LINE__); 
         goto EXIT; 
     } 
     ArrayOfPointers[4] = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pCompPrivateStructVolume;

    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pCompPrivateStruct);

    APP_DPRINT("%d :: AmrTest\n",__LINE__);
    pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor = 0xF1; 
    pCompPrivateStruct->nVersion.s.nVersionMinor = 0xF2; 
    pCompPrivateStruct->nPortIndex = OMX_DirInput; 
    error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);


        /* Send input port config */
        if(!(strcmp(argv[3],"MIME"))) 
        {
             AmrFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
        }
        else if(!(strcmp(argv[3],"NONMIME"))) 
        {
            AmrFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
        }
        else if(!(strcmp(argv[3],"IF2"))) 
        {
            AmrFrameFormat = OMX_AUDIO_AMRFrameFormatIF2;
        } 
        else 
        {
            printf("Enter proper Frame Format Type MIME, NON MIME or IF2\nExit\n");
            goto EXIT;
        }

        /*if(!(strcmp(argv[3],"NONMIME"))) {
            gMimeFlag = 1;
         }*/
         
    pCompPrivateStruct->eDir = OMX_DirInput; 
    pCompPrivateStruct->nPortIndex = OMX_DirInput; 
    pCompPrivateStruct->nBufferCountActual = numInputBuffers; 
    pCompPrivateStruct->nBufferSize = INPUT_NBAMRDEC_BUFFER_SIZE; 
    pCompPrivateStruct->format.audio.eEncoding = OMX_AUDIO_CodingAMR; 
#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                           pCompPrivateStruct);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                           pCompPrivateStruct);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    pCompPrivateStruct->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pCompPrivateStruct->nVersion.s.nVersionMajor = 0xF1; 
    pCompPrivateStruct->nVersion.s.nVersionMinor = 0xF2; 
    pCompPrivateStruct->nPortIndex = OMX_DirOutput; 
    error = OMX_GetParameter (pHandle,OMX_IndexParamPortDefinition,pCompPrivateStruct);

    /* Send output port config */
    pCompPrivateStruct->eDir = OMX_DirOutput; 
    pCompPrivateStruct->nBufferCountActual = numOutputBuffers; 
    pCompPrivateStruct->nBufferSize = OUTPUT_NBAMRDEC_BUFFER_SIZE; 

    if(dasfmode) {
    pCompPrivateStruct->nBufferCountActual = 0;
    }
#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                           pCompPrivateStruct);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamPortDefinition,
                                           pCompPrivateStruct);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }
    
    /* default setting for Mute/Unmute */ 
     pCompPrivateStructMute->nSize = sizeof (OMX_AUDIO_CONFIG_MUTETYPE); 
     pCompPrivateStructMute->nVersion.s.nVersionMajor    = 0xF1; 
     pCompPrivateStructMute->nVersion.s.nVersionMinor    = 0xF2; 
     pCompPrivateStructMute->nPortIndex                  = OMX_DirInput; 
     pCompPrivateStructMute->bMute                       = OMX_FALSE; 
   
     /* default setting for volume */ 
     pCompPrivateStructVolume->nSize = sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE); 
     pCompPrivateStructVolume->nVersion.s.nVersionMajor  = 0xF1; 
     pCompPrivateStructVolume->nVersion.s.nVersionMinor  = 0xF2; 
     pCompPrivateStructVolume->nPortIndex                = OMX_DirInput; 
     pCompPrivateStructVolume->bLinear                   = OMX_FALSE; 
     pCompPrivateStructVolume->sVolume.nValue            = 50;               /* actual volume */ 
     pCompPrivateStructVolume->sVolume.nMin              = 0;                /* min volume */ 
     pCompPrivateStructVolume->sVolume.nMax              = 100;              /* max volume */ 

     pAmrParam = newmalloc (sizeof (OMX_AUDIO_PARAM_AMRTYPE));     
      ArrayOfPointers[5] = (OMX_AUDIO_PARAM_AMRTYPE *)pAmrParam;
     if(!(strcmp(argv[4],"EFR"))) {
        /**< DTX as EFR instead of AMR standard  */
        pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXasEFR;
        IsEFRSet = OMX_TRUE;
        APP_DPRINT("%d :: AmrDecTest.c :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,
                                                    argv[4]);
    }

#ifndef USE_BUFFER

    
    for (i=0; i < numInputBuffers; i++) {
        /* allocate input buffer */
        APP_DPRINT("%d :: About to call OMX_AllocateBuffer\n",__LINE__);
        error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader[i],0,NULL,INPUT_NBAMRDEC_BUFFER_SIZE*3); /*To have enought space for    */
        APP_DPRINT("%d :: AmrDecTest.c :: called OMX_AllocateBuffer for Input Buffers\n",__LINE__);     /*handling two frames by buffer*/ 
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: AmrDecTest.c :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
            goto EXIT;
        }

    }

    for (i=0; i < numOutputBuffers; i++) {
        /* allocate output buffer */
        APP_DPRINT("%d :: AmrDecTest.c :: About to call OMX_AllocateBuffer\n",__LINE__);
        if ((testCaseNo != 7) && (testCaseNo != 9)){
              error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_NBAMRDEC_BUFFER_SIZE*1);
           }
       else{
              error = OMX_AllocateBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_NBAMRDEC_BUFFER_SIZE*2);
            }
        APP_DPRINT("%d :: AmrDecTest.c :: called OMX_AllocateBuffer for Output Buffers\n",__LINE__);
        if(error != OMX_ErrorNone) {
            APP_DPRINT("%d :: AmrDecTest.c :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
            goto EXIT;
        }
    }

#else

    APP_DPRINT("%d :: AmrDecTest.c :: About to call OMX_UseBuffer\n",__LINE__);

    for (i=0; i < numInputBuffers; i++)
    {
        pInputBuffer[i] = (OMX_U8*)newmalloc(INPUT_NBAMRDEC_BUFFER_SIZE*3 + EXTRA_BUFFBYTES);/*To have enought space for    */
        APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pInputBuffer[i]);                   /*handling two frames by buffer*/
        pInputBuffer[i] = pInputBuffer[i] + DSP_CACHE_ALIGNMENT;

        /* allocate input buffer */
        APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
        error = OMX_UseBuffer(pHandle,&pInputBufferHeader[i],0,NULL,INPUT_NBAMRDEC_BUFFER_SIZE*3,pInputBuffer[i]);
        APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
        if(error != OMX_ErrorNone)
        {
            APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
            goto EXIT;
        }

    }

    for ( i = 0 ; i < numOutputBuffers ; i++ )
    {
        if ((testCaseNo != 7) && (testCaseNo != 9)){
            pOutputBuffer[i] = (OMX_U8*)newmalloc (OUTPUT_NBAMRDEC_BUFFER_SIZE*1 + EXTRA_BUFFBYTES);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer);
            pOutputBuffer[i] = pOutputBuffer[i] + DSP_CACHE_ALIGNMENT;

            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            error = OMX_UseBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_NBAMRDEC_BUFFER_SIZE*1,pOutputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone){
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }
        }
        else{
            pOutputBuffer[i] = (OMX_U8*)newmalloc (OUTPUT_NBAMRDEC_BUFFER_SIZE*2 + EXTRA_BUFFBYTES);
            APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pOutputBuffer);
            pOutputBuffer[i] = pOutputBuffer[i] + DSP_CACHE_ALIGNMENT;

            /* allocate output buffer */
            APP_DPRINT("%d :: About to call OMX_UseBuffer\n",__LINE__);
            error = OMX_UseBuffer(pHandle,&pOutputBufferHeader[i],1,NULL,OUTPUT_NBAMRDEC_BUFFER_SIZE*2,pOutputBuffer[i]);
            APP_DPRINT("%d :: called OMX_UseBuffer\n",__LINE__);
            if(error != OMX_ErrorNone){
                APP_DPRINT("%d :: Error returned by OMX_UseBuffer()\n",__LINE__);
                goto EXIT;
            }                
        }

    }
#endif    
    APP_MEMPRINT("%d:::[TESTAPPALLOC] %p\n",__LINE__,pAmrParam);
    pAmrParam->nSize = sizeof (OMX_AUDIO_PARAM_AMRTYPE);
    pAmrParam->nVersion.s.nVersionMajor = 0xF1;
    pAmrParam->nVersion.s.nVersionMinor = 0xF2;
    pAmrParam->nPortIndex = OMX_DirInput;
    error = OMX_GetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);    
    pAmrParam->nChannels = 1;
    pAmrParam->nBitRate = 8000; 
    pAmrParam->eAMRBandMode = atoi(argv[5]);
    APP_DPRINT("\n%d :: App: pAmrParam->eAMRBandMode --> %d \n",__LINE__,
                                                    pAmrParam->eAMRBandMode);

    if(!(strcmp(argv[4],"DTXON"))) 
    {
        /**< AMR Discontinuous Transmission Mode is enabled  */
        pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnVAD1;
        APP_DPRINT("\n%d :: App: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,
                                                    argv[4]);
    }
    if(!(strcmp(argv[4],"DTXOFF"))) 
    {
        /**< AMR Discontinuous Transmission Mode is disabled */
        pAmrParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
        APP_DPRINT("%d :: AmrDecTest.c :: pAmrParam->eAMRDTXMode --> %s \n",__LINE__,argv[4]);
    }
    
    pAmrParam->eAMRFrameFormat = AmrFrameFormat;
    /*error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);*/

    /*(gMimeFlag == 0)
    { MIME MODE
                  pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
        }
        else if(gMimeFlag == 1){  NON MIME MODE
                pAmrParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;
        }
        else {
            printf("Enter proper MIME mode\n");
            printf("MIME:0\n");
            printf("NON MIME:1\n");
            goto EXIT;
        }*/
#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    pAmrParam->nPortIndex = OMX_DirOutput;
    pAmrParam->nChannels = 1;
    pAmrParam->nBitRate = 8000;   /*8000 Hz bit rate*/
#ifdef OMX_GETTIME
    GT_START();
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);
    GT_END("Set Parameter Test-SetParameter");
#else
    error = OMX_SetParameter (pHandle,OMX_IndexParamAudioAmr,pAmrParam);
#endif
    if (error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        printf ("%d:: OMX_ErrorBadParameter\n",__LINE__);
        goto EXIT;
    }

    pComponent_dasf = (OMX_COMPONENTTYPE *)pHandle;

    /* get TeeDN or ACDN mode */
    /*audioinfo->acousticMode = atoi(argv[8]);

    if (audioinfo->acousticMode == OMX_TRUE) {
        dataPath = DATAPATH_ACDN;
    }
    else if (dasfmode) {
        switch (MimoOutput)
        {
            case 0:
                dataPath= DATAPATH_MIMO_0;
                break;
            case 1:
                dataPath= DATAPATH_MIMO_1;
                break;
            case 2:
                dataPath= DATAPATH_MIMO_2;
                break;
            case 3:
                dataPath= DATAPATH_MIMO_3;
                break;
            case 4:
                dataPath= DATAPATH_MIMO_4;
                break;
            case -1:
#ifdef RTM_PATH 
                dataPath = DATAPATH_APPLICATION_RTMIXER;
#endif

#ifdef ETEEDN_PATH
                dataPath = DATAPATH_APPLICATION;
#endif
                break;
        }
    }
*/
    /*audioinfo->dasfMode = dasfmode;*/
  
  /*  
    teemode=atoi(argv[12]);
     switch(teemode)
     {
     case 0:
        audioinfo->teeMode = TEEMODE_NONE;
        break;
     case 1:
        audioinfo->teeMode = TEEMODE_PLAYBACK;
        break;
     case 2:
        audioinfo->teeMode = TEEMODE_LOOPBACK;
        break;
     case 3:
        audioinfo->teeMode = TEEMODE_PLAYLOOPBACK;
        break;
     }
*/
    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.nbamrheaderinfo",&index);
    if (error != OMX_ErrorNone) {
        printf("Error getting extension index\n");
        goto EXIT;
    }

    /*cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_CommandIsOutputStreamAvailable;*/
    
    /* for encoder, using AM_CommandIsInputStreamAvailable */
    /*cmd_data.param1 = 0;*/
#ifdef DSP_RENDERING_ON
    if((write(nbamrdecfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
        printf("%d ::OMX_AmrDecTest.c ::[NBAMR Dec Component] - send command to audio manager\n", __LINE__);
    }
    if((read(nbamrdecfdread, &cmd_data, sizeof(cmd_data)))<0) {
        printf("%d ::OMX_AmrDecTest.c ::[NBAMR Dec Component] - failure to get data from the audio manager\n", __LINE__);
        goto EXIT;
    }
#endif
    /*audioinfo->streamId = cmd_data.streamID;
    streamId = audioinfo->streamId;  
*//*
    error = OMX_SetConfig (pHandle, index, audioinfo);
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: AmrDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }
    */
    error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.nbamr.datapath",&index);
    if (error != OMX_ErrorNone) {
        printf("Error getting extension index\n");
        goto EXIT;
    }
    
/*    error = OMX_SetConfig (pHandle, index, &dataPath);
    if(error != OMX_ErrorNone) {
        error = OMX_ErrorBadParameter;
        APP_DPRINT("%d :: AmrDecTest.c :: Error from OMX_SetConfig() function\n",__LINE__);
        goto EXIT;
    }
    */
#ifdef OMX_GETTIME
    GT_START();
#endif     
    error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Idle(Init) State function\n",__LINE__);
        goto EXIT;
    }
    /* Wait for startup to complete */
    error = WaitForState(pHandle, OMX_StateIdle);
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
    if(error != OMX_ErrorNone) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Error:  WaitForState reports an error %X\n",__LINE__, error);
        goto EXIT;
    }

    if (dasfmode) {
        /* get streamID back to application */
         error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.nbamrstreamIDinfo",&index);
          if (error != OMX_ErrorNone) {
               printf("Error getting extension index\n");
                 goto EXIT;
           }
        printf("***************StreamId=%ld******************\n", streamId);
    }
    for(i = 0; i < testcnt; i++) {
            if((command == 5) || (command == 2))
                printf ("Decoding the file for %d Time\n",i+1);
        if(i > 0) {
            APP_DPRINT ("Decoding the file for %d Time\n",i+1);
            
            close(nbamrIpBuf_Pipe[0]);
            close(nbamrIpBuf_Pipe[1]);
            close(nbamrOpBuf_Pipe[0]);
            close(nbamrOpBuf_Pipe[1]);
            close(nbamrEvent_Pipe[0]);
            close(nbamrEvent_Pipe[1]);

            /* Create a pipe used to queue data from the callback. */
            retval = pipe(nbamrIpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "%d :: AmrDecTest.c :: Error:Fill Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }

            retval = pipe(nbamrOpBuf_Pipe);
            if( retval != 0) {
                APP_DPRINT( "%d :: AmrDecTest.c :: Error:Empty Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }

            retval = pipe(nbamrEvent_Pipe);
            if( retval != 0) {
                APP_DPRINT( "%d :: AmrDecTest.c :: Error:Empty Data Pipe failed to open\n",__LINE__);
                goto EXIT;
            }

            fIn = fopen(argv[1], "r");
            if(fIn == NULL) {
                fprintf(stderr, "Error:  failed to open the file %s for readonly access\n", argv[1]);
                goto EXIT;
            }
            fOut = fopen(argv[2], "a");
            if(fOut == NULL) {
                fprintf(stderr, "Error:  failed to create the output file \n");
                goto EXIT;
            }
        }
        if((command == 13)){
            if(i==0){
                printf ("Basic Function:: Sending OMX_StateExecuting Command\n");
#ifdef OMX_GETTIME
                GT_START();
#endif
                error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                if(error != OMX_ErrorNone) {
                    APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Executing State function\n",__LINE__);
                    goto EXIT;
                }
                pComponent = (OMX_COMPONENTTYPE *)pHandle;
                error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
                GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
                if(error != OMX_ErrorNone) {
                    APP_DPRINT( "%d :: AmrDecTest.c :: Error:  WaitForState reports an error %X\n",__LINE__, error);
                    goto EXIT;
                }
            }
        }else{
            printf ("Basic Function (else):: Sending OMX_StateExecuting Command\n");
#ifdef OMX_GETTIME
            GT_START();
#endif
            error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
            if(error != OMX_ErrorNone) {
                APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Executing State function\n",__LINE__);
                goto EXIT;
            }
            pComponent = (OMX_COMPONENTTYPE *)pHandle;
            error = WaitForState(pHandle, OMX_StateExecuting);
#ifdef OMX_GETTIME
            GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
            if(error != OMX_ErrorNone) {
                APP_DPRINT( "%d :: AmrDecTest.c :: Error:  WaitForState reports an error %X\n",__LINE__, error);
                goto EXIT;
            }
        }
        /*if ( dasfmode && dataPath > DATAPATH_MIMO_0)
        {
            cmd_data.hComponent = pHandle;
            cmd_data.AM_Cmd = AM_CommandWarnSampleFreqChange;  
            cmd_data.param1 = (dataPath == DATAPATH_MIMO_1 || dataPath == DATAPATH_MIMO_3) ? 8000: 16000;
       
            if(dataPath < DATAPATH_MIMO_3){
                 printf("** TEST APP : :  Paying on speakers\n");
                cmd_data.param2 = 1;          //speakers
        }else{
                printf("**TEST APP :: Playing on headset\n");
                cmd_data.param2 = 3;          //headset
        }
            if((write(nbamrdecfdwrite, &cmd_data, sizeof(cmd_data)))<0) {
                printf("send command to audio manager\n");
            }
        }*/

    for (k=0; k < numInputBuffers; k++) {
    #ifdef OMX_GETTIME
         if (k==0)
         { 
            GT_FlagE=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
            GT_START(); /* Empty Bufffer */
         }
    #endif
        error = send_input_buffer (pHandle, pInputBufferHeader[k], fIn);
    }

    if (dasfmode==0) {
        for (k=0; k < numOutputBuffers; k++) {
        #ifdef OMX_GETTIME
            if (k==0)
            { 
                GT_FlagF=1;  /* 1 = First Buffer,  0 = Not First Buffer  */
                GT_START(); /* Fill Buffer */
            }
        #endif
            pComponent->FillThisBuffer(pHandle,  pOutputBufferHeader[k]);
        }
    }

    error = pComponent->GetState(pHandle, &state);
    retval = 1;
#ifdef WAITFORRESOURCES
   /*while( 1 ){
   if(( (error == OMX_ErrorNone) && (state != OMX_StateIdle)) && (state != OMX_StateInvalid)&& (timeToExit!=1)){*/
#else
            while((error == OMX_ErrorNone) && (state != OMX_StateIdle) && 
                    (state != OMX_StateInvalid)){
   if(1){
#endif
        FD_ZERO(&rfds);
        FD_SET(nbamrIpBuf_Pipe[0], &rfds);
        FD_SET(nbamrOpBuf_Pipe[0], &rfds);
        FD_SET(nbamrEvent_Pipe[0], &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

                        
        retval = select(fdmax+1, &rfds, NULL, NULL, &tv);
        if(retval == -1) {
            perror("select()");
            APP_DPRINT ( "%d :: AmrDecTest.c :: Error \n",__LINE__);
            break;
        }
        if (retval == 0) 
           count++;
        if (count ==30){
            fprintf(stderr, "Shutting down Since there is nothing else to send nor read---------- \n");
            StopComponent(pHandle);
        }

    if(FD_ISSET(nbamrIpBuf_Pipe[0], &rfds)) {

        OMX_BUFFERHEADERTYPE* pBuffer = NULL;
        read(nbamrIpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
        frmCount++;
        pBuffer->nFlags = 0; 
            
        if( ((2==command) || (4==command) ) && (50 == frmCount)){ /*Stop Tests*/
                fprintf(stderr, "Send Stop Command to component ---------- \n");
                StopComponent(pHandle);
        }
        /**flush*/
        //if ((frmCount >= nNextFlushFrame) && (num_flush < 5000)){
        // if (frmCount >= nNextFlushFrame){
            // printf("Flush #%d\n", num_flush++);
            // error = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StatePause, NULL);
            // if(error != OMX_ErrorNone) {
                // fprintf (stderr,"Error from SendCommand-Pasue State function\n");
                // goto EXIT;
            // }
            // error = WaitForState(pHandle, OMX_StatePause);
            // if(error != OMX_ErrorNone) {
                // fprintf(stderr, "Error:  hAmrDecoder->WaitForState reports an error %X\n", error);
                // goto EXIT;
            // }
            // error = OMX_SendCommand(pHandle, OMX_CommandFlush, 0, NULL);
            // pthread_mutex_lock(&WaitForINFlush_mutex); 
            // pthread_cond_wait(&WaitForINFlush_threshold, 
                            // &WaitForINFlush_mutex);
            // pthread_mutex_unlock(&WaitForINFlush_mutex);

            // error = OMX_SendCommand(pHandle, OMX_CommandFlush, 1, NULL);
            // pthread_mutex_lock(&WaitForOUTFlush_mutex); 
            // pthread_cond_wait(&WaitForOUTFlush_threshold, 
                            // &WaitForOUTFlush_mutex);
            // pthread_mutex_unlock(&WaitForOUTFlush_mutex);    

            // error = OMX_SendCommand(pHandle, OMX_CommandStateSet,OMX_StateExecuting, NULL);
            // if(error != OMX_ErrorNone) {
                // fprintf (stderr,"Error from SendCommand-Executing State function\n");
                // goto EXIT;
            // }
            // error = WaitForState(pHandle, OMX_StateExecuting);
            // if(error != OMX_ErrorNone) {
                // fprintf(stderr, "Error:  hAmrDecoder->WaitForState reports an error %X\n", error);
                // goto EXIT;
            // }
                                    
            // fseek(fIn, 0, SEEK_SET);
            // frmCount = 0;
            // nNextFlushFrame = 5 + 50 * ((rand() * 1.0) / RAND_MAX);
            // printf("nNextFlushFrame d= %d\n", nNextFlushFrame);
        // }    
                
        if (state == OMX_StateExecuting){
            error = send_input_buffer (pHandle, pBuffer, fIn); 
            if (error != OMX_ErrorNone) { 
                printf("goto EXIT %d\n",__LINE__);
                goto EXIT; 
            } 
        }
        /***/
        if(3 == command){ /*Pause Test*/
            if(frmCount == 100) {   /*100 Frames processed */
                printf (" Sending Pause command to Codec \n");
                PauseComponent(pHandle);
                printf("5 secs sleep...\n");
                sleep(5);
                printf (" Sending Resume command to Codec \n");
                PlayComponent(pHandle);
                }                
        }
        else if ( 10 == command ){    /*Mute and UnMuteTest*/
                if(frmCount == 35){ 
                    printf("************Mute the playback stream*****************\n"); 
                    pCompPrivateStructMute->bMute = OMX_TRUE; 
                    error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
                    if (error != OMX_ErrorNone) 
                    { 
                        error = OMX_ErrorBadParameter; 
                        goto EXIT; 
                    } 
                } 
                else if(frmCount == 120) { 
                        printf("************Unmute the playback stream*****************\n"); 
                        pCompPrivateStructMute->bMute = OMX_FALSE; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioMute, pCompPrivateStructMute); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    }
            
            }
        else if ( 11 == command ) 
        { /*Set Volume Test*/
                    if(frmCount == 35) { 
                        printf("************Set stream volume to high*****************\n"); 
                        pCompPrivateStructVolume->sVolume.nValue = 0x8000; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    }
                    if(frmCount == 120) { 
                        printf("************Set stream volume to low*****************\n"); 
                        pCompPrivateStructVolume->sVolume.nValue = 0x1000; 
                        error = OMX_SetConfig(pHandle, OMX_IndexConfigAudioVolume, pCompPrivateStructVolume); 
                        if (error != OMX_ErrorNone) { 
                            error = OMX_ErrorBadParameter; 
                            goto EXIT; 
                        } 
                    }             
            }
        else if (command == 12) 
        { /* frame lost test */
                           if(frmCount == 35 || frmCount == 120) 
                    {
                             printf("************Sending lost frame*****************\n");
                             error = OMX_GetExtensionIndex(pHandle, "OMX.TI.index.config.nbamr.framelost",&index);             
                            error = OMX_SetConfig(pHandle, index, NULL);
                             if (error != OMX_ErrorNone) 
                      {
                               error = OMX_ErrorBadParameter;
                               goto EXIT;
                             }
                           }
            }
  }             
         if( FD_ISSET(nbamrOpBuf_Pipe[0], &rfds) ) 
         {     
            
            read(nbamrOpBuf_Pipe[0], &pBuf, sizeof(pBuf));
            if(pBuf->nFlags == OMX_BUFFERFLAG_EOS)
            {
                pBuf->nFlags = 0;
                                
            }
            fwrite(pBuf->pBuffer, 1, pBuf->nFilledLen, fOut);
            fflush(fOut);
            if (state == OMX_StateExecuting ) {
                pComponent->FillThisBuffer(pHandle, pBuf);
            }
        } 
        
        if( FD_ISSET(nbamrEvent_Pipe[0], &rfds) ) {
            OMX_U8 pipeContents = 0;
            read(nbamrEvent_Pipe[0], &pipeContents, sizeof(OMX_U8));
            if (pipeContents == 0) {
    
                printf("Test app received OMX_ErrorResourcesPreempted\n");
                WaitForState(pHandle,OMX_StateIdle);
               for (i=0; i < numInputBuffers; i++) {
                    error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
                    if( (error != OMX_ErrorNone)) {
                        APP_DPRINT ("%d :: AmrDecTest.c :: Error in Free Handle function\n",__LINE__);
                    }
                }

                for (i=0; i < numOutputBuffers; i++) {
                    error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
                    if( (error != OMX_ErrorNone)) {
                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                    }
                }
                         
#ifdef USE_BUFFER
                /* newfree the App Allocated Buffers */
                APP_DPRINT("%d :: AmrDecTest.c :: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
                for(i=0; i < numInputBuffers; i++) {
                    APP_MEMPRINT("%d::: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
                    if(pInputBuffer[i] != NULL){
                        pInputBuffer[i] = pInputBuffer[i] - 128;
                        newfree(pInputBuffer[i]);
                        pInputBuffer[i] = NULL;
                        }
                    }
                    for(i=0; i < numOutputBuffers; i++) {
                        APP_MEMPRINT("%d::: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
                        if(pOutputBuffer[i] != NULL){
                            pOutputBuffer[i] = pOutputBuffer[i] - 128;                            
                            newfree(pOutputBuffer[i]);
                            pOutputBuffer[i] = NULL;
                        }
                    }
#endif
                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateLoaded,NULL);
                    WaitForState(pHandle,OMX_StateLoaded);
                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateWaitForResources,NULL);
                    WaitForState(pHandle,OMX_StateWaitForResources);
                    }
                    else if (pipeContents == 1) {
                        printf("Test app received OMX_ErrorResourcesAcquired\n");

                        OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateIdle,NULL);
                        for (i=0; i < numInputBuffers; i++) {
                         /* allocate input buffer */
                         error = OMX_AllocateBuffer(pHandle,&pInputBufferHeader[i],0,NULL,INPUT_NBAMRDEC_BUFFER_SIZE*3); /*To have enought space for    */
                         if(error != OMX_ErrorNone) {
                         APP_DPRINT("%d :: AmrDecTest.c :: Error returned by OMX_AllocateBuffer()\n",__LINE__);
                         }
                        }

                    WaitForState(pHandle,OMX_StateIdle);
                    OMX_SendCommand(pHandle,OMX_CommandStateSet,OMX_StateExecuting,NULL);
                    WaitForState(pHandle,OMX_StateExecuting);
                    rewind(fIn);
                    for (i=0; i < numInputBuffers;i++) {    
                      send_input_buffer(pHandle, pInputBufferHeader[i], fIn);
                    }
                    
                   }
            if (pipeContents == 2) {
                            /* Send component to Idle */
                if(command !=13){
                    printf("%d :: AmrDecTest.c :: StopComponent\n",__LINE__);
                     StopComponent(pHandle);
                     
#ifdef WAITFORRESOURCES
                    for (i=0; i < numInputBuffers; i++) {
                        error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
                        if( (error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d :: AmrDecTest.c :: Error in Free Handle function\n",__LINE__);
                        }
                    }

                    for (i=0; i < numOutputBuffers; i++) {
                        error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
                        if( (error != OMX_ErrorNone)) {
                            APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                        }
                    }
                
#ifdef OMX_GETTIME
                    GT_START();
#endif                
                    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
                    if(error != OMX_ErrorNone) {
                         APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Idle State function\n",__LINE__);
                         printf("goto EXIT %d\n",__LINE__);
                         goto EXIT;
                    }
    
                    error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
                   GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
                   if(error != OMX_ErrorNone) {
                          APP_DPRINT( "%d :: AmrDecTest.c :: Error:  WaitForState reports an error %X\n",__LINE__, error);
                            printf("goto EXIT %d\n",__LINE__);
                           goto EXIT;
                    }
                    goto SHUTDOWN;
#endif
                }else{
                    if(i != 9){
                        printf("**RING_TONE: Lets play again!\n");
                        printf("counter= %d \n",i);
                        //playcompleted = 0;
                        goto my_exit;

                    }else{
                        StopComponent(pHandle);
                        printf("stopcomponent\n");
                        //playcompleted = 0;
                    }
                }
                
            }
        } 
        
        error = pComponent->GetState(pHandle, &state);
        if(error != OMX_ErrorNone) {
              APP_DPRINT("%d :: AmrDecTest.c :: Warning:  hAmrEncoder->GetState has returned status %X\n",
                                                                                __LINE__, error);
              goto EXIT;
        }
              
    } 
    else if (preempted) {
        sched_yield();
    }
    else {
        goto SHUTDOWN;
    }

    }/* While Loop Ending Here */
    printf ("The current state of the component = %d \n",state);
my_exit:
    fclose(fOut);
    fclose(fIn);  
    count = 0;
    FirstTime = 1;
    fileHdrReadFlag = 0;
    if(( command == 5) || (command == 2)) { /*If test is Stop & Play or Repeated*/
        /*sleep (10); */                                    /*play without deleting the component*/
    } 

    } /*Inner for loop ends here */
    
#ifndef WAITFORRESOURCES
                for (i=0; i < numInputBuffers; i++) {
                    error = OMX_FreeBuffer(pHandle,OMX_DirInput,pInputBufferHeader[i]);
                    if( (error != OMX_ErrorNone)) {
                        APP_DPRINT ("%d :: AmrDecTest.c :: Error in Free Handle function\n",__LINE__);
                goto EXIT;
                    }
                }

                for (i=0; i < numOutputBuffers; i++) {
                    error = OMX_FreeBuffer(pHandle,OMX_DirOutput,pOutputBufferHeader[i]);
                    if( (error != OMX_ErrorNone)) {
                        APP_DPRINT ("%d:: Error in Free Handle function\n",__LINE__);
                goto EXIT;
                    }
                }
                         
#ifdef USE_BUFFER
                /* newfree the App Allocated Buffers */
                APP_DPRINT("%d :: AmrDecTest.c :: Freeing the App Allocated Buffers in TestApp\n",__LINE__);
                for(i=0; i < numInputBuffers; i++) {
                    APP_MEMPRINT("%d::: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,pInputBuffer[i]);
                    if(pInputBuffer[i] != NULL){
                        pInputBuffer[i] = pInputBuffer[i] - 128;
                        newfree(pInputBuffer[i]);
                        pInputBuffer[i] = NULL;
                        }
                    }
                    for(i=0; i < numOutputBuffers; i++) {
                        APP_MEMPRINT("%d::: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, pOutputBuffer[i]);
                        if(pOutputBuffer[i] != NULL){
                            pOutputBuffer[i] = pOutputBuffer[i] - 128;                            
                            newfree(pOutputBuffer[i]);
                            pOutputBuffer[i] = NULL;
                        }
                    }
#endif
#endif

    printf ("Sending the StateLoaded Command\n");
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Idle State function\n",__LINE__);
       printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }
    
    error = WaitForState(pHandle, OMX_StateLoaded);
#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateLoaded>");
#endif
    if(error != OMX_ErrorNone) {
        APP_DPRINT( "%d :: AmrDecTest.c :: Error:  WaitForState reports an error %X\n",__LINE__, error);
                        printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }

#ifdef WAITFORRESOURCES
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateWaitForResources, NULL);
    if(error != OMX_ErrorNone) {
        APP_DPRINT ("%d :: AmrDecTest.c :: Error from SendCommand-Idle State function\n",__LINE__);
                        printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }
    error = WaitForState(pHandle, OMX_StateWaitForResources);

    /* temporarily put this here until I figure out what should really happen here */
    sleep(10);
    /* temporarily put this here until I figure out what should really happen here */
#endif    
    error = OMX_SendCommand(pHandle, OMX_CommandPortDisable, -1, NULL);

SHUTDOWN:
    /*cmd_data.hComponent = pHandle;
    cmd_data.AM_Cmd = AM_Exit;*/
#ifdef DSP_RENDERING_ON
    if((write(nbamrdecfdwrite, &cmd_data, sizeof(cmd_data)))<0)
        printf("%d ::OMX_AmrDecoder.c :: [NBAMR Dec Component] - send command to audio manager\n",__LINE__);
    
    close(nbamrdecfdwrite);
    close(nbamrdecfdread);
#endif
    printf ("Free the Component handle\n");
    /* Unload the NBAMR Encoder Component */
    
    
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pAmrParam);
    newfree(pAmrParam);
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStruct);
    newfree(pCompPrivateStruct);
    
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStructMute);
    newfree(pCompPrivateStructMute);
    
    APP_MEMPRINT("%d:::[TESTAPPFREE] %p\n",__LINE__,pCompPrivateStructVolume);
    newfree(pCompPrivateStructVolume);

    error = TIOMX_FreeHandle(pHandle);
    if( (error != OMX_ErrorNone)) 
    {
        APP_DPRINT ("%d :: AmrDecTest.c :: Error in Free Handle function\n",__LINE__);
          printf("goto EXIT %d\n",__LINE__);
        
        goto EXIT;
    }
    
    fclose(inputToSN);
    APP_DPRINT ("%d :: AmrDecTest.c :: Free Handle returned Successfully \n\n\n\n",__LINE__);
    
    } /*Outer for loop ends here */

        pthread_mutex_destroy(&WaitForState_mutex);
        pthread_cond_destroy(&WaitForState_threshold);
    /**flush    */
    pthread_mutex_destroy(&WaitForOUTFlush_mutex);
    pthread_cond_destroy(&WaitForOUTFlush_threshold);  
    pthread_mutex_destroy(&WaitForINFlush_mutex);
    pthread_cond_destroy(&WaitForINFlush_threshold);      
    /***/
    error = TIOMX_Deinit();
    error = TIOMX_Deinit();
    if( (error != OMX_ErrorNone)) {
            APP_DPRINT("APP: Error in Deinit Core function\n");
             printf("goto EXIT %d\n",__LINE__);
            goto EXIT;
    }

/*    newfree(audioinfo);
    newfree(streaminfo);
    */        
EXIT:
    if(bInvalidState==OMX_TRUE)
    {
#ifndef USE_BUFFER
        error = FreeAllResources(pHandle,
                                pInputBufferHeader[0],
                                pOutputBufferHeader[0],
                                numInputBuffers,
                                numOutputBuffers,
                                fIn,
                                fOut);
#else
        error = FreeAllResources(pHandle,
                                    pInputBuffer,
                                    pOutputBuffer,
                                    numInputBuffers,
                                    numOutputBuffers,
                                    fIn,
                                    fOut);
#endif
   }
#ifdef APP_DEBUGMEM        
    int r = 0;
    printf("\n-Printing memory not delete it-\n");
    for(r=0;r<500;r++){
        if (lines[r]!=0){
             printf(" --->%d Bytes allocated on %p File:%s Line: %d\n",bytes[r],arr[r],file[r],lines[r]);                  
             }
    }
#endif  
 
#ifdef OMX_GETTIME
    GT_END("NBAMR_DEC test <End>");
    OMX_ListDestroy(pListHead);    
#endif     
    return error;
}

/* ================================================================================= */
/**
* @fn send_input_buffer() description for send_input_buffer
send_input_buffer().
Sends input buffer to component
*
*/
/* ================================================================================ */
OMX_ERRORTYPE send_input_buffer(OMX_HANDLETYPE pHandle, OMX_BUFFERHEADERTYPE* pBuffer, FILE *fIn)
{
        OMX_ERRORTYPE error = OMX_ErrorNone;
        OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
        APP_DPRINT ("*************************\n");
        APP_DPRINT ("%d :: pBuffer = %p nRead = %d\n",__LINE__,pBuffer,nRead);
        APP_DPRINT ("*************************\n");        

        if(FirstTime){
            nRead = fill_data (pBuffer->pBuffer, AmrFrameFormat, fIn);
            pBuffer->nFilledLen = nRead;
        }
        else{
            memcpy(pBuffer->pBuffer, NextBuffer,nRead);
            pBuffer->nFilledLen = nRead;
        }
        
        nRead = fill_data (NextBuffer, AmrFrameFormat, fIn);

        
        if (AmrFrameFormat == OMX_AUDIO_AMRFrameFormatConformance){
            if( nRead<numRead )
                    pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
            else
                pBuffer->nFlags = 0;
        }else{
            if( 0 == nRead )
                pBuffer->nFlags = OMX_BUFFERFLAG_EOS;                
            else
                pBuffer->nFlags = 0; 
        }

        if(pBuffer->nFilledLen!=0){
               if (pBuffer->nFlags == OMX_BUFFERFLAG_EOS){
                printf("EOS send on last data buffer\n" );
            }
            pBuffer->nTimeStamp = rand() % 100;
                
            if (!preempted) {
                error = pComponent->EmptyThisBuffer(pHandle, pBuffer);
                if (error == OMX_ErrorIncorrectStateOperation) {
                    error = 0;
                }
            }
        }
        FirstTime=0; 
                
   return error;
}


OMX_S16 fill_data (OMX_U8 *pBuf, int mode, FILE *fIn)
{
    int nBytesRead=0;
    static OMX_S16 totalRead = 0;

     if (OMX_AUDIO_AMRFrameFormatConformance == mode) 
     {
        if (!fileHdrReadFlag) {
            fprintf (stderr, "Reading the file in Frame Format Conformance Mode\n");
            fileHdrReadFlag = 1;
        }

        if (testCaseNo == 7) { /* Multiframe with each buffer size = 2* framelenght */
            numRead = 2*STD_NBAMRDEC_BUF_SIZE;
        }else if (testCaseNo == 8) { /* Multiframe with each buffer size = 2/framelenght */
            numRead = STD_NBAMRDEC_BUF_SIZE/2;
        }else if (testCaseNo == 9) { /* Multiframe with alternating buffer size */
            if (alternate == 0) {
                numRead = 2*STD_NBAMRDEC_BUF_SIZE;
                alternate = 1;
            }
            else {
                numRead = STD_NBAMRDEC_BUF_SIZE/2;
                alternate = 0;
            }
        }else {
            numRead = STD_NBAMRDEC_BUF_SIZE;
        }
        nBytesRead = fread(pBuf, 1, numRead , fIn);
        
    } 
    
    else if (OMX_AUDIO_AMRFrameFormatIF2 == mode)
    {/*IF2*/
         static const OMX_U8 FrameLength[] = {13, 14, 16, 18, 19, 21, 26, 31, 6};/*List of valid IF2 frame length*/
         OMX_S8 size = 0;
         OMX_S16 index = 45; /* some invalid value*/
         OMX_S8 first_char = 0;
         
         if (!fileHdrReadFlag) 
         {
            fprintf (stderr, "Reading the file in IF2 Mode\n");
            fileHdrReadFlag = 1;
         }

        first_char = fgetc(fIn);
        if (EOF == first_char) 
        {
            fprintf(stderr, "Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = first_char & 0xf;

        if((index>=0) && (index<9))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((OMX_S8*)(pBuf))+1, 1, size - 1, fIn);
            nBytesRead += 1;
        } 
        else if (index == 15) 
        {
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = 1;
        } 
        else 
        {
            nRead = 0;
            printf("Invalid Index in the frame index = %d \n", index);
            goto EXIT;
        }   
    }

    else 
    {
        static const OMX_U8 FrameLength[] = {13, 14, 16, 18, 20, 21, 27, 32, 6};/*List of valid mime frame length*/
        OMX_S8 size = 0;
        OMX_S16 index = 45; /* some invalid value*/
        OMX_S8 first_char = 0;
        if (!fileHdrReadFlag) 
        {
           fprintf (stderr, "Reading the file in MIME Mode\n");
           /* Read the file header */
           
           if((fgetc(fIn) != 0x23) || (fgetc(fIn) != 0x21) || (fgetc(fIn) != 0x41) || (fgetc(fIn) != 0x4d) || (fgetc(fIn) != 0x52) || (fgetc(fIn) != 0x0a)) 
           {
               fprintf(stderr, "1er Char del file = %x", fgetc(fIn));
               fprintf(stderr, "Error: File does not have correct header\n");
           }
           fileHdrReadFlag = 1;
        }
        
        first_char = fgetc(fIn);
        if (EOF == first_char) 
        {
            fprintf(stderr, "Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = (first_char>>3) & 0xf;

        if((index>=0) && (index<9))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((OMX_S8*)(pBuf))+1, 1, size - 1, fIn);
            nBytesRead += 1;
        } 
        else if (index == 15) 
        {
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = 1;
        } 
        else 
        {
            nRead = 0;
            fprintf(stderr, "Invalid Index in the frame index = %d \n", index);
            goto EXIT;
        }
    }
    
    totalRead += nBytesRead;

EXIT:
    return nBytesRead;
}

void ConfigureAudio()
{
    int Mixer = 0, arg = 0, status = 0;

    Mixer = open("/dev/sound/mixer", O_WRONLY);
    if (Mixer < 0) {
        perror("open of /dev/sound/mixer failed");
        exit(1);
    }

    arg = NBAMRDEC_SAMPLING_FREQUENCY;       /* sampling rate */
    printf("Sampling freq set to:%d\n",arg);
    status = ioctl(Mixer, SOUND_PCM_WRITE_RATE, &arg);
    if (status == -1) {
        perror("SOUND_PCM_WRITE_RATE ioctl failed");
        printf("sample rate set to %u\n", arg);
    }
    arg = AFMT_S16_LE;            /* AFMT_S16_LE or AFMT_S32_LE */
    status = ioctl(Mixer, SOUND_PCM_SETFMT, &arg);
    if (status == -1) {
        perror("SOUND_PCM_SETFMT ioctl failed");
        printf("Bitsize set to %u\n", arg);
    }
    arg = 2;            /* Channels mono 1 stereo 2 */
    status = ioctl(Mixer, SOUND_PCM_WRITE_CHANNELS, &arg);
    if (status == -1) {
        perror("SOUND_PCM_WRITE_CHANNELS ioctl failed");
        printf("Channels set to %u\n", arg);
    }
    /* MIN 0 MAX 100 */

    arg = GAIN<<8|GAIN;
    status = ioctl(Mixer, SOUND_MIXER_WRITE_VOLUME, &arg);
    if (status == -1) {
        perror("SOUND_MIXER_WRITE_VOLUME ioctl failed");
        printf("Volume set to %u\n", arg);
    }
}

OMX_ERRORTYPE StopComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Idle(Stop) State function!!!!!!!!\n");
                    goto EXIT;
        }
    error =    WaitForState(pHandle, OMX_StateIdle);

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateIdle>");
#endif
    if(error != OMX_ErrorNone) {
                    fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
                    goto EXIT;
    }
EXIT:
    return error;
}

OMX_ERRORTYPE PauseComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StatePause, NULL);
    if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Pasue State function!!!!!!\n");
                    goto EXIT;
        }
    error =    WaitForState(pHandle, OMX_StatePause);

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StatePause>");
#endif
    if(error != OMX_ErrorNone) {
                    fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
                    goto EXIT;
    }
EXIT:
    return error;
}

OMX_ERRORTYPE PlayComponent(OMX_HANDLETYPE *pHandle)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;
#ifdef OMX_GETTIME
    GT_START();
#endif
    error = OMX_SendCommand(pHandle,OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if(error != OMX_ErrorNone) {
                    fprintf (stderr,"\nError from SendCommand-Executing State function!!!!!!!\n");
                    goto EXIT;
        }
    error =    WaitForState(pHandle, OMX_StateExecuting);

#ifdef OMX_GETTIME
    GT_END("Call to SendCommand <OMX_StateExecuting>");
#endif
    if(error != OMX_ErrorNone) {
                    fprintf(stderr, "\nError:  WaitForState reports an error %X!!!!!!!\n", error);
                    goto EXIT;
    }
EXIT:
    return error;
}

#ifdef APP_DEBUGMEM        
void * mymalloc(int line, char *s, int size)
{
   void *p = NULL;    
   int e=0;
   p = malloc(size);
   if(p==NULL){
       printf("Memory not available\n");
       exit(1);
       }
   else{
         while((lines[e]!=0)&& (e<500) ){
              e++;
         }
         arr[e]=p;
         lines[e]=line;
         bytes[e]=size;
         strcpy(file[e],s);
         printf("Allocating %d bytes on address %p, line %d file %s\n", size, p, line, s);
         return p;
   }
}

int myfree(void *dp, int line, char *s){
    int q = 0;
    if (dp==NULL){
       printf("Null Memory can not be deleted line: %d  file: %s\n", line, s);
       return 0;
    }
    for(q=0;q<500;q++){
        if(arr[q]==dp){
           printf("Deleting %d bytes on address %p, line %d file %s\n", bytes[q],dp, line, s);
           free(dp);
           dp = NULL;
           lines[q]=0;
           strcpy(file[q],"");
           break;
        }            
     }    
     if(500==q)
         printf("\n\nPointer not found. Line:%d    File%s!!\n\n",line, s);
     return 1;
}
#endif

/*=================================================================

                            Freeing All allocated resources
                            
==================================================================*/
#ifndef USE_BUFFER
int FreeAllResources( OMX_HANDLETYPE *pHandle,
                            OMX_BUFFERHEADERTYPE* pBufferIn,
                            OMX_BUFFERHEADERTYPE* pBufferOut,
                            int NIB, int NOB,
                            FILE* fileIn, FILE* fileOut)
{
    printf("%d::Freeing all resources by state invalid \n",__LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U16 i = 0; 
    for(i=0; i < NIB; i++) {
           printf("%d :: APP: About to free pInputBufferHeader[%d]\n",__LINE__, i);
           eError = OMX_FreeBuffer(pHandle, OMX_DirInput, pBufferIn+i);

    }


    for(i=0; i < NOB; i++) {
           printf("%d :: APP: About to free pOutputBufferHeader[%d]\n",__LINE__, i);
           eError = OMX_FreeBuffer(pHandle, OMX_DirOutput, pBufferOut+i);
    }

    /*i value is fixed by the number calls to malloc in the App */
    for(i=0; i<6;i++)  
    {
        if (ArrayOfPointers[i] != NULL)
            free(ArrayOfPointers[i]);
    }
    
        eError = close (nbamrIpBuf_Pipe[0]);
        eError = close (nbamrIpBuf_Pipe[1]);
        eError = close (nbamrOpBuf_Pipe[0]);
        eError = close (nbamrOpBuf_Pipe[1]);
                     close(nbamrEvent_Pipe[0]);
                     close(nbamrEvent_Pipe[1]);

    if(fileOut != NULL)    /* Could have been closed  previously */
    {
        fclose(fileOut);
        fileOut=NULL;
    }

    if(fileIn != NULL)
    {    fclose(fileIn);
        fileIn=NULL;
    }

    TIOMX_FreeHandle(pHandle);

    return eError;

}


/*=================================================================

                            Freeing the resources with USE_BUFFER define
                            
==================================================================*/
#else

int FreeAllResources(OMX_HANDLETYPE *pHandle,
                            OMX_U8* UseInpBuf[],
                            OMX_U8* UseOutBuf[],             
                            int NIB,int NOB,
                            FILE* fileIn, FILE* fileOut)
{

        OMX_ERRORTYPE eError = OMX_ErrorNone;
        OMX_U16 i = 0; 
        printf("%d::Freeing all resources by state invalid \n",__LINE__);
        /* free the UseBuffers */
        for(i=0; i < NIB; i++) {
           UseInpBuf[i] = UseInpBuf[i] - 128;
           printf("%d :: [TESTAPPFREE] pInputBuffer[%d] = %p\n",__LINE__,i,(UseInpBuf[i]));
           if(UseInpBuf[i] != NULL){
              newfree(UseInpBuf[i]);
              UseInpBuf[i] = NULL;
           }
        }

        for(i=0; i < NOB; i++) {
           UseOutBuf[i] = UseOutBuf[i] - 128;
           printf("%d :: [TESTAPPFREE] pOutputBuffer[%d] = %p\n",__LINE__,i, UseOutBuf[i]);
           if(UseOutBuf[i] != NULL){
              newfree(UseOutBuf[i]);
              UseOutBuf[i] = NULL;
           }
        }

    /*i value is fixed by the number calls to malloc in the App */
        for(i=0; i<6;i++)  
        {
            if (ArrayOfPointers[i] != NULL)
                free(ArrayOfPointers[i]);
        }
    
            eError = close (nbamrIpBuf_Pipe[0]);
            eError = close (nbamrIpBuf_Pipe[1]);
            eError = close (nbamrOpBuf_Pipe[0]);
            eError = close (nbamrOpBuf_Pipe[1]);
                     close(nbamrEvent_Pipe[0]);
                     close(nbamrEvent_Pipe[1]);


        if (fileOut != NULL)    /* Could have been closed  previously */ */
        {
            fclose(fileOut);
            fileOut=NULL;
        }
        
        if (fileIn != NULL)
        {    fclose(fileIn);
            fileIn=NULL;
        }
    
        OMX_FreeHandle(pHandle);
    
        return eError;
}

#endif
