/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
 #include <iostream>

 constexpr const char* TAG = "BLE_Server";

// Define the static instance pointer
BLE_Server* BLE_Server::instance = nullptr;

   //uint8_t char_str[] = {0x11,0x22,0x33};
  
  //esp_attr_value_t gatts_char_value = {
  //  .attr_max_len = sizeof(char_str),
  //  .attr_len = sizeof(char_str),
  //  .attr_value = char_str
  //};

 BLE_Server::BLE_Server(
    esp_ble_adv_params_t adv_params,
    esp_ble_adv_data_t adv_data,
    esp_ble_adv_data_t scan_rsp_data,

    esp_gatt_srvc_id_t service_id,
    uint16_t service_handle
 ) : 
    _adv_params(adv_params),
    _adv_data(adv_data),
    _scan_rsp_data(scan_rsp_data),
    _service_id(service_id),
    _service_handle(service_handle)
 {
    // Initialize the static instance pointer
    if (instance != nullptr) {
        ESP_LOGE(TAG, "BLE_Server instance already exists!");
        return;
    }
    instance = this;
 }
 
 BLE_Server::~BLE_Server() {
    // Clean up resources if needed
    instance = nullptr;
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    nvs_flash_deinit();
 }

 void BLE_Server::connSetup() {
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

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(PROFILE_APP_ID);
}


 void BLE_Server::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (instance) {
        instance->handle_event_gap(event, param);
    }
 }

 void BLE_Server::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (instance) {
        instance->handle_event_gatts(event, gatts_if, param);
    }
}


 void BLE_Server::handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");
        esp_ble_gap_start_advertising(&_adv_params);
        break;

    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT");
        esp_ble_gap_start_advertising(&_adv_params);
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            _is_advertising = false;
            ESP_LOGE(TAG, "Advertising start failed: %s", esp_err_to_name(param->adv_start_cmpl.status));
        } else {
            _is_advertising = true;
            ESP_LOGI(TAG, "Advertising started successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            _is_advertising = true;
            ESP_LOGE(TAG, "Advertising stop failed: %s", esp_err_to_name(param->adv_stop_cmpl.status));
        } else {
            _is_advertising = false;
            ESP_LOGI(TAG, "Advertising stopped successfully");
        }
        break;

    break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "Connection parameters updated: addr= %d, min_int= %d, max_int= %d, latency= %d, timeout= %d",
            param->update_conn_params.bda,
            param->update_conn_params.min_int,
            param->update_conn_params.max_int,
            param->update_conn_params.latency,
            param->update_conn_params.timeout);
        break;
    break;

    default:
        ESP_LOGI(TAG, "Unhandled GAP event: %d", event);
        break;
    }
}


void BLE_Server::handle_event_gatts(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT Service registered, setting device name");
        esp_ble_gap_set_device_name(device_name);
        
        //configuring advertising data
        _adv_data.service_uuid_len = sizeof(_service_id.id.uuid);
        _adv_data.p_service_uuid = (uint8_t *)&_service_id.id.uuid;
        esp_ble_gap_config_adv_data(&_adv_data);

        //configuring scan response data
        _scan_rsp_data.service_uuid_len = sizeof(_service_id.id.uuid);
        _scan_rsp_data.p_service_uuid = (uint8_t *)&_service_id.id.uuid;
        esp_ble_gap_config_adv_data(&_scan_rsp_data);
        //creating the gatt service
        esp_ble_gatts_create_service(gatts_if, &_service_id, PROFILE_NUM);
        ESP_LOGI(TAG, "GATT Service created");

    break;

    case ESP_GATTS_CREATE_EVT:
        if (param->create.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Service creation failed: %s", esp_err_to_name(param->create.status));
        }
        else {
            ESP_LOGI(TAG, "Service created successfully, handle: %d", param->create.service_handle);
            _is_connected = true;
            _is_advertising = false;
            _service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(_service_handle);

            _char_property = (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE);
            _char_value.attr_len = sizeof(_char_value_buffer);
            _char_value.attr_value = _char_value_buffer;

            esp_ble_gatts_add_char(
                _service_handle,
                &_service_id.id.uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                _char_property,
                &_char_value,
                NULL
            );
        }

        ESP_LOGI(TAG, "GATT Characteristic created (not implemented jet)");
    break;

    case ESP_GATTS_START_EVT:
        if (param->start.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Service start failed: %s", esp_err_to_name(param->start.status));
        } else {
            ESP_LOGI(TAG, "Service started successfully");
        }
    break;

    case ESP_GATTS_CONNECT_EVT:
        esp_ble_gatts_cb_param_t *connect_param = (esp_ble_gatts_cb_param_t *)param;

        ESP_LOGI(TAG, "Device connected, conn_id: %d", connect_param->connect.conn_id);

        // Store connection ID (for sending notifications later)
        _gatts_profile_inst = connect_param->connect.conn_id;

        memcpy(conn_params.bda, connect_param->connect.remote_bda, sizeof(esp_bd_addr_t));
        esp_ble_gap_update_conn_params(&conn_params);

    default:
        ESP_LOGI(TAG, "Unhandled GATTS event: %d", event);
    break;
    }
}


 void BLE_Server::send(const std::string &data) {
     std::cout << "BLE Server Sending: " << data << std::endl;
 }
 
 std::string BLE_Server::receive() {
     std::cout << "BLE Server Receiving Data" << std::endl;
     return "Received Data from BLE Client";
 }


 