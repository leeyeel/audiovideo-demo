#include <stdio.h>
#include "faad.h"
#include <string.h>

#define FRAME_MAX_LEN (1024*5)      /*用于存放aac中的数据*/
#define BUFFER_MAX_LEN (1024*1024)  /*用于存放从文件中读取的数据*/
#define BUFFER_PCM_MAX_LEN (5*1024*1024)  /*用于存放解码后的pcm数据*/

static unsigned char bufferAAC[BUFFER_MAX_LEN] = {0};
static unsigned char frame[FRAME_MAX_LEN] = {0};
static unsigned char bufferPCM[BUFFER_PCM_MAX_LEN] = {0};


/// buffer   :读取的原始数据
/// buf_size :读的数据长度
/// pframe     :返回的frame   
/// framesize:frame长度
int get_one_ADTS_frame(unsigned char* buffer, size_t buf_size, unsigned char* pframe ,size_t* framesize)
{
    size_t size = 0;
    if(!buffer || !pframe || !framesize)
        return -1;

    while(buf_size >= 7){
        //帧同步，需要12个1,即0xFFF。若满足表示是个固定头信息
        if ((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0)){
            size |= (((buffer[3] & 0x03)) << 11);//high 2 bit
            size |= (buffer[4] << 3);//middle 8 bit
            size |= ((buffer[5] & 0xe0) >> 5);//low 3bit
            int channels = ((buffer[2] & 0x01) << 2 )| ( buffer[3] >> 6);
            printf("size=%d, channels=%d\r\n", (int)size, channels);
            break;
        }
        --buf_size;
        ++buffer;
    }

    if(buf_size < size){
        return -1;
    }

    memcpy(pframe, buffer, size);
    *framesize = size;

    return 0;
}
 

int main(int argc, char *argv[])
{
    char *faad_id_string;
    char *faad_copyright_string;
    char *aacfile;
    NeAACDecHandle hDecoder;
    NeAACDecConfigurationPtr config;
    NeAACDecFrameInfo frameInfo;
    unsigned char status = -1;
    size_t size = 0;
    unsigned char* pcm_data = NULL;
    NeAACDecFrameInfo frame_info;
    
    if (argc != 2){
        printf("usage: %s [aac file] \n", argv[0]);
        return -1;
    }
    aacfile = argv[1];

    unsigned long cap = NeAACDecGetCapabilities();

    NeAACDecGetVersion(&faad_id_string, &faad_copyright_string);
    printf(" *********** Ahead Software MPEG-4 AAC hDecoder V%s ******************\n\n", faad_id_string);
    printf("%s", faad_copyright_string);
    if (cap & FIXED_POINT_CAP){
        printf("Fixed point version\n");
    }
    else{
        printf("Floating point version\n");
    }

    FILE *infile = fopen(aacfile, "rb");
    FILE *outfile = fopen("out.pcm", "wb");

    hDecoder = NeAACDecOpen();
    config = NeAACDecGetCurrentConfiguration(hDecoder);
	config->defSampleRate = 8000; //real samplerate/2
    config->defObjectType = LC;
    config->outputFormat = FAAD_FMT_16BIT; //
    config->dontUpSampleImplicitSBR = 1;
    status = NeAACDecSetConfiguration(hDecoder, config);
    if(!status){
        printf("NeAACDecSetConfiguration error\n");
    }

    //memset(bufferAAC,0, BUFFER_MAX_LEN);
    int bufsizeAAC = fread(bufferAAC, 1, BUFFER_MAX_LEN, infile);

    if (bufsizeAAC < 4){
        printf("the size of aac file is:%d \n", bufsizeAAC);
        return -1;
    }

    if (bufferAAC[0] == 'A' && bufferAAC[1] == 'D' && bufferAAC[2] == 'I' &&bufferAAC[3] == 'F'){
        printf("aac file is encoded with ADIF format\n");
        return -1;
    }else if (bufferAAC[0] == 0xFF && bufferAAC[1] >> 4 == 0xF){
        printf("aac file is encoded with ADTS format\n");
    }else{
        printf("unrecognized aac format:%x,%x,%x,%x\n", bufferAAC[0], bufferAAC[1], bufferAAC[2], bufferAAC[3]);
        return -1;
    }

    while(get_one_ADTS_frame(bufferAAC, bufsizeAAC, frame, &size) == 0){
        //decode ADTS frame
        pcm_data = (unsigned char*)NeAACDecDecode(hDecoder, &frame_info, frame, size);

        if(frame_info.error > 0){
            printf("[%d]%s\n",__LINE__, NeAACDecGetErrorMessage(frame_info.error));            
            return -1;
        }
        else if(pcm_data && frame_info.samples > 0){
            printf("frames info: bytesconsumed %lu, channels %u, header_type %u\
                    object_type %u, samples %lu, samplerate %lu\n", 
                    frame_info.bytesconsumed, 
                    frame_info.channels, frame_info.header_type, 
                    frame_info.object_type, frame_info.samples, 
                    frame_info.samplerate);

            size_t buf_sizePCM = frame_info.samples * frame_info.channels;
            memcpy(bufferPCM, pcm_data, buf_sizePCM);
            fwrite(pcm_data, 1, buf_sizePCM, outfile);
        }        
        bufsizeAAC -= size;
    }

    fclose(outfile);
    fclose(infile);

    return 0;
}

