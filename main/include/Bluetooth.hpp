/**
 * @file Bluetooth.hpp
 * @brief Defines the Bluetooth abstract base class for BLE communication.
 *
 * This file contains the declaration of the Bluetooth class, which serves as
 * an abstract base class for handling Bluetooth Low Energy (BLE) communication.
 * It provides an interface for setting up connections and managing
 * communication. The class also allows integration with a message handler
 * for processing incoming and outgoing messages.
 *
 * Dependencies:
 * - Standard libraries: <string>, <cstdint>
 * - ESP-IDF libraries: esp_log.h, nvs_flash.h, esp_bt.h, esp_gap_ble_api.h,
 *   esp_gatt_defs.h, esp_gatt_common_api.h, esp_gatts_api.h, esp_gattc_api.h,
 *   esp_bt_main.h, esp_mac.h
 * - Project-specific headers: BLE_Config.hpp, MessageHandler.hpp
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
#include "esp_gattc_api.h"
#include "esp_bt_main.h"
#include "esp_mac.h"

#include "BLE_Config.hpp"
#include "MessageHandler.hpp"

/**
 * @class Bluetooth
 * @brief Abstract base class for BLE communication.
 *
 * The Bluetooth class provides a virtual interface for BLE communication. It
 * defines methods for setting up connections, sending data, and managing
 * communication tasks. Derived classes must implement the pure virtual methods
 * to provide specific BLE functionality.
 */
class Bluetooth {
protected:
    /**
     * @brief Pointer to the message handler for communication.
     * 
     * This pointer is used to send and receive messages through the message handler.
     */
    MessageHandler* _msgHandler; // Pointer to the message handler for communication
   
public:
    /**
     * @brief Constructor for the Bluetooth class.
     * 
     * This constructor initializes the Bluetooth class.
     */
    Bluetooth();

    /**
     * @brief Virtual destructor for the Bluetooth class.
     * 
     * Cleans up resources when the derived class is destroyed.
     */
    virtual ~Bluetooth();

    /**
     * @brief Pure virtual method for setting up the connection.
     * 
     * This method is implemented by derived classes to set up the BLE connection.
     */
    virtual void connSetup() = 0;

    /**
     * @brief Pure virtual method for sending data over BLE.
     * 
     * This method is implemented by derived classes to send data over BLE.
     * 
     * @param data The data to send.
     */
    virtual void send(const char *data) = 0;

    /**
     * @brief Pure virtual method for sending a task to the message handler.
     * 
     * This method is implemented by derived classes to send a task to the message handler.
     * 
     * @param msgHandler Pointer to the message handler.
     */
    virtual void sendTask(MessageHandler* msgHandler) = 0;

    /**
     * @brief Sets the message handler for communication.
     * 
     * This method allows setting a custom message handler for the Bluetooth class.
     * 
     * @param handler Pointer to the message handler.
     */
    void setMessageHandler(MessageHandler* handler)
    {
        _msgHandler = handler;
    }
};

#endif // BLUETOOTH_HPP