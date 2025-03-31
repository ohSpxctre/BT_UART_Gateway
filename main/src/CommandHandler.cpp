/**
 * @file CommandHandler.cpp
 * @brief Implementation of the CommandHandler class, which processes commands
 *        and retrieves system information such as chip details, IDF version, 
 *        and heap statistics.
 * 
 * This file contains the implementation of the CommandHandler class methods.
 * It provides functionality to handle commands and retrieve various system-related
 * information, including chip information, IDF version, and heap memory statistics.
 *
 * @note This implementation is designed to work with the ESP-IDF framework and FreeRTOS.
 * 
 * @author meths1
 * @date 31.03.2025
 */

#include "CommandHandler.hpp"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "esp_system.h"
#include "esp_clk_tree.h"

#include <sstream>

std::string CommandHandler::processCommand(Command cmd) {
    std::string result;
    switch (cmd) {
        case Command::CHIP_INFO: {
            result = getChipInfo();
            break;
        }
        case Command::IDF_VERSION: {
            result = getIdfVersion();
            break;
        }
        case Command::FREE_HEAP: {
            result = getFreeHeap();
            break;
        }
        case Command::FREE_INTERNAL_HEAP: {
            result = getFreeInternalHeap();
            break;
        }
        case Command::FREE_MIN_HEAP: {
            result = getFreeMinHeap();
            break;
        }
        case Command::CLOCK_SPEED: {
            result = getClockSpeed();
            break;
        }
        case Command::RESET: {
            esp_restart();
            break;
        }
        default: {
            ESP_LOGI("CommandHandler", "Unknown command");
            break;
        }
    }
    return result;
}

std::string CommandHandler::getChipInfo() const {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    std::ostringstream oss;
    std::string model;
    switch (chip_info.model) {
        case CHIP_ESP32: model = "ESP32"; break;
        case CHIP_ESP32S2: model = "ESP32-S2"; break;
        case CHIP_ESP32S3: model = "ESP32-S3"; break;
        case CHIP_ESP32C3: model = "ESP32-C3"; break;
        case CHIP_ESP32C2: model = "ESP32-C2"; break;
        case CHIP_ESP32C6: model = "ESP32-C6"; break;
        case CHIP_ESP32H2: model = "ESP32-H2"; break;
        case CHIP_ESP32P4: model = "ESP32-P4"; break;
        case CHIP_ESP32C61: model = "ESP32-C61"; break;
        case CHIP_ESP32C5: model = "ESP32-C5"; break;
        case CHIP_ESP32H21: model = "ESP32-H21"; break;
        case CHIP_ESP32H4: model = "ESP32-H4"; break;
        case CHIP_POSIX_LINUX: model = "POSIX/Linux Simulator"; break;
        default: model = "Unknown"; break;
    }

    oss << "Model: " << model << "\n"
        << "Features: "
        << ((chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi BGN " : "")
        << ((chip_info.features & CHIP_FEATURE_BLE) ? "Bluetooth LE " : "")
        << ((chip_info.features & CHIP_FEATURE_BT) ? "Bluetooth Classic " : "")
        << ((chip_info.features & CHIP_FEATURE_IEEE802154) ? "IEEE 802.15.4 " : "")
        << ((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded Flash " : "")
        << ((chip_info.features & CHIP_FEATURE_EMB_PSRAM) ? "Embedded PSRAM " : "")
        << "\n"
        << "Revision: " << chip_info.revision << "\n"
        << "Cores: " << chip_info.cores;
    return oss.str();
}

std::string CommandHandler::getIdfVersion() const {
    return std::string(esp_get_idf_version());
}

std::string CommandHandler::getFreeHeap() const {
    std::ostringstream oss;
    oss << "Free heap size: " << esp_get_free_heap_size() << " bytes";
    return oss.str();
}

std::string CommandHandler::getFreeInternalHeap() const {
    std::ostringstream oss;
    oss << "Free internal heap size: " << esp_get_free_internal_heap_size() << " bytes";
    return oss.str();
}

std::string CommandHandler::getFreeMinHeap() const {
    std::ostringstream oss;
    oss << "Minimum free heap size: " << esp_get_minimum_free_heap_size() << " bytes";
    return oss.str();
}

std::string CommandHandler::getClockSpeed() const {
    uint32_t cpu_freq = 0;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &cpu_freq);

    std::ostringstream oss;
    oss << "CPU clock speed: " << (cpu_freq / 1000000) << " MHz";
    return oss.str();
}