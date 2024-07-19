#include "sound_chef_encoder.h"

#include "sound_chef/encoders/sound_chef_encoder_vorbis.h"

static ma_result sc_encoder_on_write_vfs(ma_encoder* encoder,
                                         const void* bufferIn,
                                         size_t bytesToWrite,
                                         size_t* bytesWritten)
{
    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    return ma_vfs_write(&vfs, encoder->data.vfs.file, bufferIn, bytesToWrite, bytesWritten);
}

static ma_result sc_encoder_on_seek_vfs(ma_encoder* encoder, ma_int64 offset, ma_seek_origin origin)
{
    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    return ma_vfs_seek(&vfs, encoder->data.vfs.file, offset, origin);
}

sc_encoder_config sc_encoder_config_init(
    ma_encoding_format_ext encodingFormat, ma_format format, ma_uint32 channels, ma_uint32 sampleRate, ma_uint8 quality)
{
    sc_encoder_config config;

    SC_ZERO_OBJECT(&config);

    config.baseConfig     = ma_encoder_config_init(encodingFormat, format, channels, sampleRate);
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

    if (config->baseConfig.encodingFormat == ma_encoding_format_wav)
    {
        return ma_encoder_init(onWrite, onSeek, userData, &config->baseConfig, &encoder->baseEncoder);
    }

    encoder->config             = *config;
    encoder->baseEncoder.config = config->baseConfig;

    encoder->baseEncoder.onWrite   = onWrite;
    encoder->baseEncoder.onSeek    = onSeek;
    encoder->baseEncoder.pUserData = userData;

    ma_result result = MA_SUCCESS;

    switch (encoder->config.encodingFormat)
    {
        case ma_encoding_format_vorbis:
            encoder->baseEncoder.onInit           = sc_encoder_vorbis_on_init;
            encoder->baseEncoder.onUninit         = sc_encoder_vorbis_on_uninit;
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

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    if (encoder->baseEncoder.data.vfs.file != NULL)
    {
        ma_result closeResult = ma_vfs_close(&vfs, encoder->baseEncoder.data.vfs.file);
        SC_CHECK_RESULT(closeResult);
    }

    return MA_SUCCESS;
}

sc_result sc_encoder_init_file(const char* filePath, const sc_encoder_config* config, sc_encoder* encoder)
{
    SC_CHECK_ARG(filePath != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(encoder != NULL);

    if (config->encodingFormat == ma_encoding_format_wav)
    {
        return ma_encoder_init_file(filePath, &config->baseConfig, &encoder->baseEncoder);
    }

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    ma_vfs_file file = NULL;

    ma_result result = ma_vfs_open(&vfs, filePath, MA_OPEN_MODE_WRITE, &file);
    SC_CHECK_RESULT(result);

    result = sc_encoder_init(sc_encoder_on_write_vfs, sc_encoder_on_seek_vfs, NULL, config, encoder);
    if (result != MA_SUCCESS)
    {
        ma_vfs_close(&vfs, file);
        return result;
    }

    encoder->baseEncoder.data.vfs.file = file;

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

//

sc_result sc_encoder_write_from_file(const char* decodeFilePath,
                                     const char* encodeFilePath,
                                     const sc_encoder_config* config)
{
    SC_CHECK_ARG(decodeFilePath != NULL);
    SC_CHECK_ARG(encodeFilePath != NULL);
    SC_CHECK_ARG(config != NULL);

    ma_decoder decoder;
    ma_decoder_config decoderConfig =
        ma_decoder_config_init(config->baseConfig.format, config->baseConfig.channels, config->baseConfig.sampleRate);
    ma_result decoderInitResult = ma_decoder_init_file(decodeFilePath, &decoderConfig, &decoder);
    SC_CHECK_RESULT(decoderInitResult);

    sc_encoder_config configCopy = *config;

    // If encoding to "As Input" channels, find the channel count from the decoder
    if (config->baseConfig.channels == 0)
    {
        ma_result getChannelsResult =
            ma_decoder_get_data_format(&decoder, NULL, &configCopy.baseConfig.channels, NULL, NULL, 0);
        SC_CHECK_RESULT(getChannelsResult);
    }

    sc_encoder encoder;
    sc_result encoderInitResult = sc_encoder_init_file(encodeFilePath, &configCopy, &encoder);
    SC_CHECK_RESULT(encoderInitResult);

    const ma_uint64 desiredFrameCount = 1024;
    const ma_uint64 convertedBufferSize =
        desiredFrameCount * ma_get_bytes_per_frame(config->baseConfig.format, configCopy.baseConfig.channels);
    void* outConvertedBuffer = ma_malloc(convertedBufferSize, NULL);

    for (;;)
    {
        ma_uint64 framesRead = 0;
        sc_result readResult = ma_decoder_read_pcm_frames(&decoder, outConvertedBuffer, desiredFrameCount, &framesRead);

        if (framesRead == 0 || readResult == MA_AT_END)
        {
            break;
        }

        ma_uint64 framesEncoded = 0;
        sc_result encodeResult  = sc_encoder_write_pcm_frames(&encoder, outConvertedBuffer, framesRead, &framesEncoded);
        assert(encodeResult == MA_SUCCESS);

        // Out of data
        if (framesRead < desiredFrameCount)
        {
            break;
        }
    }

    ma_free(outConvertedBuffer, NULL);

    ma_decoder_uninit(&decoder);
    sc_encoder_uninit(&encoder);

    return MA_SUCCESS;
}