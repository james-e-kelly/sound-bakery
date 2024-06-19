#ifndef SOUND_CHEF_BANK
#define SOUND_CHEF_BANK

/**
 * @file
 * @brief Defines a Soundbank.
 *
 * Soundbanks hold a number of audio files within it and can play sounds from it.
 */

#include "sound_chef/sound_chef_common.h"

#define FOURCC(a, b, c, d) ((ma_uint32)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

#define SC_BANK_ID      (FOURCC('S', 'C', 'B', 'K'))
#define SC_BANK_SUB_ID  (FOURCC('S', 'C', 'F', 'E'))

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct sc_bank sc_bank;

typedef struct sc_subChunk sc_subChunk;
typedef struct sc_riffChunk sc_riffChunk;
typedef struct sc_audioChunk sc_audioChunk;

enum
{
    SC_BANK_FILE_NAME_BUFFER_SIZE = 64
};

struct sc_subChunk
{
    ma_uint32   id;
    ma_uint32   size;
    void*       data;
};

struct sc_audioChunk
{
    ma_uint32   id;
    ma_uint32   size;
    char        name[SC_BANK_FILE_NAME_BUFFER_SIZE];
    void*       data;
};

struct sc_riffChunk
{
    ma_uint32       id;
    ma_uint32       size;
    ma_uint32       numOfSubchunks;  
    sc_subChunk**   subChunks;
};

struct sc_bank
{
    sc_riffChunk riff;

    ma_vfs_file outputFile;
};

sc_result SC_API sc_bank_init(const char* outputFile, sc_bank* bank);
sc_result SC_API sc_bank_uninit(sc_bank* bank);

sc_result SC_API sc_bank_build(sc_bank* bank, const char** inputFiles, ma_encoding_format* inputFileFormats, ma_uint32 inputFilesSize);

#ifdef __cplusplus
}
#endif

#endif