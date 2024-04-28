#include "sound_chef_encoder.h"

#include <string.h> 

sc_encoder_config sc_encoder_config_init(ma_encoding_format_ext encodingFormat,
    ma_format format,
    ma_uint32 channels,
    ma_uint32 sampleRate,
    ma_uint8 quality)
{
    sc_encoder_config config;
    memset(&config, 0, sizeof(config));

    config.baseConfig = ma_encoder_config_init(ma_encoding_format_unknown, format, channels, sampleRate);
    config.encodingFormat = encodingFormat;
    config.quality        = quality;

    return config;
}

sc_result sc_encoder_init(ma_encoder_write_proc onWrite,
    ma_encoder_seek_proc onSeek,
    void* userData,
    const sc_encoder_config* config,
    sc_encoder* encoder)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(onWrite != NULL);
    SC_CHECK_ARG(onSeek != NULL);
    SC_CHECK_ARG(encoder != NULL);
    SC_CHECK_ARG(config->encodingFormat != ma_encoding_format_unknown);
    SC_CHECK_ARG(config->baseConfig.channels != 0);
    SC_CHECK_ARG(config->baseConfig.sampleRate != 0);

    memset(encoder, 0, sizeof(*encoder));

    encoder->config = *config;
    encoder->baseEncoder.config = config->baseConfig;

    encoder->baseEncoder.onWrite   = onWrite;
    encoder->baseEncoder.onSeek    = onSeek;
    encoder->baseEncoder.pUserData = userData;

    ma_result result = MA_SUCCESS;

    switch (encoder->config.encodingFormat)
    {
        case ma_encoding_format_vorbis:
        case ma_encoding_format_opus:

        case ma_encoding_format_wav:
        case ma_encoding_format_flac:
        case ma_encoding_format_mp3:
        default:
            result = MA_NO_BACKEND;
        break;
    }

    if (result == MA_SUCCESS)
    {
        result = encoder->baseEncoder.onInit((ma_encoder*)encoder);
    }

    return result;
}