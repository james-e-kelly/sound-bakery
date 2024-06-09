#include "sound_chef_bank.h"

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

sc_result sc_bank_init(const char* outputFile, sc_bank* bank) 
{ 
	SC_CHECK_ARG(outputFile);
    SC_CHECK_ARG(bank);

	SC_ZERO_OBJECT(bank); 

	ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

	ma_vfs_file file = NULL;

	ma_result openResult = ma_vfs_open(&vfs, outputFile, MA_OPEN_MODE_WRITE, &file);
    SC_CHECK_RESULT(openResult);

	bank->outputFile = file;

	return MA_SUCCESS;
}

sc_result sc_bank_uninit(sc_bank* bank) 
{ 
	SC_CHECK_ARG(bank);
    SC_CHECK(bank->outputFile != NULL, MA_INVALID_FILE);

    ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

	return ma_vfs_close(&vfs, bank->outputFile);
}

sc_result sc_bank_build(sc_bank* bank,
                        const char** inputFiles,
                        ma_encoding_format* inputFileFormats,
                        ma_uint32 inputFilesSize)
{
    SC_CHECK_ARG(bank != NULL);
    SC_CHECK_ARG(inputFiles != NULL);
    SC_CHECK_ARG(inputFileFormats != NULL);
    SC_CHECK_ARG(inputFilesSize > 0);
	SC_CHECK(bank->outputFile != NULL, MA_INVALID_FILE);

	ma_default_vfs vfs;
    ma_default_vfs_init(&vfs, NULL);

	void** finalData = ma_malloc(sizeof(void*) * inputFilesSize, NULL);
    SC_CHECK(finalData != NULL, MA_OUT_OF_MEMORY);

    char* finalFilenames = ma_malloc((size_t)SC_BANK_FILE_NAME_BUFFER_SIZE * inputFilesSize, NULL);
    if (finalFilenames == NULL)
    {
        ma_free(finalData, NULL);
        return MA_OUT_OF_MEMORY;
    }

    size_t* finalDataSize = ma_malloc(sizeof(size_t) * inputFilesSize, NULL);
    if (finalDataSize == NULL)
    {
        ma_free(finalData, NULL);
        ma_free(finalFilenames, NULL);
        return MA_OUT_OF_MEMORY;
    }

	size_t totalDataSize = 0;

    ma_result returnResult = MA_ERROR;

	for (size_t index = 0; index < inputFilesSize; ++index)
	{
        const char* inputFile = inputFiles[index];
        SC_CHECK_AND_GOTO(inputFile != NULL, cleanup);

		void* inputFileData = NULL;
        size_t inputFileDataSize = 0;
		const char* filename = sc_path_file_name(inputFile);

		ma_result openResult = ma_vfs_open_and_read_file(&vfs, inputFile, &inputFileData, &inputFileDataSize, NULL);
        SC_CHECK_AND_GOTO(openResult == MA_SUCCESS, cleanup);

		finalData[index] = inputFileData;
        finalDataSize[index] = inputFileDataSize + SC_BANK_FILE_NAME_BUFFER_SIZE;
        memcpy_s(finalFilenames + (index * SC_BANK_FILE_NAME_BUFFER_SIZE), SC_BANK_FILE_NAME_BUFFER_SIZE, filename, strnlen_s(filename, SC_BANK_FILE_NAME_BUFFER_SIZE));
        totalDataSize += SC_CHUNK_MIN_SIZE + SC_BANK_FILE_NAME_BUFFER_SIZE + inputFileDataSize;
	}

    size_t bytesWritten = 0;

    static const ma_uint32 bankID = SC_BANK_ID;

    ma_result writeResult = ma_vfs_write(&vfs, bank->outputFile, &bankID, sizeof(bankID), &bytesWritten);
    SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

    writeResult = ma_vfs_write(&vfs, bank->outputFile, &totalDataSize, sizeof(totalDataSize), &bytesWritten);
    SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

    for (size_t index = 0; index < inputFilesSize; ++index)
    {
        static const ma_uint32 chunkID = SC_BANK_SUB_ID;
        writeResult                    = ma_vfs_write(&vfs, bank->outputFile, &chunkID, sizeof(chunkID), &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

        const ma_uint32 chunkSize = finalDataSize[index];
        writeResult = ma_vfs_write(&vfs, bank->outputFile, &chunkSize, sizeof(chunkSize), &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

        const char* filename = &finalFilenames[index];
        writeResult = ma_vfs_write(&vfs, bank->outputFile, filename, SC_BANK_FILE_NAME_BUFFER_SIZE,
                                    &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);

        writeResult = ma_vfs_write(&vfs, bank->outputFile, finalData[index], chunkSize, &bytesWritten);
        SC_CHECK_AND_GOTO(writeResult == MA_SUCCESS, cleanup);
    }

    returnResult = writeResult;

cleanup:
	for (size_t index = 0; index < inputFilesSize; ++index)
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

    return returnResult;
}