// #pragma once

#include "logger.h"
#include "utils.h"

Logger* Logger::logger = nullptr;

Logger::Logger() {}
Logger::Logger(const Logger&) {};

void Logger::init(std::string fileName)
{
    if(logger == nullptr)
    {
        logger = new Logger();
        logger->logfile = fopen(fileName.c_str(), "w");
        if(logger->logfile == NULL)
        {
            assert(1 && "Cannot open log file");
        }
        else
        {
            fprintf(logger->logfile, "-----------------------------------\n");
            fprintf(logger->logfile, "| Logs start: %s |\n", currentDateTime().c_str());
            fprintf(logger->logfile, "-----------------------------------\n");
            fflush(logger->logfile);
        }
    }
}

Logger::~Logger()
{
    if(logger->logfile != NULL)
    {
        fprintf(logger->logfile, "-----------------------------------\n");
        fprintf(logger->logfile, "| Logs end: %s   |\n", currentDateTime().c_str());
        fprintf(logger->logfile, "-----------------------------------\n");
        fclose(logger->logfile);
    }
}

void Logger::close()
{
    delete logger;
    logger = nullptr;
}

void Logger::log(const char *format, ...)
{
    va_list args;
    fprintf(logger->logfile, "%s: ", currentDateTime().c_str());

    va_start(args, format);
    vfprintf(logger->logfile, format, args);
    va_end(args);

    fprintf(logger->logfile, "\n");
    fflush(logger->logfile);
}

void Logger::log(const std::string &message)
{
    fprintf(logger->logfile, "%s: %s\n", currentDateTime().c_str(), message.c_str());
    fflush(logger->logfile);
}

void Logger::logUUID(const std::string &message, uint8_t *uuid)
{
    fprintf(logger->logfile, "%s: %s", currentDateTime().c_str(), message.c_str());
    for (int counter = 0; counter < VK_UUID_SIZE; ++counter)
    {
        fprintf(logger->logfile, "%2d", (uint32_t)uuid[counter]);

        if (counter == 3 || counter == 5 || counter == 7 || counter == 9)
        {
            fprintf(logger->logfile, "-");
        }
    }

    fprintf(logger->logfile, "\n");
    fflush(logger->logfile);
}
