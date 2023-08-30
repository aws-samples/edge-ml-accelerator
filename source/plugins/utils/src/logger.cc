/**
 * @logger.cc
 * @brief Utils for log levels
 *
 * This contains the routines of logging events
 *
 */

#include <edge-ml-accelerator/utils/logger.h>

namespace edgeml
{
  namespace utils
  {

    // Log file name. File name should be change from here only
    const std::string logFileName = "EdgeMLAccelerator.log";

    // Instance of the class
    Logger* Logger::m_Instance = 0;

    /**
      Creates the class constructor
    */
    Logger::Logger()
    {
        m_File.open(logFileName.c_str(), std::ios::out|std::ios::app);
        configure();
        logFilesCount  = 1;

        int ret=0;
        pthread_mutexattr_init(&m_Attr);
        ret = pthread_mutexattr_settype(&m_Attr, PTHREAD_MUTEX_ERRORCHECK);
        if (ret != 0)
        {
            std::cout << "[LOGGER::LOGGER] Mutex attribute not initialize!" << std::endl;
            exit(0);
        }
        ret = pthread_mutex_init(&m_Mutex, &m_Attr);
        if (ret != 0)
        {
            std::cout << "[LOGGER::LOGGER] Mutex not initialize!" << std::endl;
            exit(0);
        }
    }

    /**
      Creates the class destructor
    */
    Logger::~Logger()
    {
        m_File.close();
        pthread_mutexattr_destroy(&m_Attr);
        pthread_mutex_destroy(&m_Mutex);
    }

    Logger* Logger::getInstance() throw ()
    {
        if (m_Instance == 0)
        {
            m_Instance = new Logger();
        }
        return m_Instance;
    }

    void Logger::lock()
    {
        pthread_mutex_lock(&m_Mutex);
    }

    void Logger::unlock()
    {
        pthread_mutex_unlock(&m_Mutex);
    }

    void Logger::logIntoFile(std::string& data)
    {
        unsigned long pos = m_File.tellp();
        if (pos + data.size() > logSize)
        {
                rollLogFiles();
        }

        lock();
        m_File << getCurrentTime() << "  " << data << std::endl;
        unlock();
    }

    void Logger::logOnConsole(std::string& data)
    {
        std::cout << getCurrentTime() << "  " << data << std::endl;
    }

    std::string Logger::getCurrentTime()
    {
        std::string currTime;
        //Current date/time based on current time
        time_t now = time(0);
        // Convert current time to string
        currTime.assign(ctime(&now));

        // Last charactor of currentTime is "\n", so remove it
        std::string currentTime = currTime.substr(0, currTime.size()-1);
        return currentTime;
    }

    // Interface for Error Log
    void Logger::error(const char* text) throw()
    {
        std::string data;
        data.append("[ERROR]: ");
        data.append(text);

        // ERROR must be capture
        if (m_LogType == FILE_LOG && m_LogLevel)
        {
            logIntoFile(data);
        }
        else if (m_LogType == CONSOLE && m_LogLevel)
        {
            logOnConsole(data);
        }
    }

    void Logger::error(std::string text) throw()
    {
        error(text.data());
    }

    void Logger::error(std::string& text) throw()
    {
        error(text.data());
    }

    void Logger::error(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        error(text.data());
    }

    // Interface for Alarm Log
    void Logger::alarm(const char* text) throw()
    {
        std::string data;
        data.append("[ALARM]: ");
        data.append(text);

        // ALARM must be capture
        if (m_LogType == FILE_LOG && m_LogLevel)
        {
            logIntoFile(data);
        }
        else if (m_LogType == CONSOLE && m_LogLevel)
        {
            logOnConsole(data);
        }
    }

    void Logger::alarm(std::string text) throw()
    {
        alarm(text.data());
    }

    void Logger::alarm(std::string& text) throw()
    {
        alarm(text.data());
    }

    void Logger::alarm(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        alarm(text.data());
    }

    // Interface for Always Log
    void Logger::always(const char* text) throw()
    {
        std::string data;
        data.append("[ALWAYS]: ");
        data.append(text);

        // No check for ALWAYS logs
        if (m_LogType == FILE_LOG && m_LogLevel)
        {
            logIntoFile(data);
        }
        else if (m_LogType == CONSOLE && m_LogLevel)
        {
            logOnConsole(data);
        }
    }

    void Logger::always(std::string text) throw()
    {
        always(text.data());
    }

    void Logger::always(std::string& text) throw()
    {
        always(text.data());
    }

    void Logger::always(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        always(text.data());
    }

    // Interface for Buffer Log
    void Logger::buffer(const char* text) throw()
    {
        // Buffer is the special case. So don't add log level
        // and timestamp in the buffer message. Just log the raw bytes.
        if ((m_LogType == FILE_LOG) && (m_LogLevel <= LOG_LEVEL_BUFFER))
        {
            lock();
            m_File << text << std::endl;
            unlock();
        }
        else if ((m_LogType == CONSOLE) && (m_LogLevel <= LOG_LEVEL_BUFFER))
        {
            std::cout << text << std::endl;
        }
    }

    void Logger::buffer(std::string text) throw()
    {
        buffer(text.data());
    }

    void Logger::buffer(std::string& text) throw()
    {
        buffer(text.data());
    }

    void Logger::buffer(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        buffer(text.data());
    }

    // Interface for Info Log
    void Logger::info(const char* text) throw()
    {
        std::string data;
        data.append("[INFO]: ");
        data.append(text);

        if ((m_LogType == FILE_LOG) && (m_LogLevel <= LOG_LEVEL_INFO))
        {
            logIntoFile(data);
        }
        else if ((m_LogType == CONSOLE) && (m_LogLevel <= LOG_LEVEL_INFO))
        {
            logOnConsole(data);
        }
    }

    void Logger::info(std::string text) throw()
    {
        info(text.data());
    }

    void Logger::info(std::string& text) throw()
    {
        info(text.data());
    }

    void Logger::info(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        info(text.data());
    }

    // Interface for Trace Log
    void Logger::trace(const char* text) throw()
    {
        std::string data;
        data.append("[TRACE]: ");
        data.append(text);

        if ((m_LogType == FILE_LOG) && (m_LogLevel <= LOG_LEVEL_TRACE))
        {
            logIntoFile(data);
        }
        else if ((m_LogType == CONSOLE) && (m_LogLevel <= LOG_LEVEL_TRACE))
        {
            logOnConsole(data);
        }
    }

    void Logger::trace(std::string text) throw()
    {
        trace(text.data());
    }

    void Logger::trace(std::string& text) throw()
    {
        trace(text.data());
    }

    void Logger::trace(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        trace(text.data());
    }

    // Interface for Debug Log
    void Logger::debug(const char* text) throw()
    {
        std::string data;
        data.append("[DEBUG]: ");
        data.append(text);

        if ((m_LogType == FILE_LOG) && (m_LogLevel <= LOG_LEVEL_DEBUG))
        {
            logIntoFile(data);
        }
        else if ((m_LogType == CONSOLE) && (m_LogLevel <= LOG_LEVEL_DEBUG))
        {
            logOnConsole(data);
        }
    }

    void Logger::debug(std::string text) throw()
    {
        debug(text.data());
    }

    void Logger::debug(std::string& text) throw()
    {
        debug(text.data());
    }

    void Logger::debug(std::ostringstream& stream) throw()
    {
        std::string text = stream.str();
        debug(text.data());
    }

    // Interfaces to control log levels
    void Logger::updateLogLevel(LogLevel logLevel)
    {
        m_LogLevel = logLevel;
    }

    // Enable all log levels
    void Logger::enaleLog()
    {
        m_LogLevel = ENABLE_LOG;
    }

    // Disable all log levels, except error and alarm
    void Logger:: disableLog()
    {
        m_LogLevel = DISABLE_LOG;
    }

    // Interfaces to control log Types
    void Logger::updateLogType(LogType logType)
    {
        m_LogType = logType;
    }

    void Logger::enableConsoleLogging()
    {
        m_LogType = CONSOLE;
    }

    void Logger::enableFileLogging()
    {
        m_LogType = FILE_LOG ;
    }

    // Interfaces to control roll over mechanism
    void Logger::updateMaxLogFiles(const ssize_t maxFiles)
    {
        if (maxFiles > 0)
            maxLogFiles = maxFiles;
        else
            maxLogFiles = MAX_LOG_FILES;

    }

    void Logger::updateLogSize(const ssize_t size)
    {
        if (size > 0)
        logSize = size;

        else
            logSize = LOG_FILE_SIZE;
    }

    // Handle roll over mechanism
    void Logger::rollLogFiles()
    {
        m_File.close();

        if (maxLogFiles > 1)
        {
            std::string logFile = logFileName.substr(0, logFileName.length()-4); // removing ".log" from file name

            // To check if the maximum files have reached
            if (logFilesCount >= maxLogFiles)
            {
                std::string deleteFileName = logFile + "_" + std::to_string(maxLogFiles-1) + ".tar.gz";
                remove(deleteFileName.c_str());

                logFilesCount--;
            }

            // Renaming the files
            for (int i = logFilesCount; i >= 2; --i)
        {
                std::string oldLogFileName = logFile + "_" + std::to_string(i-1) + ".tar.gz";
                std::string newLogFileName = logFile + "_" + std::to_string(i) + ".tar.gz";

                rename(oldLogFileName.c_str(), newLogFileName.c_str());
            }

            std::string cmd = "tar -cf " + logFile + "_1.tar.gz " + logFileName;

            int ret = system(cmd.c_str()); // creating tar file
        }

        remove(logFileName.c_str());

        m_File.open(logFileName.c_str(), std::ios::out|std::ios::app);

        if (logFilesCount < maxLogFiles)
        {
            logFilesCount++;
        }
    }

    // For configuration
    // Note: The function sets the default parameters if any paramter is incorrect or missing
    void Logger::configure()
    {
        LogLevel logLevel;
        LogType logType;

        std::string logLevel_str;
        std::string logType_str;

        int logFiles;
        int logFileSize;

#if LOGS_LEVEL==0
        logLevel = DISABLE_LOG;
#elif LOGS_LEVEL==1
        logLevel = ENABLE_LOG;
#elif LOGS_LEVEL==2
        logLevel = LOG_LEVEL_INFO;
#elif LOGS_LEVEL==3
        logLevel = LOG_LEVEL_BUFFER;
#elif LOGS_LEVEL==4
        logLevel = LOG_LEVEL_TRACE;
#elif LOGS_LEVEL==5
        logLevel = LOG_LEVEL_DEBUG;
#else
        logLevel = LOG_LEVEL_TRACE;
#endif

#if LOGS_TYPE==0
        logType = NO_LOG;
#elif LOGS_TYPE==1
        logType = CONSOLE;
#elif LOGS_TYPE==2
        logType = FILE_LOG;
#else
        logType = CONSOLE;
#endif

        logFiles = MAX_LOG_FILES;
        logFileSize = LOG_FILE_SIZE;

        // Setting the parameters
        m_LogLevel = logLevel;
        m_LogType = logType;

        updateMaxLogFiles(logFiles);
        updateLogSize(logFileSize);

    }

  }
}
