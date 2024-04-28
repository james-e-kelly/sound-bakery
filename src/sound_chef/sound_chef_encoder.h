#ifndef SOUND_CHEF_ENCODER
#define SOUND_CHEF_ENCODER

/**
 * @file
 * @brief Provides extensions to miniaudio's encoding API.
 *
 * Handles encoding for soundbanks.
 */

#include "sound_chef/sound_chef_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct sc_encoder_config sc_encoder_config;
    typedef struct sc_encoder sc_encoder;

	typedef enum ma_encoding_format_ext
    {
        ma_encoding_format_opus = ma_encoding_format_vorbis + 1
    } ma_encoding_format_ext;

    struct sc_encoder_config
    {
        ma_encoder_config baseConfig;
        ma_uint8 quality;   //< quality setting for formats that allow it
        ma_encoding_format_ext encodingFormat;
    };

    struct sc_encoder
    {
        ma_encoder baseEncoder;
        sc_encoder_config config;
    };

    sc_encoder_config SC_API sc_encoder_config_init(ma_encoding_format_ext encodingFormat,
                                                    ma_format format,
                                                    ma_uint32 channels,
                                                    ma_uint32 sampleRate,
                                                    ma_uint8 quality);

    sc_result SC_API sc_encoder_init(ma_encoder_write_proc onWrite,
                                     ma_encoder_seek_proc onSeek,
                                     void* userData,
                                     const sc_encoder_config* config,
                                     sc_encoder* encoder);


    sc_result SC_API sc_encoder_uninit(sc_encoder* encoder);

#ifdef __cplusplus
}
#endif

#endif