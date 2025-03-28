/**
* @file Bluetooth.hpp
* @brief Base class for Bluetooth communication.
*/

#ifndef BLUETOOTH_HPP
#define BLUETOOTH_HPP

#include <string>
#include <cstdint> // Include for fixed-width integer types
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h" // Include the header for GATT definitions
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"


/**
* @class Bluetooth
* @brief Abstract base class for BLE communication.
*/


class Bluetooth {
   
public:
    Bluetooth();
    virtual ~Bluetooth();

    virtual void connSetup() = 0;
    virtual void send(const std::string &data) = 0;
    virtual std::string receive() = 0;
};

#endif // BLUETOOTH_HPP