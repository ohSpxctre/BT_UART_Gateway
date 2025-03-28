/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
 #include <iostream>
    

 BLE_Server::BLE_Server() {
    // Initialize NVS (needed for BLE storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    std::copy(std::begin(SERVICE_UUID), std::end(SERVICE_UUID), std::begin(this->service_uuid128));
    
    // Initialize Bluetooth controller and enable BLE mode
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(PROFILE_APP_ID);

 }
 
 BLE_Server::~BLE_Server() {
     std::cout << "BLE Server Destructor" << std::endl;
 }

 void BLE_Server::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
        adv_config_done = true;
        break;

    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT");
        scan_rsp_config_done = true;
        break;
    default:
        ESP_LOGI(TAG, "Unhandled GAP event: %d", event);
        break;
    }

    if (adv_config_done && scan_rsp_config_done) {
        esp_ble_gap_start_advertising(&adv_params);
        ESP_LOGI(TAG, "Advertising started");
    }
 }

void BLE_Server::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT Service registered, setting device name");
        esp_ble_gap_set_device_name(device_name);

        //configuring advertising data
        esp_ble_gap_config_adv_data(&adv_data);
        //configuring scan response data
        esp_ble_gap_config_adv_data(&scan_rsp_data);
        //creating the gatt service
        esp_ble_gatts_create_service(gatts_if, (esp_gatt_srvc_id_t*)SERVICE_UUID, PROFILE_NUM);
        ESP_LOGI(TAG, "GATT Service created");

    break;

    case ESP_GATTS_CREATE_EVT:
        service_handle = param->create.service_handle;
/*
        esp_ble_gatts_add_char(
            service_handle,
            (esp_bt_uuid_t)CHAR_UUID,
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            ESP_GATT_CHAR_PROP_READ | ESP_GATT_CHAR_PROP_WRITE,
            &gatts_char_value,
            sizeof(char_str),
            &char_handle
        )
        char_handle = param->create.char_handle;
        descr_handle = param->create.descr_handle;*/
        ESP_LOGI(TAG, "GATT Characteristic created (not implemented jet)");
    break;

    default:
        ESP_LOGI(TAG, "Unhandled GATTS event: %d", event);
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


 