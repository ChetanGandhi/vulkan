#pragma once

#include "platform.h"

#include <iomanip>
#include <cstdarg>

#ifndef NDEBUG

#define logf(x_message, ...) Logger::log(__FILE__, __FUNCTION__, __LINE__, x_message, ## __VA_ARGS__)
#define log_uuid(x_message, u_uuid) Logger::logUUID(__FILE__, __FUNCTION__, __LINE__, x_message, u_uuid)

#else

#define logf(x_message, ...) ((void)0)
#define log_uuid(x_message, u_uuid) ((void)0)

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
