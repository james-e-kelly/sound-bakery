#include "sound_chef/sound_chef.h"

enum
{
    SC_CHUNK_MIN_SIZE = 8
};

static const char* sc_path_file_name(const char* path)
{
    const char* fileName;

    if (path == NULL)
    {
        return NULL;
    }

    fileName = path;

    /* We just loop through the path until we find the last slash. */
    while (path[0] != '\0')
    {
        if (path[0] == '/' || path[0] == '\\')
        {
            fileName = path;
        }

        path += 1;
    }

    /* At this point the file name is sitting on a slash, so just move forward. */
    while (fileName[0] != '\0' && (fileName[0] == '/' || fileName[0] == '\\'))
    {
        fileName += 1;
    }

    return fileName;
}

static const ma_uint32 s_bankID      = SC_BANK_ID;
static const ma_uint32 s_bankVersion = SC_BANK_VERSION;

sbk_status sc_bank_init(sc_bank* bank, const char* outputFile, ma_open_mode_flags openFlags)
{
    SC_CHECK_ARG(outputFile);
    SC_CHECK_ARG(bank);

    SC_ZERO_OBJECT(bank);

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    ma_vfs_file file = NULL;

    const sbk_status openResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_open(&vfs, outputFile, openFlags, &file));
    SC_CHECK_STATUS(openResult);

    bank->outputFile = file;

    return openResult;
}

sbk_status sc_bank_uninit(sc_bank* bank)
{
    SC_CHECK_ARG(bank);
    SC_CHECK(bank->outputFile != NULL, SC_STATUS_FROM_MA_RESULT(MA_INVALID_FILE));

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    if (bank->riff != NULL)
    {
        if (bank->riff->numOfSubchunks > 0)
        {
            if (bank->riff->subChunks != NULL)
            {
                for (size_t chunkIndex = 0; chunkIndex < bank->riff->numOfSubchunks; ++chunkIndex)
                {
                    sc_audio_chunk* const audioChunk = bank->riff->subChunks[chunkIndex];

                    if (audioChunk != NULL)
                    {
                        if (audioChunk->data != NULL)
                        {
                            ma_free(audioChunk->data, NULL);
                        }

                        ma_free(audioChunk, NULL);
                    }
                }

                ma_free(bank->riff->subChunks, NULL);
            }
        }

        ma_free(bank->riff, NULL);
    }

    return SC_STATUS_FROM_MA_RESULT(ma_vfs_close(&vfs, bank->outputFile));
}

sbk_status sc_bank_build(sc_bank* bank,
                         const char** inputFiles,
                         sc_encoding_format* inputFileFormats,
                         ma_uint32 inputFilesSize)
{
    SC_CHECK_ARG(bank != NULL);
    SC_CHECK_ARG(inputFiles != NULL);
    SC_CHECK_ARG(inputFileFormats != NULL);
    SC_CHECK_ARG(inputFilesSize > 0);
    SC_CHECK(bank->outputFile != NULL, SC_STATUS_FROM_MA_RESULT(MA_INVALID_FILE));

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    const size_t fileDataSize = sizeof(void*) * inputFilesSize;

    void** finalData = ma_malloc(fileDataSize, NULL);
    SC_CHECK(finalData != NULL, SBK_ERR_OUT_OF_MEMORY);
    memset(finalData, 0, fileDataSize);

    const size_t finalFilenamesSize = (size_t)SC_BANK_FILE_NAME_BUFFER_SIZE * inputFilesSize;

    char* finalFilenames = ma_malloc(finalFilenamesSize, NULL);
    if (finalFilenames == NULL)
    {
        ma_free(finalData, NULL);
        return SBK_ERR_OUT_OF_MEMORY;
    }
    memset(finalFilenames, 0, finalFilenamesSize);

    const size_t finalDataSizeSize = sizeof(size_t) * inputFilesSize;

    size_t* finalDataSize = ma_malloc(finalDataSizeSize, NULL);
    if (finalDataSize == NULL)
    {
        ma_free(finalData, NULL);
        ma_free(finalFilenames, NULL);
        return SBK_ERR_OUT_OF_MEMORY;
    }
    memset(finalDataSize, 0, finalDataSizeSize);

    size_t totalDataSize = sizeof(s_bankVersion);
    size_t filesRead     = 0;

    ma_result returnResult = MA_ERROR;

    for (size_t index = 0; index < inputFilesSize; ++index)
    {
        const char* inputFile = inputFiles[index];
        SC_CHECK_AND_GOTO(inputFile != NULL, cleanup);

        void* inputFileData      = NULL;
        size_t inputFileDataSize = 0;
        const char* filename     = sc_path_file_name(inputFile);

        ma_result openResult = ma_vfs_open_and_read_file(&vfs, inputFile, &inputFileData, &inputFileDataSize, NULL);
        SC_CHECK_AND_GOTO(openResult == MA_SUCCESS, cleanup);

        finalData[index]     = inputFileData;
        finalDataSize[index] = inputFileDataSize + SC_BANK_FILE_NAME_BUFFER_SIZE;

        // The stored name slot is a fixed SC_BANK_FILE_NAME_BUFFER_SIZE bytes. Cap the copy so a
        // long filename can't overrun into the next slot; finalFilenames is zero-filled, so
        // truncating here leaves the trailing byte as a null terminator for the reader.
        const size_t maxNameLength = SC_BANK_FILE_NAME_BUFFER_SIZE - 1;
        size_t filenameLength      = strlen(filename);
        if (filenameLength > maxNameLength)
        {
            filenameLength = maxNameLength;
        }
        memcpy(finalFilenames + (index * SC_BANK_FILE_NAME_BUFFER_SIZE), filename, filenameLength);
        totalDataSize += SC_CHUNK_MIN_SIZE + SC_BANK_FILE_NAME_BUFFER_SIZE + inputFileDataSize;

        ++filesRead;
    }

    size_t bytesWritten = 0;

    ma_result writeResult = ma_vfs_write(&vfs, bank->outputFile, &s_bankID, sizeof(s_bankID), &bytesWritten);
    SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

    writeResult = ma_vfs_write(&vfs, bank->outputFile, &totalDataSize, sizeof(totalDataSize), &bytesWritten);
    SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

    writeResult = ma_vfs_write(&vfs, bank->outputFile, &s_bankVersion, sizeof(s_bankVersion), &bytesWritten);
    SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

    writeResult = ma_vfs_write(&vfs, bank->outputFile, &inputFilesSize, sizeof(inputFilesSize), &bytesWritten);
    SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

    for (size_t index = 0; index < inputFilesSize; ++index)
    {
        static const ma_uint32 chunkID = SC_BANK_SUB_ID;
        writeResult                    = ma_vfs_write(&vfs, bank->outputFile, &chunkID, sizeof(chunkID), &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

        const ma_uint32 chunkSize = (ma_uint32)finalDataSize[index];
        writeResult               = ma_vfs_write(&vfs, bank->outputFile, &chunkSize, sizeof(chunkSize), &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

        const char* filename = finalFilenames + (index * SC_BANK_FILE_NAME_BUFFER_SIZE);
        writeResult          = ma_vfs_write(&vfs, bank->outputFile, filename, SC_BANK_FILE_NAME_BUFFER_SIZE, &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

        writeResult = ma_vfs_write(&vfs, bank->outputFile, finalData[index], chunkSize - SC_BANK_FILE_NAME_BUFFER_SIZE,
                                   &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);
    }

    returnResult = writeResult;

cleanup:
    for (size_t index = 0; index < filesRead; ++index)
    {
        void* data = finalData[index];

        if (data != NULL)
        {
            ma_free(data, NULL);
        }
    }

    ma_free(finalData, NULL);
    ma_free(finalDataSize, NULL);
    ma_free(finalFilenames, NULL);

    return SC_STATUS_FROM_MA_RESULT(returnResult);
}

sbk_status sc_bank_read(sc_bank* bank)
{
    SC_CHECK_ARG(bank != NULL);
    SC_CHECK_ARG(bank->outputFile != NULL);
    SC_CHECK(bank->riff == NULL, SC_STATUS_FROM_MA_RESULT(MA_ALREADY_EXISTS));

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

    ma_file_info fileInfo;
    ma_vfs_info(&vfs, bank->outputFile, &fileInfo);
    SC_CHECK(fileInfo.sizeInBytes > SC_CHUNK_MIN_SIZE, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

    ma_uint32 numOfSubchunks = 0;

    // Riff Chunk
    ma_uint32 bankID = 0;
    size_t bytesRead = 0;

    sbk_status readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &bankID, 4, &bytesRead));
    SC_CHECK_STATUS(readResult);
    SC_CHECK(bankID == SC_BANK_ID, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

    size_t totalFileSize = 0;

    readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &totalFileSize, sizeof(totalFileSize), &bytesRead));
    SC_CHECK_STATUS(readResult);
    SC_CHECK(totalFileSize > 4, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

    ma_uint32 bankVersion = 0;

    readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &bankVersion, sizeof(bankVersion), &bytesRead));
    SC_CHECK_STATUS(readResult);
    SC_CHECK(bankVersion == SC_BANK_VERSION, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

    readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &numOfSubchunks, sizeof(ma_uint32), &bytesRead));
    SC_CHECK_STATUS(readResult);
    SC_CHECK(numOfSubchunks > 0, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

    // A corrupt or hostile bank could claim a subchunk count far larger than the file can hold. Each
    // subchunk needs at least its id, size and name on disk, so bound the count to the file size
    // before allocating the pointer array below.
    const ma_uint64 minBytesPerSubchunk = (ma_uint64)SC_CHUNK_MIN_SIZE + SC_BANK_FILE_NAME_BUFFER_SIZE;
    SC_CHECK(numOfSubchunks <= fileInfo.sizeInBytes / minBytesPerSubchunk, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

    bank->riff = ma_malloc(sizeof(sc_riff_chunk), NULL);
    SC_CHECK_MEM(bank->riff);
    memset(bank->riff, 0, sizeof(sc_riff_chunk));

    bank->riff->id             = bankID;
    bank->riff->size           = (ma_uint32)totalFileSize;
    bank->riff->numOfSubchunks = numOfSubchunks;
    bank->riff->subChunks      = ma_malloc(sizeof(sc_sub_chunk**) * numOfSubchunks, NULL);
    SC_CHECK_MEM(bank->riff->subChunks);
    memset(bank->riff->subChunks, 0, sizeof(sc_sub_chunk**) * numOfSubchunks);

    // Sub chunks

    for (size_t chunkIndex = 0; chunkIndex < numOfSubchunks; ++chunkIndex)
    {
        ma_uint32 chunkID = 0;

        readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &chunkID, 4, &bytesRead));
        SC_CHECK_STATUS(readResult);
        SC_CHECK(chunkID == SC_BANK_SUB_ID, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

        ma_uint32 chunkSize = 0;

        readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &chunkSize, 4, &bytesRead));
        SC_CHECK_STATUS(readResult);

        // chunkSize covers the name buffer plus the audio payload. Guard against an underflow when
        // subtracting the name buffer, and against a value larger than the whole file, which would
        // otherwise request an absurd allocation for the chunk data below.
        SC_CHECK(chunkSize >= SC_BANK_FILE_NAME_BUFFER_SIZE, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));
        SC_CHECK(chunkSize <= fileInfo.sizeInBytes, SC_STATUS_FROM_MA_RESULT(MA_INVALID_DATA));

        char chunkName[SC_BANK_FILE_NAME_BUFFER_SIZE];

        readResult = SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, &chunkName, SC_BANK_FILE_NAME_BUFFER_SIZE, &bytesRead));
        SC_CHECK_STATUS(readResult);

        const size_t dataLength = chunkSize - SC_BANK_FILE_NAME_BUFFER_SIZE;

        bank->riff->subChunks[chunkIndex] = ma_malloc(sizeof(sc_audio_chunk), NULL);
        SC_CHECK_MEM(bank->riff->subChunks[chunkIndex]);
        memset(bank->riff->subChunks[chunkIndex], 0, sizeof(sc_audio_chunk));

        bank->riff->subChunks[chunkIndex]->data = ma_malloc(dataLength, NULL);
        SC_CHECK_MEM(bank->riff->subChunks[chunkIndex]->data);
        memset(bank->riff->subChunks[chunkIndex]->data, 0, dataLength);

        bank->riff->subChunks[chunkIndex]->id   = chunkID;
        bank->riff->subChunks[chunkIndex]->size = chunkSize;
        memcpy(bank->riff->subChunks[chunkIndex]->name, chunkName, SC_BANK_FILE_NAME_BUFFER_SIZE);
        bank->riff->subChunks[chunkIndex]->name[SC_BANK_FILE_NAME_BUFFER_SIZE - 1] = '\0';

        readResult =
            SC_STATUS_FROM_MA_RESULT(ma_vfs_read(&vfs, bank->outputFile, bank->riff->subChunks[chunkIndex]->data, dataLength, &bytesRead));
        SC_CHECK_STATUS(readResult);
    }

    return SBK_SUCCESS;
}
