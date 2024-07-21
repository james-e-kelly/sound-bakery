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

    struct sc_encoder_config
    {
        ma_encoder_config baseConfig;
        ma_uint8 quality;  //< quality setting for formats that allow it
        sc_encoding_format encodingFormat;
    };

    struct sc_encoder
    {
        ma_encoder baseEncoder;
        sc_encoder_config config;
    };

    sc_encoder_config SC_API sc_encoder_config_init(sc_encoding_format encodingFormat,
                                                    ma_format format,
                                                    ma_uint32 channels,
                                                    ma_uint32 sampleRate,
                                                    ma_uint8 quality);

    sc_result SC_API sc_encoder_init(ma_encoder_write_proc onWrite,
                                     ma_encoder_seek_proc onSeek,
                                     void* userData,
                                     const sc_encoder_config* config,
                                     sc_encoder* encoder);

    sc_result SC_API sc_encoder_init_file(const char* filePath, const sc_encoder_config* config, sc_encoder* encoder);

    sc_result SC_API sc_encoder_write_pcm_frames(sc_encoder* encoder,
                                                 const void* framesIn,
                                                 ma_uint64 frameCount,
                                                 ma_uint64* framesWritten);

    sc_result SC_API sc_encoder_uninit(sc_encoder* encoder);

    //

    sc_result SC_API sc_encoder_write_from_file(const char* decodeFilePath,
                                                const char* encodeFilePath,
                                                const sc_encoder_config* config);

#ifdef __cplusplus
}
#endif

#endif