#include "sound_chef/sound_chef.h"

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

#define SC_MINUS_3DB 0.70710678f
#define SC_PI        3.14159265358979323846

enum
{
    SC_MAX_AMBISONIC_ORDER = 5,  //< 36 channels, see SC_MAX_CHANNELS
    SC_MAX_FUMA_ORDER      = 3,  //< FuMa isn't defined past third order
    SC_MAX_FOLD_DEPTH      = 4,
    SC_MAX_DECODER_POINTS  = SC_MAX_CHANNELS + 2  //< Every output plus an imaginary speaker at each pole
};

/**************************************************************************************************************************************************************

Layouts

**************************************************************************************************************************************************************/

static sc_channel_layout sc_make_speakers(const char* name, const ma_channel* channelMap, sc_uint32 channelCount)
{
    sc_channel_layout layout;
    SC_ZERO_OBJECT(&layout);

    snprintf(layout.name, sizeof(layout.name), "%s", name);
    layout.family       = sc_channel_layout_family_speakers;
    layout.channelCount = SC_MIN(channelCount, (sc_uint32)SC_MAX_CHANNELS);
    memcpy(layout.channelMap, channelMap, layout.channelCount * sizeof(ma_channel));
    return layout;
}

#define SC_MAKE_SPEAKERS(name, ...) sc_make_speakers((name), (const ma_channel[]){__VA_ARGS__}, (sc_uint32)(sizeof((const ma_channel[]){__VA_ARGS__}) / sizeof(ma_channel)))

static sc_channel_layout sc_make_ambisonic(sc_channel_layout_family family, sc_uint32 order)
{
    static const char* const s_orderNames[] = {"", "1st", "2nd", "3rd", "4th", "5th"};

    sc_channel_layout layout;
    SC_ZERO_OBJECT(&layout);

    snprintf(layout.name, sizeof(layout.name), "%s %s order", family == sc_channel_layout_family_ambix ? "AmbiX" : "FuMa", s_orderNames[order]);
    layout.family       = family;
    layout.channelCount = (order + 1) * (order + 1);
    return layout;
}

static sc_channel_layout sc_make_direct(sc_uint32 channelCount)
{
    sc_channel_layout layout;
    SC_ZERO_OBJECT(&layout);

    snprintf(layout.name, sizeof(layout.name), "Direct (channel n to output n)");
    layout.family       = sc_channel_layout_family_direct;
    layout.channelCount = channelCount;
    return layout;
}

/**
 * @return The order whose full set of components is @p channelCount channels, or 0 if there isn't one.
 */
static sc_uint32 sc_ambisonic_order_for(sc_uint32 channelCount, sc_uint32 maxOrder)
{
    for (sc_uint32 order = 1; order <= maxOrder; ++order)
    {
        if ((order + 1) * (order + 1) == channelCount)
        {
            return order;
        }
    }
    return 0;
}

/**
 * @brief Layouts used when a file doesn't say, in the order WAV, FLAC and most DAWs write channels.
 */
static sc_channel_layout sc_guess_wav_order_layout(sc_uint32 channelCount)
{
    switch (channelCount)
    {
        case 1:
            return SC_MAKE_SPEAKERS("Mono", MA_CHANNEL_MONO);
        case 2:
            return SC_MAKE_SPEAKERS("Stereo", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT);
        case 3:
            return SC_MAKE_SPEAKERS("LCR (L R C)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER);
        case 5:
            return SC_MAKE_SPEAKERS("5.0 (L R C Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
        case 6:
            return SC_MAKE_SPEAKERS("5.1 (L R C LFE Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
        case 7:
            return SC_MAKE_SPEAKERS("6.1 (L R C LFE Cs Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_BACK_CENTER, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
        case 8:
            return SC_MAKE_SPEAKERS("7.1 (L R C LFE Lss Rss Lrs Rrs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT);
        case 10:
            return SC_MAKE_SPEAKERS("5.1.4 (L R C LFE Ls Rs Ltf Rtf Ltr Rtr)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_TOP_FRONT_LEFT, MA_CHANNEL_TOP_FRONT_RIGHT, MA_CHANNEL_TOP_BACK_LEFT, MA_CHANNEL_TOP_BACK_RIGHT);
        case 12:
            return SC_MAKE_SPEAKERS("7.1.4 (L R C LFE Lss Rss Lrs Rrs Ltf Rtf Ltr Rtr)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_TOP_FRONT_LEFT, MA_CHANNEL_TOP_FRONT_RIGHT, MA_CHANNEL_TOP_BACK_LEFT, MA_CHANNEL_TOP_BACK_RIGHT);
        default:
            break;
    }

    // 4, 9, 16, 25 and 36 channels without a channel mask are almost always ambisonics; quad and other speaker
    // layouts with those counts normally carry a mask.
    const sc_uint32 order = sc_ambisonic_order_for(channelCount, SC_MAX_AMBISONIC_ORDER);
    if (order > 0)
    {
        return sc_make_ambisonic(sc_channel_layout_family_ambix, order);
    }

    return sc_make_direct(channelCount);
}

/**
 * @brief Ogg Vorbis and Opus put the centre between the fronts and the LFE last.
 */
static sc_channel_layout sc_guess_vorbis_order_layout(sc_uint32 channelCount)
{
    switch (channelCount)
    {
        case 3:
            return SC_MAKE_SPEAKERS("LCR (L C R)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT);
        case 4:
            return SC_MAKE_SPEAKERS("Quad (L R Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT);
        case 5:
            return SC_MAKE_SPEAKERS("5.0 Vorbis (L C R Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT);
        case 6:
            return SC_MAKE_SPEAKERS("5.1 Vorbis (L C R Ls Rs LFE)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_LFE);
        case 7:
            return SC_MAKE_SPEAKERS("6.1 Vorbis (L C R Lss Rss Cs LFE)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_CENTER, MA_CHANNEL_LFE);
        case 8:
            return SC_MAKE_SPEAKERS("7.1 Vorbis (L C R Lss Rss Lrs Rrs LFE)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_LFE);
        default:
            return sc_guess_wav_order_layout(channelCount);
    }
}

/**
 * @brief Positions a layout can place a channel at. Anything else, like the AUX channels, has no known position.
 */
static ma_channel sc_layout_position(ma_channel channel)
{
    return (channel == MA_CHANNEL_MONO || (channel >= MA_CHANNEL_FRONT_LEFT && channel <= MA_CHANNEL_TOP_BACK_RIGHT)) ? channel : MA_CHANNEL_NONE;
}

/**
 * @brief Positions an output channel can be at. A mono output is a single speaker in front.
 */
static ma_channel sc_output_position(ma_channel channel)
{
    return channel == MA_CHANNEL_MONO ? MA_CHANNEL_FRONT_CENTER : sc_layout_position(channel);
}

static sc_bool sc_channel_map_contains(const ma_channel* channelMap, sc_uint32 channels, ma_channel position)
{
    for (sc_uint32 channel = 0; channel < channels; ++channel)
    {
        if (channelMap[channel] == position)
        {
            return SC_TRUE;
        }
    }
    return SC_FALSE;
}

/**
 * @brief Whether two maps hold the same positions, in any order.
 */
static sc_bool sc_channel_map_same_positions(const ma_channel* left, const ma_channel* right, sc_uint32 channels)
{
    sc_uint32 leftCounts[MA_CHANNEL_POSITION_COUNT]  = {0};
    sc_uint32 rightCounts[MA_CHANNEL_POSITION_COUNT] = {0};

    for (sc_uint32 channel = 0; channel < channels; ++channel)
    {
        ++leftCounts[left[channel] % MA_CHANNEL_POSITION_COUNT];
        ++rightCounts[right[channel] % MA_CHANNEL_POSITION_COUNT];
    }
    return memcmp(leftCounts, rightCounts, sizeof(leftCounts)) == 0;
}

sc_bool sc_channel_layout_is_ambisonic(const sc_channel_layout* layout)
{
    return layout != NULL && (layout->family == sc_channel_layout_family_ambix || layout->family == sc_channel_layout_family_fuma);
}

sc_uint32 sc_channel_layout_get_ambisonic_order(const sc_channel_layout* layout)
{
    return sc_channel_layout_is_ambisonic(layout) ? (sc_uint32)lround(sqrt((double)layout->channelCount)) - 1 : 0;
}

/**************************************************************************************************************************************************************

Labels

**************************************************************************************************************************************************************/

static const char* sc_speaker_label(ma_channel position, sc_bool hasSideAndBack)
{
    switch (position)
    {
        case MA_CHANNEL_FRONT_LEFT:
            return "L";
        case MA_CHANNEL_FRONT_RIGHT:
            return "R";
        case MA_CHANNEL_FRONT_CENTER:
            return "C";
        case MA_CHANNEL_LFE:
            return "LFE";
        case MA_CHANNEL_BACK_LEFT:
            return hasSideAndBack ? "Lrs" : "Ls";
        case MA_CHANNEL_BACK_RIGHT:
            return hasSideAndBack ? "Rrs" : "Rs";
        case MA_CHANNEL_FRONT_LEFT_CENTER:
            return "Lc";
        case MA_CHANNEL_FRONT_RIGHT_CENTER:
            return "Rc";
        case MA_CHANNEL_BACK_CENTER:
            return "Cs";
        case MA_CHANNEL_SIDE_LEFT:
            return hasSideAndBack ? "Lss" : "Ls";
        case MA_CHANNEL_SIDE_RIGHT:
            return hasSideAndBack ? "Rss" : "Rs";
        case MA_CHANNEL_TOP_CENTER:
            return "Tc";
        case MA_CHANNEL_TOP_FRONT_LEFT:
            return "Ltf";
        case MA_CHANNEL_TOP_FRONT_CENTER:
            return "Tfc";
        case MA_CHANNEL_TOP_FRONT_RIGHT:
            return "Rtf";
        case MA_CHANNEL_TOP_BACK_LEFT:
            return "Ltr";
        case MA_CHANNEL_TOP_BACK_CENTER:
            return "Tbc";
        case MA_CHANNEL_TOP_BACK_RIGHT:
            return "Rtr";
        case MA_CHANNEL_MONO:
            return "M";
        default:
            return NULL;
    }
}

static const char* const s_acnLabels[]  = {"W", "Y", "Z", "X", "V", "T", "R", "S", "U", "Q", "O", "M", "K", "L", "N", "P"};
static const char* const s_fumaLabels[] = {"W", "X", "Y", "Z", "R", "S", "T", "U", "V", "K", "L", "M", "N", "O", "P", "Q"};

sbk_status sc_channel_layout_get_channel_label(const sc_channel_layout* layout, sc_uint32 channel, char* label, size_t labelSize)
{
    SC_CHECK_ARG(layout != NULL);
    SC_CHECK_ARG(label != NULL);
    SC_CHECK_ARG(labelSize > 0);

    const char* name = NULL;

    switch (layout->family)
    {
        case sc_channel_layout_family_speakers:
        {
            if (channel < layout->channelCount)
            {
                const ma_channel* const map = layout->channelMap;
                const sc_uint32 count       = layout->channelCount;
                const sc_bool hasSide       = sc_channel_map_contains(map, count, MA_CHANNEL_SIDE_LEFT) || sc_channel_map_contains(map, count, MA_CHANNEL_SIDE_RIGHT);
                const sc_bool hasBack       = sc_channel_map_contains(map, count, MA_CHANNEL_BACK_LEFT) || sc_channel_map_contains(map, count, MA_CHANNEL_BACK_RIGHT);
                name                        = sc_speaker_label(map[channel], hasSide && hasBack);
            }
            break;
        }
        case sc_channel_layout_family_ambix:
            // Letters only name components up to third order, so higher ones go by ACN.
            if (channel >= SC_COUNTOF(s_acnLabels))
            {
                snprintf(label, labelSize, "A%u", channel);
                return SBK_SUCCESS;
            }
            name = s_acnLabels[channel];
            break;
        case sc_channel_layout_family_fuma:
            if (channel < SC_COUNTOF(s_fumaLabels))
            {
                name = s_fumaLabels[channel];
            }
            break;
        case sc_channel_layout_family_direct:
            break;
    }

    if (name != NULL)
    {
        snprintf(label, labelSize, "%s", name);
    }
    else
    {
        snprintf(label, labelSize, "%u", channel + 1);
    }
    return SBK_SUCCESS;
}

/**************************************************************************************************************************************************************

Presets

**************************************************************************************************************************************************************/

sbk_status sc_channel_layout_get_presets(sc_uint32 channelCount, sc_channel_layout* presets, sc_uint32 presetCapacity, sc_uint32* outPresetCount)
{
    SC_CHECK_ARG(presets != NULL || presetCapacity == 0);

    sc_channel_layout found[SC_MAX_CHANNEL_LAYOUT_PRESETS];
    sc_uint32 count = 0;

    switch (channelCount)
    {
        case 1:
            found[count++] = SC_MAKE_SPEAKERS("Mono", MA_CHANNEL_MONO);
            break;
        case 2:
            found[count++] = SC_MAKE_SPEAKERS("Stereo", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT);
            break;
        case 3:
            found[count++] = SC_MAKE_SPEAKERS("LCR (L R C)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER);
            found[count++] = SC_MAKE_SPEAKERS("LCR (L C R)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT);
            break;
        case 4:
            found[count++] = SC_MAKE_SPEAKERS("Quad (L R Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT);
            found[count++] = SC_MAKE_SPEAKERS("LCRS (L R C S)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_BACK_CENTER);
            break;
        case 5:
            found[count++] = SC_MAKE_SPEAKERS("5.0 (L R C Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
            found[count++] = SC_MAKE_SPEAKERS("5.0 Film (L C R Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
            break;
        case 6:
            found[count++] = SC_MAKE_SPEAKERS("5.1 (L R C LFE Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
            found[count++] = SC_MAKE_SPEAKERS("5.1 Film (L C R Ls Rs LFE)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_LFE);
            found[count++] = SC_MAKE_SPEAKERS("5.1 DTS (L R Ls Rs C LFE)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE);
            break;
        case 7:
            found[count++] = SC_MAKE_SPEAKERS("6.1 (L R C LFE Cs Ls Rs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_BACK_CENTER, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
            break;
        case 8:
            found[count++] = SC_MAKE_SPEAKERS("7.1 (L R C LFE Lss Rss Lrs Rrs)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT);
            found[count++] = SC_MAKE_SPEAKERS("7.1 WAV mask (L R C LFE Lrs Rrs Lss Rss)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT);
            found[count++] = SC_MAKE_SPEAKERS("7.1 Film (L C R Lss Rss Lrs Rrs LFE)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_LFE);
            break;
        case 10:
            found[count++] = SC_MAKE_SPEAKERS("5.1.4 (L R C LFE Ls Rs Ltf Rtf Ltr Rtr)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_TOP_FRONT_LEFT, MA_CHANNEL_TOP_FRONT_RIGHT, MA_CHANNEL_TOP_BACK_LEFT, MA_CHANNEL_TOP_BACK_RIGHT);
            found[count++] = SC_MAKE_SPEAKERS("7.1.2 (L R C LFE Lss Rss Lrs Rrs Ltm Rtm)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_TOP_FRONT_LEFT, MA_CHANNEL_TOP_FRONT_RIGHT);
            break;
        case 12:
            found[count++] = SC_MAKE_SPEAKERS("7.1.4 (L R C LFE Lss Rss Lrs Rrs Ltf Rtf Ltr Rtr)", MA_CHANNEL_FRONT_LEFT, MA_CHANNEL_FRONT_RIGHT, MA_CHANNEL_FRONT_CENTER, MA_CHANNEL_LFE, MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT, MA_CHANNEL_BACK_LEFT, MA_CHANNEL_BACK_RIGHT, MA_CHANNEL_TOP_FRONT_LEFT, MA_CHANNEL_TOP_FRONT_RIGHT, MA_CHANNEL_TOP_BACK_LEFT, MA_CHANNEL_TOP_BACK_RIGHT);
            break;
        default:
            break;
    }

    const sc_uint32 order = sc_ambisonic_order_for(channelCount, SC_MAX_AMBISONIC_ORDER);
    if (order > 0)
    {
        found[count++] = sc_make_ambisonic(sc_channel_layout_family_ambix, order);
        if (order <= SC_MAX_FUMA_ORDER)
        {
            found[count++] = sc_make_ambisonic(sc_channel_layout_family_fuma, order);
        }
    }

    if (channelCount > 0)
    {
        found[count++] = sc_make_direct(channelCount);
    }

    SC_ASSERT(count <= SC_MAX_CHANNEL_LAYOUT_PRESETS);

    const sc_uint32 copied = SC_MIN(count, presetCapacity);
    if (copied > 0)
    {
        memcpy(presets, found, copied * sizeof(sc_channel_layout));
    }

    if (outPresetCount != NULL)
    {
        *outPresetCount = copied;
    }
    return copied < count ? SBK_ERR_FULL : SBK_SUCCESS;
}

/**************************************************************************************************************************************************************

Detection

**************************************************************************************************************************************************************/

typedef struct sc_wav_format_info
{
    sc_uint16 formatTag;
    sc_uint16 channels;
    sc_uint32 channelMask;
    sc_bool isAmbisonicBFormat;
} sc_wav_format_info;

static sc_uint16 sc_read_le16(const sc_uint8* bytes)
{
    return (sc_uint16)(bytes[0] | (bytes[1] << 8));
}

static sc_uint32 sc_read_le32(const sc_uint8* bytes)
{
    return (sc_uint32)bytes[0] | ((sc_uint32)bytes[1] << 8) | ((sc_uint32)bytes[2] << 16) | ((sc_uint32)bytes[3] << 24);
}

static sc_bool sc_vfs_read_exact(ma_vfs* vfs, ma_vfs_file file, void* destination, size_t size)
{
    size_t bytesRead = 0;
    return ma_vfs_read(vfs, file, destination, size, &bytesRead) == MA_SUCCESS && bytesRead == size;
}

/**
 * @brief Reads the fmt chunk of a RIFF or RF64 WAV, skipping any chunks (bext, iXML...) before it.
 */
static sc_bool sc_read_wav_format(ma_vfs* vfs, const char* filePath, sc_wav_format_info* info)
{
    enum
    {
        s_waveFormatExtensible = 0xFFFE,
        s_maxChunksBeforeFmt   = 64
    };

    // KSDATAFORMAT_SUBTYPE_AMBISONIC_B_FORMAT_PCM/IEEE_FLOAT share everything after the first two bytes.
    static const sc_uint8 s_bFormatGuidTail[14] = {0x00, 0x00, 0x21, 0x07, 0xD3, 0x11, 0x86, 0x44, 0xC8, 0xC1, 0xCA, 0x00, 0x00, 0x00};

    ma_vfs_file file = NULL;
    if (ma_vfs_open(vfs, filePath, MA_OPEN_MODE_READ, &file) != MA_SUCCESS)
    {
        return SC_FALSE;
    }

    sc_bool found = SC_FALSE;
    sc_uint8 header[12];

    if (sc_vfs_read_exact(vfs, file, header, sizeof(header)) && (memcmp(header, "RIFF", 4) == 0 || memcmp(header, "RF64", 4) == 0 || memcmp(header, "BW64", 4) == 0) && memcmp(header + 8, "WAVE", 4) == 0)
    {
        for (int chunk = 0; chunk < s_maxChunksBeforeFmt; ++chunk)
        {
            sc_uint8 chunkHeader[8];
            if (!sc_vfs_read_exact(vfs, file, chunkHeader, sizeof(chunkHeader)))
            {
                break;
            }

            const sc_uint32 chunkSize = sc_read_le32(chunkHeader + 4);

            if (memcmp(chunkHeader, "fmt ", 4) != 0)
            {
                if (ma_vfs_seek(vfs, file, (ma_int64)chunkSize + (chunkSize & 1), ma_seek_origin_current) != MA_SUCCESS)
                {
                    break;
                }
                continue;
            }

            sc_uint8 fmt[40]         = {0};
            const size_t bytesToRead = SC_MIN((size_t)chunkSize, sizeof(fmt));
            if (bytesToRead < 16 || !sc_vfs_read_exact(vfs, file, fmt, bytesToRead))
            {
                break;
            }

            SC_ZERO_OBJECT(info);
            info->formatTag = sc_read_le16(fmt);
            info->channels  = sc_read_le16(fmt + 2);

            if (info->formatTag == s_waveFormatExtensible && bytesToRead >= 40)
            {
                info->channelMask        = sc_read_le32(fmt + 20);
                info->isAmbisonicBFormat = memcmp(fmt + 26, s_bFormatGuidTail, sizeof(s_bFormatGuidTail)) == 0;
            }

            found = SC_TRUE;
            break;
        }
    }

    ma_vfs_close(vfs, file);
    return found;
}

static sc_uint32 sc_popcount32(sc_uint32 value)
{
    sc_uint32 count = 0;
    for (; value != 0; value &= value - 1)
    {
        ++count;
    }
    return count;
}

/**
 * @brief WAVEFORMATEXTENSIBLE speaker bits run in the same order as miniaudio's positions from MA_CHANNEL_FRONT_LEFT
 * to MA_CHANNEL_TOP_BACK_RIGHT, lowest bit first, which is also the order the channels are stored in.
 */
static void sc_channel_map_from_channel_mask(sc_uint32 mask, ma_channel* channelMap, sc_uint32 channelCount)
{
    const sc_uint32 maskBits = MA_CHANNEL_TOP_BACK_RIGHT - MA_CHANNEL_FRONT_LEFT + 1;

    sc_uint32 channel = 0;
    for (sc_uint32 bit = 0; bit < maskBits && channel < channelCount; ++bit)
    {
        if (mask & (1u << bit))
        {
            channelMap[channel++] = (ma_channel)(MA_CHANNEL_FRONT_LEFT + bit);
        }
    }
    for (; channel < channelCount; ++channel)
    {
        channelMap[channel] = MA_CHANNEL_NONE;
    }
}

/**
 * @brief WAV masks often call 5.1 surrounds "back" where the presets say "side", so those count as the same when
 * there's only one pair.
 */
static void sc_channel_map_surrounds_as_side(ma_channel* channelMap, sc_uint32 channels)
{
    if (sc_channel_map_contains(channelMap, channels, MA_CHANNEL_SIDE_LEFT))
    {
        return;
    }

    for (sc_uint32 channel = 0; channel < channels; ++channel)
    {
        if (channelMap[channel] == MA_CHANNEL_BACK_LEFT)
        {
            channelMap[channel] = MA_CHANNEL_SIDE_LEFT;
        }
        else if (channelMap[channel] == MA_CHANNEL_BACK_RIGHT)
        {
            channelMap[channel] = MA_CHANNEL_SIDE_RIGHT;
        }
    }
}

static sc_bool sc_extension_equals(const char* filePath, const char* extension)
{
    const char* const fileExtension = sc_filename_get_ext(filePath);
    if (fileExtension == NULL || strlen(fileExtension) != strlen(extension))
    {
        return SC_FALSE;
    }

    for (size_t index = 0; extension[index] != '\0'; ++index)
    {
        if (tolower((unsigned char)fileExtension[index]) != extension[index])
        {
            return SC_FALSE;
        }
    }
    return SC_TRUE;
}

sbk_status sc_channel_layout_detect(ma_vfs* vfs, const char* filePath, sc_uint32 channelCount, sc_channel_layout* outLayout, const char** outReason)
{
    SC_CHECK_ARG(filePath != NULL);
    SC_CHECK_ARG(outLayout != NULL);
    SC_CHECK_ARG(channelCount > 0);

    const char* reason = NULL;

    if (sc_extension_equals(filePath, "ogg") || sc_extension_equals(filePath, "oga") || sc_extension_equals(filePath, "opus"))
    {
        *outLayout = sc_guess_vorbis_order_layout(channelCount);
        reason     = "Vorbis channel order";
    }
    else
    {
        ma_default_vfs defaultVFS;
        if (vfs == NULL)
        {
            ma_default_vfs_init(&defaultVFS, NULL);
            vfs = &defaultVFS;
        }

        sc_wav_format_info wavFormat;
        if (channelCount <= SC_MAX_CHANNELS && sc_read_wav_format(vfs, filePath, &wavFormat) && wavFormat.channels == channelCount)
        {
            const sc_uint32 fumaOrder = sc_ambisonic_order_for(channelCount, SC_MAX_FUMA_ORDER);
            if (wavFormat.isAmbisonicBFormat && fumaOrder > 0)
            {
                *outLayout = sc_make_ambisonic(sc_channel_layout_family_fuma, fumaOrder);
                reason     = "B-Format WAV";
            }
            else if (wavFormat.channelMask != 0 && sc_popcount32(wavFormat.channelMask) >= channelCount)
            {
                ma_channel channelMap[SC_MAX_CHANNELS];
                sc_channel_map_from_channel_mask(wavFormat.channelMask, channelMap, channelCount);
                *outLayout = sc_make_speakers("From channel mask", channelMap, channelCount);

                // Named after the matching preset where there is one.
                ma_channel maskAsSide[SC_MAX_CHANNELS];
                memcpy(maskAsSide, channelMap, channelCount * sizeof(ma_channel));
                sc_channel_map_surrounds_as_side(maskAsSide, channelCount);

                sc_channel_layout presets[SC_MAX_CHANNEL_LAYOUT_PRESETS];
                sc_uint32 presetCount = 0;
                sc_channel_layout_get_presets(channelCount, presets, SC_COUNTOF(presets), &presetCount);

                for (sc_uint32 preset = 0; preset < presetCount; ++preset)
                {
                    if (presets[preset].family != sc_channel_layout_family_speakers)
                    {
                        continue;
                    }

                    ma_channel presetAsSide[SC_MAX_CHANNELS];
                    memcpy(presetAsSide, presets[preset].channelMap, channelCount * sizeof(ma_channel));
                    sc_channel_map_surrounds_as_side(presetAsSide, channelCount);

                    if (memcmp(presetAsSide, maskAsSide, channelCount * sizeof(ma_channel)) == 0)
                    {
                        memcpy(outLayout->name, presets[preset].name, sizeof(outLayout->name));
                        break;
                    }
                }
                reason = "WAV channel mask";
            }
        }
    }

    if (reason == NULL)
    {
        *outLayout = sc_guess_wav_order_layout(channelCount);
        reason     = channelCount <= 2 ? "Channel count" : "Guessed from channel count";
    }

    if (outReason != NULL)
    {
        *outReason = reason;
    }
    return SBK_SUCCESS;
}

sbk_status sc_channel_map_describe(const ma_channel* channelMap, sc_uint32 channels, char* description, size_t descriptionSize)
{
    SC_CHECK_ARG(channelMap != NULL || channels == 0);
    SC_CHECK_ARG(description != NULL);
    SC_CHECK_ARG(descriptionSize > 0);
    SC_CHECK_ARG(channels <= SC_MAX_CHANNELS);

    ma_channel positions[SC_MAX_CHANNELS];
    for (sc_uint32 channel = 0; channel < channels; ++channel)
    {
        positions[channel] = sc_output_position(channelMap[channel]);
    }

    sc_channel_layout presets[SC_MAX_CHANNEL_LAYOUT_PRESETS];
    sc_uint32 presetCount = 0;
    sc_channel_layout_get_presets(channels, presets, SC_COUNTOF(presets), &presetCount);

    for (sc_uint32 preset = 0; preset < presetCount; ++preset)
    {
        if (presets[preset].family == sc_channel_layout_family_speakers && sc_channel_map_same_positions(presets[preset].channelMap, positions, channels))
        {
            // Presets name their channel order in brackets, which doesn't apply to a device.
            const char* const bracket = strstr(presets[preset].name, " (");
            const int nameLength      = bracket != NULL ? (int)(bracket - presets[preset].name) : (int)strlen(presets[preset].name);
            snprintf(description, descriptionSize, "%.*s", nameLength, presets[preset].name);
            return SBK_SUCCESS;
        }
    }

    if (channels == 1)
    {
        snprintf(description, descriptionSize, "Mono");
    }
    else if (channels == 2)
    {
        snprintf(description, descriptionSize, "Stereo");
    }
    else
    {
        snprintf(description, descriptionSize, "%u channels", channels);
    }
    return SBK_SUCCESS;
}

/**************************************************************************************************************************************************************

Speaker Routing

**************************************************************************************************************************************************************/

typedef struct sc_fold_target
{
    ma_channel position;
    float gain;
} sc_fold_target;

typedef struct sc_fold_option
{
    sc_uint32 targetCount;
    sc_fold_target targets[2];
} sc_fold_option;

typedef struct sc_fold_options
{
    sc_uint32 optionCount;
    sc_fold_option options[3];
} sc_fold_options;

/**
 * @brief Where a speaker's signal goes when the output doesn't have it, best option first.
 * Follows the ITU-R BS.775 downmix: centre and surrounds at -3 dB, LFE dropped.
 */
static const sc_fold_options s_foldOptions[MA_CHANNEL_TOP_BACK_RIGHT + 1] = {
    [MA_CHANNEL_MONO]               = {2, {{2, {{MA_CHANNEL_FRONT_LEFT, 1.0f}, {MA_CHANNEL_FRONT_RIGHT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_CENTER, 1.0f}}}}},
    [MA_CHANNEL_FRONT_CENTER]       = {1, {{2, {{MA_CHANNEL_FRONT_LEFT, SC_MINUS_3DB}, {MA_CHANNEL_FRONT_RIGHT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_FRONT_LEFT]         = {2, {{1, {{MA_CHANNEL_FRONT_LEFT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_CENTER, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_FRONT_LEFT_CENTER]  = {2, {{1, {{MA_CHANNEL_FRONT_LEFT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_CENTER, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_FRONT_RIGHT]        = {2, {{1, {{MA_CHANNEL_FRONT_RIGHT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_CENTER, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_FRONT_RIGHT_CENTER] = {2, {{1, {{MA_CHANNEL_FRONT_RIGHT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_CENTER, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_SIDE_LEFT]          = {2, {{1, {{MA_CHANNEL_BACK_LEFT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_LEFT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_SIDE_RIGHT]         = {2, {{1, {{MA_CHANNEL_BACK_RIGHT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_RIGHT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_BACK_LEFT]          = {2, {{1, {{MA_CHANNEL_SIDE_LEFT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_LEFT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_BACK_RIGHT]         = {2, {{1, {{MA_CHANNEL_SIDE_RIGHT, 1.0f}}}, {1, {{MA_CHANNEL_FRONT_RIGHT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_BACK_CENTER]        = {3, {{2, {{MA_CHANNEL_BACK_LEFT, SC_MINUS_3DB}, {MA_CHANNEL_BACK_RIGHT, SC_MINUS_3DB}}}, {2, {{MA_CHANNEL_SIDE_LEFT, SC_MINUS_3DB}, {MA_CHANNEL_SIDE_RIGHT, SC_MINUS_3DB}}}, {2, {{MA_CHANNEL_FRONT_LEFT, 0.5f}, {MA_CHANNEL_FRONT_RIGHT, 0.5f}}}}},
    [MA_CHANNEL_TOP_FRONT_LEFT]     = {1, {{1, {{MA_CHANNEL_FRONT_LEFT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_TOP_FRONT_RIGHT]    = {1, {{1, {{MA_CHANNEL_FRONT_RIGHT, SC_MINUS_3DB}}}}},
    [MA_CHANNEL_TOP_FRONT_CENTER]   = {2, {{1, {{MA_CHANNEL_FRONT_CENTER, SC_MINUS_3DB}}}, {2, {{MA_CHANNEL_FRONT_LEFT, 0.5f}, {MA_CHANNEL_FRONT_RIGHT, 0.5f}}}}},
    [MA_CHANNEL_TOP_BACK_LEFT]      = {3, {{1, {{MA_CHANNEL_BACK_LEFT, SC_MINUS_3DB}}}, {1, {{MA_CHANNEL_SIDE_LEFT, SC_MINUS_3DB}}}, {1, {{MA_CHANNEL_FRONT_LEFT, 0.5f}}}}},
    [MA_CHANNEL_TOP_BACK_RIGHT]     = {3, {{1, {{MA_CHANNEL_BACK_RIGHT, SC_MINUS_3DB}}}, {1, {{MA_CHANNEL_SIDE_RIGHT, SC_MINUS_3DB}}}, {1, {{MA_CHANNEL_FRONT_RIGHT, 0.5f}}}}},
    [MA_CHANNEL_TOP_BACK_CENTER]    = {3, {{2, {{MA_CHANNEL_BACK_LEFT, 0.5f}, {MA_CHANNEL_BACK_RIGHT, 0.5f}}}, {2, {{MA_CHANNEL_SIDE_LEFT, 0.5f}, {MA_CHANNEL_SIDE_RIGHT, 0.5f}}}, {2, {{MA_CHANNEL_FRONT_LEFT, 0.35f}, {MA_CHANNEL_FRONT_RIGHT, 0.35f}}}}},
    [MA_CHANNEL_TOP_CENTER]         = {1, {{2, {{MA_CHANNEL_FRONT_LEFT, 0.5f}, {MA_CHANNEL_FRONT_RIGHT, 0.5f}}}}},
};

/**
 * @return The first output channel at @p position, or -1 if the output doesn't have it.
 */
static int sc_find_output(const ma_channel* outputPositions, sc_uint32 outputChannels, ma_channel position)
{
    for (sc_uint32 output = 0; output < outputChannels; ++output)
    {
        if (outputPositions[output] == position)
        {
            return (int)output;
        }
    }
    return -1;
}

/**
 * @brief Adds a speaker's gains onto the output, folding it onto other speakers when the output doesn't have it.
 */
static void sc_route_speaker(ma_channel position, float gain, const ma_channel* outputPositions, sc_uint32 outputChannels, float* outputGains, int depth)
{
    if (position != MA_CHANNEL_MONO)
    {
        const int output = sc_find_output(outputPositions, outputChannels, position);
        if (output >= 0)
        {
            outputGains[output] += gain;
            return;
        }
    }

    if (position > MA_CHANNEL_TOP_BACK_RIGHT || depth >= SC_MAX_FOLD_DEPTH)
    {
        return;
    }

    const sc_fold_options* const options = &s_foldOptions[position];
    if (options->optionCount == 0)
    {
        return;
    }

    for (sc_uint32 optionIndex = 0; optionIndex < options->optionCount; ++optionIndex)
    {
        const sc_fold_option* const option = &options->options[optionIndex];

        sc_bool outputHasAll = SC_TRUE;
        for (sc_uint32 target = 0; target < option->targetCount; ++target)
        {
            outputHasAll = outputHasAll && sc_find_output(outputPositions, outputChannels, option->targets[target].position) >= 0;
        }

        if (outputHasAll)
        {
            for (sc_uint32 target = 0; target < option->targetCount; ++target)
            {
                outputGains[sc_find_output(outputPositions, outputChannels, option->targets[target].position)] += gain * option->targets[target].gain;
            }
            return;
        }
    }

    const sc_fold_option* const lastResort = &options->options[options->optionCount - 1];
    for (sc_uint32 target = 0; target < lastResort->targetCount; ++target)
    {
        if (lastResort->targets[target].position != position)
        {
            sc_route_speaker(lastResort->targets[target].position, gain * lastResort->targets[target].gain, outputPositions, outputChannels, outputGains, depth + 1);
        }
    }
}

/**************************************************************************************************************************************************************

Ambisonic Decoding

**************************************************************************************************************************************************************/

typedef struct sc_ambisonic_component
{
    sc_uint32 acn;
    float toSn3d;  //< Gain that brings the channel to SN3D
} sc_ambisonic_component;

/**
 * @brief Where each FuMa channel sits in ACN order, and the weight that undoes FuMa's normalisation.
 */
static sc_ambisonic_component sc_fuma_component(sc_uint32 fumaIndex)
{
    const float s2nd = 2.0f / sqrtf(3.0f);
    const float sLM  = sqrtf(45.0f / 32.0f);
    const float sNO  = 3.0f / sqrtf(5.0f);
    const float sPQ  = sqrtf(8.0f / 5.0f);

    const sc_ambisonic_component s_components[16] = {
        {0, 1.41421356f},  // W
        {3, 1.0f},         // X
        {1, 1.0f},         // Y
        {2, 1.0f},         // Z
        {6, 1.0f},         // R
        {7, s2nd},         // S
        {5, s2nd},         // T
        {8, s2nd},         // U
        {4, s2nd},         // V
        {12, 1.0f},        // K
        {13, sLM},         // L
        {11, sLM},         // M
        {14, sNO},         // N
        {10, sNO},         // O
        {15, sPQ},         // P
        {9, sPQ},          // Q
    };

    SC_ASSERT(fumaIndex < SC_COUNTOF(s_components));
    return s_components[fumaIndex];
}

/**
 * @brief Associated Legendre function P(n, m) without the Condon-Shortley phase, as ambisonics uses.
 */
static double sc_associated_legendre(int degree, int order, double x)
{
    // P(m, m), then up through the degrees with the standard recurrence.
    const double sinTheta = sqrt(SC_MAX(0.0, 1.0 - x * x));
    double pmm            = 1.0;
    for (int index = 1; index <= order; ++index)
    {
        pmm *= (double)(2 * index - 1) * sinTheta;
    }
    if (degree == order)
    {
        return pmm;
    }

    double pPrevious = pmm;
    double pCurrent  = x * (double)(2 * order + 1) * pmm;
    for (int n = order + 2; n <= degree; ++n)
    {
        const double pNext = ((double)(2 * n - 1) * x * pCurrent - (double)(n + order - 1) * pPrevious) / (double)(n - order);
        pPrevious          = pCurrent;
        pCurrent           = pNext;
    }
    return pCurrent;
}

static double sc_factorial(int value)
{
    double result = 1.0;
    for (int index = 2; index <= value; ++index)
    {
        result *= (double)index;
    }
    return result;
}

/**
 * @brief Real spherical harmonics with SN3D normalisation, indexed by ACN, for a direction in radians.
 * Azimuth turns anticlockwise (to the left) from straight ahead.
 */
static float sc_spherical_harmonic_sn3d(sc_uint32 acn, float azimuth, float elevation)
{
    const int degree = (int)sqrt((double)acn);
    const int index  = (int)acn - degree * degree - degree;  // -degree..degree
    const int order  = abs(index);

    const double normalisation = sqrt((order == 0 ? 1.0 : 2.0) * sc_factorial(degree - order) / sc_factorial(degree + order));
    const double legendreValue = sc_associated_legendre(degree, order, sin((double)elevation));
    const double azimuthal     = index >= 0 ? cos(order * (double)azimuth) : sin(order * (double)azimuth);

    return (float)(normalisation * legendreValue * azimuthal);
}

typedef struct sc_direction
{
    float azimuthDegrees;
    float elevationDegrees;
} sc_direction;

/**
 * @brief Nominal speaker positions (ITU-R BS.2051 style), used to decode ambisonics onto the output.
 */
static sc_bool sc_speaker_direction(ma_channel position, sc_direction* direction)
{
    switch (position)
    {
        case MA_CHANNEL_FRONT_LEFT:
            *direction = (sc_direction){30.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_FRONT_RIGHT:
            *direction = (sc_direction){-30.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_FRONT_CENTER:
            *direction = (sc_direction){0.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_BACK_LEFT:
            *direction = (sc_direction){145.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_BACK_RIGHT:
            *direction = (sc_direction){-145.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_FRONT_LEFT_CENTER:
            *direction = (sc_direction){15.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_FRONT_RIGHT_CENTER:
            *direction = (sc_direction){-15.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_BACK_CENTER:
            *direction = (sc_direction){180.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_SIDE_LEFT:
            *direction = (sc_direction){100.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_SIDE_RIGHT:
            *direction = (sc_direction){-100.0f, 0.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_CENTER:
            *direction = (sc_direction){0.0f, 90.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_FRONT_LEFT:
            *direction = (sc_direction){45.0f, 40.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_FRONT_CENTER:
            *direction = (sc_direction){0.0f, 40.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_FRONT_RIGHT:
            *direction = (sc_direction){-45.0f, 40.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_BACK_LEFT:
            *direction = (sc_direction){135.0f, 40.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_BACK_CENTER:
            *direction = (sc_direction){180.0f, 40.0f};
            return SC_TRUE;
        case MA_CHANNEL_TOP_BACK_RIGHT:
            *direction = (sc_direction){-135.0f, 40.0f};
            return SC_TRUE;
        default:
            return SC_FALSE;
    }
}

static float sc_to_radians(float degrees)
{
    return degrees * (float)SC_PI / 180.0f;
}

typedef struct sc_vec3
{
    double x;
    double y;
    double z;
} sc_vec3;

static sc_vec3 sc_vec3_sub(sc_vec3 a, sc_vec3 b)
{
    return (sc_vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static double sc_vec3_dot(sc_vec3 a, sc_vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static sc_vec3 sc_vec3_cross(sc_vec3 a, sc_vec3 b)
{
    return (sc_vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static double sc_vec3_determinant(sc_vec3 a, sc_vec3 b, sc_vec3 c)
{
    return sc_vec3_dot(a, sc_vec3_cross(b, c));
}

static sc_vec3 sc_unit_vector(sc_direction direction)
{
    const double azimuth   = sc_to_radians(direction.azimuthDegrees);
    const double elevation = sc_to_radians(direction.elevationDegrees);
    return (sc_vec3){cos(elevation) * cos(azimuth), cos(elevation) * sin(azimuth), sin(elevation)};
}

/**
 * @brief One of @p count points spread almost evenly over the unit sphere.
 */
static sc_vec3 sc_fibonacci_sphere_point(sc_uint32 index, sc_uint32 count)
{
    const double goldenAngle = SC_PI * (3.0 - sqrt(5.0));

    const double z      = 1.0 - 2.0 * ((double)index + 0.5) / (double)count;
    const double radius = sqrt(1.0 - z * z);
    const double angle  = goldenAngle * (double)index;
    return (sc_vec3){radius * cos(angle), radius * sin(angle), z};
}

typedef struct sc_triangle
{
    sc_uint8 corners[3];
} sc_triangle;

/**
 * @brief Working space for building a decoder, too big for the stack.
 */
typedef struct sc_ambisonic_decoder
{
    float gains[SC_MAX_CHANNELS][SC_MAX_CHANNELS];  //< [output][ACN]

    sc_uint32 positionedOutputs[SC_MAX_CHANNELS];
    sc_direction positionedDirections[SC_MAX_CHANNELS];
    sc_vec3 points[SC_MAX_DECODER_POINTS];
    double imaginaryShares[2][SC_MAX_CHANNELS];
    double realGains[SC_MAX_CHANNELS][SC_MAX_CHANNELS];  //< [positioned output][ACN]
    double components[SC_MAX_CHANNELS];
    double vbapGains[SC_MAX_DECODER_POINTS];

    sc_triangle* faces;
    size_t faceCount;
} sc_ambisonic_decoder;

/**
 * @brief Faces of the convex hull around points on the unit sphere: every triangle with no points on its outer side.
 * Coplanar points give overlapping faces, which @ref sc_vbap_gains copes with.
 */
static sbk_status sc_convex_hull_triangles(sc_ambisonic_decoder* decoder, sc_uint32 pointCount)
{
    const double s_epsilon = 1e-9;

    const size_t capacity = (size_t)pointCount * (pointCount - 1) * (pointCount - 2) / 6;
    decoder->faces        = (sc_triangle*)ma_malloc(SC_MAX(capacity, (size_t)1) * sizeof(sc_triangle), NULL);
    decoder->faceCount    = 0;
    SC_CHECK_MEM(decoder->faces);

    const sc_vec3* const points = decoder->points;

    for (sc_uint32 first = 0; first < pointCount; ++first)
    {
        for (sc_uint32 second = first + 1; second < pointCount; ++second)
        {
            for (sc_uint32 third = second + 1; third < pointCount; ++third)
            {
                const sc_vec3 normal = sc_vec3_cross(sc_vec3_sub(points[second], points[first]), sc_vec3_sub(points[third], points[first]));
                if (sc_vec3_dot(normal, normal) < s_epsilon)
                {
                    continue;
                }

                sc_bool anyAbove = SC_FALSE;
                sc_bool anyBelow = SC_FALSE;
                for (sc_uint32 other = 0; other < pointCount; ++other)
                {
                    const double side = sc_vec3_dot(normal, sc_vec3_sub(points[other], points[first]));
                    anyAbove          = anyAbove || side > s_epsilon;
                    anyBelow          = anyBelow || side < -s_epsilon;
                }

                if (!(anyAbove && anyBelow))
                {
                    decoder->faces[decoder->faceCount++] = (sc_triangle){{(sc_uint8)first, (sc_uint8)second, (sc_uint8)third}};
                }
            }
        }
    }
    return SBK_SUCCESS;
}

/**
 * @brief Energy-normalised VBAP gains that place @p target between the speakers at the decoder's points, written to
 * @ref sc_ambisonic_decoder::vbapGains.
 *
 * Picks the smallest face that holds the direction, so overlapping faces from coplanar speakers don't skip the one in
 * the middle. Layouts that only cover part of the sphere (LCR) leave some directions outside every face; those go to
 * the face they're nearest, without its negative gains.
 */
static void sc_vbap_gains(sc_ambisonic_decoder* decoder, sc_vec3 target, sc_uint32 pointCount)
{
    const double s_tolerance = 1e-6;

    const sc_triangle* inside = NULL;
    double insideGains[3]     = {0};
    double insideArea         = DBL_MAX;

    const sc_triangle* nearest = NULL;
    double nearestGains[3]     = {0};
    double nearestMinGain      = -DBL_MAX;

    for (size_t faceIndex = 0; faceIndex < decoder->faceCount; ++faceIndex)
    {
        const sc_triangle* const face = &decoder->faces[faceIndex];
        const sc_vec3 a               = decoder->points[face->corners[0]];
        const sc_vec3 b               = decoder->points[face->corners[1]];
        const sc_vec3 c               = decoder->points[face->corners[2]];
        const double base             = sc_vec3_determinant(a, b, c);
        if (fabs(base) < 1e-12)
        {
            continue;
        }

        const double gains[3] = {sc_vec3_determinant(target, b, c) / base, sc_vec3_determinant(a, target, c) / base, sc_vec3_determinant(a, b, target) / base};
        const double minGain  = SC_MIN(gains[0], SC_MIN(gains[1], gains[2]));

        if (minGain >= -s_tolerance)
        {
            const sc_vec3 normal = sc_vec3_cross(sc_vec3_sub(b, a), sc_vec3_sub(c, a));
            const double area    = sc_vec3_dot(normal, normal);
            if (area < insideArea)
            {
                insideArea = area;
                inside     = face;
                memcpy(insideGains, gains, sizeof(gains));
            }
        }
        else if (minGain > nearestMinGain)
        {
            nearestMinGain = minGain;
            nearest        = face;
            memcpy(nearestGains, gains, sizeof(gains));
        }
    }

    for (sc_uint32 point = 0; point < pointCount; ++point)
    {
        decoder->vbapGains[point] = 0.0;
    }

    const sc_triangle* const chosen = inside != NULL ? inside : nearest;
    const double* const chosenGains = inside != NULL ? insideGains : nearestGains;
    if (chosen == NULL)
    {
        return;
    }

    double energy = 0.0;
    for (sc_uint32 corner = 0; corner < 3; ++corner)
    {
        const double gain                           = SC_MAX(0.0, chosenGains[corner]);
        decoder->vbapGains[chosen->corners[corner]] = gain;
        energy += gain * gain;
    }

    if (energy > 0.0)
    {
        for (sc_uint32 point = 0; point < pointCount; ++point)
        {
            decoder->vbapGains[point] /= sqrt(energy);
        }
    }
}

static void sc_encode_direction(sc_ambisonic_decoder* decoder, sc_vec3 point, sc_uint32 componentCount)
{
    const float azimuth   = (float)atan2(point.y, point.x);
    const float elevation = (float)asin(SC_MAX(-1.0, SC_MIN(1.0, point.z)));
    for (sc_uint32 acn = 0; acn < componentCount; ++acn)
    {
        decoder->components[acn] = sc_spherical_harmonic_sn3d(acn, azimuth, elevation);
    }
}

/**
 * @brief Fills @ref sc_ambisonic_decoder::gains with the gain from each ACN component (SN3D) to each output channel.
 *
 * Two speakers get a pair of virtual cardioids at +-60 degrees, which images well on headphones and stereo speakers.
 * More get an AllRAD decoder (Zotter and Frank): a max-rE sampling decoder onto an even sphere of virtual speakers,
 * where it behaves at any order, with each virtual speaker panned onto the real ones by VBAP. Gaps in the layout, like
 * the back of 5.1, are bridged by panning instead of left as holes.
 */
static sbk_status sc_build_ambisonic_decoder(sc_ambisonic_decoder* decoder, sc_uint32 fileOrder, const ma_channel* outputPositions, sc_uint32 outputChannels)
{
    const sc_uint32 componentCount = (fileOrder + 1) * (fileOrder + 1);

    sc_uint32 positionedCount = 0;
    for (sc_uint32 output = 0; output < outputChannels; ++output)
    {
        sc_direction direction;
        if (sc_speaker_direction(outputPositions[output], &direction))
        {
            decoder->positionedOutputs[positionedCount]    = output;
            decoder->positionedDirections[positionedCount] = direction;
            ++positionedCount;
        }
    }

    if (positionedCount == 0)
    {
        if (outputChannels > 0)
        {
            decoder->gains[0][0] = 1.0f;
        }
        return SBK_SUCCESS;
    }

    if (positionedCount == 1)
    {
        decoder->gains[decoder->positionedOutputs[0]][0] = 1.0f;
        return SBK_SUCCESS;
    }

    if (positionedCount == 2)
    {
        const float s_cardioidAzimuth = 60.0f;

        for (sc_uint32 positioned = 0; positioned < positionedCount; ++positioned)
        {
            const sc_uint32 output    = decoder->positionedOutputs[positioned];
            const float azimuth       = sc_to_radians(decoder->positionedDirections[positioned].azimuthDegrees >= 0.0f ? s_cardioidAzimuth : -s_cardioidAzimuth);
            decoder->gains[output][0] = 0.5f;
            decoder->gains[output][1] = 0.5f * sinf(azimuth);
            decoder->gains[output][3] = 0.5f * cosf(azimuth);
        }
        return SBK_SUCCESS;
    }

    float lowestElevation  = FLT_MAX;
    float highestElevation = -FLT_MAX;
    for (sc_uint32 positioned = 0; positioned < positionedCount; ++positioned)
    {
        decoder->points[positioned] = sc_unit_vector(decoder->positionedDirections[positioned]);
        lowestElevation             = SC_MIN(lowestElevation, decoder->positionedDirections[positioned].elevationDegrees);
        highestElevation            = SC_MAX(highestElevation, decoder->positionedDirections[positioned].elevationDegrees);
    }
    const sc_uint32 realCount = positionedCount;
    sc_uint32 pointCount      = realCount;

    // The hull has to surround the listener, so layouts with nothing overhead or below get an imaginary speaker there,
    // shared between the real speakers next to it.
    const float s_poleCoverage = 60.0f;
    if (highestElevation < s_poleCoverage)
    {
        decoder->points[pointCount++] = (sc_vec3){0.0, 0.0, 1.0};
    }
    if (lowestElevation > -s_poleCoverage)
    {
        decoder->points[pointCount++] = (sc_vec3){0.0, 0.0, -1.0};
    }

    SC_CHECK_STATUS(sc_convex_hull_triangles(decoder, pointCount));

    const sc_uint32 imaginaryCount = pointCount - realCount;
    for (sc_uint32 imaginary = 0; imaginary < imaginaryCount; ++imaginary)
    {
        const sc_uint32 pole = realCount + imaginary;

        sc_bool isNeighbour[SC_MAX_CHANNELS] = {SC_FALSE};
        for (size_t faceIndex = 0; faceIndex < decoder->faceCount; ++faceIndex)
        {
            const sc_triangle* const face = &decoder->faces[faceIndex];
            if (face->corners[0] == pole || face->corners[1] == pole || face->corners[2] == pole)
            {
                for (sc_uint32 corner = 0; corner < 3; ++corner)
                {
                    if (face->corners[corner] < realCount)
                    {
                        isNeighbour[face->corners[corner]] = SC_TRUE;
                    }
                }
            }
        }

        sc_uint32 neighbourCount = 0;
        for (sc_uint32 real = 0; real < realCount; ++real)
        {
            neighbourCount += isNeighbour[real] ? 1 : 0;
        }
        for (sc_uint32 real = 0; real < realCount; ++real)
        {
            decoder->imaginaryShares[imaginary][real] = (isNeighbour[real] && neighbourCount > 0) ? 1.0 / sqrt((double)neighbourCount) : 0.0;
        }
    }

    const float maxRe = cosf(sc_to_radians(137.9f / ((float)fileOrder + 1.51f)));

    double weightEnergy                             = 0.0;
    double orderWeights[SC_MAX_AMBISONIC_ORDER + 1] = {0};
    for (sc_uint32 order = 0; order <= fileOrder; ++order)
    {
        orderWeights[order] = (float)sc_associated_legendre((int)order, 0, (double)maxRe);
        weightEnergy += (double)(2 * order + 1) * orderWeights[order] * orderWeights[order];
    }

    const sc_uint32 s_virtualSpeakerCount = 400;
    const double virtualNormalisation     = 1.0 / sqrt((double)s_virtualSpeakerCount * weightEnergy);

    for (sc_uint32 virtualIndex = 0; virtualIndex < s_virtualSpeakerCount; ++virtualIndex)
    {
        const sc_vec3 virtualSpeaker = sc_fibonacci_sphere_point(virtualIndex, s_virtualSpeakerCount);
        sc_vbap_gains(decoder, virtualSpeaker, pointCount);
        sc_encode_direction(decoder, virtualSpeaker, componentCount);

        for (sc_uint32 real = 0; real < realCount; ++real)
        {
            double gain = decoder->vbapGains[real];
            for (sc_uint32 imaginary = 0; imaginary < imaginaryCount; ++imaginary)
            {
                gain += decoder->vbapGains[realCount + imaginary] * decoder->imaginaryShares[imaginary][real];
            }

            if (gain == 0.0)
            {
                continue;
            }

            for (sc_uint32 acn = 0; acn < componentCount; ++acn)
            {
                const sc_uint32 order = (sc_uint32)sqrt((double)acn);
                decoder->realGains[real][acn] += gain * virtualNormalisation * (double)(2 * order + 1) * orderWeights[order] * decoder->components[acn];
            }
        }
    }

    // Panning neighbouring virtual speakers onto the same real ones adds them up in amplitude, so level the result to
    // unit energy averaged over every direction.
    const sc_uint32 s_levelDirectionCount = 256;
    double totalEnergy                    = 0.0;
    for (sc_uint32 directionIndex = 0; directionIndex < s_levelDirectionCount; ++directionIndex)
    {
        sc_encode_direction(decoder, sc_fibonacci_sphere_point(directionIndex, s_levelDirectionCount), componentCount);
        for (sc_uint32 real = 0; real < realCount; ++real)
        {
            double sample = 0.0;
            for (sc_uint32 acn = 0; acn < componentCount; ++acn)
            {
                sample += decoder->realGains[real][acn] * decoder->components[acn];
            }
            totalEnergy += sample * sample;
        }
    }
    const double level = totalEnergy > 0.0 ? 1.0 / sqrt(totalEnergy / (double)s_levelDirectionCount) : 1.0;

    for (sc_uint32 real = 0; real < realCount; ++real)
    {
        for (sc_uint32 acn = 0; acn < componentCount; ++acn)
        {
            decoder->gains[decoder->positionedOutputs[real]][acn] = (float)(decoder->realGains[real][acn] * level);
        }
    }

    return SBK_SUCCESS;
}

/**************************************************************************************************************************************************************

Mix Matrix

**************************************************************************************************************************************************************/

sbk_status sc_channel_layout_build_mix_matrix(const sc_channel_layout* layout, const sc_uint32* channelOrder, const sc_bool* soloedChannels, const ma_channel* outputChannelMap, sc_uint32 outputChannels, float* outMatrix)
{
    SC_CHECK_ARG(layout != NULL);
    SC_CHECK_ARG(outputChannelMap != NULL || outputChannels == 0);
    SC_CHECK_ARG(outMatrix != NULL);
    SC_CHECK_ARG(layout->channelCount <= SC_MAX_CHANNELS);
    SC_CHECK_ARG(outputChannels <= SC_MAX_CHANNELS);
    SC_CHECK_ARG(layout->family != sc_channel_layout_family_fuma || layout->channelCount <= (SC_MAX_FUMA_ORDER + 1) * (SC_MAX_FUMA_ORDER + 1));

    const sc_uint32 fileChannels = layout->channelCount;
    memset(outMatrix, 0, (size_t)fileChannels * outputChannels * sizeof(float));

    ma_channel outputPositions[SC_MAX_CHANNELS];
    for (sc_uint32 output = 0; output < outputChannels; ++output)
    {
        outputPositions[output] = sc_output_position(outputChannelMap[output]);
    }

    sc_bool anySolo = SC_FALSE;
    for (sc_uint32 fileChannel = 0; soloedChannels != NULL && fileChannel < fileChannels; ++fileChannel)
    {
        anySolo = anySolo || soloedChannels[fileChannel];
    }

#define SC_FILE_CHANNEL_FOR_SLOT(slot) (channelOrder != NULL ? channelOrder[(slot)] : (slot))
#define SC_IS_MUTED(fileChannel)       (anySolo && !soloedChannels[(fileChannel)])

    float outputGains[SC_MAX_CHANNELS];

    // A single ambisonic component has no direction of its own, so soloed components play raw on the front speakers.
    if (sc_channel_layout_is_ambisonic(layout) && anySolo)
    {
        for (sc_uint32 fileChannel = 0; fileChannel < fileChannels; ++fileChannel)
        {
            if (!SC_IS_MUTED(fileChannel))
            {
                memset(outputGains, 0, sizeof(outputGains));
                sc_route_speaker(MA_CHANNEL_MONO, 1.0f, outputPositions, outputChannels, outputGains, 0);
                for (sc_uint32 output = 0; output < outputChannels; ++output)
                {
                    outMatrix[output * fileChannels + fileChannel] += outputGains[output];
                }
            }
        }
        return SBK_SUCCESS;
    }

    switch (layout->family)
    {
        case sc_channel_layout_family_speakers:
        {
            for (sc_uint32 slot = 0; slot < fileChannels; ++slot)
            {
                const sc_uint32 fileChannel = SC_FILE_CHANNEL_FOR_SLOT(slot);
                if (fileChannel < fileChannels && !SC_IS_MUTED(fileChannel))
                {
                    memset(outputGains, 0, sizeof(outputGains));
                    sc_route_speaker(sc_layout_position(layout->channelMap[slot]), 1.0f, outputPositions, outputChannels, outputGains, 0);
                    for (sc_uint32 output = 0; output < outputChannels; ++output)
                    {
                        outMatrix[output * fileChannels + fileChannel] += outputGains[output];
                    }
                }
            }
            break;
        }
        case sc_channel_layout_family_direct:
        {
            for (sc_uint32 slot = 0; slot < fileChannels && slot < outputChannels; ++slot)
            {
                const sc_uint32 fileChannel = SC_FILE_CHANNEL_FOR_SLOT(slot);
                if (fileChannel < fileChannels && !SC_IS_MUTED(fileChannel))
                {
                    outMatrix[slot * fileChannels + fileChannel] = 1.0f;
                }
            }
            break;
        }
        case sc_channel_layout_family_ambix:
        case sc_channel_layout_family_fuma:
        {
            sc_ambisonic_decoder* const decoder = (sc_ambisonic_decoder*)ma_calloc(sizeof(sc_ambisonic_decoder), NULL);
            SC_CHECK_MEM(decoder);

            const sbk_status status = sc_build_ambisonic_decoder(decoder, sc_channel_layout_get_ambisonic_order(layout), outputPositions, outputChannels);
            if (status == SBK_SUCCESS)
            {
                for (sc_uint32 slot = 0; slot < fileChannels; ++slot)
                {
                    const sc_uint32 fileChannel = SC_FILE_CHANNEL_FOR_SLOT(slot);
                    if (fileChannel >= fileChannels)
                    {
                        continue;
                    }

                    const sc_ambisonic_component component = layout->family == sc_channel_layout_family_fuma ? sc_fuma_component(slot) : (sc_ambisonic_component){slot, 1.0f};
                    for (sc_uint32 output = 0; output < outputChannels; ++output)
                    {
                        outMatrix[output * fileChannels + fileChannel] += decoder->gains[output][component.acn] * component.toSn3d;
                    }
                }
            }

            ma_free(decoder->faces, NULL);
            ma_free(decoder, NULL);
            return status;
        }
    }

#undef SC_FILE_CHANNEL_FOR_SLOT
#undef SC_IS_MUTED

    return SBK_SUCCESS;
}
