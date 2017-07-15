#pragma once

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
        logger->logfileOutStream.open(fileName.c_str(), std::ios::out | std::ios::app);
        logger->logfileOutStream<<"-----------------------------------\n";
        logger->logfileOutStream<<"| Logs start: "<<currentDateTime()<<" |\n";
        logger->logfileOutStream<<"-----------------------------------\n";
    }
}

Logger::~Logger()
{
    logger->logfileOutStream<<"-----------------------------------\n";
    logger->logfileOutStream<<"| Logs end: "<<currentDateTime()<<"   |\n";
    logger->logfileOutStream<<"-----------------------------------\n";

    logger->logfileOutStream.flush();
    logger->logfileOutStream.close();
}

void Logger::close()
{
    delete logger;
    logger = nullptr;
}

void Logger::log(const char * format, ...)
{
    char* message = NULL;
    int length = 0;

    va_list args;
    va_start(args, format);

    //  Return the number of characters in the string referenced the list of arguments.
    // _vscprintf doesn't count terminating '\0' (that's why +1)
    length = _vscprintf(format, args) + 1;
    message = new char[length];
    vsprintf_s(message, length, format, args);

    logger->logfileOutStream<<currentDateTime()<<": "<<message<<"\n";

    va_end(args);

    delete [] message;

    logger->logfileOutStream.flush();
}

void Logger::log(const std::string &message)
{
    logger->logfileOutStream<<currentDateTime()<<": "<<message<<"\n";
    logger->logfileOutStream.flush();
}

void Logger::logUUID(const std::string &message, uint8_t *uuid)
{
    logger->logfileOutStream<<currentDateTime()<<": "<<message;

    for (int counter = 0; counter < VK_UUID_SIZE; ++counter)
    {
        logger->logfileOutStream<<std::setw(2)<<(uint32_t)uuid[counter];

        if (counter == 3 || counter == 5 || counter == 7 || counter == 9)
        {
            logger->logfileOutStream<<'-';
        }
    }

    logger->logfileOutStream<<"\n";
    logger->logfileOutStream.flush();
}
