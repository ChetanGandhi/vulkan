#pragma once

#include "platform.h"

#include <iomanip>
#include <cstdarg>

#ifndef NDEBUG

#define logf(x_message, ...) xr::Logger::log(__FILE__, __FUNCTION__, __LINE__, x_message, ## __VA_ARGS__)
#define log_uuid(x_message, u_uuid) xr::Logger::logUUID(__FILE__, __FUNCTION__, __LINE__, x_message, u_uuid)

#else

#define logf(x_message, ...) ((void)0)
#define log_uuid(x_message, u_uuid) ((void)0)

#endif

namespace xr {
    class Logger
    {

    public:
        XR_API static bool initialize(const char* fileName);
        XR_API static void close();
        XR_API static void log(const char* file, const char* function, const uint32_t line, const char* message, ...);
        XR_API static void logUUID(const char* file, const char* function, const uint32_t line, const char* message, const uint8_t *uuid);

    private:
        Logger();
        ~Logger();
        Logger(const Logger&);

        static Logger* logger;

        FILE *logfile = nullptr;
    };
}
