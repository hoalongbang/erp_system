// Modules/Logger/Logger.h
#ifndef MODULES_LOGGER_LOGGER_H
#define MODULES_LOGGER_LOGGER_H
#include <string>       // For std::string
#include <fstream>      // For std::ofstream (file output)
#include <iostream>     // For std::cout, std::cerr (console output)
#include <chrono>       // For std::chrono::system_clock (timestamps)
#include <ctime>        // For std::time_t, std::localtime (time formatting)
#include <iomanip>      // For std::put_time (time formatting)
#include <sstream>      // For std::stringstream (building log messages)
#include <memory>       // For std::shared_ptr (managing Logger instance)
#include <mutex>        // For std::mutex (thread safety for logging)
#include <map>          // For string conversions

// Rút gọn include paths
#include "Common.h" // For LogSeverity enum (from Common module)

namespace ERP {
namespace Logger {

/**
 * @brief The Logger class provides a simple logging system for the application.
 * It allows logging messages at different severity levels (debug, info, warning, error, critical)
 * to various outputs (console, file, and potentially a remote service).
 * Implemented as a Singleton.
 */
class Logger {
public:
    /**
     * @brief Gets the singleton instance of the Logger.
     * @return A reference to the Logger instance.
     */
    static Logger& getInstance();

    // Delete copy constructor and assignment operator to enforce singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Sets the minimum log level to output. Messages with severity below this level will be ignored.
     * @param level The minimum log level.
     */
    void setLogLevel(ERP::Common::LogSeverity level);

    /**
     * @brief Sets the output file for logging.
     * @param filePath The path to the log file.
     * @return True if the file was successfully opened, false otherwise.
     */
    bool setLogFile(const std::string& filePath);

    /**
     * @brief Logs a debug message.
     * @param message The message to log.
     * @param category Optional category for the log message (e.g., "Database", "UI", "Network").
     */
    void debug(const std::string& message, const std::string& category = "General");

    /**
     * @brief Logs an informational message.
     * @param message The message to log.
     * @param category Optional category for the log message.
     */
    void info(const std::string& message, const std::string& category = "General");

    /**
     * @brief Logs a warning message.
     * @param message The message to log.
     * @param category Optional category for the log message.
     */
    void warning(const std::string& message, const std::string& category = "General");

    /**
     * @brief Logs an error message.
     * @param message The message to log.
     * @param category Optional category for the log message.
     */
    void error(const std::string& message, const std::string& category = "General");

    /**
     * @brief Logs a critical message (application-breaking error).
     * @param message The message to log.
     * @param category Optional category for the log message.
     */
    void critical(const std::string& message, const std::string& category = "General");

private:
    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    Logger();

    /**
     * @brief Helper method to format and write the log message to various outputs.
     * @param level The severity level of the message.
     * @param message The log message.
     * @param category The category for the log message.
     */
    void log(ERP::Common::LogSeverity level, const std::string& message, const std::string& category);

    ERP::Common::LogSeverity currentLogLevel_;
    std::ofstream logFile_;
    std::mutex mutex_; // Mutex for thread-safe logging
};

} // namespace Logger
} // namespace ERP
#endif // MODULES_LOGGER_LOGGER_H