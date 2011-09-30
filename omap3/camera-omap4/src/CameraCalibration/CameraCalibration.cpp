#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "OMXCameraAdapter.h"

#include "TICameraParameters.h"
#include "CameraProperties.h"

#define SENSOR_EEPROM_SIZE (256)

// Offset of CameraID read from OTP EEPROM data:
#define OTP_BYTE_CAMERA_ID (0x28) // Module Serial Number - 4 bytes (uint32)


#define CRC_CHECK_SUCCESS (0) // CRC check is successful
#define CRC_CHECK_FAILED  (1) // CRC check failed


using namespace android;

const uint8_t header_dcc[] = {
    0x0F, 0x04, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC5, 0x0A, 0x65, 0x22,
    0x6D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x46, 0x06, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};



/* Input options variables */
int         options_help = 0;
char*       options_eeprom_out_file = NULL;
uint16_t    options_col_temp = 0;
int         options_dbg_mode = 0;
char*       options_raw_out_file = NULL;
int         options_overwrite = 0;


int32_t *CamCalPrvbufs, *CamCalImgbufs;

int CamCalRet = 0;

int prv_width, prv_height;
int img_width, img_height;

/* Classes */
OMXCameraAdapter::BuffersDescriptor buffdesc;
sp<MemoryManager> CamCalMemoryManager;
BufferProvider* CamCalBufProvider;
CameraParameters *CamCalCamparam;
FrameProvider *CamCalFrameProvider, *CamCalPrvFrameProvider;
CameraFrame* CamCalFrame;
Semaphore *CamCalCaptureSem, *CamCalPreviewSem;
static OMXCameraAdapter *CamCalOMXCameraAdapter;

/* Wait that much frames for the Auto Exposure to converge */
unsigned int WaitPreviewFrames;

/* Eorror status */
int CamCalError;

uint32_t CameraID;
uint32_t CameraID_read_from_file;
uint8_t  pOTPEepromData[SENSOR_EEPROM_SIZE];

/* file types defines */
#define FILE_SAVE_TYPE_DEBUG_IMAGE 0x0
#define FILE_SAVE_TYPE_EEPROM_DATA 0x1

/* Error status defines */
#define CAMCAL_ERR_NOERROR                  0 // No error, application is executed successfully
#define CAMCAL_ERR_FILE                     1 // Error creating or writing in the calibration file
#define CAMCAL_ERR_CAMERA_ERROR             2 // Error starting the camera, configuring camera, taking picture, or error when initializing Ducati
#define CAMCAL_ERR_CAMERA_ID_INVALID        3 // Camera ID from the OTP file does not match the camera ID written in the calibration file
#define CAMCAL_ERR_CAMERA_OTP_ERROR         4 // error reading OTP data
#define CAMCAL_ERR_CALIBRATION_LSC_RATIO    5 // Error, LCS ration between center and corners is out of range
#define CAMCAL_ERR_CALIBRATION_SENS_RATIO   6 // Error, ration between R, G, B in the center is out of range
#define CAMCAL_ERR_CALIBRATION_DATA_DARK    7 // Error from eeprom parse
#define CAMCAL_ERR_CALIBRATION_DATA_BRIGHT  8 // Error from eeprom parse
#define CAMCAL_ERR_CALIBRATION_DATA_CONVEX  9 // Error from eeprom parse

const char * error_code_strings[]=
{
"CAMCAL_ERR_NOERROR 0 - No error, application is executed successfully",
"CAMCAL_ERR_FILE 1 - Error creating or writing in the calibration file",
"CAMCAL_ERR_CAMERA_ERROR 2 - Error starting the camera, configuring camera, taking picture, or error when initializing Ducati",
"CAMCAL_ERR_CAMERA_ID_INVALID 3 - Camera ID from the OTP file does not match the camera ID written in the calibration file",
"CAMCAL_ERR_CAMERA_OTP_ERROR 4 - error reading OTP data",
"CAMCAL_ERR_CALIBRATION_LSC_RATIO 5 - Error, LCS ration between center and corners is out of range",
"CAMCAL_ERR_CALIBRATION_SENS_RATIO 6 - Error, ration between R, G, B in the center is out of range",
"CAMCAL_ERR_CALIBRATION_DATA_DARK  7 - Error from eeprom parse",
"CAMCAL_ERR_CALIBRATION_DATA_BRIGHT 8 - Error from eeprom parse",
"CAMCAL_ERR_CALIBRATION_DATA_CONVEX  9 - Error from eeprom parse",
};

/* EEPROM parse defines */
#define EEPROM_ELEMENTS (9*7)

/* Calibration EV compensation - to reach needed target level */
#define CALIBRATE_EV_COMPENSATION (7)

/* multiplied by 3 for each color*/
uint16_t eeprom_buff[EEPROM_ELEMENTS * 4];

#define PIX_VAL_MIN             (50)  // PIX_VAL_MAX/20
#define PIX_VAL_MAX             (1000) // 0.9*2^10 - for 10 bit sensor
#define CENT2CORNER_RATIO_MAX   (6)

#define MIN_RATIO_BG (64)  // 0.25*256
#define MAX_RATIO_BG (512) // 2*256

#define MIN_RATIO_RG (51)  // 0.2*256
#define MAX_RATIO_RG (768) // 3*256


/*
#define CALC_EEPROM_RET_OK          (0)
#define CALC_EEPROM_RET_TOO_DARK    (1)
#define CALC_EEPROM_RET_TOO_BRIGHT  (2)
#define CALC_EEPROM_RET_TOO_CONVEX  (3)*/

int parse_eeprom_data(uint16_t *img_buff, uint32_t line_size_bytes, uint16_t
*output_buff)
{
    /*
    All sizes and offsets couples XxY below correspond to horizontal x
vertical
    respectively
    Assumes raw image 8Mpix 3280x2464 with active area (which will be
encoded)
    3264x2448
    The raw image is assumed to start with B pixel, 16bits-per-pixel,
10bit pixel
    alligned bit0 to bit0
    9x7 zones 32x32 pixels each are averaged to form 9x7 average values of
R,G,B
    upper-left corner of upper-left zone is offset from the RAW image UL
corner
    by 8x10 pixels
    distance between UL pixels of adjacent diagonal zones is 404x402
    The R,G,B average 10bit values are saved in this order for all zones
    one-by-one from left to right then from top to bottom
    */

    int ee_row, ee_col;
    uint16_t *p_img;
    uint16_t *p_out;
    int pax_row, pax_col;
    uint32_t r_sum;
    uint32_t gr_sum;
    uint32_t b_sum;
    uint32_t gb_sum;
    int ret;

    CAMHAL_LOGEA( "Enter");

    p_out = output_buff;
    ret = CAMCAL_ERR_NOERROR;

    CAMHAL_LOGEB("line_size_bytes=%ul", line_size_bytes);

    for(ee_row = 0; ee_row<7; ee_row++){
        for(ee_col = 0; ee_col<9; ee_col++){
            p_img = img_buff;
            p_img += (ee_row*400 + 2 + 2) * line_size_bytes/2;
            p_img += ee_col*402 + 0 + 4;
            r_sum = 0;
            gr_sum = 0;
            b_sum = 0;
            gb_sum = 0;

            CAMHAL_LOGEB("ee_row=%d ee_col=%d", \
                                ee_row, ee_col);

            /* IMX046 color pattern = RGGB*/
            for(pax_row = 0; pax_row<32; pax_row+=2){
                /* first row is BGBGBG */
                for(pax_col = 0; pax_col<32; pax_col+=2){
                    b_sum += *p_img++;
                    gb_sum += *p_img++;
                }
                p_img += line_size_bytes/2 - 32;
                /* second row is GRGRGRGR */
                for(pax_col = 0; pax_col<32; pax_col+=2){
                    gr_sum += *p_img++;
                    r_sum += *p_img++;
                }
                p_img += line_size_bytes/2 - 32;
            }

            r_sum = (r_sum + (32*32/4)/2) / (32*32/4);
            gr_sum = (gr_sum + (32*32/4)/2) / (32*32/4);
            b_sum = (b_sum + (32*32/4)/2) / (32*32/4);
            gb_sum = (gb_sum + (32*32/4)/2) / (32*32/4);

            CAMHAL_LOGEB("r_sum=%d gr_sum=%d b_sum=%d gb_sum=%d", \
                                        r_sum, gr_sum, b_sum, gb_sum);

            if((r_sum < PIX_VAL_MIN) ||
               (gr_sum < PIX_VAL_MIN) ||
               (b_sum < PIX_VAL_MIN) ||
               (gb_sum < PIX_VAL_MIN)) {
                CAMHAL_LOGEA("CAMCAL_ERR_CALIBRATION_DATA_DARK");
                ret = CAMCAL_ERR_CALIBRATION_DATA_DARK;
                goto fail;
            }
            if((r_sum > PIX_VAL_MAX) ||
               (gr_sum > PIX_VAL_MAX) ||
               (b_sum > PIX_VAL_MAX) ||
               (gb_sum > PIX_VAL_MAX)) {
                CAMHAL_LOGEA("CAMCAL_ERR_CALIBRATION_DATA_BRIGHT");
                ret = CAMCAL_ERR_CALIBRATION_DATA_BRIGHT;
                goto fail;
            }

            *p_out++ = r_sum;
            *p_out++ = gr_sum;
            *p_out++ = b_sum;
            *p_out++ = gb_sum;
        }
    }

    CAMHAL_LOGEB("Center brightness: r_sum=%d gr_sum=%d b_sum=%d gb_sum=%d", \
                            output_buff[4*(4*9+4)+0],
                            output_buff[4*(4*9+4)+1],
                            output_buff[4*(4*9+4)+2],
                            output_buff[4*(4*9+4)+3]);

    if (
        /* center to upper-left R,Gr,B,Gb */
        ((output_buff[4*(4*9+4)+0] / output_buff[4*0+0]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+1] / output_buff[4*0+1]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+2] / output_buff[4*0+2]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+3] / output_buff[4*0+3]) >
            CENT2CORNER_RATIO_MAX) ||
        /* center to upper-righ R,Gr,B,Gb */
        ((output_buff[4*(4*9+4)+0] / output_buff[4*8+0]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+1] / output_buff[4*8+1]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+2] / output_buff[4*8+2]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+3] / output_buff[4*8+3]) >
            CENT2CORNER_RATIO_MAX) ||
        /* center to lower-left R,Gr,B,Gb */
        ((output_buff[4*(4*9+4)+0] / output_buff[4*6*9+0]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+1] / output_buff[4*6*9+1]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+2] / output_buff[4*6*9+2]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+3] / output_buff[4*6*9+2]) >
            CENT2CORNER_RATIO_MAX) ||
        /* center to lower-righ R,Gr,B,Gb */
        ((output_buff[4*(4*9+4)+0] / output_buff[4*(6*9+8)+0]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+1] / output_buff[4*(6*9+8)+1]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+2] / output_buff[4*(6*9+8)+2]) >
            CENT2CORNER_RATIO_MAX) ||
        ((output_buff[4*(4*9+4)+3] / output_buff[4*(6*9+8)+3]) >
            CENT2CORNER_RATIO_MAX)
        ){
        CAMHAL_LOGEA("CAMCAL_ERR_CALIBRATION_LSC_RATIO");
        ret = CAMCAL_ERR_CALIBRATION_LSC_RATIO;
        goto fail;
    }

    r_sum = output_buff[4*(4*9+4)+0];//R
    gr_sum = (output_buff[4*(4*9+4)+1] + output_buff[4*(4*9+4)+3])/2; // (Gr+Gb)/ 2
    b_sum = output_buff[4*(4*9+4)+2];//B
    if( (((r_sum*256)/gr_sum) < MIN_RATIO_RG) ||(((r_sum*256)/gr_sum) > MAX_RATIO_RG) ||
        (((b_sum*256)/gr_sum) < MIN_RATIO_BG) ||(((b_sum*256)/gr_sum) > MAX_RATIO_BG)
      ){
       CAMHAL_LOGEA("CAMCAL_ERR_CALIBRATION_SENS_RATIO");
       ret = CAMCAL_ERR_CALIBRATION_SENS_RATIO;
       goto fail;
    }

 fail:
    CAMHAL_LOGEB( "Exit %s", (!ret) ? "ok" : "fail");
    return ret;
}

uint16_t calcEepromCRC(uint8_t *pData, uint32_t nStart, uint32_t nEnd)
{
    uint16_t crc = 0x0000;
    uint32_t i,j;
    uint32_t tmp;

    for(i = nStart; i <= nEnd+2; i ++) {
        if(i > nEnd) {
            tmp = 0;
        } else {
            tmp = pData[i] & 0xff;
        }
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x8005;
            } else {
                crc = (crc << 1);
            }
            if(tmp & 0x80) {
                tmp = tmp << 1;
                crc = crc ^ 0x0001;
            } else {
                tmp = tmp << 1;
            }
        }
    }

    return crc;
}


/**
*  Check CRC sum of OTP EEPROM data.
*
*  pDestOTPEepromData : pointer to the EEPROM data array.
*  nSize              : size of array
*
*  returns:
*           CRC_CHECK_SUCCESS (0) - CRC check is successful;
*           CRC_CHECK_FAILED  (1) - CRC check failed.
*/
int CRCCheck(uint8_t * pOTPEepromData, uint32_t size)
{
    uint16_t crc_eeprom = 0;
    uint16_t crc_calculated = 0;
    int ret;

    crc_eeprom = ((pOTPEepromData[0xFE]<<8)|
                  (pOTPEepromData[0xFF]));

    crc_calculated = calcEepromCRC(pOTPEepromData, 0x0, 0x4b);
    CAMHAL_LOGDB("CRC calced range1[0x0,0x4b]: 0x%04x", crc_calculated);

    if (crc_eeprom != crc_calculated)
    {
        // sharp changed the eeprom range ...
        crc_calculated = calcEepromCRC(pOTPEepromData, 0x10, 0x4b);
        CAMHAL_LOGDB("CRC calced range2[0x10,0x4b]: 0x%04x", crc_calculated);

        if (crc_eeprom == crc_calculated)
        {
            CAMHAL_LOGDA("CRC check succeded in calculation range2[0x10,0x4b]");
            ret = CRC_CHECK_SUCCESS;
        }
        else
        {
            CAMHAL_LOGEA("CRC check failed in both calculation ranges: [0x00,0x4b] and [0x10,0x4b]");
            ret = CRC_CHECK_FAILED;
        }
    }
    else
    {
        CAMHAL_LOGDA("CRC check succeeded in calculation range1[0x0,0x4b]");
        ret = CRC_CHECK_SUCCESS;
    }

    return ret;
}

int CamCalSaveFile(char *filename , void * buffer, size_t lenght, int file_save_type ) {
    unsigned char   *buff = NULL;
    int             size;
    int             fd = -1;
    int res = NO_ERROR;

    // Read CameraID from EEPROM
    res = CamCalOMXCameraAdapter->getOTPEeprom(pOTPEepromData, SENSOR_EEPROM_SIZE);
    if (NO_ERROR != res) {
        CAMHAL_LOGEA("OTP_ERROR: OMXCameraAdapter::getOTPEeprom() failed. Sensor OTP EEPROM data is not available. Exit.");
        goto err_fail_OTP_error;
    }
    res = CRCCheck(pOTPEepromData, SENSOR_EEPROM_SIZE);
    if (CRC_CHECK_SUCCESS != res)
        {
        CAMHAL_LOGEA("OTP_ERROR: CRC check failed.");
        goto err_fail_OTP_error;
        }
    CameraID = ((pOTPEepromData[OTP_BYTE_CAMERA_ID]<<24)|
                (pOTPEepromData[OTP_BYTE_CAMERA_ID+1]<<16)|
                (pOTPEepromData[OTP_BYTE_CAMERA_ID+2]<<8)|
                (pOTPEepromData[OTP_BYTE_CAMERA_ID+3]));
    CAMHAL_LOGDB("CameraID read from sensor OTP EEPROM is: 0x%08x", CameraID);

    if (buffer == NULL)
        goto err_fail_file;


    if(FILE_SAVE_TYPE_EEPROM_DATA == file_save_type){
         if(options_overwrite!=0){
            /* Overwrite file. Write dcc header and 4 bytes of  CameraID at start */
            fd = open(filename, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
                if(fd < 0) {
                    LOGE("Unable to open file %s: %s", filename, strerror(fd));
                    goto err_fail_file;
                }
            write(fd, &header_dcc, sizeof(header_dcc));
            write(fd, &CameraID, sizeof(CameraID));
         }
         else{
             fd = open(filename, O_APPEND| O_RDWR | O_SYNC , 0777);

             if(fd < 0){
                /* The file doesn't exist. Write dcc header and 4 bytes of  CameraID at start */
                fd = open(filename, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
                    if(fd < 0) {
                        LOGE("Unable to open file %s: %s", filename, strerror(fd));
                        goto err_fail_file;
                    }
                write(fd, &header_dcc, sizeof(header_dcc));
                write(fd, &CameraID, sizeof(CameraID));
             } else {
                /* Verifying CameraID from output file */
                lseek(fd, sizeof(header_dcc), SEEK_SET);
                read(fd, &CameraID_read_from_file,sizeof(CameraID_read_from_file));

                if(CameraID != CameraID_read_from_file) {
                    /* CameraID does not match. Leaving output file unchanged. */
                    goto err_fail_cameraID;
                }

                /* Verified CameraID. The same file will be used to append calibration data. */
                lseek(fd, 0, SEEK_END);
             }
         }
         /* Write 2 bytes of color temperature */
         write(fd, &options_col_temp, sizeof(options_col_temp));

    } else if(FILE_SAVE_TYPE_DEBUG_IMAGE== file_save_type){
        fd = open(filename, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    } else {
        LOGE("Wrong Saving Type");
        goto err_fail_file;
    }

    if(fd < 0) {
        LOGE("Unable to open file %s: %s", filename, strerror(fd));
        goto err_fail_file;
    }

    size = lenght;
    if (size <= 0) {
        LOGE("IMemory object is of zero size");
        goto err_fail_file;
    }

    buff = (unsigned char *)buffer;
    if (!buff) {
        LOGE("Buffer pointer is invalid");
        goto err_fail_file;
    }

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", filename, errno, strerror(errno));

    printf("%s: buffer=%08X, size=%d saved in %s \n",
           __FUNCTION__, (int)buff, size, filename);

    if (fd >= 0)
        close(fd);

    return CAMCAL_ERR_NOERROR;

err_fail_OTP_error:
    if (fd >= 0)
        close(fd);
    return CAMCAL_ERR_CAMERA_OTP_ERROR;

err_fail_cameraID:
    if (fd >= 0)
        close(fd);
    return CAMCAL_ERR_CAMERA_ID_INVALID;

err_fail_file:
    if (fd >= 0)
        close(fd);
    return CAMCAL_ERR_FILE;
}


void CamCalCallbackPrv (CameraFrame *cameraFrame){
    CAMHAL_LOGEB("WaitPreviewFrames %d cameraFrame->mBuffer 0x%08Xu cameraFrame->FrameType %d",
                    WaitPreviewFrames, (uint32_t) cameraFrame->mBuffer, cameraFrame->mFrameType);
    WaitPreviewFrames--;
    CamCalOMXCameraAdapter->returnFrame((void*)cameraFrame->mBuffer, (CameraFrame::FrameType) cameraFrame->mFrameType);

    if(WaitPreviewFrames == 0){
        CamCalPrvFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
        CamCalPreviewSem->Signal();
    }

}

void CamCalCallbackImg (CameraFrame *cameraFrame)
{
    int RetL;
    if(options_dbg_mode){
        CamCalRet = CamCalSaveFile(options_raw_out_file, cameraFrame->mBuffer, cameraFrame->mLength, FILE_SAVE_TYPE_DEBUG_IMAGE);
        if(CamCalRet != CAMCAL_ERR_NOERROR)
            goto out;
    }

    CamCalRet = parse_eeprom_data((uint16_t*)cameraFrame->mBuffer, img_width * 2, eeprom_buff);
    if(CamCalRet != CAMCAL_ERR_NOERROR)
    {
        CAMHAL_LOGEB("parse_eepron_data returned error: %d", CamCalRet);
    }
    else
    {
        RetL = CamCalSaveFile(options_eeprom_out_file, eeprom_buff, sizeof(eeprom_buff), FILE_SAVE_TYPE_EEPROM_DATA);
        if(RetL != CAMCAL_ERR_NOERROR)
        {
            CamCalRet = RetL;
            CAMHAL_LOGEB("CamCalSaveFile returned error: %d", CamCalRet);
            goto out;
        }
    }

    CAMHAL_LOGEB("CamCalCallback: cameraFrame->mBuffer 0x%x  cameraFrame->FrameType %d",
                    (uint32_t) cameraFrame->mBuffer, cameraFrame->mFrameType);

out:
    CamCalCaptureSem->Signal();
}

void CamCalPrintUsage (void){
printf("\nUsage:\n");
printf("CameraCal -o <output_file_name> -k <colour_temperature> [-d <file_name>] [-r]\n\n");
printf("Options:\n");
printf("-o <output file name> - specifies the name of the file to store calibration data. \n \
\t In case the file does not exist it is created and camera ID is added at the beginning. \n \
\t In case the file already exist will append the new section with colour temperature and calibration data\n\n");
printf("-k <colour temperature> setting the colour temperature as integer value in Kelvin. Example: 3200, 6500 etc.\n\n");
printf("-d - enables debug mode - will store raw image using file name set\n\n");
printf("-r - restart calibration - overwrite existing output file, given by -o option, instead of appending\n\n");
}


static int CamCalParseCmdline(int argc, char **argv)
{
    int opt;
    int err = 0;

    #define OPTION_DEBUG_MODE          'd'
    #define OPTION_COLOR_TEMPERATURE   'k'
    #define OPTION_OUTPUT_FILE         'o'
    #define OPTION_OVERWRITE           'r'
    #define OPTION_HELP                '?'


    /* parse options */
    while (1) {

        opt = getopt(argc, argv, "?:o:k:d:r");

        switch(opt) {
        case -1:
            goto break_while;
        case OPTION_DEBUG_MODE:
            options_dbg_mode = 1;
            options_raw_out_file = optarg;
            break;
        case OPTION_COLOR_TEMPERATURE:
            options_col_temp = atoi(optarg);
            break;
        case OPTION_OUTPUT_FILE:
            options_eeprom_out_file = optarg;
            break;
        case OPTION_OVERWRITE:
            options_overwrite = 1;
            break;
        case OPTION_HELP:
            options_help = 1;
            goto out;
        default:
            err = -1; /* don't allow unrecognized options */
        }
    }

break_while:

    if (err) {
        return err;
    }

    if(!options_col_temp){
        printf("Please enter color temperature with -k option. \n");
        err = -1;
        goto err_out;
    }

    if(options_eeprom_out_file == NULL){
        printf("Please enter eeprom output file with -o option. \n");
        err = -1;
        goto err_out;
    }

    if(options_raw_out_file == NULL){
        printf("Please enter raw output file with -d option. \n");
        err = -1;
        goto err_out;
    }

    if (optind != argc) {
        err = -1;
        goto err_out; /* don't allow extraneous arguments */
    }

out:
    return 0;
err_out:
    return err;
}

int main(int argc, char *argv[])
{

    int PreviewLength,ImageLength;
    size_t ImageLength_sizet;

    int ret;

    CamCalRet = CAMCAL_ERR_NOERROR;

    CamCalPrvbufs = NULL;
    CamCalImgbufs = NULL;
    memset(pOTPEepromData, 0x0, SENSOR_EEPROM_SIZE);

    ret = CamCalParseCmdline(argc, argv);

    if (ret < 0 || options_help){
        CamCalPrintUsage();
        return 0;
    }

    CamCalOMXCameraAdapter = new OMXCameraAdapter();
    CamCalMemoryManager = new MemoryManager();
    CamCalCamparam = new CameraParameters();
    CamCalFrame = new CameraFrame;
    CamCalCaptureSem = new Semaphore();
    CamCalPreviewSem = new Semaphore();
    WaitPreviewFrames = 40;

    CamCalCaptureSem->Create(0);
    CamCalPreviewSem->Create(0);

    prv_width = 640;
    prv_height= 480;

    img_width = 3264;
    img_height = 2448;

    CamCalOMXCameraAdapter->initialize(0);

    /* Set up preview and capture parameters */
    CamCalCamparam->setPreviewSize(prv_width,prv_height);
    CamCalCamparam->setPreviewFormat("yuv420sp");
    CamCalCamparam->setPreviewFrameRate(30);
    CamCalCamparam->set(TICameraParameters::KEY_MINFRAMERATE, 30);
    CamCalCamparam->set(TICameraParameters::KEY_MAXFRAMERATE, 30);
    CamCalCamparam->setPictureSize(img_width, img_height);
    CamCalCamparam->setPictureFormat("raw");
    CamCalCamparam->set(TICameraParameters::KEY_EXPOSURE_MODE, TICameraParameters::EXPOSURE_MODE_SPOTLIGHT);
    CamCalCamparam->set(CameraParameters::KEY_EXPOSURE_COMPENSATION, CALIBRATE_EV_COMPENSATION);
    CamCalCamparam->set(CameraParameters::KEY_FLASH_MODE, "off");
    CamCalOMXCameraAdapter->setParameters(*CamCalCamparam);

    CamCalFrameProvider = new FrameProvider(CamCalOMXCameraAdapter, NULL, CamCalCallbackImg);
    CamCalFrameProvider->enableFrameNotification(CameraFrame::RAW_FRAME);

    CamCalPrvFrameProvider = new FrameProvider(CamCalOMXCameraAdapter, NULL, CamCalCallbackPrv);
    CamCalPrvFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);

    CamCalBufProvider = (BufferProvider*) CamCalMemoryManager.get();

    PreviewLength = 0; // 0 means allocate 2D buffers - for preview
    CamCalPrvbufs = (int32_t *) CamCalBufProvider->allocateBuffer(prv_width, prv_height, "yuv420sp", PreviewLength, 8);
    DBGUTILS_LOGDB("CameraCal debug: allocateBuffer() returned CamCalPrvbufs = %p, PreviewLength = %d\n",
        CamCalPrvbufs, PreviewLength);
    if (NULL == CamCalPrvbufs){
        CAMHAL_LOGEA("CameraCal error: allocateBuffer() returned CamCalPrvbufs NULL! CameraCal exit.\n");
        printf("CameraCal error: allocateBuffer() returned CamCalPrvbufs NULL! CameraCal exit.\n");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }
    buffdesc.mBuffers = CamCalPrvbufs;
    buffdesc.mLength = PreviewLength;
    buffdesc.mCount = 8;

    ret = CamCalOMXCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS,CameraAdapter::CAMERA_PREVIEW,(int) &buffdesc);
    if(ret != NO_ERROR){
        CAMHAL_LOGEA("CameraCal error: CAMERA_USE_BUFFERS command failed");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }

    ret = CamCalOMXCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_PREVIEW);
    if(ret != NO_ERROR){
        CAMHAL_LOGEA("CameraCal error: CAMERA_START_PREVIEW command failed");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }

    /* Wait few preview frames for the Autoexposure */
    CamCalPreviewSem->Wait();

    CamCalOMXCameraAdapter->getPictureBufferSize(ImageLength_sizet, 1);
    ImageLength = ImageLength_sizet;
    CamCalImgbufs = (int32_t *) CamCalBufProvider->allocateBuffer(0, 0, "raw", ImageLength, 1);
    DBGUTILS_LOGDB("CameraCal debug: allocateBuffer() returned CamCalImgbufs = %p, ImageLength = %d\n",
        CamCalImgbufs, ImageLength);
    if (NULL == CamCalImgbufs){
        CAMHAL_LOGEA("CameraCal error: allocateBuffer() returned CamCalImgbufs NULL! CameraCal exit.\n");
        printf("CameraCal error: allocateBuffer() returned CamCalImgbufs NULL! CameraCal exit.\n");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }

    /* Set up the image buffer and do capture*/
    buffdesc.mBuffers = CamCalImgbufs;
    buffdesc.mLength = ImageLength_sizet;
    buffdesc.mCount = 1;
    ret = CamCalOMXCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS,CameraAdapter::CAMERA_IMAGE_CAPTURE,(int) &buffdesc);
    if(ret != NO_ERROR){
        CAMHAL_LOGEA("CameraCal error: CAMERA_USE_BUFFERS command failed");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }

    ret = CamCalOMXCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE);
    if(ret != NO_ERROR){
        CAMHAL_LOGEA("CameraCal error: CAMERA_START_IMAGE_CAPTURE command failed");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }


    /* Wait for the capture to finish */
    CamCalCaptureSem->Wait();

    ret = CamCalOMXCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);
    if(ret != NO_ERROR){
        CAMHAL_LOGEA("CameraCal error: CAMERA_STOP_IMAGE_CAPTURE command failed");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }

    ret = CamCalOMXCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_PREVIEW);
    if(ret != NO_ERROR){
        CAMHAL_LOGEA("CameraCal error: CAMERA_STOP_PREVIEW command failed");
        CamCalRet = CAMCAL_ERR_CAMERA_ERROR;
        goto camcal_is_done;
    }


camcal_is_done:

    /* Free the used buffers */
    CamCalBufProvider->freeBuffer(CamCalImgbufs);
    CamCalBufProvider->freeBuffer(CamCalPrvbufs);

    delete CamCalOMXCameraAdapter;
    delete CamCalFrame;
    delete CamCalCaptureSem;
    delete CamCalPreviewSem;
    delete CamCalCamparam;
    delete CamCalFrameProvider;
    CamCalMemoryManager.clear();

    CAMHAL_LOGEB("CameraCal exited with code: %d : \n %s", CamCalRet, error_code_strings[CamCalRet]);
    return CamCalRet;
}

