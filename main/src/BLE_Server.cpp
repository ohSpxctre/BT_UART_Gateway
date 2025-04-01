/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
 #include <iostream>

 constexpr const char* TAG = "BLE_Server";

// Define the static instance pointer
BLE_Server* BLE_Server::instance = nullptr;


 BLE_Server::BLE_Server(
    esp_ble_adv_params_t adv_params,
    esp_ble_adv_data_t adv_data,
    esp_ble_adv_data_t scan_rsp_data
 ) : 
    _adv_params(adv_params),
    _adv_data(adv_data),
    _scan_rsp_data(scan_rsp_data)
 {
    // Initialize the static instance pointer
    if (instance != nullptr) {
        ESP_LOGE(TAG, "BLE_Server instance already exists!");
        return;
    }
    instance = this;

    _adv_data.service_uuid_len = ESP_UUID_LEN_128;
    _adv_data.p_service_uuid = (uint8_t *)&_gatts_profile_inst.service_id.id.uuid;

    _adv_data.service_data_len = sizeof(service_data_buffer);
    _adv_data.p_service_data = service_data_buffer;

    //configuring scan response data
    _scan_rsp_data.service_uuid_len = ESP_UUID_LEN_128;
    _scan_rsp_data.p_service_uuid = (uint8_t *)&_gatts_profile_inst.service_id.id.uuid;
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

    memcpy(_gatts_profile_inst.char_value_buffer, CHAR_VALUE_DEFAULT, sizeof(CHAR_VALUE_DEFAULT));
    _gatts_profile_inst.char_value.attr_len = sizeof(_gatts_profile_inst.char_value_buffer);
    _gatts_profile_inst.char_value.attr_value = _gatts_profile_inst.char_value_buffer;

    ESP_LOGW(TAG, "BLE Server initialized with default values");
    ESP_LOGI(TAG, "BLE Server char len: %D", _gatts_profile_inst.char_value.attr_len);

        
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

    esp_err_t start_adv_ret = ESP_OK;

    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        start_adv_ret = esp_ble_gap_start_advertising(&_adv_params);
        if (start_adv_ret) {
            ESP_LOGE(TAG, "Failed to start advertising, error code = %x", start_adv_ret);
        } else {
            ESP_LOGI(TAG, "Advertising started successfully");
        }
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

    default:
        ESP_LOGI(TAG, "Unhandled GAP event: %d", event);
        break;
    }
}


void BLE_Server::handle_event_gatts(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    
    esp_ble_gatts_cb_param_t *connect_param = (esp_ble_gatts_cb_param_t *)param;
    esp_err_t set_adv_data_ret = ESP_OK;
    esp_err_t set_dev_name_ret = ESP_OK;
    esp_err_t add_char_ret = ESP_OK;
    esp_err_t add_sevc_ret = ESP_OK;
    esp_gatt_rsp_t  gatt_rsp = {};
    const char *response = "Hello response from BLE Server"; 
    char uuid_str[ESP_UUID_LEN_128 * 2 + 1] = {0}; // Platz für 32 Hex-Zeichen + Nullterminator


    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        _gatts_profile_inst.gatts_if = gatts_if;
        ESP_LOGI(TAG, "GATT Server registered, gatts_if: %d", gatts_if);
        set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        
        //configuring advertising data
        set_adv_data_ret = esp_ble_gap_config_adv_data(&_adv_data);
        if (set_adv_data_ret) {
            ESP_LOGE(TAG, "set adv data failed, error code = %x", set_adv_data_ret);
        } else {
            ESP_LOGI(TAG, "Advertising data set successfully");
        }
        
        esp_ble_gap_config_adv_data(&_scan_rsp_data);
        
        //creating the gatt service
        add_sevc_ret = esp_ble_gatts_create_service(gatts_if, &_gatts_profile_inst.service_id, PROFILE_NUM);
        
        if(add_sevc_ret) {
            ESP_LOGE(TAG, "create service failed, error code = %x", add_sevc_ret);
        } else {
            ESP_LOGI(TAG, "Service creation started successfully");
        }

    break;

    case ESP_GATTS_CREATE_EVT:
        if (param->create.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Service creation failed: %s", esp_err_to_name(param->create.status));
        }
        else {
            _gatts_profile_inst.service_handle = param->create.service_handle;
            ESP_LOGI(TAG, "Service created successfully, handle: %d", _gatts_profile_inst.service_handle);
            
            esp_ble_gatts_start_service(_gatts_profile_inst.service_handle);
                       
        }
    break;

    case ESP_GATTS_START_EVT:
        if (param->start.status != ESP_GATT_OK) {
            
            ESP_LOGI(TAG, "Service start failed: %s", esp_err_to_name(param->start.status));
        } else {
            ESP_LOGI(TAG, "Service started successfully");
            for (int i = 0; i < ESP_UUID_LEN_128; i++) {
                sprintf(&uuid_str[i * 2], "%02x", _gatts_profile_inst.char_uuid.uuid.uuid128[i]);
            }
            for (int i = 0; i < ESP_UUID_LEN_128; i++) {
                sprintf(&uuid_str[i * 2], "%02x", _gatts_profile_inst.service_id.id.uuid.uuid.uuid128[i]);
            }
            ESP_LOGI(TAG, "Characteristic handle: %d", _gatts_profile_inst.char_handle);
            ESP_LOGI(TAG, "Characteristic UUID: %s", uuid_str);
            ESP_LOGI(TAG, "Char uuid len: %d", _gatts_profile_inst.char_uuid.len);
            ESP_LOGI(TAG, "Service handle: %d", _gatts_profile_inst.service_handle);
            for (int i = 0; i < ESP_UUID_LEN_128; i++) {
                sprintf(&uuid_str[i * 2], "%02x", _gatts_profile_inst.service_id.id.uuid.uuid.uuid128[i]);
            }
            ESP_LOGI(TAG, "Service UUID: %s\n", uuid_str);
            ESP_LOGI(TAG, "permitions: %d, property: %d", _gatts_profile_inst.perm, _gatts_profile_inst.property);
            ESP_LOGI(TAG, "char value len: %d", _gatts_profile_inst.char_value.attr_len);             
        }

        //esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &_gatts_profile_inst.char_value.attr_len, _gatts_profile_inst.char_value.attr_value);
        ESP_LOGI(TAG, "Characteristic value: %s", (char *)_gatts_profile_inst.char_value.attr_value);
        ESP_LOGI(TAG, "Characteristic value length: %d", _gatts_profile_inst.char_value.attr_len);
        //add_char_ret = esp_ble_gatts_add_char(
        //    _gatts_profile_inst.service_handle,
        //    &_gatts_profile_inst.char_uuid,
        //    _gatts_profile_inst.perm,
        //    _gatts_profile_inst.property,
        //    &_gatts_profile_inst.char_value,
        //    NULL
        //); // No descriptor for now
        if (add_char_ret) {
            ESP_LOGE(TAG, "Add char failed, error code = %x", add_char_ret);
        } else {
            ESP_LOGI(TAG, "Adding Characteristic started successfully");
        }
        break;
       

    break;

    case ESP_GATTS_ADD_CHAR_EVT:
    switch (param->add_char.status) {
        case ESP_GATT_OK:
            _gatts_profile_inst.char_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "✅ Characteristic added successfully.");
            break;
    
        case ESP_GATT_INVALID_HANDLE:
            ESP_LOGE(TAG, "❌ Error: Invalid handle.");
            break;
    
        case ESP_GATT_INVALID_PDU:
            ESP_LOGE(TAG, "❌ Error: Invalid PDU.");
            break;
    
        case ESP_GATT_INSUF_AUTHENTICATION:
            ESP_LOGE(TAG, "❌ Error: Insufficient authentication.");
            break;
    
        case ESP_GATT_INSUF_AUTHORIZATION:
            ESP_LOGE(TAG, "❌ Error: Insufficient authorization.");
            break;
    
        case ESP_GATT_INSUF_ENCRYPTION:
            ESP_LOGE(TAG, "❌ Error: Insufficient encryption.");
            break;
    
        case ESP_GATT_INVALID_OFFSET:
            ESP_LOGE(TAG, "❌ Error: Invalid offset.");
            break;
    
        case ESP_GATT_NO_RESOURCES:
            ESP_LOGE(TAG, "❌ Error: No resources available.");
            break;
    
        case ESP_GATT_INTERNAL_ERROR:
            ESP_LOGE(TAG, "❌ Error: Internal error.");
            break;
    
        case ESP_GATT_ERROR:
            ESP_LOGE(TAG, "❌ Error: Generic GATT error.");
            break;
    
        default:
            ESP_LOGE(TAG, "❌ Error: Unknown status code: %d", param->add_char.status);
            break;
    }
    
    break;


    case ESP_GATTS_CONNECT_EVT:
       
        ESP_LOGI(TAG, "Device connected, conn_id: %d", connect_param->connect.conn_id);

        // Store connection ID (for sending notifications later)
        _gatts_profile_inst.conn_id = connect_param->connect.conn_id;
        _gatts_profile_inst.gatts_if = gatts_if;
        _is_connected = true;
        esp_ble_gap_stop_advertising();
        _is_advertising = false;
        ESP_LOGI(TAG, "Advertising stopped after connection");
    break;

 

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Device disconnected, conn_id: %d", param->disconnect.conn_id);
        _is_connected = false;
        esp_ble_gap_start_advertising(&_adv_params);
        _is_advertising = true;
        ESP_LOGI(TAG, "Advertising restarted after disconnection");
    break;

    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(TAG, "Write event, conn_id: %d", param->write.conn_id);
        ESP_LOGI(TAG, "Data written: %s", (char *)param->write.value);
        break;

    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "Read event, conn_id: %d", param->read.conn_id);
        gatt_rsp.attr_value.handle = param->read.handle;
        gatt_rsp.attr_value.len = sizeof(response);
        memcpy(gatt_rsp.attr_value.value, response, gatt_rsp.attr_value.len);
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &gatt_rsp);

        break;
    
    case ESP_GATTS_EXEC_WRITE_EVT:
    case ESP_GATTS_MTU_EVT:
    case ESP_GATTS_CONF_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    case ESP_GATTS_DELETE_EVT:
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
        ESP_LOGI(TAG, "Unhandled but expected GATT event: %d", event);
        break;

    default:
        ESP_LOGI(TAG, "Unhandled GATTS event: %d", event);
        break;
    }
}


 void BLE_Server::send(const std::string &data) {
        if (_is_connected) {
            ESP_LOGI(TAG, "Sending data: %s", data.c_str());
            _gatts_profile_inst.char_value.attr_len = data.length();
            _gatts_profile_inst.char_value.attr_value = (uint8_t *)data.c_str();
            
            esp_ble_gatts_send_indicate(_gatts_profile_inst.gatts_if, _gatts_profile_inst.conn_id, _gatts_profile_inst.char_handle,
                                        data.length(), (uint8_t *)data.c_str(), false);
        } else {
           // ESP_LOGI(TAG, "Cannot send data, not connected to a client");
        }
 }
 
 std::string BLE_Server::receive() {
     std::cout << "BLE Server Receiving Data" << std::endl;
     return "Received Data from BLE Client";
 }


 