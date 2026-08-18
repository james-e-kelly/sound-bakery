
// clang-format off
#include "sound_chef/sound_chef.h"

#if defined(MA_DLL)
    #undef MA_API
    #define MA_API MA_DLL_EXPORT
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "extras/miniaudio_libopus.h"
// clang-format on

static ma_result ma_decoding_backend_init__libopus(void* pUserData,
                                                   ma_read_proc onRead,
                                                   ma_seek_proc onSeek,
                                                   ma_tell_proc onTell,
                                                   void* pReadSeekTellUserData,
                                                   const ma_decoding_backend_config* pConfig,
                                                   const ma_allocation_callbacks* pAllocationCallbacks,
                                                   ma_data_source** ppBackend)
{
    ma_result result;
    ma_libopus* pOpus;

    (void)pUserData;

    pOpus = (ma_libopus*)ma_malloc(sizeof(*pOpus), pAllocationCallbacks);
    if (pOpus == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libopus_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pOpus);
    if (result != MA_SUCCESS)
    {
        ma_free(pOpus, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pOpus;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__libopus(void* pUserData,
                                                        const char* pFilePath,
                                                        const ma_decoding_backend_config* pConfig,
                                                        const ma_allocation_callbacks* pAllocationCallbacks,
                                                        ma_data_source** ppBackend)
{
    ma_result result;
    ma_libopus* pOpus;

    (void)pUserData;

    pOpus = (ma_libopus*)ma_malloc(sizeof(*pOpus), pAllocationCallbacks);
    if (pOpus == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libopus_init_file(pFilePath, pConfig, pAllocationCallbacks, pOpus);
    if (result != MA_SUCCESS)
    {
        ma_free(pOpus, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pOpus;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__libopus(void* pUserData,
                                                ma_data_source* pBackend,
                                                const ma_allocation_callbacks* pAllocationCallbacks)
{
    ma_libopus* pOpus = (ma_libopus*)pBackend;

    (void)pUserData;

    ma_libopus_uninit(pOpus, pAllocationCallbacks);
    ma_free(pOpus, pAllocationCallbacks);
}

static ma_result ma_decoding_backend_get_channel_map__libopus(void* pUserData,
                                                              ma_data_source* pBackend,
                                                              ma_channel* pChannelMap,
                                                              size_t channelMapCap)
{
    ma_libopus* pOpus = (ma_libopus*)pBackend;

    (void)pUserData;

    return ma_libopus_get_data_format(pOpus, NULL, NULL, NULL, pChannelMap, channelMapCap);
}

ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libopus = 
{
    ma_decoding_backend_init__libopus, 
    ma_decoding_backend_init_file__libopus, 
    NULL,   /* onInitFileW() */
    NULL,   /* onInitMemory() */
    ma_decoding_backend_uninit__libopus
};