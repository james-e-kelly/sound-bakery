#include "sound_chef_encoder.h"

#include "sound_chef/encoders/sound_chef_encoder_vorbis.h"

static ma_result sc_encoder_on_write_vfs(ma_encoder* encoder,
                                          const void* bufferIn,
                                          size_t bytesToWrite,
                                          size_t* bytesWritten)
{
    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    return ma_vfs_write(&vfs, encoder->data.vfs.file, bufferIn, bytesToWrite,
                                   bytesWritten);
}

static ma_result sc_encoder_on_seek_vfs(ma_encoder* encoder, ma_int64 offset, ma_seek_origin origin)
{
    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    return ma_vfs_seek(&vfs, encoder->data.vfs.file, offset, origin);
}

sc_encoder_config sc_encoder_config_init(ma_encoding_format_ext encodingFormat,
    ma_format format,
    ma_uint32 channels,
    ma_uint32 sampleRate,
    ma_uint8 quality)
{
    sc_encoder_config config;

    SC_ZERO_OBJECT(&config);

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

    SC_ZERO_OBJECT(encoder);

    encoder->config = *config;
    encoder->baseEncoder.config = config->baseConfig;

    encoder->baseEncoder.onWrite   = onWrite;
    encoder->baseEncoder.onSeek    = onSeek;
    encoder->baseEncoder.pUserData = userData;

    ma_result result = MA_SUCCESS;

    switch (encoder->config.encodingFormat)
    {
        case ma_encoding_format_vorbis:
            encoder->baseEncoder.onInit = sc_encoder_vorbis_on_init;
            encoder->baseEncoder.onUninit = sc_encoder_vorbis_on_uninit;
            encoder->baseEncoder.onWritePCMFrames = sc_encoder_vorbis_write_pcm_frames;
            break;
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

        if (result != MA_SUCCESS)
        {
            encoder->baseEncoder.onUninit((ma_encoder*)encoder);
        }
    }

    return result;
}

sc_result sc_encoder_uninit(sc_encoder* encoder) 
{ 
    SC_CHECK_ARG(encoder != NULL); 

    ma_encoder_uninit(&encoder->baseEncoder);

    return MA_SUCCESS;
}

sc_result sc_encoder_init_file(const char* filePath,
    const sc_encoder_config* config,
    sc_encoder* encoder)
{
    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    ma_vfs_file file = NULL;

    ma_result result = ma_vfs_open(&vfs, filePath, MA_OPEN_MODE_WRITE, &file);
    SC_CHECK_RESULT(result);

    encoder->baseEncoder.data.vfs.file = file;

    result = sc_encoder_init(sc_encoder_on_write_vfs, sc_encoder_on_seek_vfs, NULL, config, encoder);
    if (result != MA_SUCCESS)
    {
        ma_vfs_close(&vfs, file);
        return result;
    }

    return MA_SUCCESS;
}


sc_result sc_encoder_write_pcm_frames(sc_encoder* encoder,
    const void* framesIn,
    ma_uint64 frameCount,
    ma_uint64* framesWritten)
{
    SC_CHECK_ARG(encoder != NULL);
    SC_CHECK_ARG(framesIn != NULL);
    SC_CHECK_ARG(framesWritten != NULL);
    SC_CHECK(&encoder->baseEncoder.onWritePCMFrames != NULL, MA_ERROR);

    return encoder->baseEncoder.onWritePCMFrames(&encoder->baseEncoder, framesIn, frameCount, framesWritten);
}