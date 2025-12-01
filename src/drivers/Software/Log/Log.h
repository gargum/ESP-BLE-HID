/**
 * @file Log.h
 * @brief Asynchronous logger with log queue
 */

#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <queue>
#include <string>
#include <functional>
#include <cstdarg>

// Platform detection - Only ESP/nRF for now
#if defined(ARDUINO_ARCH_ESP32)
  #define SQUIDHID_PLATFORM_ESP32
  #include "esp_log.h"
#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)
  #define SQUIDHID_PLATFORM_NRF52
  #include <nrf_log.h>
  #include <nrf_log_ctrl.h>
  #include <nrf_log_default_backends.h>
#else
  #error "This library only supports ESP32 and nRF52 platforms with NimBLE support currently, I'm sorry."
#endif

// Unified log levels - matches both ESP32 and nRF52 semantics
enum class LogLevel {
    NONE = 0,
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    VERBOSE
};

// Log entry structure
struct LogEntry {
    uint32_t timestamp;
    LogLevel level;
    std::string tag;
    std::string message;
    
    LogEntry(uint32_t ts, LogLevel lvl, const std::string& t, const std::string& msg)
        : timestamp(ts), level(lvl), tag(t), message(msg) {}
};

// Async Logger class - ALWAYS uses queue across all platforms
class SQUIDLOGS {
private:
    std::queue<LogEntry> logQueue;
    bool initialized = false;
    uint32_t maxQueueSize = 100; // Prevent memory exhaustion
    LogLevel currentLogLevel = LogLevel::INFO; // Default level
    std::function<void(const LogEntry&)> outputHandler;
    
public:
    static SQUIDLOGS& getInstance() {
        static SQUIDLOGS instance;
        return instance;
    }
    
    SQUIDLOGS() = default;
    
    void initialize(std::function<void(const LogEntry&)> handler = nullptr);
    void log(LogLevel level, const std::string& tag, const std::string& message);
    void processQueue();
    void flush();
    
    // Unified log level control
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const { return currentLogLevel; }
    
    void setMaxQueueSize(uint32_t size) { maxQueueSize = size; }
    size_t getQueueSize() const { return logQueue.size(); }
    bool isInitialized() const { return initialized; }
    bool isQueueEmpty() const { return logQueue.empty(); }
};

inline void _bleLogHelper(LogLevel level, const std::string& tag, const char* format, ...) {
    // Check if this log level should be processed
    if (static_cast<int>(level) > static_cast<int>(SQUIDLOGS::getInstance().getLogLevel())) {
        return;
    }
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    SQUIDLOGS::getInstance().log(level, tag, buffer);
}

// Convenience macros for logging
#define SQUID_LOG_VERBOSE(tag, format, ...) \
    _bleLogHelper(LogLevel::VERBOSE, tag, format, ##__VA_ARGS__)

#define SQUID_LOG_DEBUG(tag, format, ...) \
    _bleLogHelper(LogLevel::DEBUG, tag, format, ##__VA_ARGS__)

#define SQUID_LOG_INFO(tag, format, ...) \
    _bleLogHelper(LogLevel::INFO, tag, format, ##__VA_ARGS__)

#define SQUID_LOG_WARN(tag, format, ...) \
    _bleLogHelper(LogLevel::WARNING, tag, format, ##__VA_ARGS__)

#define SQUID_LOG_ERROR(tag, format, ...) \
    _bleLogHelper(LogLevel::ERROR, tag, format, ##__VA_ARGS__)

// Process queue macro (ALWAYS needed now)
#define SQUID_LOG_PROCESS() SQUIDLOGS::getInstance().processQueue()

// Force flush all logs (blocks until complete)
#define SQUID_LOG_FLUSH() SQUIDLOGS::getInstance().flush()

// Check if log queue is empty
#define SQUID_LOG_QUEUE_EMPTY() SQUIDLOGS::getInstance().isQueueEmpty()

// Unified log level control - NEW SIMPLE INTERFACE
#define SQUID_LOG_SET_LEVEL(level) SQUIDLOGS::getInstance().setLogLevel(level)

#define LOGGER_NONE    LogLevel::NONE
#define LOGGER_INFO    LogLevel::INFO
#define LOGGER_WARN    LogLevel::WARN
#define LOGGER_ERROR   LogLevel::ERROR
#define LOGGER_DEBUG   LogLevel::DEBUG
#define LOGGER_VERBOSE LogLevel::VERBOSE

// Backward compatibility macros
#if defined(SQUIDHID_PLATFORM_ESP32)
  #define SQUID_LOG_SET_PLATFORM_LEVEL(level) SQUIDLOGS::getInstance().setESP32LogLevel(level)
#elif defined(SQUIDHID_PLATFORM_NRF52)
  #define SQUID_LOG_SET_PLATFORM_LEVEL(level) SQUIDLOGS::getInstance().setNRF52LogLevel(level)
#endif

#endif
