/**
 * @file BLE_Client.cpp
 * @brief Implementation of BLE_Client class.
 */

 #include "BLE_Client.hpp"
 #include <iostream>
 
 BLE_Client::BLE_Client(
        esp_ble_scan_params_t scan_params,
        esp_bt_uuid_t remote_service_uuid,
        esp_bt_uuid_t remote_char_uuid
 ) :
        _scan_params(scan_params),
        _remote_service_uuid(remote_service_uuid),
        _remote_char_uuid(remote_char_uuid)
 {
        // Initialize the static instance pointer
        if (Client_instance != nullptr) {
            std::cerr << "BLE_Client instance already exists!" << std::endl;
            return;
        }
        Client_instance = this;
    
        // Initialize NVS (needed for BLE storage)
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        }
    
        // Initialize Bluetooth controller and enable BLE mode
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        esp_bt_controller_init(&bt_cfg);
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
        esp_bluedroid_init();
        esp_bluedroid_enable();
 }
 
 BLE_Client::~BLE_Client() {
        // Clean up resources if needed
        Client_instance = nullptr;
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        nvs_flash_deinit();
        ESP_LOGI(TAG, "BLE Client Deinitialized");
 }

 void BLE_Client::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (Client_instance) {
        Client_instance->handle_event_gap(event, param);
    }
 }

 void BLE_Client::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gattc_cb_param_t *param) {
    if (Client_instance) {
        Client_instance->handle_event_gattc(event, gatts_if, param);
    }
 }

 void BLE_Client::handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        // Handle various GAP events here
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            // Scan parameters set successfully
            break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            // Handle scan results
            break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            // Scan started successfully
            break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            // Scan stopped successfully
            break;
        default:
        ESP_LOGI(TAG_GAP, "Unhandled GAP event: %d", event);
        break;
    }
 }

 void handle_event_gatts(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    switch (event) {
            // Handle various GATT events here
            case ESP_GATTC_REG_EVT:
                // GATT client registered successfully
                break;
           
            default:
                ESP_LOGI(TAG_GATTS, "Unhandled GATTC event: %d", event);
                break;
        }
    }


 
 void BLE_Client:: connSetup() {
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gattc_register_callback(gattc_event_handler);
    esp_ble_gattc_app_register(PROFILE_APP_ID);
    esp_ble_gatt_set_local_mtu(ESP_GATT_MAX_ATTR_LEN);
    ESP_LOGI(TAG, "BLE Client initialized and callbacks registered");
 }
 
 void BLE_Client::send(const std::string &data) {
     std::cout << "BLE Client Sending: " << data << std::endl;
 }
 
 std::string BLE_Client::receive() {
     std::cout << "BLE Client Receiving Data" << std::endl;
     return "Received Data from BLE Server";
 }