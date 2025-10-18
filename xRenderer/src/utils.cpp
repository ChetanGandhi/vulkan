#include "utils.h"

XR_API bool xrReadFile(const char *fileName, char **data, size_t *fileSize)
{
    FILE *file = fopen(fileName, "rb");

    if (!file)
    {
        return false;
    }

    fseek(file, 0L, SEEK_END);
    *fileSize = ftell(file);

    if (*fileSize == 0)
    {
        if (file)
        {
            fclose(file);
            file = NULL;
        }

        return false;
    }

    fseek(file, 0L, SEEK_SET);

    *data = (char *)malloc(sizeof(char) * (*fileSize));
    memset((void *)*data, 0, sizeof(char) * (*fileSize));

    size_t freadResult = fread(*data, *fileSize, 1, file);

    if (freadResult != 1)
    {
        if (file)
        {
            fclose(file);
            file = NULL;
        }

        return false;
    }

    if (file)
    {
        fclose(file);
        file = NULL;
    }

    return true;
}

XR_API size_t xrCurrentDateTime(char *dateTimeString, size_t size)
{
    time_t now = time(NULL);
    struct tm tmStruct;

#if defined(_WIN32) // check for Windows

    _localtime64_s(&tmStruct, &now);

#elif defined(__linux) // check for Linux

    localtime_r(&now, &tmStruct);

#endif

    return strftime(dateTimeString, size, "%d-%m-%Y %H:%M:%S", &tmStruct);
}
