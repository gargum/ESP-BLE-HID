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
  #define BLEHID_PLATFORM_ESP32
  #include "esp_log.h"
#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)
  #define BLEHID_PLATFORM_NRF52
  #include <nrf_log.h>
  #include <nrf_log_ctrl.h>
  #include <nrf_log_default_backends.h>
#else
  #error "This library only supports ESP32 and nRF52 platforms with NimBLE support currently, I'm sorry."
#endif

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
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
class BLELOGS {
private:
    std::queue<LogEntry> logQueue;
    bool initialized = false;
    uint32_t maxQueueSize = 100; // Prevent memory exhaustion
    std::function<void(const LogEntry&)> outputHandler;
public:
    static BLELOGS& getInstance() {
        static BLELOGS instance;
        return instance;
    }
    
    BLELOGS() = default;
    
    void initialize(std::function<void(const LogEntry&)> handler = nullptr);
    void log(LogLevel level, const std::string& tag, const std::string& message);
    void processQueue();
    void flush();
    void setMaxQueueSize(uint32_t size) { maxQueueSize = size; }
    size_t getQueueSize() const { return logQueue.size(); }
    bool isInitialized() const { return initialized; }
    bool isQueueEmpty() const { return logQueue.empty(); }
    
    // Platform-specific control methods
    #if defined(BLEHID_PLATFORM_ESP32)
    void setESP32LogLevel(esp_log_level_t level);
    #elif defined(BLEHID_PLATFORM_NRF52)
    void setNRF52LogLevel(nrf_log_severity_t severity);
    #endif
};

inline void _bleLogHelper(LogLevel level, const std::string& tag, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    BLELOGS::getInstance().log(level, tag, buffer);
}

// Convenience macros for logging
#define BLE_LOG_DEBUG(tag, format, ...) \
    _bleLogHelper(LogLevel::DEBUG, tag, format, ##__VA_ARGS__)

#define BLE_LOG_INFO(tag, format, ...) \
    _bleLogHelper(LogLevel::INFO, tag, format, ##__VA_ARGS__)

#define BLE_LOG_WARN(tag, format, ...) \
    _bleLogHelper(LogLevel::WARNING, tag, format, ##__VA_ARGS__)

#define BLE_LOG_ERROR(tag, format, ...) \
    _bleLogHelper(LogLevel::ERROR, tag, format, ##__VA_ARGS__)

#define BLE_LOG_CRITICAL(tag, format, ...) \
    _bleLogHelper(LogLevel::CRITICAL, tag, format, ##__VA_ARGS__)

// Process queue macro (ALWAYS needed now)
#define BLE_LOG_PROCESS() BLELOGS::getInstance().processQueue()

// Force flush all logs (blocks until complete)
#define BLE_LOG_FLUSH() BLELOGS::getInstance().flush()

// Check if log queue is empty
#define BLE_LOG_QUEUE_EMPTY() BLELOGS::getInstance().isQueueEmpty()

// Platform-specific utility macros
#if defined(BLEHID_PLATFORM_ESP32)
  #define BLE_LOG_SET_LEVEL(level) BLELOGS::getInstance().setESP32LogLevel(level)
#elif defined(BLEHID_PLATFORM_NRF52)
  #define BLE_LOG_SET_LEVEL(level) BLELOGS::getInstance().setNRF52LogLevel(level)
#endif

#endif
