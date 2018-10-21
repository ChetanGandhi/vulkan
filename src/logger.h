#pragma once

#include <string>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>

#if defined (ENABLE_DEBUG)

#define logf(x_message, ...) Logger::log(__FILE__, __FUNCTION__, __LINE__, x_message, __VA_ARGS__)
#define LOG_UUID(x_message, u_uuid) Logger::logUUID(__FILE__, __FUNCTION__, __LINE__, x_message, u_uuid)

#else

#define logf(x_message, ...)
#define LOG_UUID(x_message, u_uuid)

#endif

class Logger
{

public:
    static bool initialize(std::string fileName);
    static void close();
    static void log(std::string file, std::string function, int line, std::string message, ...);
    static void logUUID(std::string file, std::string function, int line, std::string message, uint8_t *uuid);

private:
    Logger();
    ~Logger();
    Logger(const Logger&);

    static Logger* logger;

    FILE *logfile = nullptr;
};
