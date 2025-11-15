/**
 * @file Log.cpp
 * @brief Implementation of the asynchronous logger
 */

#include "Log.h"

void SQUIDLOGS::initialize(std::function<void(const LogEntry&)> handler) {
    if (initialized) return;
    
    if (handler) {
        outputHandler = handler;
    } else {
        // Platform-specific default handlers
        #if defined(SQUIDHID_PLATFORM_ESP32)
            outputHandler = [](const LogEntry& entry) {
                const char* levelStr = "";
                esp_log_level_t espLevel = ESP_LOG_INFO;
                
                switch (entry.level) {
                    case LogLevel::VERBOSE:
                        levelStr = "V";
                        espLevel = ESP_LOG_VERBOSE;
                        break;
                    case LogLevel::DEBUG:
                        levelStr = "D";
                        espLevel = ESP_LOG_DEBUG;
                        break;
                    case LogLevel::INFO:
                        levelStr = "I";
                        espLevel = ESP_LOG_INFO;
                        break;
                    case LogLevel::WARNING:
                        levelStr = "W";
                        espLevel = ESP_LOG_WARN;
                        break;
                    case LogLevel::ERROR:
                        levelStr = "E";
                        espLevel = ESP_LOG_ERROR;
                        break;
                    default:
                        levelStr = "U";
                        espLevel = ESP_LOG_INFO;
                        break;
                }
                
                // Use ESP32 logging system but through our async queue
                esp_log_write(espLevel, entry.tag.c_str(), "[%08lu] [%s] [%s] %s\n", entry.timestamp, levelStr, entry.tag.c_str(), entry.message.c_str());
            };
            
        #elif defined(SQUIDHID_PLATFORM_NRF52)
            outputHandler = [](const LogEntry& entry) {
                const char* levelStr = "";
                
                switch (entry.level) {
                    case LogLevel::VERBOSE:  levelStr = "V"; break;
                    case LogLevel::DEBUG:    levelStr = "D"; break;
                    case LogLevel::INFO:     levelStr = "I"; break;
                    case LogLevel::WARNING:  levelStr = "W"; break;
                    case LogLevel::ERROR:    levelStr = "E"; break;
                    default:                 levelStr = "U"; break;
                }
                
                // Use nRF52 logging system but through our async queue
                NRF_LOG_INFO("[%08lu] [%s] %s: %s", entry.timestamp, levelStr, entry.tag.c_str(), entry.message.c_str());
                NRF_LOG_FLUSH();
            };
        #endif
    }
    
    // Platform-specific initialization
    #if defined(SQUIDHID_PLATFORM_NRF52)
        ret_code_t err_code = NRF_LOG_INIT(NULL);
        NRF_LOG_DEFAULT_BACKENDS_INIT();
    #endif
    
    initialized = true;
}

void SQUIDLOGS::log(LogLevel level, const std::string& tag, const std::string& message) {
    // Early return if logging is disabled for this level
    if (!initialized || static_cast<int>(level) > static_cast<int>(currentLogLevel)) {
        return;
    }
    
    // Check queue size
    if (logQueue.size() >= maxQueueSize) {
        // Optionally log a warning about queue overflow
        return;
    }
    
    uint32_t timestamp = millis();
    logQueue.emplace(timestamp, level, tag, message);
}

void SQUIDLOGS::setLogLevel(LogLevel level) {
    currentLogLevel = level;
    
    // Also set platform-specific log levels for underlying systems
    #if defined(SQUIDHID_PLATFORM_ESP32)
        esp_log_level_t espLevel;
        switch (level) {
            case LogLevel::NONE:     espLevel = ESP_LOG_NONE; break;
            case LogLevel::ERROR:    espLevel = ESP_LOG_ERROR; break;
            case LogLevel::WARNING:  espLevel = ESP_LOG_WARN; break;
            case LogLevel::INFO:     espLevel = ESP_LOG_INFO; break;
            case LogLevel::DEBUG:    espLevel = ESP_LOG_DEBUG; break;
            case LogLevel::VERBOSE:  espLevel = ESP_LOG_VERBOSE; break;
            default:                 espLevel = ESP_LOG_INFO; break;
        }
        esp_log_level_set("*", espLevel);
        
    #elif defined(SQUIDHID_PLATFORM_NRF52)
        nrf_log_severity_t nrfLevel;
        switch (level) {
            case LogLevel::NONE:     nrfLevel = NRF_LOG_SEVERITY_NONE; break;
            case LogLevel::ERROR:    nrfLevel = NRF_LOG_SEVERITY_ERROR; break;
            case LogLevel::WARNING:  nrfLevel = NRF_LOG_SEVERITY_WARNING; break;
            case LogLevel::INFO:     nrfLevel = NRF_LOG_SEVERITY_INFO; break;
            case LogLevel::DEBUG:    nrfLevel = NRF_LOG_SEVERITY_DEBUG; break;
            case LogLevel::VERBOSE:  nrfLevel = NRF_LOG_SEVERITY_INFO; break; // nRF doesn't have VERBOSE
            default:                 nrfLevel = NRF_LOG_SEVERITY_INFO; break;
        }
        NRF_LOG_DEFAULT_LEVEL = nrfLevel;
    #endif
    
    // Log the level change
    const char* levelStr = "";
    switch (level) {
        case LogLevel::NONE:     levelStr = "NONE"; break;
        case LogLevel::ERROR:    levelStr = "ERROR"; break;
        case LogLevel::WARNING:  levelStr = "WARNING"; break;
        case LogLevel::INFO:     levelStr = "INFO"; break;
        case LogLevel::DEBUG:    levelStr = "DEBUG"; break;
        case LogLevel::VERBOSE:  levelStr = "VERBOSE"; break;
    }
    
    log(LogLevel::INFO, "LOG", std::string("Log level set to: ") + levelStr);
}

void SQUIDLOGS::processQueue() {
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
    #if defined(SQUIDHID_PLATFORM_NRF52)
        NRF_LOG_FLUSH();
    #endif
}

void SQUIDLOGS::flush() {
    processQueue();
    
    // Small delay to ensure platform-specific logging completes
    #if defined(SQUIDHID_PLATFORM_NRF52)
        delay(1); // nRF52 may need a moment for log flushing
    #elif defined(SQUIDHID_PLATFORM_ESP32)
        delay(1); // ESP32 logging is generally fast but safe to wait
    #endif
}

// Platform-specific implementations (backward compatibility)
#if defined(SQUIDHID_PLATFORM_ESP32)
void SQUIDLOGS::setESP32LogLevel(esp_log_level_t level) {
    // Map ESP32 level to our unified level
    LogLevel unifiedLevel;
    switch (level) {
        case ESP_LOG_NONE:    unifiedLevel = LogLevel::NONE; break;
        case ESP_LOG_ERROR:   unifiedLevel = LogLevel::ERROR; break;
        case ESP_LOG_WARN:    unifiedLevel = LogLevel::WARNING; break;
        case ESP_LOG_INFO:    unifiedLevel = LogLevel::INFO; break;
        case ESP_LOG_DEBUG:   unifiedLevel = LogLevel::DEBUG; break;
        case ESP_LOG_VERBOSE: unifiedLevel = LogLevel::VERBOSE; break;
        default:              unifiedLevel = LogLevel::INFO; break;
    }
    setLogLevel(unifiedLevel);
}
#elif defined(SQUIDHID_PLATFORM_NRF52)
void SQUIDLOGS::setNRF52LogLevel(nrf_log_severity_t severity) {
    // Map nRF52 level to our unified level
    LogLevel unifiedLevel;
    switch (severity) {
        case NRF_LOG_SEVERITY_NONE:      unifiedLevel = LogLevel::NONE; break;
        case NRF_LOG_SEVERITY_ERROR:     unifiedLevel = LogLevel::ERROR; break;
        case NRF_LOG_SEVERITY_WARNING:   unifiedLevel = LogLevel::WARNING; break;
        case NRF_LOG_SEVERITY_INFO:      unifiedLevel = LogLevel::INFO; break;
        case NRF_LOG_SEVERITY_DEBUG:     unifiedLevel = LogLevel::DEBUG; break;
        default:                         unifiedLevel = LogLevel::INFO; break;
    }
    setLogLevel(unifiedLevel);
}
#endif
