/**
 * @file BLE_Client.hpp
 * @brief Header file for the BLE_Client class, which implements a BLE (Bluetooth Low Energy) client.
 * 
 * This file defines the BLE_Client class, which is responsible for scanning, connecting, 
 * and interacting with BLE GATT (Generic Attribute Profile) servers. It provides methods 
 * for setting up connections, sending data, and handling BLE events.
 * 
 * Usage:
 * - Create an instance of the BLE_Client class.
 * - Call the connSetup() method to initialize the BLE client.
 * - Use sendTask() method for a Task to transmit data over BLE.
 * - Receiving data is handled through the event handler methods and written directly into the message queue.
 * 
 * @note This class is designed for use with the ESP32 platform and the ESP-IDF framework.
 * @note The class is intended to be used with a message handler for managing by using message queues.
 * @note The class provides functionality, including scanning, connection management, and data transmission for a BLE Client.
 * @note 
 * @author hoyed1
 * @date 09.04.2025
 */

 #ifndef BLE_CLIENT_HPP
 #define BLE_CLIENT_HPP
 
 #include "Bluetooth.hpp"
 #include "MessageHandler.hpp"
 
 /**
  * @class BLE_Client
  * @brief BLE Client implementation.
  * 
  * This class handles scanning, connecting, and interacting with BLE GATT servers.
  */
 class BLE_Client : public Bluetooth {
 private:
 
     /**
      * @brief Static instance pointer for singleton-like access.
      */
     static BLE_Client* Client_instance;
 
     /**
      * @brief GATT client profile instance configuration.
      */
     gattc_profile_inst _gattc_profile_inst = {
         .gattc_cb = gattc_event_handler,
         .gattc_if = ESP_GATT_IF_NONE,
         .app_id = PROFILE_APP_ID,
         .conn_id = 0,
         .service_start_handle = 0,
         .service_end_handle = 0,
         .char_handle = 0,
         .remote_bda = {0},
         .local_mtu = MTU_DEFAULT,
     };
 
     /**
      * @brief BLE scan parameters.
      */
     esp_ble_scan_params_t _scan_params;
 
     /**
      * @brief UUID of the target remote service.
      */
     esp_bt_uuid_t _remote_service_uuid;
 
     /**
      * @brief UUID of the target remote characteristic.
      */
     esp_bt_uuid_t _remote_char_uuid;
 
     /**
      * @brief UUID of the target remote descriptor.
      */
     esp_bt_uuid_t _remote_descr_uuid;
 
     /**
      * @brief Buffer to store outgoing data for BLE transmission.
      */
     uint8_t _char_send_buffer[ESP_GATT_MAX_ATTR_LEN] = {"Hello from ESP32 BLE Client"};
 
     /**
      * @brief Buffer to store incoming data received via BLE.
      */
     uint8_t _char_recv_buffer[ESP_GATT_MAX_ATTR_LEN] = {0};
 
     /**
      * @brief Flag indicating whether the client is currently connected.
      */
     bool _is_connected = false;
 
     /**
      * @brief Flag indicating whether the target GATT server has been found.
      */
     bool _get_server = false;
 
    /**
     * @brief Static handler for GAP events.
     * 
     * This static method calls the non-static event handler function.
     * It is used to register the callback function which has to be static and then calls the non-static event handler.
     * 
     * @param event The GAP event.
     * @param param The GAP callback parameters.
     */
    static void gap_event_handler(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

    /**
     * @brief Static GATT client event handler.
     * 
     * This static method calls the non-static event handler function.
     * It is used to register the callback function which has to be static and then calls the non-static event handler.
     * 
     * @param event The GATT client event.
     * @param gattc_if The GATT interface.
     * @param param The GATT client callback parameters.
     */
    static void gattc_event_handler(esp_gattc_cb_event_t, esp_gatt_if_t, esp_ble_gattc_cb_param_t *);
 
     /**
      * @brief Handles GAP events.
      * 
      * The function handles the GAP events and calls the appropriate functions to handle them.
      * 
      * Key responsibilities:
      * - Scanning for devices
      * - Establishing and terminating connections
      * - Handling connection parameters (interval, latency, timeout)
      * 
      * @param event The GAP event to handle.
      * @param param The GAP callback parameters.
      */
     void handle_event_gap(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

     /**
      * @brief Handle GATT client events.
      * 
      * The GATT Client interacts with remote GATT Servers to access their services, 
      * characteristics, and descriptors. It initiates requests to read, write, or 
      * subscribe to notifications/indications from the server.
      * 
      * Key responsibilities:
      * - Discovering services, characteristics, and descriptors on the server
      * - Reading and writing characteristic or descriptor values
      * - Subscribing for notifications or indications for real-time updates
      * - Managing the connection and communication with the GATT server
      * 
      * @param event The GATT client event.
      * @param gattc_if The GATT interface.
      * @param param The GATT client callback parameters.
      */
     void handle_event_gattc(esp_gattc_cb_event_t, esp_gatt_if_t, esp_ble_gattc_cb_param_t *);
 
     /**
      * @brief Utility function to get a readable GATT client event name.
      * @param event The GATT client event to get the name for.
      * @return A string representation of the GATT client event name.
      */
     const char* get_gattc_event_name(esp_gattc_cb_event_t);
 
     /**
      * @brief Utility function to get a readable GAP event name.
      * @param event The GAP event to get the name for.
      * @return A string representation of the GAP event name.
      */
     const char* get_gap_event_name(esp_gap_ble_cb_event_t);
 
 public:
 
     /**
      * @brief Constructor for BLE_Client.
      * 
      * @param scan_params BLE scan parameters.
      * @param remote_service_uuid UUID of the remote service to connect to.
      * @param remote_char_uuid UUID of the characteristic to use.
      * @param remote_descr_uuid UUID of the descriptor associated with the characteristic.
      * @param msgHandler Optional pointer to a message handler for async tasks.
      */
     BLE_Client(esp_ble_scan_params_t scan_params = BLE_SCAN_PARAMS_DEFAULT,
                 esp_bt_uuid_t remote_service_uuid = REMOTE_FILTER_SERVICE_UUID,
                 esp_bt_uuid_t remote_char_uuid = REMOTE_FILTER_CHAR_UUID,
                 esp_bt_uuid_t remote_descr_uuid = REMOTE_DESCR_UUID_DEFAULT,
                 MessageHandler* msgHandler = nullptr
     );
 
     /**
      * @brief Destructor for BLE_Client.
      * 
      * This method deinitializes the Bluetooth stack and frees resources.
      */
     ~BLE_Client();
 
     /**
      * @brief Setup connection to BLE device.
      * 
      * This function registers the callback methods, registers the BLE client application,
      * and sets the local MTU size.
      */
     void connSetup() override;
 
     /**
      * @brief Send data over BLE to connected device.
      * 
      * If a BLE connection is established, this method sends data to the connected device.
      * 
      * @param data The data to send.
      */
     void send(const char *data) override;
 
     /**
      * @brief Handles periodic sending tasks using the provided MessageHandler.
      * 
      * This Task reads data form the message handlers queue and sends it to the BLE device.
      * 
      * @param msgHandler Pointer to the message handler.
      */
     void sendTask(MessageHandler* msgHandler) override;
 };
 
 #endif // BLE_CLIENT_HPP
 