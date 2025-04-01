/**
 * @file BLE_Client.hpp
 * @brief BLE Client class inheriting from Bluetooth.
 */

 #ifndef BLE_CLIENT_HPP
 #define BLE_CLIENT_HPP
 
 #include "Bluetooth.hpp"

 constexpr esp_bt_uuid_t REMOTE_FILTER_SERVICE_UUID = SERVICE_UUID_DEFAULT;

 constexpr esp_bt_uuid_t REMOTE_FILTER_CHAR_UUID = CHAR_UUID_DEFAULT;


 constexpr esp_ble_scan_params_t BLE_SCAN_PARAMS_DEFAULT = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

 struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

 /**
  * @class BLE_Client
  * @brief BLE Client implementation.
  */
 class BLE_Client : public Bluetooth {
 private:

    static BLE_Client* Client_instance; // Static instance pointer

    gattc_profile_inst _gattc_profile_inst = {
        .gattc_cb = gattc_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,
        .app_id = PROFILE_APP_ID,
        .conn_id = 0,
        .service_start_handle = 0,
        .service_end_handle = 0,
        .char_handle = 0,
        .remote_bda = {0}
    };

    esp_ble_scan_params_t _scan_params;
    esp_bt_uuid_t _remote_service_uuid;
    esp_bt_uuid_t _remote_char_uuid;
 
    bool _is_connected = false;
    bool _is_scanning = false;
    bool _is_service_found = false;
    bool _is_char_found = false;
    bool _is_char_desc_found = false;

    static void gattc_event_handler(esp_gattc_cb_event_t, esp_gatt_if_t, esp_ble_gattc_cb_param_t *);
    static void gap_event_handler(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

    void handle_event_gattc(esp_gattc_cb_event_t, esp_gatt_if_t, esp_ble_gattc_cb_param_t *);
    void handle_event_gap(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

 public:
     BLE_Client(esp_ble_scan_params_t scan_params = BLE_SCAN_PARAMS_DEFAULT,
                esp_bt_uuid_t remote_service_uuid = REMOTE_FILTER_SERVICE_UUID,
                esp_bt_uuid_t remote_char_uuid = REMOTE_FILTER_CHAR_UUID
     );
     
     ~BLE_Client();
     
     void connSetup() override;
     void send(const std::string &data) override;
     std::string receive() override;
 };
 
 #endif // BLE_CLIENT_HPP