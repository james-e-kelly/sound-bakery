#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vorbis/vorbisenc.h>
//
//#ifdef _WIN32 /* We need the following two to set stdin/stdout to binary */
//    #include <fcntl.h>
//    #include <io.h>
//#endif
//
//#if defined(__MACOS__) && defined(__MWERKS__)
//    #include <console.h> /* CodeWarrior's Mac "command-line" support */
//#endif
//
//#define READ 1024
//signed char readbuffer[READ * 4 + 44]; /* out of the data segment, not the stack */
//
//int main()
//{
//    ogg_stream_state oggStream;     /* take physical pages, weld into a logical stream of packets */
//    ogg_page oggPage;               /* one Ogg bitstream page.  Vorbis packets are inside */
//    ogg_packet oggPacket;           /* one raw packet of data for decode */
//
//    vorbis_info vorbisInfo;         /* struct that stores all the static vorbis bitstream settings */
//    vorbis_comment vorbisComment;   /* struct that stores all the user comments */
//
//    vorbis_dsp_state vorbisDSP;     /* central working state for the packet->PCM decoder */
//    vorbis_block vorbisBlock;       /* local working space for packet->PCM decode */
//
//    int endOfStream = 0;
//    int returnValue = 0;
//    int founddata   = 0;
//
//
//
//    readbuffer[0] = '\0';
//    for (int i = 0; i < 30 && !feof(stdin) && !ferror(stdin); i++)
//    {
//        fread(readbuffer, 1, 2, stdin);
//
//        if (!strncmp((char *)readbuffer, "da", 2))
//        {
//            founddata = 1;
//            fread(readbuffer, 1, 6, stdin);
//            break;
//        }
//    }
//
//    /********** Encode setup ************/
//
//    vorbis_info_init(&vorbisInfo);
//
//    returnValue = vorbis_encode_init_vbr(&vorbisInfo, 2, 44100, 0.1);
//
//    if (returnValue)
//        exit(1);
//
//    vorbis_comment_init(&vorbisComment);
//    vorbis_comment_add_tag(&vorbisComment, "ENCODER", "encoder_example.c");
//
//    vorbis_analysis_init(&vorbisDSP, &vorbisInfo);
//    vorbis_block_init(&vorbisDSP, &vorbisBlock);
//
//    srand(time(NULL));
//    ogg_stream_init(&oggStream, rand());
//
//    /* Vorbis streams begin with three headers; the initial header (with
//       most of the codec setup parameters) which is mandated by the Ogg
//       bitstream spec.  The second header holds any comment fields.  The
//       third header holds the bitstream codebook.  We merely need to
//       make the headers, then pass them to libvorbis one at a time;
//       libvorbis handles the additional Ogg bitstream constraints */
//
//    {
//        ogg_packet header;
//        ogg_packet header_comm;
//        ogg_packet header_code;
//
//        vorbis_analysis_headerout(&vorbisDSP, &vorbisComment, &header, &header_comm, &header_code);
//
//        ogg_stream_packetin(&oggStream, &header); /* automatically placed in its own page */
//        ogg_stream_packetin(&oggStream, &header_comm);
//        ogg_stream_packetin(&oggStream, &header_code);
//
//        /* This ensures the actual
//         * audio data will start on a new page, as per spec
//         */
//        while (!endOfStream)
//        {
//            int result = ogg_stream_flush(&oggStream, &oggPage);
//            if (result == 0)
//                break;
//            fwrite(oggPage.header, 1, oggPage.header_len, stdout);
//            fwrite(oggPage.body, 1, oggPage.body_len, stdout);
//        }
//    }
//
//    while (!endOfStream)
//    {
//        long i;
//        long bytes = fread(readbuffer, 1, READ * 4, stdin); /* stereo hardwired here */
//
//        if (bytes == 0)
//        {
//            /* end of file.  this can be done implicitly in the mainline,
//               but it's easier to see here in non-clever fashion.
//               Tell the library we're at end of stream so that it can handle
//               the last frame and mark end of stream in the output properly */
//            vorbis_analysis_wrote(&vorbisDSP, 0);
//        }
//        else
//        {
//            /* data to encode */
//
//            /* expose the buffer to submit data */
//            float **buffer = vorbis_analysis_buffer(&vorbisDSP, READ);
//
//            /* uninterleave samples */
//            for (i = 0; i < bytes / 4; i++)
//            {
//                buffer[0][i] = ((readbuffer[i * 4 + 1] << 8) | (0x00ff & (int)readbuffer[i * 4])) / 32768.f;
//                buffer[1][i] = ((readbuffer[i * 4 + 3] << 8) | (0x00ff & (int)readbuffer[i * 4 + 2])) / 32768.f;
//            }
//
//            /* tell the library how much we actually submitted */
//            vorbis_analysis_wrote(&vorbisDSP, i);
//        }
//
//        /* vorbis does some data preanalysis, then divvies up blocks for
//           more involved (potentially parallel) processing.  Get a single
//           block for encoding now */
//        while (vorbis_analysis_blockout(&vorbisDSP, &vorbisBlock) == 1)
//        {
//
//            /* analysis, assume we want to use bitrate management */
//            vorbis_analysis(&vorbisBlock, NULL);
//            vorbis_bitrate_addblock(&vorbisBlock);
//
//            while (vorbis_bitrate_flushpacket(&vorbisDSP, &oggPacket))
//            {
//
//                /* weld the packet into the bitstream */
//                ogg_stream_packetin(&oggStream, &oggPacket);
//
//                /* write out pages (if any) */
//                while (!endOfStream)
//                {
//                    int result = ogg_stream_pageout(&oggStream, &oggPage);
//                    if (result == 0)
//                        break;
//                    fwrite(oggPage.header, 1, oggPage.header_len, stdout);
//                    fwrite(oggPage.body, 1, oggPage.body_len, stdout);
//
//                    /* this could be set above, but for illustrative purposes, I do
//                       it here (to show that vorbis does know where the stream ends) */
//
//                    if (ogg_page_eos(&oggPage))
//                        endOfStream = 1;
//                }
//            }
//        }
//    }
//
//    ogg_stream_clear(&oggStream);
//    vorbis_block_clear(&vorbisBlock);
//    vorbis_dsp_clear(&vorbisDSP);
//    vorbis_comment_clear(&vorbisComment);
//    vorbis_info_clear(&vorbisInfo);
//
//    return (0);
//}