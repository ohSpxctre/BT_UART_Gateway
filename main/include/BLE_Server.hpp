/**
 * @file BLE_Server.hpp
 * @brief Header file for the BLE_Server class, which implements a Bluetooth Low Energy (BLE) server.
 * 
 * This file defines the BLE_Server class, which is responsible for managing BLE server functionality.
 * The class provides methods for setting up BLE connections, handling GATT and GAP events, and sending
 * data over BLE.
 * 
 * Usage:
 * - Create an instance of the BLE_Server class.
 * - Call the `connSetup()` method to initialize the BLE server.
 * - Use `sendTask` methods for a Task to transmit data over BLE.
 * - Receiving data is handled through the event handler methods and written directly into the message queue.
 * 
 * @note This class is designed for use with the ESP32 platform and the ESP-IDF framework.
 * @note The class is intended to be used with a message handler for managing by using message queues.
 * @note The class provides functionality, including advertising, connection management, and data transmission for a BLE server. 
 * 
 * @author hoyed1
 * @date 09.04.2025
 */

 #pragma once

#include "Bluetooth.hpp"
#include "MessageHandler.hpp"

#include <cstdint> // Include for fixed-width integer types


/**
 * @class BLE_Server
 * @brief A class that implements a Bluetooth Low Energy (BLE) server.
 * 
 * This class provides functionality to set up and manage a BLE server, including
 * advertising, handling GATT events, and sending/receiving data.
 */
class BLE_Server : public Bluetooth {

private:

  bool _new_data_rcv = false; /**< Flag indicating if new data is available. */

  BLE_Defaults::ConnectionState _conn_state = BLE_Defaults::ConnectionState::NOT_CONNECTED; /**< Current connection state of the server. */
  bool _conn_state_change = true;  /**< Flag indicating if the connection state has changed. */

  /**
   * @brief Static instance pointer for the BLE_Server class.
   */
  static BLE_Server* Server_instance;

  /**
   * @brief Buffer for BLE advertising data.
   */
  uint8_t _adv_data_buffer[ESP_BLE_ADV_DATA_LEN_MAX] = "Hello World!";

  /**
   * @brief Buffer for BLE characteristic data.
   */
  uint8_t _char_data_buffer[BLE_Defaults::MTU_DEFAULT-3] = "Hello Server!";

  uint8_t _characteristic_value[BLE_Defaults::MTU_DEFAULT-3] = {0}; /**< Buffer for characteristic value. */

  /**
   * @brief Buffer for BLE characteristic descriptor data.
   */
  uint8_t _descr_data_buffer[128] = "Characteristic Descriptor Data";

  /**
   * @brief Buffer for receiving BLE characteristic data.
   */
  uint8_t _char_rcv_buffer[BLE_Defaults::MTU_DEFAULT-3] = {0};

  /**
   * @brief GATT profile instance structure.
   */
  BLE_Defaults::gatts_profile_inst _gatts_profile_inst = {
    .gatts_cb = gatts_event_handler,                                                                          /**< callback event handler method */
    .gatts_if = ESP_GATT_IF_NONE,                                                                             /**< BLE Interface*/
    .app_id = BLE_Defaults::PROFILE_APP_ID,                                                                   /**< Application ID */
    .conn_id = 0,                                                                                             /**< Connection ID */
    .service_handle = 0,                                                                                      /**< Handle for BLE Service*/
    .service_id = BLE_Defaults::SERVICE_ID_DEFAULT,                                                           /**< Structure with ID for Service */
    .char_handle = 0,                                                                                         /**< Handle for BLE Characteristic */
    .char_uuid = BLE_Defaults::CHAR_UUID_DEFAULT,                                                             /**< Structure with UUID for Characteristic */
    .char_resp_ctrl = ESP_GATT_AUTO_RSP,                                                                      /**< Response control for characteristic */
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,                                                         /**< Permissions for characteristic */
    .property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_INDICATE, /**< Properties for characteristic */
    .descr_handle = 0,                                                                                        /**< Handle for characteristic descriptor */
    .descr_value = 0,                                                                                          /**< Value for characteristic descriptor, (Indication / Notification saved here)*/
    .descr_uuid = BLE_Defaults::DESCR_UUID_DEFAULT,                                                           /**< Structure with UUID for characteristic descriptor */
    .local_mtu = BLE_Defaults::MTU_DEFAULT,                                                                   /**< Local MTU size (Describes size of data paket to send) */
  };

  /**
   * @brief BLE advertising parameters.
   */
  esp_ble_adv_params_t _adv_params;

  /**
   * @brief BLE advertising data.
   */
  esp_ble_adv_data_t _adv_data;

  /**
   * @brief Flag indicating whether the server is advertising.
   */
  bool _is_advertising = false;

  /**
   * @brief Flag indicating whether the server is connected to a client.
   */
  bool _is_connected = false;

  /**
   * @brief BLE characteristic value attribute.
   */
  esp_attr_value_t _char_value = {            
    .attr_max_len = ESP_GATT_MAX_ATTR_LEN,    /**< Maximum length of characteristic value */
    .attr_len = sizeof(_characteristic_value),    /**< Current length of characteristic value */
    .attr_value = _characteristic_value           /**< Pointer to characteristic value array */
  };

  /**
   * @brief BLE characteristic descriptor value attribute.
   */
  esp_attr_value_t _descr_value = {
    .attr_max_len = ESP_GATT_MAX_ATTR_LEN,    /**< Maximum length of descriptor value */
    .attr_len = sizeof(_descr_data_buffer),   /**< Current length of descriptor value */
    .attr_value = _descr_data_buffer          /**< Pointer to descriptor value array */
  };

  /**
   * @brief Static handler for GAP events.
   * 
   * This static method calls the non-static event handler function.
   * It is used to register the callback function which has to be static and then calls the non-static event handler.
   * 
   * @param event The GAP event.
   * @param param The GAP callback parameters.
   */
  static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);


  /**
   * @brief Static handler for GATT server events.
   * 
   * This static function calls the non-static event handler function.
   * It is used to register the callback function which has to be static. It then calls the non-static event handler.
   * 
   * @param event The GATT server event.
   * @param gatts_if The GATT interface.
   * @param param The GATT server callback parameters.
   */
  static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

  /**
   * @brief Handle GAP events.
   * 
   * The function handles the GAP events and calls the appropriate functions to handle them.
   * 
   * Key responsibilities:
   * - Advertising to devices
   * - Establishing and terminating connections
   * - Handling connection parameters (interval, latency, timeout)
   * 
   * @param event The GAP event.
   * @param param The GAP callback parameters.
   */
  void handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

  /**
   * @brief Handle GATT server events.
   * 
   * The GATT Server defines how BLE devices exchange data using concepts like 
   * services, characteristics, and descriptors. It stores and provides access 
   * to structured data that remote GATT Clients can read or write.
   * 
   * Key responsibilities:
   * - Hosting services and characteristics
   * - Handling read/write requests from clients
   * - Sending notifications or indications when data changes
   * - Defining the data model accessible over BLE
   * 
   * @param event The GATT server event.
   * @param gatts_if The GATT interface.
   * @param param The GATT server callback parameters.
   */
  void handle_event_gatts(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

  /**
   * @brief Get the name of a GATT server event.
   * @param event The GATT server event.
   * @return The name of the event as a string.
   */
  const char* get_gatts_event_name(esp_gatts_cb_event_t event);

  /**
   * @brief Get the name of a GAP event.
   * @param event The GAP event.
   * @return The name of the event as a string.
   */
  const char* get_gap_event_name(esp_gap_ble_cb_event_t event);

public:
  /**
   * @brief Constructor for the BLE_Server class.
   * 
   * In the Constructor the initialization of nvs flash, Bluetooth controller initialization and
   * Bluedroid stack initialization is done.
   * 
   * @param adv_params BLE advertising parameters (default: ADV_PARAMS_DEFAULT).
   * @param adv_data BLE advertising data (default: ADV_DATA_DEFAULT).
   * @param msgHandler Pointer to a message handler (default: nullptr).
   */
  BLE_Server(esp_ble_adv_params_t adv_params = BLE_Defaults::ADV_PARAMS_DEFAULT,
             esp_ble_adv_data_t adv_data = BLE_Defaults::ADV_DATA_DEFAULT,
             MessageHandler* msgHandler = nullptr);

  /**
   * @brief Destructor for the BLE_Server class.
   * 
   * This method deinitializes the Bluetooth stack and frees resources.
   */
  ~BLE_Server();

  /**
   * @brief Set up the BLE connection.
   * 
   * This function registers the callback method, registers the BLE server application
   * and sets the local MTU size
   */
  void connSetup(void) override;

  /**
   * @brief Send data over BLE.
   * 
   * If a BLE connection is established, this method sends the data over BLE.
   * 
   * @param data The data to send.
   */
  void send(const char *data) override;

  /**
   * @brief Task to send data using a message handler.
   * 
   * This Task reads data from the message handler's queue and sends it over BLE.
   * If no data is available, it will send dummy packets to keep the connection alive.
   * 
   * @param msgHandler Pointer to the message handler.
   */
  void sendTask(MessageHandler* msgHandler) override;  

  bool receive(uint8_t *data) override;

  void receiveTask(MessageHandler* msgHandler) override; // Receive data from the BLE device and send it to the UART interface

  void logBleState (MessageHandler * msgHandler) override; // Log the BLE state to the UART interface
};