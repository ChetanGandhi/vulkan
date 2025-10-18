#include "logger.h"
#include "common.h"
#include "utils.h"

XR_API VkResult xrCreateLogger(const char* fileName, XrLogger** logger)
{
    char dateTime[100] = {0};
    memset((void*)&dateTime, 0, _ARRAYSIZE(dateTime));
    xrCurrentDateTime(dateTime, _ARRAYSIZE(dateTime));

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
            memset((void*)&dateTime, 0, _ARRAYSIZE(dateTime));
            xrCurrentDateTime(dateTime, _ARRAYSIZE(dateTime));

            fprintf((*logger)->logfile, "-----------------------------------\n");
            fprintf((*logger)->logfile, "| Logs end: %s   |\n", dateTime);
            fprintf((*logger)->logfile, "-----------------------------------\n");
            fflush((*logger)->logfile);
            fclose((*logger)->logfile);
            (*logger)->logfile = VK_NULL_HANDLE;
        }

        XR_FREE(*logger);
    }
}
