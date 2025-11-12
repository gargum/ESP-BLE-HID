#pragma once

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
class AsyncLogger {
private:
    std::queue<LogEntry> logQueue;
    bool initialized = false;
    uint32_t maxQueueSize = 100; // Prevent memory exhaustion
    std::function<void(const LogEntry&)> outputHandler;
    
    // Singleton pattern
    AsyncLogger() = default;
    
public:
    static AsyncLogger& getInstance() {
        static AsyncLogger instance;
        return instance;
    }
    
    void initialize(std::function<void(const LogEntry&)> handler = nullptr) {
        if (initialized) return;
        
        if (handler) {
            outputHandler = handler;
        } else {
            // Platform-specific default handlers
            #if defined(BLEHID_PLATFORM_ESP32)
                outputHandler = [](const LogEntry& entry) {
                    const char* levelStr = "";
                    esp_log_level_t espLevel = ESP_LOG_INFO;
                    
                    switch (entry.level) {
                        case LogLevel::DEBUG:
                            levelStr = "DEBUG";
                            espLevel = ESP_LOG_DEBUG;
                            break;
                        case LogLevel::INFO:
                            levelStr = "INFO";
                            espLevel = ESP_LOG_INFO;
                            break;
                        case LogLevel::WARNING:
                            levelStr = "WARN";
                            espLevel = ESP_LOG_WARN;
                            break;
                        case LogLevel::ERROR:
                            levelStr = "ERROR";
                            espLevel = ESP_LOG_ERROR;
                            break;
                        case LogLevel::CRITICAL:
                            levelStr = "CRITICAL";
                            espLevel = ESP_LOG_ERROR;
                            break;
                    }
                    
                    // Use ESP32 logging system but through our async queue
                    esp_log_write(espLevel, entry.tag.c_str(), "[%08lu] %s: %s", 
                                 entry.timestamp, levelStr, entry.message.c_str());
                };
                
            #elif defined(BLEHID_PLATFORM_NRF52)
                outputHandler = [](const LogEntry& entry) {
                    const char* levelStr = "";
                    
                    switch (entry.level) {
                        case LogLevel::DEBUG:    levelStr = "D"; break;
                        case LogLevel::INFO:     levelStr = "I"; break;
                        case LogLevel::WARNING:  levelStr = "W"; break;
                        case LogLevel::ERROR:    levelStr = "E"; break;
                        case LogLevel::CRITICAL: levelStr = "C"; break;
                    }
                    
                    // Use nRF52 logging system but through our async queue
                    NRF_LOG_INFO("[%08lu] [%s] %s: %s", 
                                entry.timestamp, levelStr, entry.tag.c_str(), entry.message.c_str());
                    NRF_LOG_FLUSH();
                };
            #endif
        }
        
        // Platform-specific initialization
        #if defined(BLEHID_PLATFORM_NRF52)
            ret_code_t err_code = NRF_LOG_INIT(NULL);
            NRF_LOG_DEFAULT_BACKENDS_INIT();
        #endif
        
        initialized = true;
    }
    
    void log(LogLevel level, const std::string& tag, const std::string& message) {
        // ALWAYS use queue for consistency across all platforms
        if (!initialized || logQueue.size() >= maxQueueSize) return;
        
        uint32_t timestamp = millis();
        logQueue.emplace(timestamp, level, tag, message);
    }
    
    void processQueue() {
        if (!initialized) return;
        
        // Process all queued messages
        while (!logQueue.empty()) {
            const LogEntry& entry = logQueue.front();
            if (outputHandler) {
                outputHandler(entry);
            }
            logQueue.pop();
        }
        
        // Platform-specific flush if needed
        #if defined(BLEHID_PLATFORM_NRF52)
            NRF_LOG_FLUSH();
        #endif
    }
    
    void setMaxQueueSize(uint32_t size) {
        maxQueueSize = size;
    }
    
    size_t getQueueSize() const {
        return logQueue.size();
    }
    
    bool isInitialized() const {
        return initialized;
    }
    
    // Force process queue and wait for completion (useful for critical sections)
    void flush() {
        processQueue();
        
        // Small delay to ensure platform-specific logging completes
        #if defined(BLEHID_PLATFORM_NRF52)
            delay(1); // nRF52 may need a moment for log flushing
        #elif defined(BLEHID_PLATFORM_ESP32)
            delay(1); // ESP32 logging is generally fast but safe to wait
        #endif
    }
    
    // Check if queue is empty (useful for testing/debugging)
    bool isQueueEmpty() const {
        return logQueue.empty();
    }
    
    // Platform-specific control methods
    #if defined(BLEHID_PLATFORM_ESP32)
    void setESP32LogLevel(esp_log_level_t level) {
        esp_log_level_set("*", level);
    }
    #elif defined(BLEHID_PLATFORM_NRF52)
    void setNRF52LogLevel(nrf_log_severity_t severity) {
        NRF_LOG_DEFAULT_LEVEL = severity;
    }
    #endif
};

// Helper function implementation
inline void _bleLogHelper(LogLevel level, const std::string& tag, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    AsyncLogger::getInstance().log(level, tag, buffer);
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
#define BLE_LOG_PROCESS() AsyncLogger::getInstance().processQueue()

// Force flush all logs (blocks until complete)
#define BLE_LOG_FLUSH() AsyncLogger::getInstance().flush()

// Check if log queue is empty
#define BLE_LOG_QUEUE_EMPTY() AsyncLogger::getInstance().isQueueEmpty()

// Platform-specific utility macros
#if defined(BLEHID_PLATFORM_ESP32)
  #define BLE_LOG_SET_LEVEL(level) AsyncLogger::getInstance().setESP32LogLevel(level)
#elif defined(BLEHID_PLATFORM_NRF52)
  #define BLE_LOG_SET_LEVEL(level) AsyncLogger::getInstance().setNRF52LogLevel(level)
#endif
