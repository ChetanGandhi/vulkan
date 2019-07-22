#include "logger.h"
#include "utils.h"

namespace xr {
	Logger *Logger::logger = nullptr;

    Logger::Logger() {}
    Logger::Logger(const Logger&) {};

    bool Logger::initialize(std::string fileName)
    {
        bool logFileCreated = true;

        if(logger == nullptr)
        {
            char dateTime[100] = {0};
            size_t size = currentDateTime(dateTime, sizeof(dateTime));

            logger = new Logger();

            #if defined (_WIN32) // check for Windows

            fopen_s(&logger->logfile, fileName.c_str(), "w");

            #elif defined (__linux) // check for Linux

            logger->logfile = fopen(fileName.c_str(), "w");

            #endif

            if(logger->logfile == NULL)
            {
                assert(1 && "Cannot open log file");
                logFileCreated = false;
            }
            else
            {
                fprintf(logger->logfile, "-----------------------------------\n");
                fprintf(logger->logfile, "| Logs start: %s |\n", dateTime);
                fprintf(logger->logfile, "-----------------------------------\n");
                fflush(logger->logfile);
            }
        }

        return logFileCreated;
    }

    Logger::~Logger()
    {
        if(logger->logfile == NULL)
        {
            return;
        }

        char dateTime[100] = {0};
        size_t size = currentDateTime(dateTime, sizeof(dateTime));

        fprintf(logger->logfile, "-----------------------------------\n");
        fprintf(logger->logfile, "| Logs end: %s   |\n", dateTime);
        fprintf(logger->logfile, "-----------------------------------\n");
        fflush(logger->logfile);
        fclose(logger->logfile);
        logger->logfile = nullptr;
    }

    void Logger::close()
    {
        delete logger;
        logger = nullptr;
    }

    void Logger::log(std::string file, std::string function, int line, std::string message, ...)
    {
        if(logger->logfile == NULL)
        {
            return;
        }

        char dateTime[100] = {0};
        size_t size = currentDateTime(dateTime, sizeof(dateTime));
        fprintf(logger->logfile, "%s | %s:%04d | %s | ", dateTime, file.c_str(), line, function.c_str());

        va_list args;
        va_start(args, message);
        vfprintf(logger->logfile, message.c_str(), args);
        va_end(args);

        fprintf(logger->logfile, "\n");
        fflush(logger->logfile);
    }

    void Logger::logUUID(std::string file, std::string function, int line, std::string message, uint8_t *uuid)
    {
        if(logger->logfile == NULL)
        {
            return;
        }

        char dateTime[100] = {0};
        size_t size = currentDateTime(dateTime, sizeof(dateTime));

        fprintf(logger->logfile, "%s | [UUID] | %s:%04d | %s | %s: ", dateTime, file.c_str(), line, function.c_str(), message.c_str());

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
}
