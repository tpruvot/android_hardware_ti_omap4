#ifndef _TIOMXPLAYER_H_
#define _TIOMXPLAYER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Used for tiomxplayerutils.c */
#include <pthread.h>
#include <sys/time.h>

#include <error.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <unistd.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <signal.h>

#define OMAP3
#define IN_BUFFER_SIZE 1024*2
#define OUT_BUFFER_SIZE 4096*10

#define STD_NBAMRDEC_BUF_SIZE 118
#define STD_WBAMRDEC_BUF_SIZE 116
#define NBAMRENC_MIME_HEADER_LEN 6
#define WBAMRENC_MIME_HEADER_LEN 9
#define NBAMRENC_BUFFER_SIZE 320
#define WBAMRENC_BUFFER_SIZE 640

/* G729 frame size */
#define SPEECH_FRAME_SIZE 80
#define G729_SID_FRAME_SIZE 15
#define G729SID_OCTET_FRAME_SIZE 16
#define G729SYNC_WORD 0x6b21
/* G729 frame type */
#define G729NO_TX_FRAME_TYPE 0
#define G729SPEECH_FRAME_TYPE 1
#define G729SID_FRAME_TYPE 2
#define G729ERASURE_FRAME 3
#define G729ITU_INPUT_SIZE 82
#define G729SPEECHPACKETSIZE 10
#define G729SIDPACKETSIZE 2

#define CACHE_ALIGNMENT 128
#define EXTRA_BYTES 256

#define FILE_MODE 0
#define ALSA_MODE 1
#define DASF_MODE 2

#define PLAYER  0
#define RECORD  1

#define APP_DEBUG
#ifdef APP_DEBUG
#define APP_DPRINT(...) fprintf(stderr,"%d %s-",__LINE__,__func__);	\
  fprintf(stderr,__VA_ARGS__);
#else
    #define APP_DPRINT(...)
#endif

#ifdef ANDROID
    #define OMX_GetHandle TIOMX_GetHandle
    #define OMX_FreeHandle TIOMX_FreeHandle
    #define OMX_Init TIOMX_Init
    #define OMX_Deinit TIOMX_Deinit
#endif
/**=========================================================================**/
/**
 * @def OMX_INIT_STRUCT - Initializes the data structure using a ptr to
 *  structure This initialization sets up the nSize and nVersin fields of the
 *  structure
 *=========================================================================**/
#define OMX_INIT_STRUCT(_s_, _name_)            \
  memset((_s_), 0x0, sizeof(_name_));           \
  (_s_)->nSize = sizeof(_name_);                \
  (_s_)->nVersion.s.nVersionMajor = 0x1;        \
  (_s_)->nVersion.s.nVersionMinor = 0x1;        \
  (_s_)->nVersion.s.nRevision  = 0x0;           \
  (_s_)->nVersion.s.nStep   = 0x0;

typedef struct Event_t{
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  unsigned int semval;
}Event_t;

typedef struct{
  char *device ;
  snd_pcm_t *playback_handle;
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_format_t format;
  snd_pcm_uframes_t frames;
}alsaPrvtSt;

typedef struct G711DEC_FTYPES{
    unsigned short FrameSizeType;
    unsigned short NmuNLvl;
    unsigned short NoiseLp;
    unsigned short  dBmNoise;
    unsigned short plc;
}G711DEC_FTYPES;

typedef struct{
  OMX_HANDLETYPE phandle;
  OMX_PARAM_PORTDEFINITIONTYPE *in_port;
  OMX_PARAM_PORTDEFINITIONTYPE *out_port;
  OMX_BUFFERHEADERTYPE **in_buffers;
  OMX_BUFFERHEADERTYPE **out_buffers;
  OMX_AUDIO_PARAM_PCMMODETYPE *pcm;
  OMX_AUDIO_PARAM_MP3TYPE *mp3;
  OMX_AUDIO_PARAM_AACPROFILETYPE *aac;
  OMX_AUDIO_PARAM_WMATYPE *wma;
  OMX_AUDIO_PARAM_AMRTYPE *nbamr;
  OMX_AUDIO_PARAM_AMRTYPE *wbamr;
  OMX_AUDIO_PARAM_PCMMODETYPE *g711;
  OMX_AUDIO_PARAM_PCMMODETYPE *g722;
  OMX_AUDIO_PARAM_G726TYPE *g726;
  OMX_AUDIO_PARAM_G729TYPE *g729;
  OMX_PARAM_COMPONENTROLETYPE *pCompRoleStruct;
  OMX_AUDIO_CODINGTYPE eEncoding;
  OMX_U8 mode;
  OMX_U8 bandMode;
  OMX_U8 frameFormat;
  OMX_U8 dtxMode;
  OMX_U8 appType;
  OMX_U16 iterations;
  OMX_U8 channels;
  OMX_U32 samplerate;
  OMX_U32 bitrate;
  OMX_U32 profile;
  OMX_AUDIO_AACSTREAMFORMATTYPE fileformat;
  OMX_U16 nIpBuf;
  OMX_U16 IpBufSize;
  OMX_U16 nOpBuf;
  OMX_U16 OpBufSize;
  OMX_BOOL done_flag;
  OMX_BOOL raw;
  OMX_BOOL amr_mode;
  OMX_U8 tc;
  Event_t *eos;
  Event_t *flush;
  Event_t *state;
  Event_t *pause;
  alsaPrvtSt *alsaPrvt;
  OMX_U32 processed_buffers;
  OMX_U32 Device;
  OMX_U8 fileReRead;
  OMX_U32 recordTime;
  OMX_U32 frameSizeRecord;
  volatile OMX_U8 comthrdstop;
  volatile OMX_U8 stateToPause;
  pthread_t commFunc;
  OMX_U16 wd_timeout;
  OMX_U8 wd_isSet;
}appPrivateSt;

/** Initializes the event at a given value
 *
 * @param event the event to initialize
 *
 * @param val the initial value of the event
 */
void event_init(Event_t* event, unsigned int val);

/** Destroy the event
 *
 * @param event the event to destroy
 */
void event_deinit(Event_t* event);

/** Decreases the value of the event. Blocks if the event
 * value is zero.
 *
 * @param event the event to decrease
 */
void event_block(Event_t* event);

/** Increases the value of the event
 *
 * @param event the event to increase
 */
void event_wakeup(Event_t* event);

/** Reset the value of the event
 *
 * @param event the event to reset
 */
void event_reset(Event_t* event);

/** Wait on the condition.
 *
 * @param event the event to wait
 */
void event_wait(Event_t* event);

/** Signal the condition,if waiting
 *
 * @param event the event to signal
 */
void event_signal(Event_t* event);

/**
 *
 * @param
 */
void alsa_setAudioParams(appPrivateSt *appPrvt) ;

/**
 *
 * @param
 */
void alsa_pcm_write(appPrivateSt *appPrvt, OMX_BUFFERHEADERTYPE* pBuffer);

/** use_buffer: Application allocates the buffers
 *
 * @param handle Handle to the component.
 *
 */
int use_buffer(appPrivateSt *appPrvt);

/** allocate_buffer: OMX allocates the buffers
 *
 * @param handle Handle to the component.
 *
 */
int allocate_buffer(appPrivateSt *appPrvt);

/** Call to OMX_EmptyThisBuffer
 *
 * @param handle Handle to the component.
 * @param buffer IN buffer header pointer
 *
 */
int (*send_input_buffer)(appPrivateSt* appPrvt,
                      OMX_BUFFERHEADERTYPE* buffer);

/** Call to OMX_EmptyThisBuffer for decoders
 *
 * @param handle Handle to the component.
 * @param buffer IN buffer header pointer
 *
 */
int send_dec_input_buffer(appPrivateSt* appPrvt,OMX_BUFFERHEADERTYPE *buffer);
/** Call to OMX_EmptyThisBuffer for encoders
 *
 * @param handle Handle to the component.
 * @param buffer IN buffer header pointer
 *
 */
int send_enc_input_buffer(appPrivateSt* appPrvt,OMX_BUFFERHEADERTYPE *buffer);
/** test: Switch to different test cases
 *
 * @param handle Handle to the component.
 *
 */
int test(appPrivateSt* appPrvt);

/** test_pause_resume: Pause-resume
 *
 * @param appPrvt Handle to the app component structure.
 *
 */
int test_pause_resume(appPrivateSt *appPrvt);

/** test_repeat: Test repeat
 *
 * @param appPrvt Handle to the app component structure.
 *
 */
int test_repeat(appPrivateSt *appPrvt);

/** test_stop_play: Stop-Play
 *
 * @param appPrvt Handle to the app component structure.
 *
 */
int test_stop_play(appPrivateSt *appPrvt);
/** Waits for the OMX component to change to the desired state
 *
 * @param pHandle Handle to the component
 * @param Desiredstate State to be reached
 * @param event the event to hold on
 *
 */
OMX_ERRORTYPE WaitForState(OMX_HANDLETYPE pHandle,
                           OMX_STATETYPE DesiredState,
                           Event_t* pevent);

/** Handles events reported by the OMX component
 *
 * @param pAppData Application variables
 * @param eEvent Event reported
 * @param nData1 Optional parameter 1
 * @param nData2 Optional parameter 2
 * @param pEventData Event Data
 *
 */
OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE hComponent,
                           OMX_PTR pAppData,
                           OMX_EVENTTYPE eEvent,
                           OMX_U32 nData1,
                           OMX_U32 nData2,
                           OMX_PTR pEventData);

/** Callback received when codec finishes processing output buffers
 *
 * @param ptr Application Data
 * @param pBuffer output buffer
 *
 */
OMX_ERRORTYPE FillBufferDone (OMX_HANDLETYPE hComponent,
                              OMX_PTR ptr,
                              OMX_BUFFERHEADERTYPE* pBuffer);

/** Callback received when codec finishes consuming input buffers
 *
 * @param ptr Application Data
 * @param pBuffer input buffer
 *
 */
OMX_ERRORTYPE EmptyBufferDone(OMX_HANDLETYPE hComponent,
                              OMX_PTR ptr,
                              OMX_BUFFERHEADERTYPE* pBuffer);

/** Configure mp3 input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_mp3(appPrivateSt* appPrvt);
/** Configure aac input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_aac(appPrivateSt* appPrvt);
/** Configure wna input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_wma(appPrivateSt* appPrvt);
/** Configure nbamr input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_nbamr(appPrivateSt* appPrvt);
/** Configure wbamr input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_wbamr(appPrivateSt* appPrvt);
/** Configure g711 input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_g711(appPrivateSt* appPrvt);
/** Configure g722 input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_g722(appPrivateSt* appPrvt);
/** Configure g726 input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_g726(appPrivateSt* appPrvt);
/** Configure g729 input params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_g729(appPrivateSt* appPrvt);
/** Configure pcm output params
 *
 * @param appPrvt Aplication private variables
 *
 */
int config_pcm(appPrivateSt* appPrvt);

/** Process nb amr buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_nbamr(appPrivateSt* appPrvt, OMX_U8* buffer);
/** Process wb amr buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_wbamr(appPrivateSt* appPrvt, OMX_U8* buffer);
/** Process nb amr enc buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
OMX_BOOL process_nbamr_enc(appPrivateSt* appPrvt, OMX_BUFFERHEADERTYPE *buffer);
/** Process wb amr enc buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
OMX_BOOL process_wbamr_enc(appPrivateSt* appPrvt, OMX_BUFFERHEADERTYPE *buffer);
/** Process g729dec buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_g729(appPrivateSt* appPrvt, OMX_U8* buffer);
/** Process wnadec buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_wma(appPrivateSt* appPrvt, OMX_U8* buffer);
/** Process unparse_rca
 *
 * @param appPrvt Aplication private variables
 *
 * The data received is in RCA format
 */
int unparse_rca (OMX_U8* pBuffer, int * payload);

/** Process CommandListener
 *
 * @param
 *
 * Thread that waits for user commands
 */
void* TIOMX_CommandListener();

/** Process SetState
 *
 * @param appPrvt Aplication private variables
 *
 * Transition to a desired state
 */
int SetOMXState(appPrivateSt *appPrvt,OMX_STATETYPE DesiredState);

/**
 * Process: startSendingbuffers
 *
 * @param appPrvt Application private variables
 *
 * Application starts to send input and output buffers
*/
void startSendingBuffers(appPrivateSt *appPrvt);
/**
 * Process: handle_watchdog
 *
 * @param int signal
 *
 * Application timeout, need to kill the process to continue
*/
void handle_watchdog(int sig);
#endif
