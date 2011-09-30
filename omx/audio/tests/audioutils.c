#include "tiomxplayer.h"

extern FILE* infile;

/** Configure mp3 input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_mp3(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;

  OMX_INIT_STRUCT(appPrvt->mp3,OMX_AUDIO_PARAM_MP3TYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioMp3\n");
  appPrvt->mp3->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioMp3,
                            appPrvt->mp3);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }

  APP_DPRINT("call set_parameter for OMX_IndexParamAudioMp3\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioMp3,
                            appPrvt->mp3);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }


  return 0;
}
/** Configure aac input params
 *
 * @param appPrvt Handle to the app component structure.
 *
 */
int config_aac(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;
  int framesPerOutput = 512;
  OMX_INDEXTYPE index = 0;
  OMX_INIT_STRUCT(appPrvt->aac, OMX_AUDIO_PARAM_AACPROFILETYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioAac\n");
  appPrvt->aac->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioAac,
                            appPrvt->aac);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  if(RECORD == appPrvt->appType){
      appPrvt->aac->eAACProfile = appPrvt->profile;
      appPrvt->aac->nBitRate = appPrvt->bitrate;
      appPrvt->aac->eAACStreamFormat = appPrvt->fileformat;
      error = OMX_GetExtensionIndex(appPrvt->phandle, "OMX.TI.index.config.aacencframesPerOutBuf",&index);
      if (error != OMX_ErrorNone)
      {
          APP_DPRINT("%d :: APP: Error getting extension index\n",__LINE__);
      }
      error = OMX_SetConfig (appPrvt->phandle, index, &framesPerOutput);
      if (error != OMX_ErrorNone)
      {
          APP_DPRINT("%d :: APP: Error in SetConfig\n",__LINE__);
      }
  }
  appPrvt->aac->nSampleRate = appPrvt->samplerate;
  appPrvt->aac->nChannels = appPrvt->channels;

  if(appPrvt->raw){
      APP_DPRINT("Configure in RAW format with samplerate %ld and channels %ld\n",
                 appPrvt->aac->nSampleRate,
                 appPrvt->aac->nChannels);
    appPrvt->aac->eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
  }

  APP_DPRINT("call set_parameter for OMX_IndexParamAudioAac\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioAac,
                            appPrvt->aac);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }


  return 0;
}
/** Configure wma input params
 *
 * @param appPrvt Handle to the app component structure.
 *
 */
int config_wma(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;

  OMX_INIT_STRUCT(appPrvt->wma, OMX_AUDIO_PARAM_WMATYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioWma\n");
  appPrvt->wma->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioWma,
                            appPrvt->wma);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  APP_DPRINT("call set_parameter for OMX_IndexParamAudioWma\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioWma,
                            appPrvt->wma);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  return 0;
}

/** Process wmadec buffers
 *
 * The data received is in RCA format
 *
 * @param appPrvt Aplication private variables
 */
int process_wma(appPrivateSt* appPrvt, OMX_U8* pBuf){
    static OMX_S16 payload=0;
    static OMX_S16 buffer_state=0;
    OMX_S16 nBytesRead=0;
    OMX_U8 temp;
    if(appPrvt->fileReRead){
        buffer_state=0;
        appPrvt->fileReRead=OMX_FALSE;
    }

    if(buffer_state==0){
        nBytesRead = unparse_rca(pBuf, &payload);
        buffer_state=1;
        return nBytesRead;
    }
    else if(buffer_state==1){
        nBytesRead = fread(pBuf, 1, payload, infile);
        buffer_state=2;
        return nBytesRead;
    }
    else
        fread(&temp,5,1,infile); /* moving file 5 bytes*/
    nBytesRead = fread(pBuf, 1, payload, infile);
    return nBytesRead;
}

/** Process unparse_rca
 *
 * @param appPrvt Aplication private variables
 *
 * The data received is in RCA format
 */
int unparse_rca(OMX_U8* pBuffer, int * payload){
  OMX_U8* tempBuffer= malloc(75);
  memset(pBuffer,0x00,75);
  memset(tempBuffer,0x00,75);
  fread(tempBuffer,75,1,infile);
  tempBuffer+=42;
  memcpy(pBuffer,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=2;
  memcpy(pBuffer+2,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=2;
  memcpy(pBuffer+4,tempBuffer,sizeof(OMX_U32));
  tempBuffer+=4;
  memcpy(pBuffer+8,tempBuffer,sizeof(OMX_U32));
  tempBuffer+=4;
  memcpy(pBuffer+12,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=2;
  memcpy(pBuffer+14,tempBuffer,sizeof(OMX_U16));
  tempBuffer+=8;
  memcpy(pBuffer+22,tempBuffer,sizeof(OMX_U16));
  tempBuffer += 7;
  memcpy(payload,tempBuffer,2);
  return 28;
}
