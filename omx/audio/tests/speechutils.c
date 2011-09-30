#include "tiomxplayer.h"

extern FILE* infile;
extern FILE* outfile;

/** Configure nbamr input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_nbamr(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;

  OMX_INIT_STRUCT(appPrvt->nbamr,OMX_AUDIO_PARAM_AMRTYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioAmr\n");
  appPrvt->nbamr->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioAmr,
                            appPrvt->nbamr);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }

  /* select the amr format: Mime, IF2 or Conformance */
  appPrvt->nbamr->eAMRFrameFormat  = appPrvt->frameFormat;
  appPrvt->nbamr->eAMRBandMode = appPrvt->bandMode;
  appPrvt->nbamr->eAMRDTXMode = appPrvt->dtxMode;

  APP_DPRINT("call set_parameter for OMX_IndexParamAudioAmr\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioAmr,
                            appPrvt->nbamr);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }


  return 0;
}
/** Configure wbamr input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_wbamr(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;

  OMX_INIT_STRUCT(appPrvt->wbamr,OMX_AUDIO_PARAM_AMRTYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioAmr\n");
  appPrvt->wbamr->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioAmr,
                            appPrvt->wbamr);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }

  /* select the amr format: Mime, IF2 or Conformance */
  appPrvt->wbamr->eAMRFrameFormat = appPrvt->frameFormat;
  appPrvt->wbamr->eAMRBandMode = appPrvt->bandMode;
  appPrvt->wbamr->eAMRDTXMode = appPrvt->dtxMode;

  APP_DPRINT("call set_parameter for OMX_IndexParamAudioAmr\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioAmr,
                            appPrvt->wbamr);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }


  return 0;
}

/** Configure g711 input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_g711(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;
  G711DEC_FTYPES frameinfo;
  OMX_INDEXTYPE index = 0;

  OMX_INIT_STRUCT(appPrvt->g711,OMX_AUDIO_PARAM_PCMMODETYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioG711\n");
  appPrvt->g711->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioPcm,
                            appPrvt->g711);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  appPrvt->g711->nChannels = 1; /* mono */
  appPrvt->g711->eNumData = OMX_NumericalDataUnsigned;
  appPrvt->g711->eEndian = OMX_EndianLittle;
  appPrvt->g711->bInterleaved = OMX_FALSE;
  appPrvt->g711->nBitPerSample = 8;
  appPrvt->g711->nSamplingRate = 0; /* means undefined in the OMX standard */

  /** Getting the frame params */
  frameinfo.FrameSizeType = appPrvt->fileformat;
  frameinfo.NmuNLvl = 0;
  frameinfo.NoiseLp = 2;
  frameinfo.dBmNoise = 5;
  frameinfo.plc = -70;

  error = OMX_GetExtensionIndex(appPrvt->phandle, "OMX.TI.index.config.g711dec.frameparams",&index);
  if (error != OMX_ErrorNone) {
      printf("Error getting extension index\n");
  }

  error = OMX_SetConfig (appPrvt->phandle, index, &frameinfo);
  if(error != OMX_ErrorNone) {
      APP_DPRINT("%d :: Error from OMX_SetConfig() function\n",__LINE__);
  }

  /*Alaw = 1 MuLaw=2 */
  appPrvt->g711->ePCMMode = appPrvt->profile;

  APP_DPRINT("call set_parameter for OMX_IndexParamAudioG711\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioPcm,
                            appPrvt->g711);
  if (error != OMX_ErrorNone) {
      APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
      return 1;
  }

  printf("Exiting config g711 \n");
  return 0;
}
/** Configure g722 input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_g722(appPrivateSt* appPrvt){
  OMX_ERRORTYPE error = OMX_ErrorNone;

  OMX_INIT_STRUCT(appPrvt->g722,OMX_AUDIO_PARAM_PCMMODETYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioG722\n");
  appPrvt->g722->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioPcm,
                            appPrvt->g722);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }

  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioPcm,
                            appPrvt->g722);
  if (error != OMX_ErrorNone) {
      APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
      return 1;
  }

  printf("Exiting config g722 \n");
  return 0;
};

/** Configure g726 input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_g726(appPrivateSt* appPrvt){
  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_INIT_STRUCT(appPrvt->g726,OMX_AUDIO_PARAM_G726TYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioG726\n");
  appPrvt->g726->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioG726,
                            appPrvt->g726);
  if (error != OMX_ErrorNone) {
      APP_DPRINT("OMX_ErrorBadParameter in GetParameter\n");
      /*FIX ME*/
      /*GetParameter returns error*/
      //return 1;
  }

  appPrvt->g726->nSize                    = sizeof(OMX_AUDIO_PARAM_G726TYPE);
  appPrvt->g726->nVersion.s.nVersionMajor = 1;
  appPrvt->g726->nVersion.s.nVersionMinor = 1;
  appPrvt->g726->nPortIndex               = OMX_DirInput;
  appPrvt->g726->nChannels                = 1; /* mono */

  switch(appPrvt->bitrate){
  case 16:
      appPrvt->g726->eG726Mode = OMX_AUDIO_G726Mode16;
      break;
  case 24:
      appPrvt->g726->eG726Mode = OMX_AUDIO_G726Mode24;
      break;
  case 32:
      appPrvt->g726->eG726Mode = OMX_AUDIO_G726Mode32;
      break;
  case 40:
      appPrvt->g726->eG726Mode = OMX_AUDIO_G726Mode40;
      break;
  default:
      printf("Invalid Bitrate!\n");
      break;
  }
  APP_DPRINT("call set_parameter for OMX_IndexParamAudioG726\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioG726,
                            appPrvt->g726);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  printf("Exiting config g726 \n");
  return 0;
}

/** Configure g729 input params
 *
 * @param appPrvt appPrvt Handle to the app component structure.
 *
 */
int config_g729(appPrivateSt* appPrvt){

  OMX_ERRORTYPE error = OMX_ErrorNone;
  OMX_INIT_STRUCT(appPrvt->g729,OMX_AUDIO_PARAM_G729TYPE);

  APP_DPRINT("call get_parameter for OMX_IndexParamAudioG729\n");
  appPrvt->g729->nPortIndex = OMX_DirInput;
  error = OMX_GetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioG729,
                            appPrvt->g729);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  appPrvt->g729->eBitType = OMX_AUDIO_G729AB;

  APP_DPRINT("call set_parameter for OMX_IndexParamAudioG729\n");
  error = OMX_SetParameter (appPrvt->phandle,
                            OMX_IndexParamAudioG729,
                            appPrvt->g729);
  if (error != OMX_ErrorNone) {
    APP_DPRINT("OMX_ErrorBadParameter in SetParameter\n");
    return 1;
  }
  printf("Exiting config g729 \n");
  return 0;
}

/** Process nb amr buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_nbamr(appPrivateSt* appPrvt, OMX_U8* pBuf){
    int nBytesRead=0;
    static OMX_S16 totalRead = 0;
    static OMX_S16 fileHdrReadFlag = 0;
    OMX_S8 first_char = 0;

    if(appPrvt->fileReRead){
        totalRead = 0;
    }

    if (appPrvt->nbamr->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatIF2){
        static const OMX_U8 FrameLength[] = {13, 14, 16, 18, 19, 21, 26, 31, 6};/*List of valid IF2 frame length*/
        OMX_S8 size = 0;
        OMX_S16 index = 45; /* some invalid value*/

        if (!fileHdrReadFlag)
        {
            APP_DPRINT ("Reading the file in IF2 Mode\n");
            fileHdrReadFlag = 1;
        }

        first_char = fgetc(infile);
        if (EOF == first_char){
            APP_DPRINT("Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = first_char & 0xf;

        if((index>=0) && (index<9)){
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((OMX_S8*)(pBuf))+1, 1, size - 1, infile);
            nBytesRead += 1;
        }
        else if (index == 15){
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = 1;
        }else{
            nBytesRead = 0;
            printf("Invalid Index in the frame index = %d \n", index);
            goto EXIT;
        }
    }else if (appPrvt->nbamr->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatConformance) {
      if (!fileHdrReadFlag) {
        APP_DPRINT("Reading the file in Frame Format Conformance Mode\n");
        fileHdrReadFlag = 1;
      }

      nBytesRead = fread(pBuf,
                         1,
                         STD_NBAMRDEC_BUF_SIZE,
                         infile);

    }else if (appPrvt->nbamr->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatFSF){
        static const OMX_U8 FrameLength[] = {13, 14, 16, 18, 20, 21, 27, 32, 6};/*List of valid mime frame length*/
        OMX_S8 size = 0;
        OMX_S16 index = 45; /* some invalid value*/

        if (!fileHdrReadFlag)
        {
            APP_DPRINT ("Reading the file in MIME Mode\n");
            /* Read the file header */

            if((fgetc(infile) != 0x23) || (fgetc(infile) != 0x21) ||
               (fgetc(infile) != 0x41) || (fgetc(infile) != 0x4d) ||
               (fgetc(infile) != 0x52) || (fgetc(infile) != 0x0a))
            {
                APP_DPRINT("1er Char del file = %x\n", fgetc(infile));
                APP_DPRINT("Error: File does not have correct header\n");
            }
            fileHdrReadFlag = 1;
        }

        first_char = fgetc(infile);
        if (EOF == first_char)
        {
            APP_DPRINT("Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = (first_char>>3) & 0xf;

        if((index>=0) && (index<9))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((OMX_S8*)(pBuf))+1, 1, size - 1, infile);
            nBytesRead += 1;
        }
        else if (index == 15)
        {
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = 1;
        }
        else
        {
            nBytesRead = 0;
            APP_DPRINT("Invalid Index in the frame index = %d \n", index);
            goto EXIT;
        }
    }
    totalRead += nBytesRead;
 EXIT:
    return nBytesRead;

}
/** Process wb amr buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_wbamr(appPrivateSt* appPrvt, OMX_U8* pBuf){
    int nBytesRead=0;
    static OMX_S16 totalRead = 0;
    static OMX_S16 fileHdrReadFlag = 0;
    OMX_S8 first_char = 0;

    if(appPrvt->fileReRead){
        totalRead = 0;
    }

    if (appPrvt->wbamr->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatIF2){
        static const OMX_U8 FrameLength[] = {18, 23, 33, 37, 41, 47, 51, 59, 61, 6};/*List of valid IF2 frame length*/
        OMX_S8 size = 0;
        OMX_S16 index = 45; /* some invalid value*/

        if (!fileHdrReadFlag) {
            APP_DPRINT ("Reading the file in IF2 Mode\n");
            fileHdrReadFlag = 1;
        }

        first_char = fgetc(infile);
        if (EOF == first_char) {
            APP_DPRINT("Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = (first_char>>4) & 0xf;
        if((index>=0) && (index<=9))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((OMX_S8*)(pBuf))+1, 1, size - 1, infile);
            nBytesRead += 1;
        } else if (index == 14) {
            *((char*)(pBuf)) = first_char;
            nBytesRead = 1;
        } else if (index == 15) {
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = 1;
        } else {
            nBytesRead = 0;
            printf("Invalid Index in the frame index2 = %d \n", index);
            goto EXIT;
        }
    }else if (appPrvt->wbamr->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatConformance) {
      if (!fileHdrReadFlag) {
        APP_DPRINT ("Reading the file in Frame Format Conformance Mode\n");
        fileHdrReadFlag = 1;
      }

      nBytesRead = fread(pBuf,
                         1,
                         10*STD_WBAMRDEC_BUF_SIZE,
                         infile);

    }else if (appPrvt->wbamr->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatFSF){
        static const unsigned char FrameLength[] = {18, 24, 33, 37, 41, 47, 51, 59, 61,6};
        char size = 0;
        int index = 45; /* some invalid value*/
        signed char first_char;
        if (!fileHdrReadFlag) {
            APP_DPRINT ("Reading the file in MIME Mode\n");
            /* Read the file header */
            if((fgetc(infile) != 0x23) || (fgetc(infile) != 0x21) ||
               (fgetc(infile) != 0x41) || (fgetc(infile) != 0x4d) ||
               (fgetc(infile) != 0x52) || (fgetc(infile) != 0x2d) ||
               (fgetc(infile) != 0x57) || (fgetc(infile) != 0x42) ||
               (fgetc(infile) != 0x0a)) {
                APP_DPRINT("Error:  File does not have correct header\n");
            }
            fileHdrReadFlag = 1;
        }
        first_char = fgetc(infile);
        if (EOF == first_char) {
            APP_DPRINT("Got EOF character\n");
            nBytesRead = 0;
            goto EXIT;
        }
        index = (first_char>>3) & 0xf;

        if((index>=0) && (index<10))
        {
            size = FrameLength[index];
            *((OMX_S8*)(pBuf)) = first_char;
            nBytesRead = fread(((char*)(pBuf))+1, 1, size - 1, infile);
            nBytesRead += 1;
        }
        else if (index == 14) {
            *((char*)(pBuf)) = first_char;
            nBytesRead = 1;
        }
        else if (index == 15) {
            *((char*)(pBuf)) = first_char;
            nBytesRead = 1;
        }
        else {
            nBytesRead = 0;
            APP_DPRINT("Invalid Index in the frame index1 = %d \n", index);
            goto EXIT;
        }
    }
    totalRead += nBytesRead;
 EXIT:
    return nBytesRead;

}
/** Process nb amr enc buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
OMX_BOOL process_nbamr_enc(appPrivateSt* appPrvt, OMX_BUFFERHEADERTYPE *buffer){
    static OMX_BOOL eos_flag = OMX_FALSE;
    static int nread;
    static OMX_BOOL first_time = OMX_TRUE;
    static OMX_U8 NextBuffer[NBAMRENC_BUFFER_SIZE*3] = {0};

    if(first_time){
        nread = fread(buffer->pBuffer,
                  1,
                  NBAMRENC_BUFFER_SIZE,
                  infile);
        if(appPrvt->frameFormat== OMX_AUDIO_AMRFrameFormatFSF){        //MIME header
            char MimeHeader[] = {0x23, 0x21, 0x41, 0x4d, 0x52, 0x0a};
            fwrite(MimeHeader, 1, NBAMRENC_MIME_HEADER_LEN, outfile);
        }

        first_time = OMX_FALSE;
    }else{
        memcpy(buffer->pBuffer, NextBuffer,nread);
    }

    buffer->nFlags |= 0;
    buffer->nFilledLen = nread;

    nread = fread(NextBuffer, 1, NBAMRENC_BUFFER_SIZE, infile);
    if(nread < NBAMRENC_BUFFER_SIZE) {
        /*set the buffer flag*/
        buffer->nFlags |= OMX_BUFFERFLAG_EOS;
        eos_flag = OMX_TRUE;
        APP_DPRINT("EOS marked!\n");
    }
    return eos_flag;
}
/** Process wb amr enc buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
OMX_BOOL process_wbamr_enc(appPrivateSt* appPrvt, OMX_BUFFERHEADERTYPE *buffer){
    static OMX_BOOL eos_flag = OMX_FALSE;
    static int nread;
    static OMX_BOOL first_time = OMX_TRUE;
    static OMX_U8 NextBufferWB[WBAMRENC_BUFFER_SIZE*3] = {0};

    if(first_time){
        nread = fread(buffer->pBuffer,
                  1,
                  WBAMRENC_BUFFER_SIZE,
                  infile);
        if(appPrvt->frameFormat== OMX_AUDIO_AMRFrameFormatFSF){        //MIME header
            char MimeHeader[] = {0x23, 0x21, 0x41, 0x4d, 0x52, 0x2d, 0x57, 0x42, 0x0a};
            fwrite(MimeHeader, 1, WBAMRENC_MIME_HEADER_LEN, outfile);
        }

        first_time = OMX_FALSE;
    }else{
        memcpy(buffer->pBuffer, NextBufferWB,nread);
    }

    buffer->nFlags |= 0;
    buffer->nFilledLen = nread;

    nread = fread(NextBufferWB, 1, WBAMRENC_BUFFER_SIZE, infile);
    if(nread < WBAMRENC_BUFFER_SIZE) {
        /*set the buffer flag*/
        buffer->nFlags |= OMX_BUFFERFLAG_EOS;
        eos_flag = OMX_TRUE;
        APP_DPRINT("EOS marked!\n");
    }
    return eos_flag;
}
/** Process g729dec buffers
 *
 * @param appPrvt Aplication private variables
 *
 */
int process_g729(appPrivateSt* appPrvt, OMX_U8* pBuf){
    int nRead = 0;
    int dRead = 0;
    unsigned int j = 0;
    int n = 0, k = 0, m = 0;
    /* BFI + num of bits in frame + serial bitstream */
    OMX_S16 serial[G729ITU_INPUT_SIZE];
    OMX_S16 frame_type = 0;     /* G729 frame type */
    OMX_U32 nBytes = 0;     /* Number of data bytes in packet */
    OMX_U8 *packet = NULL;    /* G729 packet */
    OMX_U8 offset = 0;    /* Offset in bytes in input buffer */
    /*@TODO: Configurable parameter ?*/
    unsigned long int numPackets=1; /* packetsPerBuffer */
    unsigned long int packetLength[6];

    for(j = 0; j < numPackets; j++){      /*number of packets in input buffer */
        /* read BFI and number of bits in frame */
        nRead = fread(serial, sizeof(OMX_S16), 2 , infile);
        if(nRead != 0){
            /* Number of data bytes in packet */
            nBytes = serial[1]>>3;
            packetLength[j] = nBytes + 1;
            /* read ITU serial bitstream  */
            dRead = fread(&serial[2], sizeof(OMX_S16), serial[1], infile);
            if(dRead != serial[1]){
                APP_DPRINT("WARN: Error in input file\n");
                dRead = -1; /*error flag */
            }
            /* set frame type */
            switch(nBytes){
            case G729SPEECHPACKETSIZE:
                frame_type = G729SPEECH_FRAME_TYPE;
                break;
            case G729SIDPACKETSIZE:
                frame_type = G729SID_FRAME_TYPE;
                break;
            case G729NO_TX_FRAME_TYPE:
                frame_type = G729NO_TX_FRAME_TYPE;
                break;
            default:
                frame_type = G729ERASURE_FRAME;
            }
            if(serial[0]!= G729SYNC_WORD){  /* Untransmitted frame => Frame erasure flag */
                frame_type = G729ERASURE_FRAME;
            }
            /* Add G729 frame type header to G729 input packet */
            *((OMX_U8 *)(&pBuf[0]+offset)) = frame_type;
            /* Convert ITU format to bitstream */
            packet = (OMX_U8 *)(&pBuf[0]+offset+1);
            if(frame_type == G729SPEECH_FRAME_TYPE){
                n = 2;
                k = 0;
                while(n<SPEECH_FRAME_SIZE){
                    packet[k] = 0;
                    for(m=7;m>=0;m--){
                        serial[n] = (~(serial[n]) & 0x2)>>1;
                        packet[k] = packet[k] + (serial[n]<<m);
                        n++;
                    }
                    k++;
                }
            }
            if(frame_type == G729SID_FRAME_TYPE){
                n = 2;
                k = 0;
                while(n<G729SID_OCTET_FRAME_SIZE){
                    packet[k] = 0;
                    for(m=7;m>=0;m--){
                        serial[n] = (~(serial[n]) & 0x2)>>1;
                        packet[k] = packet[k] + (serial[n]<<m);
                        n++;
                    }
                    k++;
                }
            }
            offset = offset + nBytes + 1;
        }
        else{
            if(offset == 0){/* End of file on a dummy frame */
                APP_DPRINT("End of file on a dummy frame \n");
            }
            else{/* End of file on valid frame */
                APP_DPRINT("End of file on a valid frame \n");
            }
            j = numPackets; ;  /*packetsPerBuffer*/
        }
    }
    return dRead;
}
