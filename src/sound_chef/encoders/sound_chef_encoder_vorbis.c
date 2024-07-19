#include "sound_chef_encoder_vorbis.h"

#define SC_ENCODER_VORBIS_DEFAULT_QUALITY 0.3f

#include <stdlib.h>
#include <time.h>
#include <vorbis/vorbisenc.h>

typedef struct sc_encoder_vorbis sc_encoder_vorbis;

struct sc_encoder_vorbis
{
    vorbis_info info;
    vorbis_comment comment;
    vorbis_dsp_state dsp;
    vorbis_block block;
    ogg_stream_state oggStream;

    sc_bool hasWrittenHeader;
};

static sc_result ma_encoder_vorbis_write_stream(ma_encoder* encoder)
{
    SC_CHECK_ARG(encoder != NULL);

    sc_encoder_vorbis* vorbisEncoder = (sc_encoder_vorbis*)encoder->pInternalEncoder;
    SC_CHECK(vorbisEncoder != NULL, MA_ERROR);

    for (;;)
    {
        ogg_page oggPage;

        int pageResult = ogg_stream_flush(&vorbisEncoder->oggStream, &oggPage);

        if (pageResult == 0)
        {
            break;
        }

        ma_result headerWriteResult = encoder->onWrite(encoder, oggPage.header, oggPage.header_len, NULL);
        ma_result bodyWriteResult   = encoder->onWrite(encoder, oggPage.body, oggPage.body_len, NULL);

        assert(headerWriteResult == MA_SUCCESS);
        assert(bodyWriteResult == MA_SUCCESS);

        SC_CHECK_RESULT(headerWriteResult);
        SC_CHECK_RESULT(bodyWriteResult);
    }

    return MA_SUCCESS;
}

static sc_result sc_encoder_vorbis_write_header(ma_encoder* encoder)
{
    SC_CHECK_ARG(encoder != NULL);

    sc_encoder_vorbis* vorbisEncoder = (sc_encoder_vorbis*)encoder->pInternalEncoder;
    SC_CHECK(vorbisEncoder != NULL, MA_ERROR);

    ogg_packet header;
    ogg_packet headerComment;
    ogg_packet headerCode;

    int headerResult =
        vorbis_analysis_headerout(&vorbisEncoder->dsp, &vorbisEncoder->comment, &header, &headerComment, &headerCode);
    SC_CHECK(headerResult == 0, MA_ERROR);

    ogg_stream_packetin(&vorbisEncoder->oggStream, &header);
    ogg_stream_packetin(&vorbisEncoder->oggStream, &headerComment);
    ogg_stream_packetin(&vorbisEncoder->oggStream, &headerCode);

    vorbisEncoder->hasWrittenHeader = MA_TRUE;

    return ma_encoder_vorbis_write_stream(encoder);
}

sc_result sc_encoder_vorbis_on_init(ma_encoder* encoder)
{
    SC_CHECK_ARG(encoder != NULL);

    sc_encoder_vorbis* vorbisEncoder = (sc_encoder_vorbis*)ma_malloc(sizeof(sc_encoder_vorbis), NULL);
    SC_CHECK_MEM(vorbisEncoder);
    SC_ZERO_OBJECT(vorbisEncoder);

    encoder->pInternalEncoder = vorbisEncoder;

    int vorbisInitResultCode = -1;

    vorbis_info_init(&vorbisEncoder->info);
    vorbisInitResultCode = vorbis_encode_init_vbr(&vorbisEncoder->info, encoder->config.channels,
                                                  encoder->config.sampleRate, SC_ENCODER_VORBIS_DEFAULT_QUALITY);
    SC_CHECK(vorbisInitResultCode == 0, MA_ERROR);

    vorbis_comment_init(&vorbisEncoder->comment);
    vorbis_comment_add_tag(&vorbisEncoder->comment, "ENCODER", "SOUND_CHEF");

    vorbisInitResultCode = vorbis_analysis_init(&vorbisEncoder->dsp, &vorbisEncoder->info);
    SC_CHECK(vorbisInitResultCode == 0, MA_ERROR);

    vorbisInitResultCode = vorbis_block_init(&vorbisEncoder->dsp, &vorbisEncoder->block);
    SC_CHECK(vorbisInitResultCode == 0, MA_ERROR);

    srand(time(NULL));
    vorbisInitResultCode = ogg_stream_init(&vorbisEncoder->oggStream, rand());
    SC_CHECK(vorbisInitResultCode == 0, MA_ERROR);

    return MA_SUCCESS;
}

sc_result sc_encoder_vorbis_write_pcm_frames(ma_encoder* encoder,
                                             const void* framesIn,
                                             ma_uint64 frameCount,
                                             ma_uint64* framesWritten)
{
    SC_CHECK_ARG(encoder != NULL);
    SC_CHECK_ARG(framesIn != NULL);
    SC_CHECK_ARG(frameCount > 0);

    sc_encoder_vorbis* vorbisEncoder = (sc_encoder_vorbis*)encoder->pInternalEncoder;
    SC_CHECK(vorbisEncoder != NULL, MA_ERROR);

    if (framesWritten)
    {
        *framesWritten = 0;
    }

    if (!vorbisEncoder->hasWrittenHeader)
    {
        sc_result headerResult = sc_encoder_vorbis_write_header(encoder);
        SC_CHECK(headerResult == MA_SUCCESS, MA_ERROR);
    }

    float** const analysisBuffer = vorbis_analysis_buffer(&vorbisEncoder->dsp, (int)frameCount);

    ma_deinterleave_pcm_frames(ma_format_f32, encoder->config.channels, frameCount, framesIn, (void**)analysisBuffer);

    int wroteResult = vorbis_analysis_wrote(&vorbisEncoder->dsp, (int)frameCount);
    assert(wroteResult == 0);

    if (framesWritten)
    {
        *framesWritten = frameCount;
    }

    while (vorbis_analysis_blockout(&vorbisEncoder->dsp, &vorbisEncoder->block) == 1)
    {
        int analysisResult = vorbis_analysis(&vorbisEncoder->block, NULL);
        SC_CHECK(analysisResult == 0, MA_ERROR);

        int addBlockResult = vorbis_bitrate_addblock(&vorbisEncoder->block);
        SC_CHECK(addBlockResult == 0, MA_ERROR);

        ogg_packet oggPacket;

        while (vorbis_bitrate_flushpacket(&vorbisEncoder->dsp, &oggPacket))
        {
            ogg_stream_packetin(&vorbisEncoder->oggStream, &oggPacket);
        }
    }

    return ma_encoder_vorbis_write_stream(encoder);
}

void sc_encoder_vorbis_on_uninit(ma_encoder* encoder)
{
    if (!encoder)
    {
        return;
    }

    if (!encoder->pInternalEncoder)
    {
        return;
    }

    sc_encoder_vorbis* vorbisEncoder = (sc_encoder_vorbis*)encoder->pInternalEncoder;

    if (!vorbisEncoder)
    {
        return;
    }

    // Do final write on uninit. We know the stream of data has ended and the user
    // doesn't need to do something explicit

    {
        vorbis_analysis_wrote(&vorbisEncoder->dsp, 0);

        while (vorbis_analysis_blockout(&vorbisEncoder->dsp, &vorbisEncoder->block) == 1)
        {
            vorbis_analysis(&vorbisEncoder->block, NULL);
            vorbis_bitrate_addblock(&vorbisEncoder->block);

            ogg_packet oggPacket;

            while (vorbis_bitrate_flushpacket(&vorbisEncoder->dsp, &oggPacket))
            {
                ogg_stream_packetin(&vorbisEncoder->oggStream, &oggPacket);
            }
        }

        ma_encoder_vorbis_write_stream(encoder);
    }

    ogg_stream_clear(&vorbisEncoder->oggStream);
    vorbis_block_clear(&vorbisEncoder->block);
    vorbis_dsp_clear(&vorbisEncoder->dsp);
    vorbis_comment_clear(&vorbisEncoder->comment);
    vorbis_info_clear(&vorbisEncoder->info);

    ma_free(vorbisEncoder, &encoder->config.allocationCallbacks);
}