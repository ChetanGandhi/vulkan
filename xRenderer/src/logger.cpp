#include "logger.h"
#include "utils.h"

XR_API VkResult xrCreateLogger(const char* fileName, XrLogger** logger)
{
    char dateTime[100] = {0};
    size_t size = xrCurrentDateTime(dateTime, sizeof(dateTime));

    *logger = (XrLogger*)malloc(sizeof(XrLogger));
    memset((void*)*logger, 0, sizeof(XrLogger));

#if defined(_WIN32) // check for Windows

    fopen_s(&(*logger)->logfile, fileName, "w");

#elif defined(__linux) // check for Linux

    (*logger)->logfile = fopen(fileName, "w");

#endif

    if (!(*logger)->logfile)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    fprintf((*logger)->logfile, "-----------------------------------\n");
    fprintf((*logger)->logfile, "| Logs start: %s |\n", dateTime);
    fprintf((*logger)->logfile, "-----------------------------------\n");
    fflush((*logger)->logfile);

    return VK_SUCCESS;
}

XR_API void xrDestroyLogger(XrLogger** logger)
{
    if ((*logger))
    {
        if ((*logger)->logfile)
        {
            char dateTime[100];
            memset((void*)&dateTime, 0, sizeof(dateTime));
            size_t size = xrCurrentDateTime(dateTime, sizeof(dateTime));

            fprintf((*logger)->logfile, "-----------------------------------\n");
            fprintf((*logger)->logfile, "| Logs end: %s   |\n", dateTime);
            fprintf((*logger)->logfile, "-----------------------------------\n");
            fflush((*logger)->logfile);
            fclose((*logger)->logfile);
            (*logger)->logfile = VK_NULL_HANDLE;
        }

        free(*logger);
        *logger = VK_NULL_HANDLE;
    }
}
