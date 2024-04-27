#ifndef SOUND_CHEF_ENCODER
#define SOUND_CHEF_ENCODER

#include "sound_chef/sound_chef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum SC_ENCODING_FORMAT;
    typedef struct SC_ENCODER_CONFIG SC_ENCODER_CONFIG;
    typedef struct SC_ENCODER SC_ENCODER;

	typedef enum SC_ENCODING_FORMAT
    {
        SC_ENCODING_FORMAT_WAV,
        SC_ENCODING_FORMAT_VORBIS,
        SC_ENCODING_FORMAT_OPUS
    } SC_ENCODING_FORMAT;

    struct SC_ENCODER_CONFIG
    {
        ma_encoder_config m_baseConfig;
        ma_uint8 m_quality;
    };

    struct SC_ENCODER
    {
        SC_ENCODER_CONFIG m_config;
        ma_data_source_base* m_dataSource;
    };

    SC_ENCODER_CONFIG SC_API SC_Encoder_Config_Init(
        ma_encoding_format encodingFormat, ma_format format, ma_uint32 channels, ma_uint32 sampleRate, ma_uint8 quality)
    {
    }


    SC_RESULT SC_API SC_Encoder_Init(ma_encoder_write_proc onWrite,
                                     ma_encoder_seek_proc onSeek,
                                     void* userData,
                                     const SC_ENCODER_CONFIG* config,
                                     SC_ENCODER* encoder);

    SC_RESULT SC_API SC_Encoder_WriteFrames() {}

    SC_RESULT SC_API SC_Encoder_Uninit(SC_ENCODER* encoder) {}

#ifdef __cplusplus
}
#endif

#endif