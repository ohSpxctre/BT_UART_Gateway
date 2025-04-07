/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
#include <iostream>

#define PREPARE_BUFF_MAX_LEN 1024

// Define the static instance pointer
BLE_Server* BLE_Server::Server_instance = nullptr;


 BLE_Server::BLE_Server(
    esp_ble_adv_params_t adv_params,
    esp_ble_adv_data_t adv_data,
    esp_ble_adv_data_t scan_rsp_data
 )  :
    _adv_params(adv_params),
    _adv_data(adv_data),
    _scan_rsp_data(scan_rsp_data)
 {
    // Initialize the static instance pointer
    if (Server_instance != nullptr) {
        ESP_LOGE(TAG_SERVER, "BLE_Server instance already exists!");
        return;
    }
    Server_instance = this;

    _adv_data.service_uuid_len = _gatts_profile_inst.service_id.id.uuid.len;
    _adv_data.p_service_uuid = (uint8_t *)_gatts_profile_inst.service_id.id.uuid.uuid.uuid128;

    //hie isch öppis komisch gloub
    _adv_data.service_data_len = sizeof(_adv_data_buffer);
    _adv_data.p_service_data = _adv_data_buffer;

    //configuring scan response data
    //_scan_rsp_data.service_uuid_len = _gatts_profile_inst.service_id.id.uuid.len;
    //_scan_rsp_data.p_service_uuid = (uint8_t *)_gatts_profile_inst.service_id.id.uuid.uuid.uuid128;

   //print_adv_data(_adv_data.p_service_uuid, _adv_data.service_data_len);
   //print_adv_data(_scan_rsp_data.p_service_uuid, _scan_rsp_data.service_uuid_len);

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
    Server_instance = nullptr;
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    nvs_flash_deinit();
    ESP_LOGI(TAG_SERVER, "BLE Server Deinitialized");
 }

 void BLE_Server::connSetup() {

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(PROFILE_APP_ID);
    esp_ble_gatt_set_local_mtu(_gatts_profile_inst.local_mtu);
    ESP_LOGI(TAG_SERVER, "BLE Server initialized and callbacks registered");
    }

 void BLE_Server::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (Server_instance) {
        Server_instance->handle_event_gap(event, param);
    }
 }

 void BLE_Server::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (Server_instance) {
        Server_instance->handle_event_gatts(event, gatts_if, param);
    }
 }


 void BLE_Server::handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGW(TAG_SERVER, "GAP event: %s", get_gap_event_name(event));
    switch (event)
    {
    //--------------------------------------------------------------------------------------------------------
    // GAP event for setting advertising data
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&_adv_params);
        //_adv_config_done &= (~adv_config_flag);
        //if (_adv_config_done == 0) {
        //    esp_ble_gap_start_advertising(&_adv_params);
        //    }
    break;    

    //--------------------------------------------------------------------------------------------------------
    // GAP event for setting scan response data
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&_adv_params);

        //_adv_config_done &= (~scan_rsp_config_flag);
        //if (_adv_config_done == 0) {
        //    esp_ble_gap_start_advertising(&_adv_params);
        //}
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
        //ESP_LOGI(TAG_GAP, "Connection parameters updated: addr= %d, min_int= %d, max_int= %d, latency= %d, timeout= %d",
        //    param->update_conn_params.bda,
        //    param->update_conn_params.min_int,
        //    param->update_conn_params.max_int,
        //    param->update_conn_params.latency,
        //    param->update_conn_params.timeout);
        break;
    
    //--------------------------------------------------------------------------------------------------------
    // GAP default case
    //--------------------------------------------------------------------------------------------------------
    default:
        ESP_LOGE(TAG_GAP, "Unhandled GAP event: %s", get_gap_event_name(event));
        break;
    }
}


void BLE_Server::handle_event_gatts(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    
    esp_ble_conn_update_params_t conn_params = {0};
    esp_err_t ret = ESP_OK;
    
    uint16_t length = 0;
    const uint8_t *prf_char;

    esp_gatt_rsp_t  rsp = {};
    uint8_t response[ESP_GATT_MAX_ATTR_LEN] = {0};
    uint16_t offset = 0;

    uint16_t mtu_size =_gatts_profile_inst.local_mtu - 1;
    uint16_t mtu_send_len = 0;
    uint8_t notify_data[mtu_size-3] = {0};
    uint8_t indicate_data[mtu_size-3] = {"Hello Client"};

    //Loging event type
    ESP_LOGW(TAG_SERVER, "GATT event: %s", get_gatts_event_name(event));

    switch (event)
    {
    //--------------------------------------------------------------------------------------------------------
    // GATT Server reg event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_REG_EVT:

        _gatts_profile_inst.gatts_if = gatts_if;        
        //setting up the service id
        ESP_LOGI(TAG_GATTS, "Device Name: %s", DEVICE_NAME_SERVER);
        ret = esp_ble_gap_set_device_name(DEVICE_NAME_SERVER);

        if (ret){
            ESP_LOGE(TAG_GATTS, "set device name failed, error code = %x", ret);
        }
        
        //configuring advertising data
        ret = esp_ble_gap_config_adv_data(&_adv_data);
        // Check if the advertising data was set successfully
        if (ret) {
            ESP_LOGE(TAG_GATTS, "set adv data failed, error code = %x", ret);
        }
        _adv_config_done |= adv_config_flag;
        
        //configuring scan response data
       //ret = esp_ble_gap_config_adv_data(&_scan_rsp_data);
       //if (ret) {
       //    ESP_LOGE(TAG_GATTS, "set scan response data failed, error code = %x", ret);
       //}
       //_adv_config_done |= scan_rsp_config_flag;

        //creating the gatt service
        ret = esp_ble_gatts_create_service(gatts_if, &_gatts_profile_inst.service_id, HANDLE_NUM);
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
       //else
       //{   //when started successfully, add char to the service
       //    ESP_LOGI(TAG_GATTS, "Service started successfully");
       //    
       //}
    break;

   //--------------------------------------------------------------------------------------------------------
    // GATT Read event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG_GATTS, "Characteristic read, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        
        // Check if if read response is needed
        if (!param->read.need_rsp) {
            ESP_LOGI(TAG_GATTS, "No response needed for read request");
            return;
        }
        
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;

        // Handle descriptor read request
        if (param->read.handle == _gatts_profile_inst.descr_handle) {
            memcpy(rsp.attr_value.value, &_gatts_profile_inst.descr_value, sizeof(_gatts_profile_inst.descr_value));
            rsp.attr_value.len = sizeof(_gatts_profile_inst.descr_value);
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            return;
        }

        // Handle characteristic read request
        if (param->read.handle == _gatts_profile_inst.char_handle) {
            offset = param->read.offset;
            // Validate read offset
            if (param->read.is_long && offset > sizeof(_char_data_buffer)) {
                ESP_LOGW(TAG_GATTS, "Read offset (%d) out of range (0-%d)", offset, sizeof(_char_data_buffer));
                rsp.attr_value.len = 0;
                esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_INVALID_OFFSET, &rsp);
                return;
            }

            // Determine response length based on MTU
            mtu_send_len = (ESP_GATT_MAX_ATTR_LEN - offset > mtu_size) ? mtu_size : (ESP_GATT_MAX_ATTR_LEN - offset);
            //copy the data to the response buffer               
            memcpy(rsp.attr_value.value, &_char_data_buffer[offset], mtu_send_len);
            // Set the length of the response
            rsp.attr_value.len = mtu_send_len;
            // send response and check for errors
            ret = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG_GATTS, "Send response failed, error: %s", esp_err_to_name(ret));
            }
            return;
        }

        //rsp.attr_value.len = sizeof(response);
        //memcpy(rsp.attr_value.value, response, rsp.attr_value.len);
        //esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
    break;


    //--------------------------------------------------------------------------------------------------------
    // GATT Write event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(TAG_GATTS, "Characteristic write, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep) {
            ESP_LOGI(TAG_GATTS, "value len %d, value ", param->write.len);
            ESP_LOG_BUFFER_HEX(TAG_GATTS, param->write.value, param->write.len);
            if (_gatts_profile_inst.descr_handle == param->write.handle && param->write.len == 2) {
                _gatts_profile_inst.descr_value = param->write.value[1]<<8 | param->write.value[0];
                ESP_LOGI(TAG_GATTS, "Descriptor value: %d", _gatts_profile_inst.descr_value);
                switch (_gatts_profile_inst.descr_value) {
                    case 0x0001:
                        if (_gatts_profile_inst.property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                            ESP_LOGI(TAG_GATTS, "Notification enabled should not happen!");
                            //the size of notify_data[] need less than MTU indicate_data
                            esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, _gatts_profile_inst.char_handle, sizeof(indicate_data), indicate_data, false);
                        }
                        break;
                    case 0x0002:
                        if (_gatts_profile_inst.property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                            ESP_LOGI(TAG_GATTS, "Indication enabled");
                            
                             //the size of indicate_data[] need less than MTU size
                            esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, _gatts_profile_inst.char_handle, sizeof(indicate_data), indicate_data, true);
                        }
                        break;
                    case 0x0000:
                        ESP_LOGI(TAG_GATTS, "Notification/Indication disabled");
                        break;
                    default:
                        ESP_LOGI(TAG_GATTS, "Unknown descriptor value: %d", _gatts_profile_inst.descr_value);
                        ESP_LOG_BUFFER_HEX(TAG_GATTS, param->write.value, param->write.len);
                        break;
                }
            }
            //call the user function to handle the write event
            handle_write_event(gatts_if, &_prepare_write_env, param);
        }
    break;


    //--------------------------------------------------------------------------------------------------------
    // GATT Execute Write event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGE(TAG_GATTS, "Execute write event, conn_id: %d", param->exec_write.conn_id);
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        // Call the user function to handle the execute write event
        handle_exec_write_event(&_prepare_write_env, param);
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT MTU exchange event
    //--------------------------------------------------------------------------------------------------------
    // This event is triggered when the MTU size is exchanged between the client and server.
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG_GATTS, "MTU exchange, MTU %d", param->mtu.mtu);
        _gatts_profile_inst.local_mtu = param->mtu.mtu;
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server start service event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_START_EVT:
        if (param->start.status != ESP_GATT_OK) {    
            ESP_LOGI(TAG_GATTS, "Service start failed: %s", esp_err_to_name(param->start.status));
        } else {
            ESP_LOGI(TAG_GATTS, "Service started successfully");     
            ESP_LOGI(TAG_GATTS, "Service handle: %d", param->start.service_handle);
            ESP_LOGI(TAG_GATTS, "Service UUID: %s", _gatts_profile_inst.service_id.id.uuid.uuid.uuid128);
            ESP_LOGI(TAG_GATTS, "Characteristic UUID: %s", _gatts_profile_inst.char_uuid.uuid.uuid128);

            ret = esp_ble_gatts_add_char(_gatts_profile_inst.service_handle,
                                            &_gatts_profile_inst.char_uuid,
                                            _gatts_profile_inst.perm,
                                            _gatts_profile_inst.property,
                                            &_char_value,
                                            NULL);
        
            if (ret == ESP_OK) {
                ESP_LOGI(TAG_GATTS, "Char add okey, result: %d", ret);
            }
            else {
                ESP_LOGE(TAG_GATTS, "add char failed, error code = %x", ret);
            }
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
        else{
            ESP_LOGI(TAG_GATTS, "Char handle: %d", param->add_char.attr_handle);
            ESP_LOGI(TAG_GATTS, "Char value length: %d", length);
            ESP_LOGI(TAG_GATTS, "Char value: ");
            ESP_LOG_BUFFER_HEX(TAG_GATTS, prf_char, length);
        }

        ret = esp_ble_gatts_add_char_descr(_gatts_profile_inst.service_handle,
                                            &_gatts_profile_inst.descr_uuid,
                                            _gatts_profile_inst.perm,
                                            &_descr_value,
                                            NULL);

        if (ret) {
            ESP_LOGE(TAG_GATTS, "Add descriptor failed, error code = %x", ret);
        }
        else {
            ESP_LOGI(TAG_GATTS, "Descriptor added successfully, handle: %d", param->add_char.attr_handle);
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
        //ESP_LOGI(TAG_GATTS, "Connected, conn_id %u, remote "ESP_BD_ADDR_STR"", param->connect.conn_id, ESP_BD_ADDR_HEX(param->connect.remote_bda));

        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.min_int = 0x190;    // min_int = 0x10*1.25ms = 20ms
        conn_params.max_int = 0x320;    // max_int = 0x20*1.25ms = 40ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms

        //start sent the update connection parameters to the peer device.

        //ret = esp_ble_gap_update_conn_params(&conn_params);
        //if (ret) {
        //    ESP_LOGE(TAG_GATTS, "Update connection params failed, error code = %x", ret);
        //}
        ret = esp_ble_gap_stop_advertising();
        if (ret) {
            ESP_LOGE(TAG_GATTS, "Stop advertising failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Disconnect event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_DISCONNECT_EVT:
        //ESP_LOGI(TAG_GATTS, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x", ESP_BD_ADDR_HEX(param->disconnect.remote_bda), param->disconnect.reason);
        _is_connected = false;
        ret = esp_ble_gap_start_advertising(&_adv_params);
        if (ret) {
            ESP_LOGE(TAG_GATTS, "Start advertising failed");
        }
        _is_advertising = true;
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT confirmation event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG_GATTS, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
        ESP_LOG_BUFFER_HEX(TAG_GATTS, param->conf.value, param->conf.len);
        //if (param->conf.status != ESP_GATT_OK){
        //    ESP_LOG_BUFFER_HEX(TAG_GATTS, param->conf.value, param->conf.len);
        //}
    break;

    //--------------------------------------------------------------------------------------------------------
    // default case
    //--------------------------------------------------------------------------------------------------------
    default:
        ESP_LOGE(TAG_GATTS, "Unhandled GATTS event: %s", get_gatts_event_name(event));
        break;
    }
}

void BLE_Server::handle_write_event (esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param) {

    esp_gatt_status_t status = ESP_GATT_OK;
    esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
    esp_err_t ret = ESP_OK;


    if (param->write.need_rsp) {
        if (param->write.is_prep) {
            // Check if the offset is valid
            if(param->write.offset > PREPARE_BUFF_MAX_LEN) {
                status = ESP_GATT_INVALID_OFFSET;
            }
            // Check if the length is valid
            else  if ((param->write.offset + param->write.len) > PREPARE_BUFF_MAX_LEN) {
                status = ESP_GATT_INVALID_ATTR_LEN;
            }
            // Check if the buffer is not NULL and then allocate memory for it
            if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL){
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUFF_MAX_LEN*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(TAG_GATTS, "Gatt_server prep no mem");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }

            if (gatt_rsp) {
                gatt_rsp->attr_value.len = param->write.len;
                gatt_rsp->attr_value.handle = param->write.handle;
                gatt_rsp->attr_value.offset = param->write.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
                memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);

                ret = esp_ble_gatts_send_response (gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
                if (ret != ESP_OK){
                    ESP_LOGE(TAG_GATTS, "Send response error\n");
                }
                free(gatt_rsp);
                gatt_rsp = NULL;
            }
            else {
                ESP_LOGE(TAG_GATTS, "malloc failed, no resource to send response error\n");
                status = ESP_GATT_NO_RESOURCES;
            }

            if (status != ESP_GATT_OK){
                return;
            }
            
            // Prepare write request, store the data in the buffer
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                param->write.value,
                param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        }
        else {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void BLE_Server::handle_exec_write_event (prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param) {
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        ESP_LOG_BUFFER_HEX(TAG_GATTS, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(TAG_GATTS,"Prepare write cancel");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

 void BLE_Server::send(const std::string &data) {

        _char_value.attr_value = (uint8_t*)(data.c_str());
   
        //uint8_t chunk_size = _gatts_profile_inst.local_mtu - 3;  // Maximum payload per packet (due to MTU size)
        //uint8_t string_len = data.length();  // Length of the string
        //uint8_t num_chunks = (string_len + chunk_size - 1) / chunk_size;  // Number of chunks required

        if (_is_connected) {
            
            esp_ble_gatts_send_indicate(_gatts_profile_inst.gatts_if, _gatts_profile_inst.conn_id, _gatts_profile_inst.char_handle, _char_value.attr_len, _char_value.attr_value, true);
            // Send the string in chunks
            //for (uint8_t i = 0; i < num_chunks; i++) {
            //    uint8_t offset = i * chunk_size;
            //    uint8_t len = (i == num_chunks - 1) ? (string_len - offset) : chunk_size;
        //
            //    uint8_t chunk_data[chunk_size];
            //    memcpy(chunk_data, &data[offset], len);
        //
            //    // Send an indication with the chunk data
            //    esp_ble_gatts_send_indicate(_gatts_profile_inst.gatts_if, _gatts_profile_inst.conn_id, _gatts_profile_inst.char_handle, len, chunk_data, false);
            //    ESP_LOGI(TAG_GATTS, "Sending chunk %d of %d: %.*s", i + 1, num_chunks, (int)len, chunk_data);
            //}
          
        } else {
           // ESP_LOGI(TAG, "Cannot send data, not connected to a client");
        }
 }
 
 std::string BLE_Server::receive() {
     std::cout << "BLE Server Receiving Data" << std::endl;
     return "Received Data from BLE Client";
 }


void BLE_Server::print_adv_data(const uint8_t *adv_data, uint8_t adv_data_len) {
    char adv_str[3 * adv_data_len + 1]; // Each byte needs 2 chars + space, +1 for null terminator
    char *ptr = adv_str;
    
    for (int i = 0; i < adv_data_len; i++) {
        ptr += sprintf(ptr, "%02X ", adv_data[i]);
    }
    
    ESP_LOGI("BLE", "Advertisement Data: %s", adv_str);
}

const char* BLE_Server::get_gatts_event_name(esp_gatts_cb_event_t event) {
    switch (event) {
        case ESP_GATTS_REG_EVT: return "ESP_GATTS_REG_EVT";
        case ESP_GATTS_READ_EVT: return "ESP_GATTS_READ_EVT";
        case ESP_GATTS_WRITE_EVT: return "ESP_GATTS_WRITE_EVT";
        case ESP_GATTS_EXEC_WRITE_EVT: return "ESP_GATTS_EXEC_WRITE_EVT";
        case ESP_GATTS_MTU_EVT: return "ESP_GATTS_MTU_EVT";
        case ESP_GATTS_CONF_EVT: return "ESP_GATTS_CONF_EVT";
        case ESP_GATTS_UNREG_EVT: return "ESP_GATTS_UNREG_EVT";
        case ESP_GATTS_CREATE_EVT: return "ESP_GATTS_CREATE_EVT";
        case ESP_GATTS_ADD_INCL_SRVC_EVT: return "ESP_GATTS_ADD_INCL_SRVC_EVT";
        case ESP_GATTS_ADD_CHAR_EVT: return "ESP_GATTS_ADD_CHAR_EVT";
        case ESP_GATTS_ADD_CHAR_DESCR_EVT: return "ESP_GATTS_ADD_CHAR_DESCR_EVT";
        case ESP_GATTS_DELETE_EVT: return "ESP_GATTS_DELETE_EVT";
        case ESP_GATTS_START_EVT: return "ESP_GATTS_START_EVT";
        case ESP_GATTS_STOP_EVT: return "ESP_GATTS_STOP_EVT";
        case ESP_GATTS_CONNECT_EVT: return "ESP_GATTS_CONNECT_EVT";
        case ESP_GATTS_DISCONNECT_EVT: return "ESP_GATTS_DISCONNECT_EVT";
        case ESP_GATTS_CANCEL_OPEN_EVT: return "ESP_GATTS_CANCEL_OPEN_EVT";
        case ESP_GATTS_CLOSE_EVT: return "ESP_GATTS_CLOSE_EVT";
        case ESP_GATTS_LISTEN_EVT: return "ESP_GATTS_LISTEN_EVT";
        case ESP_GATTS_CONGEST_EVT: return "ESP_GATTS_CONGEST_EVT";
        case ESP_GATTS_RESPONSE_EVT: return "ESP_GATTS_RESPONSE_EVT";
        case ESP_GATTS_CREAT_ATTR_TAB_EVT: return "ESP_GATTS_CREAT_ATTR_TAB_EVT";
        case ESP_GATTS_SET_ATTR_VAL_EVT: return "ESP_GATTS_SET_ATTR_VAL_EVT";
        case ESP_GATTS_SEND_SERVICE_CHANGE_EVT: return "ESP_GATTS_SEND_SERVICE_CHANGE_EVT";
        default: return "UNKNOWN_GATTS_EVENT";
    }
}
 
 const char* BLE_Server::get_gap_event_name(esp_gap_ble_cb_event_t event) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: return "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT";
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT: return "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT";
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: return "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT";
        case ESP_GAP_BLE_SCAN_RESULT_EVT: return "ESP_GAP_BLE_SCAN_RESULT_EVT";
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT: return "ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT";
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT: return "ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT";
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT: return "ESP_GAP_BLE_ADV_START_COMPLETE_EVT";
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: return "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT";
        case ESP_GAP_BLE_AUTH_CMPL_EVT: return "ESP_GAP_BLE_AUTH_CMPL_EVT";
        case ESP_GAP_BLE_KEY_EVT: return "ESP_GAP_BLE_KEY_EVT";
        case ESP_GAP_BLE_SEC_REQ_EVT: return "ESP_GAP_BLE_SEC_REQ_EVT";
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: return "ESP_GAP_BLE_PASSKEY_NOTIF_EVT";
        case ESP_GAP_BLE_PASSKEY_REQ_EVT: return "ESP_GAP_BLE_PASSKEY_REQ_EVT";
        case ESP_GAP_BLE_OOB_REQ_EVT: return "ESP_GAP_BLE_OOB_REQ_EVT";
        case ESP_GAP_BLE_LOCAL_IR_EVT: return "ESP_GAP_BLE_LOCAL_IR_EVT";
        case ESP_GAP_BLE_LOCAL_ER_EVT: return "ESP_GAP_BLE_LOCAL_ER_EVT";
        case ESP_GAP_BLE_NC_REQ_EVT: return "ESP_GAP_BLE_NC_REQ_EVT";
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: return "ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT";
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: return "ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT";
        case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT: return "ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT";
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: return "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT";
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT: return "ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT";
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: return "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT";
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: return "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT";
        case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT: return "ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT";
        case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT: return "ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT";
        case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: return "ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT";
        case ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT: return "ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT";
        case ESP_GAP_BLE_EVT_MAX: return "ESP_GAP_BLE_EVT_MAX";
        default: return "UNKNOWN_GAP_EVENT";
    }
 }