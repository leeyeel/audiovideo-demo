#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "faad.h"

#define FRAME_MAX_LEN (1024*5)      /*用于存放aac中的数据*/
#define BUFFER_MAX_LEN (1024*1024)  /*用于存放从文件中读取的数据*/
#define BUFFER_PCM_MAX_LEN (5*1024*1024)  /*用于存放解码后的pcm数据*/
#define NAME_LEN    (128)

struct {
    NeAACDecHandle hDecoder;                    /*handle*/
    NeAACDecConfigurationPtr conf;
    char *aacfile;                          /*aac文件名称*/
    char *pcmfile;                          /*pcm文件名称*/
    unsigned long samplerate;                   /*采样率*/ 
    unsigned char channels;                     /*通道数*/
    unsigned char bufferAAC[BUFFER_MAX_LEN];    /*从文件中读取到的数据buffer*/
    unsigned char frame[FRAME_MAX_LEN];         /*adts每帧数据buffer,待优化*/
}aac;

//获取一帧aac数据
static int adts_get_one_frame(unsigned char *buffer, unsigned buffer_size, unsigned char *pframe, unsigned long *frame_size)
{
    unsigned long size = 0;
    int status = -1;

    if(!buffer || buffer_size < 7 || !pframe || !frame_size){
        printf("%s[%d] wrong input parameters\n", __func__, __LINE__);
        return status;
    }

    //while(buffer_size > 6 && buffer_size > size){
    while(buffer_size > 6){
        if (buffer[0] == 0xFF && (buffer[1] & 0xF6) == 0xF0){
            size |= (((buffer[3] & 0x03)) << 11);
            size |= (buffer[4] << 3);
            size |= ((buffer[5] & 0xe0) >> 5);
            status = 0;
            break;
        }
        buffer_size--;
        buffer++;
    }

    if(buffer_size < size){
        status = -1;
    }

    if(!status){
        memcpy(pframe, buffer, size);
        *frame_size = size;
    }

    return status;
}

//adts流程
static int adts_parse_process(unsigned char *buffer, unsigned buffer_size)
{
    long status = -1;
    unsigned long frame_size = 0;
    NeAACDecFrameInfo frame_info;
    unsigned char*pcm_data;

    FILE *out = fopen(aac.pcmfile,"wb");
    memset(aac.frame, 0, FRAME_MAX_LEN);
    status = adts_get_one_frame(buffer, buffer_size, aac.frame, &frame_size);
    if (status < 0){
        printf("%s[%d] adts_get_one_frame status :%ld \n", __func__, __LINE__, status);
        return -1;
    }

    status = NeAACDecInit(aac.hDecoder, aac.frame, frame_size, &aac.samplerate, &aac.channels);

    if (status < 0){
        printf("%s[%d] adts_get_one_frame status :%ld \n", __func__, __LINE__, status);
        return -1;
    }

    printf("%s[%d] samplerate[%lu], channels[%u]\n", __func__, __LINE__, aac.samplerate, aac.channels);

    while(!adts_get_one_frame(buffer, buffer_size, aac.frame, &frame_size)){
        pcm_data = (unsigned char*)NeAACDecDecode(aac.hDecoder, &frame_info, aac.frame, frame_size);
        if(frame_info.error > 0){
            printf("%s[%d] %s; size[%lu]\n",
                    __func__, __LINE__, NeAACDecGetErrorMessage(frame_info.error), frame_size);            
            return -1;
        }else if(pcm_data && frame_info.samples > 0){
            int write_len = fwrite(pcm_data, 1, frame_info.samples * frame_info.channels, out);
            if(write_len != frame_info.samples * frame_info.channels){
                printf("%s[%d] write[%d] bytes rather than %lu bytes \n", 
                        __func__, __LINE__, write_len, frame_info.samples * frame_info.channels);
                break;
            }
        }        
        buffer += frame_size;
        buffer_size -= frame_size;
    }

    NeAACDecClose(aac.hDecoder);
    fclose(out);

    return 0;
}

static int adif_parse_process(unsigned char *buffer, unsigned length)
{
    return 0;
}
 

int main(int argc, char *argv[])
{
    char *faad_id_string;
    char *faad_copyright_string;
    unsigned char status = -1;
    FILE *infile;
    unsigned long bufsizeAAC; 

    aac.aacfile = calloc(1, NAME_LEN);
    aac.pcmfile = calloc(1, NAME_LEN);

    if (argc < 3){
        printf("usage: %s [aac file] \n", argv[0]);
        aac.aacfile = "sample.aac";
        aac.pcmfile = "pcm.raw";
    }else{
        /*aac file name */
        strncpy(aac.aacfile, argv[1], NAME_LEN);
        strncpy(aac.pcmfile, argv[2], NAME_LEN);
    }

    unsigned long cap = NeAACDecGetCapabilities();
    NeAACDecGetVersion(&faad_id_string, &faad_copyright_string);
    printf(" *********** Ahead Software MPEG-4 AAC hDecoder V%s ******************\n\n", faad_id_string);
    printf("%s", faad_copyright_string);
    if (cap & FIXED_POINT_CAP){
        printf(" Fixed point version\n");
    }
    else{
        printf(" Floating point version\n");
    }

    aac.hDecoder = NeAACDecOpen();

    aac.conf = NeAACDecGetCurrentConfiguration(aac.hDecoder);
    if (!aac.conf)
	{
        printf("NeAACDecGetCurrentConfiguration() failed");
        NeAACDecClose(aac.hDecoder);
        return -1;
    }

	aac.conf->defObjectType           = LC;
	aac.conf->outputFormat            = 1;
	aac.conf->dontUpSampleImplicitSBR = 1;
    status = NeAACDecSetConfiguration(aac.hDecoder, aac.conf);
    if(status != 1){
        printf("NeAACDecSetConfiguration error, status:%d\n", status);
        return -1;
    }

    infile = fopen(aac.aacfile, "rb");
    if(!infile){
        printf("%s[%d] open %s failed\n", __func__, __LINE__, aac.aacfile);
        return -1;
    }

    bufsizeAAC = fread(aac.bufferAAC, 1, BUFFER_MAX_LEN, infile);

    if (bufsizeAAC < 4){
        printf("%s[%d] the size of aac file is:%ld \n", __func__, __LINE__, bufsizeAAC);
        return -1;
    }
    if (aac.bufferAAC[0] == 'A' && aac.bufferAAC[1] == 'D' && aac.bufferAAC[2] == 'I' && aac.bufferAAC[3] == 'F'){
        printf(" %s is encoded with ADIF format\n", aac.aacfile);
        adif_parse_process(aac.bufferAAC, bufsizeAAC);
    }else if ((aac.bufferAAC[0] == 0xff) && (aac.bufferAAC[1] & 0xF6) == 0xF0){
        printf(" %s is encoded with ADTS format\n", aac.aacfile);
        adts_parse_process(aac.bufferAAC, bufsizeAAC);
    }else{
        printf(" unrecognized aac format:%x,%x,%x,%x\n", 
                aac.bufferAAC[0], aac.bufferAAC[1], aac.bufferAAC[2], aac.bufferAAC[3]);
        fclose(infile);
        return -1;
    }
    fclose(infile);

    return 0;
}
