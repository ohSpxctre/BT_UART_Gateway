/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
 #include <iostream>

 constexpr const char* TAG = "BLE_Server";
 constexpr const char* TAG_GAP = "GAP";
 constexpr const char* TAG_GATTS = "GATT";

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

    _adv_data.service_uuid_len = _gatts_profile_inst.service_id.id.uuid.len;
    _adv_data.p_service_uuid = (uint8_t *)&_gatts_profile_inst.service_id.id.uuid.uuid.uuid128;

    _adv_data.service_data_len = sizeof(_char_value_buffer);
    _adv_data.p_service_data = (uint8_t *)_char_value_buffer;

    //configuring scan response data
    _scan_rsp_data.service_uuid_len = _gatts_profile_inst.service_id.id.uuid.len;
    _scan_rsp_data.p_service_uuid = (uint8_t *)&_gatts_profile_inst.service_id.id.uuid.uuid.uuid128;

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

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(PROFILE_APP_ID);
    esp_ble_gatt_set_local_mtu(ESP_GATT_MAX_ATTR_LEN);
    ESP_LOGI(TAG, "BLE Server initialized and callbacks registered");
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

    ESP_LOGW(TAG, "GAP event: %d", event);

    switch (event)
    {
    //--------------------------------------------------------------------------------------------------------
    // GAP event for setting advertising data
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        _adv_config_done &= (~adv_config_flag);
        if (_adv_config_done == 0) {
            esp_ble_gap_start_advertising(&_adv_params);
            }
    break;    

    //--------------------------------------------------------------------------------------------------------
    // GAP event for setting scan response data
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        _adv_config_done &= (~scan_rsp_config_flag);
        if (_adv_config_done == 0) {
            esp_ble_gap_start_advertising(&_adv_params);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GAP event for starting advertising complete
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG_GAP, "Advertising start failed, status %d", param->adv_start_cmpl.status);
        } else {
            ESP_LOGI(TAG_GAP, "Advertising start successfully");
        }
        break;
    //--------------------------------------------------------------------------------------------------------
    // GAP event for stopping advertising complete
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            _is_advertising = true;
            ESP_LOGE(TAG_GAP, "Advertising stop failed: %s", esp_err_to_name(param->adv_stop_cmpl.status));
        } else {
            _is_advertising = false;
            ESP_LOGI(TAG_GAP, "Advertising stopped successfully");
        }
        break;

    break;
    
    //--------------------------------------------------------------------------------------------------------
    // GAP event for connection parameters update
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "Connection parameters updated: addr= %d, min_int= %d, max_int= %d, latency= %d, timeout= %d",
            param->update_conn_params.bda,
            param->update_conn_params.min_int,
            param->update_conn_params.max_int,
            param->update_conn_params.latency,
            param->update_conn_params.timeout);
        break;
        
    //--------------------------------------------------------------------------------------------------------
    // GAP default case
    //--------------------------------------------------------------------------------------------------------
    default:
        ESP_LOGI(TAG, "Unhandled GAP event: %d", event);
        break;
    }
}


void BLE_Server::handle_event_gatts(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    
    esp_ble_conn_update_params_t conn_params = {0};
    esp_err_t ret = ESP_OK;
    
    uint16_t length = 0;
    const uint8_t *prf_char;

    esp_gatt_rsp_t  rsp = {};

    //char uuid_str[ESP_UUID_LEN_128 * 2 + 1] = {0}; // Platz für 32 Hex-Zeichen + Nullterminator

    //Loging event type
    ESP_LOGW(TAG, "GATT event: %d", event);

    switch (event)
    {
    //--------------------------------------------------------------------------------------------------------
    // GATT Server reg event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_REG_EVT:
        _gatts_profile_inst.gatts_if = gatts_if;
        ESP_LOGI(TAG_GATTS, "GATT server register, status %d, app_id %d, gatts_if %d", param->reg.status, param->reg.app_id, gatts_if);
        
        //setting up the service id
        ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (ret){
            ESP_LOGE(TAG, "set device name failed, error code = %x", ret);
        }
        
        //configuring advertising data
        ret = esp_ble_gap_config_adv_data(&_adv_data);
        if (ret) {
            ESP_LOGE(TAG_GATTS, "set adv data failed, error code = %x", ret);
        }
        _adv_config_done |= adv_config_flag;
        
        //configuring scan response data
        ret = esp_ble_gap_config_adv_data(&_scan_rsp_data);
        if (ret) {
            ESP_LOGE(TAG_GATTS, "set scan response data failed, error code = %x", ret);
        }
        _adv_config_done |= scan_rsp_config_flag;

        //creating the gatt service
        ret = esp_ble_gatts_create_service(gatts_if, &_gatts_profile_inst.service_id, PROFILE_NUM);
        if(ret) {
            ESP_LOGE(TAG_GATTS, "create service failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server create service event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG_GATTS, "Service create, status %d, service_handle %d", param->create.status, param->create.service_handle);
        
        //saving the service handle
        _gatts_profile_inst.service_handle = param->create.service_handle;
        //starting the service
        ret = esp_ble_gatts_start_service(_gatts_profile_inst.service_handle);
        if (ret)
        {
            ESP_LOGE(TAG_GATTS, "start service failed, error code = %x", ret);
        }
        else
        {   //when started successfully, add char to the service
            ESP_LOGI(TAG_GATTS, "Service started successfully");
            ret = esp_ble_gatts_add_char(
                _gatts_profile_inst.service_handle,
                &_gatts_profile_inst.char_uuid,
                _gatts_profile_inst.perm,
                _gatts_profile_inst.property,
                &_char_value,
                NULL
            );
            if (ret) {
                ESP_LOGE(TAG_GATTS, "add char failed, error code = %x", ret);
            }
        }
    break;

   //--------------------------------------------------------------------------------------------------------
    // GATT Read event
    //--------------------------------------------------------------------------------------------------------
   // case ESP_GATTS_READ_EVT:
   //     ESP_LOGI(TAG_GATTS, "Characteristic read, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
   //     memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
   //     
   //     rsp.attr_value.handle = param->read.handle;
   //     rsp.attr_value.len = sizeof(response);
   //     memcpy(rsp.attr_value.value, response, rsp.attr_value.len);
   //     esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
   // break;


    //--------------------------------------------------------------------------------------------------------
    // GATT Write event
    //--------------------------------------------------------------------------------------------------------
    //case ESP_GATTS_WRITE_EVT:
    //    ESP_LOGE(TAG_GATTS, "Write event (not implemented), conn_id: %d", param->write.conn_id);
    //break;


    //--------------------------------------------------------------------------------------------------------
    // GATT Execute Write event
    //--------------------------------------------------------------------------------------------------------
    //case ESP_GATTS_EXEC_WRITE_EVT:
    //    ESP_LOGE(TAG_GATTS, "Execute write event (not implemented), conn_id: %d", param->exec_write.conn_id);
    //break;

    //--------------------------------------------------------------------------------------------------------
    // GATT MTU exchange event
    //--------------------------------------------------------------------------------------------------------
    // This event is triggered when the MTU size is exchanged between the client and server.
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG_GATTS, "MTU exchange, MTU %d", param->mtu.mtu);
        _is_connected = true;
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Unregister event (does nothing for now)
    //--------------------------------------------------------------------------------------------------------
    //case ESP_GATTS_UNREG_EVT:
    //break;


    //--------------------------------------------------------------------------------------------------------
    // GATT Server start service event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_START_EVT:
        if (param->start.status != ESP_GATT_OK) {
            
            ESP_LOGI(TAG, "Service start failed: %s", esp_err_to_name(param->start.status));
        } else {
            ESP_LOGI(TAG, "Service started successfully");
            //for (int i = 0; i < ESP_UUID_LEN_128; i++) {
            //    sprintf(&uuid_str[i * 2], "%02x", _gatts_profile_inst.char_uuid.uuid.uuid128[i]);
            //}
            //for (int i = 0; i < ESP_UUID_LEN_128; i++) {
            //    sprintf(&uuid_str[i * 2], "%02x", _gatts_profile_inst.service_id.id.uuid.uuid.uuid128[i]);
            //}
            //ESP_LOGI(TAG, "Characteristic handle: %d", _gatts_profile_inst.char_handle);
            //ESP_LOGI(TAG, "Characteristic UUID: %s", uuid_str);
            //ESP_LOGI(TAG, "Char uuid len: %d", _gatts_profile_inst.char_uuid.len);
            //ESP_LOGI(TAG, "Service handle: %d", _gatts_profile_inst.service_handle);
            //for (int i = 0; i < ESP_UUID_LEN_128; i++) {
            //    sprintf(&uuid_str[i * 2], "%02x", _gatts_profile_inst.service_id.id.uuid.uuid.uuid128[i]);
            //}
            //ESP_LOGI(TAG, "Service UUID: %s\n", uuid_str);
            //ESP_LOGI(TAG, "permitions: %d, property: %d", _gatts_profile_inst.perm, _gatts_profile_inst.property);
            //ESP_LOGI(TAG, "char value len: %d", _gatts_profile_inst.char_value.attr_len);             
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server add char event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG_GATTS, "Characteristic add, status %d, attr_handle %d, service_handle %d",
            param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        //reading the char handle
        _gatts_profile_inst.char_handle = param->add_char.attr_handle;
        
        ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (ret == ESP_FAIL){
            ESP_LOGE(TAG_GATTS, "ILLEGAL HANDLE");
        }

        ESP_LOGI(TAG_GATTS, "the gatts demo char length = %x", length);
        for(int i = 0; i < length; i++){
            ESP_LOGI(TAG_GATTS, "prf_char[%x] =%x",i,prf_char[i]);
        }


        ret = esp_ble_gatts_add_char_descr(
            _gatts_profile_inst.service_handle,
            &_gatts_profile_inst.descr_uuid,
            _gatts_profile_inst.perm,
            NULL,
            NULL
        );

        if (ret) {
            ESP_LOGE(TAG_GATTS, "Add descriptor failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server add char descriptor event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        _gatts_profile_inst.descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(TAG_GATTS, "Descriptor add, status %d, attr_handle %d, service_handle %d",
                param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
    break;


    //--------------------------------------------------------------------------------------------------------
    // GATT Connect event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CONNECT_EVT:
        _is_connected = true;
        _gatts_profile_inst.conn_id = param->connect.conn_id;
        ESP_LOGI(TAG_GATTS, "Connected, conn_id %u, remote "ESP_BD_ADDR_STR"", param->connect.conn_id, ESP_BD_ADDR_HEX(param->connect.remote_bda));

        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms

        //start sent the update connection parameters to the peer device.
        ret = esp_ble_gap_update_conn_params(&conn_params);
        if (ret) {
            ESP_LOGE(TAG_GATTS, "Update connection params failed, error code = %x", ret);
        }
        ret = esp_ble_gap_stop_advertising();
        if (ret) {
            ESP_LOGE(TAG_GATTS, "Stop advertising failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Disconnect event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG_GATTS, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x", ESP_BD_ADDR_HEX(param->disconnect.remote_bda), param->disconnect.reason);
        _is_connected = false;
        ret = esp_ble_gap_start_advertising(&_adv_params);
        if (ret) {
            ESP_LOGE(TAG_GATTS, "Start advertising failed, error code = %x", ret);
        }
        _is_advertising = true;
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT confirmation event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG_GATTS, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            ESP_LOG_BUFFER_HEX(TAG_GATTS, param->conf.value, param->conf.len);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // default case
    //--------------------------------------------------------------------------------------------------------
    default:
        ESP_LOGI(TAG, "Unhandled GATTS event: %d", event);
        break;
    }
}


 void BLE_Server::send(const std::string &data) {
        if (_is_connected) {
            ESP_LOGI(TAG, "Sending data: %s", data.c_str());
            _char_value.attr_len = data.length();
            memcpy(_char_value_buffer, data.c_str(), data.length());
            
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


 