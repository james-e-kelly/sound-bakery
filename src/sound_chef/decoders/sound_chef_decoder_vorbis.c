
// clang-format off
#include "sound_chef/sound_chef.h"

#if defined(MA_DLL)
    #undef MA_API
    #define MA_API MA_DLL_EXPORT
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "extras/miniaudio_libvorbis.h"
// clang-format on

/**
 * @brief miniaudio's libvorbis wrapper always reports a length of 0, which leaves sounds without a duration
 * and stops decoders from reading them in one go. libvorbis knows the length of any seekable stream.
 */
static ma_result sc_libvorbis_ds_get_length(ma_data_source* pDataSource, ma_uint64* pLength)
{
    ma_libvorbis* const pVorbis = (ma_libvorbis*)pDataSource;

    *pLength = 0;

    const ogg_int64_t totalFrames = ov_pcm_total(&pVorbis->vf, -1);
    if (totalFrames > 0)
    {
        *pLength = (ma_uint64)totalFrames;
    }

    return MA_SUCCESS;
}

static ma_data_source_vtable s_libvorbisDataSourceVtable =
{
    ma_libvorbis_ds_read,
    ma_libvorbis_ds_seek,
    ma_libvorbis_ds_get_data_format,
    ma_libvorbis_ds_get_cursor,
    sc_libvorbis_ds_get_length
};

static ma_result ma_decoding_backend_init__libvorbis(void* pUserData,
                                                     ma_read_proc onRead,
                                                     ma_seek_proc onSeek,
                                                     ma_tell_proc onTell,
                                                     void* pReadSeekTellUserData,
                                                     const ma_decoding_backend_config* pConfig,
                                                     const ma_allocation_callbacks* pAllocationCallbacks,
                                                     ma_data_source** ppBackend)
{
    ma_result result;
    ma_libvorbis* pVorbis;

    (void)pUserData;

    pVorbis = (ma_libvorbis*)ma_malloc(sizeof(*pVorbis), pAllocationCallbacks);
    if (pVorbis == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libvorbis_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pVorbis);
    if (result != MA_SUCCESS)
    {
        ma_free(pVorbis, pAllocationCallbacks);
        return result;
    }

    pVorbis->ds.vtable = &s_libvorbisDataSourceVtable;
    *ppBackend         = pVorbis;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__libvorbis(void* pUserData,
                                                          const char* pFilePath,
                                                          const ma_decoding_backend_config* pConfig,
                                                          const ma_allocation_callbacks* pAllocationCallbacks,
                                                          ma_data_source** ppBackend)
{
    ma_result result;
    ma_libvorbis* pVorbis;

    (void)pUserData;

    pVorbis = (ma_libvorbis*)ma_malloc(sizeof(*pVorbis), pAllocationCallbacks);
    if (pVorbis == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libvorbis_init_file(pFilePath, pConfig, pAllocationCallbacks, pVorbis);
    if (result != MA_SUCCESS)
    {
        ma_free(pVorbis, pAllocationCallbacks);
        return result;
    }

    pVorbis->ds.vtable = &s_libvorbisDataSourceVtable;
    *ppBackend         = pVorbis;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__libvorbis(void* pUserData,
                                                  ma_data_source* pBackend,
                                                  const ma_allocation_callbacks* pAllocationCallbacks)
{
    ma_libvorbis* pVorbis = (ma_libvorbis*)pBackend;

    (void)pUserData;

    ma_libvorbis_uninit(pVorbis, pAllocationCallbacks);
    ma_free(pVorbis, pAllocationCallbacks);
}

static ma_result ma_decoding_backend_get_channel_map__libvorbis(void* pUserData,
                                                                ma_data_source* pBackend,
                                                                ma_channel* pChannelMap,
                                                                size_t channelMapCap)
{
    ma_libvorbis* pVorbis = (ma_libvorbis*)pBackend;

    (void)pUserData;

    return ma_libvorbis_get_data_format(pVorbis, NULL, NULL, NULL, pChannelMap, channelMapCap);
}

ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libvorbis = 
{
    ma_decoding_backend_init__libvorbis, 
    ma_decoding_backend_init_file__libvorbis, 
    NULL,   /* onInitFileW() */
    NULL,   /* onInitMemory() */
    ma_decoding_backend_uninit__libvorbis
};