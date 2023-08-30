/**
 * @logger.h
 * @brief Utils for log levels
 *
 * This contains the routines of logging events
 *
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <errno.h>
#include <pthread.h>

namespace edgeml
{
    namespace utils
    {

        #define LOG_ERROR(x) Logger::getInstance()->error(x)
        #define LOG_ALARM(x) Logger::getInstance()->alarm(x)
        #define LOG_ALWAYS(x) Logger::getInstance()->always(x)
        #define LOG_INFO(x) Logger::getInstance()->info(x)
        #define LOG_BUFFER(x) Logger::getInstance()->buffer(x)
        #define LOG_TRACE(x) Logger::getInstance()->trace(x)
        #define LOG_DEBUG(x) Logger::getInstance()->debug(x)

        // Default value for maximum number of log files
        #define MAX_LOG_FILES 1

        // Default size of a log file in bytes
        #define LOG_FILE_SIZE 10000

        typedef enum LOG_LEVEL
        {
            DISABLE_LOG	= 0,
            ENABLE_LOG = 1,
            LOG_LEVEL_INFO = 2,
            LOG_LEVEL_BUFFER = 3,
            LOG_LEVEL_TRACE = 4,
            LOG_LEVEL_DEBUG = 5
        } LogLevel;

        typedef enum LOG_TYPE
        {
            NO_LOG = 0,
            CONSOLE = 1,
            FILE_LOG = 2,
        } LogType;

        class Logger
        {

            public:
                static Logger* getInstance() throw ();

                // Interface for Error Log
                void error(const char* text) throw();
                void error(std::string text) throw();
                void error(std::string& text) throw();
                void error(std::ostringstream& stream) throw();

                // Interface for Alarm Log
                void alarm(const char* text) throw();
                void alarm(std::string text) throw();
                void alarm(std::string& text) throw();
                void alarm(std::ostringstream& stream) throw();

                // Interface for Always Log
                void always(const char* text) throw();
                void always(std::string text) throw();
                void always(std::string& text) throw();
                void always(std::ostringstream& stream) throw();

                // Interface for Buffer Log
                void buffer(const char* text) throw();
                void buffer(std::string text) throw();
                void buffer(std::string& text) throw();
                void buffer(std::ostringstream& stream) throw();

                // Interface for Info Log
                void info(const char* text) throw();
                void info(std::string text) throw();
                void info(std::string& text) throw();
                void info(std::ostringstream& stream) throw();

                // Interface for Trace log
                void trace(const char* text) throw();
                void trace(std::string text) throw();
                void trace(std::string& text) throw();
                void trace(std::ostringstream& stream) throw();

                // Interface for Debug log
                void debug(const char* text) throw();
                void debug(std::string text) throw();
                void debug(std::string& text) throw();
                void debug(std::ostringstream& stream) throw();

                // Error and Alarm log must be always enable
                // Hence, there is no interfce to control error and alarm logs

                // Interfaces to control log levels
                void updateLogLevel(LogLevel logLevel);
                void enaleLog();  // Enable all log levels
                void disableLog(); // Disable all log levels, except error and alarm

                // Interfaces to control log Types
                void updateLogType(LogType logType);
                void enableConsoleLogging();
                void enableFileLogging();

                // Interfaces to control roll over mechanism
                void updateMaxLogFiles(const ssize_t maxFiles);
                void updateLogSize(const ssize_t size);

            protected:
                Logger();
                ~Logger();
                void lock();
                void unlock();
                std::string getCurrentTime();

            private:
                void logIntoFile(std::string& data);
                void logOnConsole(std::string& data);
                Logger(const Logger& obj) {}
                void operator=(const Logger& obj) {}
                void rollLogFiles();
                void configure();

            private:
                static Logger* m_Instance;
                std::ofstream m_File;
                pthread_mutexattr_t m_Attr;
                pthread_mutex_t m_Mutex;
                LogLevel m_LogLevel;
                LogType m_LogType;
                unsigned int logSize;
                unsigned int maxLogFiles;
                unsigned int logFilesCount;

        };

    }
}

#endif