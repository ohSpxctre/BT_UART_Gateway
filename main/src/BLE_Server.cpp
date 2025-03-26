/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
 #include <iostream>
    

 BLE_Server::BLE_Server() {
    // Initialize NVS (needed for BLE storage)
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    std::copy(std::begin(SERVICE_UUID128), std::end(SERVICE_UUID128), std::begin(this->adv_service_uuid128));

    // Initialize Bluetooth controller and enable BLE mode
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    this->gl_profile_tab[PROFILE_APP_ID].gatts_cb = gatts_event_handler;
    this->gl_profile_tab[PROFILE_APP_ID].gatts_if = ESP_GATT_IF_NONE; // Not get the gatt_if, so initial is ESP_GATT_IF_NONE

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);

    esp_ble_gatts_app_register(PROFILE_APP_ID);

 }
 
 BLE_Server::~BLE_Server() {
     std::cout << "BLE Server Destructor" << std::endl;
 }

void BLE_Server::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    std::cout << "event handler called";
 }

void BLE_Server::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        
        break;

    case ESP_GATTS_READ_EVT:
        break;
    
    case ESP_GATTS_WRITE_EVT:
        break;

    default:
        break;
    }
 }
 
 void BLE_Server::connSetup() {
     std::cout << "BLE Server Initialized" << std::endl;
 }
 
 void BLE_Server::send(const std::string &data) {
     std::cout << "BLE Server Sending: " << data << std::endl;
 }
 
 std::string BLE_Server::receive() {
     std::cout << "BLE Server Receiving Data" << std::endl;
     return "Received Data from BLE Client";
 }


 