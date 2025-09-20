#pragma once

#include "platform.h"
#include "utils.h"

#include <iomanip>
#include <cstdarg>

#ifndef NDEBUG

#define XR_LOG(logger, xr_tag, xr_message, ...) logger->xrLog(__FILE__, __FUNCTION__, __LINE__, xr_tag, xr_message, ##__VA_ARGS__)
#define XR_LOG_INFO(logger, xr_message, ...) logger->xrLog(__FILE__, __FUNCTION__, __LINE__, "INFO", xr_message, ##__VA_ARGS__)
#define XR_LOG_WARNING(logger, xr_message, ...) logger->xrLog(__FILE__, __FUNCTION__, __LINE__, "WARNING", xr_message, ##__VA_ARGS__)
#define XR_LOG_ERROR(logger, xr_message, ...) logger->xrLog(__FILE__, __FUNCTION__, __LINE__, "ERROR", xr_message, ##__VA_ARGS__)
#define XR_LOG_UUID(logger, xr_message, xr_u_uuid) logger->xrLogUUID(__FILE__, __FUNCTION__, __LINE__, xr_message, xr_u_uuid)

#else

#define XR_LOG(logger, xr_tag, xr_message, ...) ((void)0)
#define XR_LOG_INFO(logger, xr_message, ...) ((void)0)
#define XR_LOG_WARNING(logger, xr_message, ...) ((void)0)
#define XR_LOG_ERROR(logger, xr_message, ...) ((void)0)
#define XR_LOG_UUID(logger, xr_message, xr_u_uuid) ((void)0)

#endif

typedef struct XrLogger
{
    FILE* logfile = VK_NULL_HANDLE;

    void xrLog(const char* file, const char* function, const uint32_t line, const char* tag, const char* message, ...)
    {
        if (!this->logfile)
        {
            return;
        }

        char dateTime[100];
        memset((void*)&dateTime, 0, sizeof(dateTime));
        size_t size = xrCurrentDateTime(dateTime, sizeof(dateTime));
        fprintf_s(this->logfile, "%s | %s:%04d | %s | [%s] | ", dateTime, file, line, function, tag);

        va_list args;
        va_start(args, message);
        vfprintf(this->logfile, message, args);
        va_end(args);

        fprintf_s(this->logfile, "\n");
        fflush(this->logfile);
    }

    void xrLogUUID(const char* file, const char* function, const uint32_t line, const char* message, const uint8_t* uuid)
    {
        if (!this->logfile)
        {
            return;
        }

        char dateTime[100];
        memset((void*)&dateTime, 0, sizeof(dateTime));
        size_t size = xrCurrentDateTime(dateTime, sizeof(dateTime));

        fprintf_s(this->logfile, "%s | %s:%04d | %s | [UUID] | %s", dateTime, file, line, function, message);

        for (int counter = 0; counter < VK_UUID_SIZE; ++counter)
        {
            fprintf_s(this->logfile, "%2d", (uint32_t)uuid[counter]);

            if (counter == 3 || counter == 5 || counter == 7 || counter == 9)
            {
                fprintf_s(this->logfile, "-");
            }
        }

        fprintf_s(this->logfile, "\n");
        fflush(this->logfile);
    }
} XrLogger;

XR_API VkResult xrCreateLogger(const char* fileName, XrLogger** logger);
XR_API void xrDestroyLogger(XrLogger** logger);
