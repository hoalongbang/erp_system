// Modules/Logger/Logger.cpp
#include "Logger.h"
#include "Common.h" // Standard includes

namespace ERP {
namespace Logger {

// Static member initialization
Logger* Logger::instance_ = nullptr;
std::once_flag Logger::onceFlag_;

Logger& Logger::getInstance() {
    // Use std::call_once to ensure thread-safe and one-time initialization
    std::call_once(onceFlag_, []() {
        instance_ = new Logger();
    });
    return *instance_;
}

Logger::Logger() : currentLogLevel_(ERP::Common::LogSeverity::INFO) {
    // Default constructor, log file not set by default.
    // Console logging is always enabled.
    ERP::Logger::Logger::getInstance().info("Logger: Constructor called. Default log level set to INFO.", "System");
}

void Logger::setLogLevel(ERP::Common::LogSeverity level) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentLogLevel_ = level;
    // Log the change in log level
    log(ERP::Common::LogSeverity::INFO, "Log level set to " + ERP::Common::logSeverityToString(currentLogLevel_), "Logger");
}

bool Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFile_.open(filePath, std::ios::app); // Open in append mode
    if (logFile_.is_open()) {
        log(ERP::Common::LogSeverity::INFO, "Log file set to: " + filePath, "Logger");
        return true;
    } else {
        std::cerr << "ERROR: Failed to open log file: " << filePath << std::endl;
        // Cannot use internal logging if log file fails and console output is suppressed.
        // For now, write to cerr.
        return false;
    }
}

void Logger::debug(const std::string& message, const std::string& category) {
    log(ERP::Common::LogSeverity::DEBUG, message, category);
}

void Logger::info(const std::string& message, const std::string& category) {
    log(ERP::Common::LogSeverity::INFO, message, category);
}

void Logger::warning(const std::string& message, const std::string& category) {
    log(ERP::Common::LogSeverity::WARNING, message, category);
}

void Logger::error(const std::string& message, const std::string& category) {
    log(ERP::Common::LogSeverity::ERROR, message, category);
}

void Logger::critical(const std::string& message, const std::string& category) {
    log(ERP::Common::LogSeverity::CRITICAL, message, category);
}

void Logger::log(ERP::Common::LogSeverity level, const std::string& message, const std::string& category) {
    if (level < currentLogLevel_) {
        return; // Message severity is below the set log level, so don't log.
    }

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    std::string formattedMessage = ss.str() + " [" + ERP::Common::logSeverityToString(level) + "] [" + category + "] " + message;

    std::lock_guard<std::mutex> lock(mutex_); // Lock for thread safety

    // Output to console
    if (level >= ERP::Common::LogSeverity::ERROR) {
        std::cerr << formattedMessage << std::endl;
    } else {
        std::cout << formattedMessage << std::endl;
    }

    // Output to file if open
    if (logFile_.is_open()) {
        logFile_ << formattedMessage << std::endl;
        logFile_.flush(); // Ensure message is written immediately
    }
}

} // namespace Logger
} // namespace ERP