#pragma once

#include <fstream>
#include <iostream>
#include <cstdarg>
#include <string>
#include <iomanip>
#include <stdlib.h>

#if defined (ENABLE_DEBUG)

#define LOG(x) Logger::log(x)
#define LOGF(x, ...) Logger::log(x, __VA_ARGS__)
#define LOG_UUID(x, y) Logger::logUUID(x, y)

#else

#define LOG(x)
#define LOGF(x, ...)
#define LOG_UUID(x, y)

#endif

class Logger
{

public:
    static void init(std::string fileName);
    static void close();
    static void log(const std::string& sMessage);
    static void log(const char * format, ...);
    static void logUUID(const std::string &message, uint8_t *uuid);

private:
    Logger();
    ~Logger();
    Logger(const Logger&);

    static Logger* logger;

    FILE *logfile;
};
