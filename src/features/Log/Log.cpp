/**
 * @file Log.cpp
 * @brief Implementation of the asynchronous logger
 */

#include "Log.h"

void BLELOGS::initialize(std::function<void(const LogEntry&)> handler) {
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
                esp_log_write(espLevel, entry.tag.c_str(), "[%08lu] %s: %s\n", entry.timestamp, levelStr, entry.message.c_str());
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
                NRF_LOG_INFO("[%08lu] [%s] %s: %s\n", entry.timestamp, levelStr, entry.tag.c_str(), entry.message.c_str());
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

void BLELOGS::log(LogLevel level, const std::string& tag, const std::string& message) {
    // ALWAYS use queue for consistency across all platforms
    if (!initialized || logQueue.size() >= maxQueueSize) return;
    
    uint32_t timestamp = millis();
    logQueue.emplace(timestamp, level, tag, message);
}

void BLELOGS::processQueue() {
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

void BLELOGS::flush() {
    processQueue();
    
    // Small delay to ensure platform-specific logging completes
    #if defined(BLEHID_PLATFORM_NRF52)
        delay(1); // nRF52 may need a moment for log flushing
    #elif defined(BLEHID_PLATFORM_ESP32)
        delay(1); // ESP32 logging is generally fast but safe to wait
    #endif
}

// Platform-specific implementations
#if defined(BLEHID_PLATFORM_ESP32)
void BLELOGS::setESP32LogLevel(esp_log_level_t level) {
    esp_log_level_set("*", level);
}
#elif defined(BLEHID_PLATFORM_NRF52)
void BLELOGS::setNRF52LogLevel(nrf_log_severity_t severity) {
    NRF_LOG_DEFAULT_LEVEL = severity;
}
#endif
