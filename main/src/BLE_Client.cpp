/**
 * @file BLE_Client.cpp
 * @brief Implementation of BLE_Client class.
 */
#if 1
 #include "BLE_Client.hpp"
 #include <iostream>

 // Define the static variable outside the class
BLE_Client* BLE_Client::Client_instance = nullptr;
 
 BLE_Client::BLE_Client(
        esp_ble_scan_params_t scan_params,
        esp_bt_uuid_t remote_service_uuid,
        esp_bt_uuid_t remote_char_uuid,
        esp_bt_uuid_t remote_descr_uuid
 ) :
        _scan_params(scan_params),
        _remote_service_uuid(remote_service_uuid),
        _remote_char_uuid(remote_char_uuid),
        _remote_descr_uuid(remote_descr_uuid) 
 {
        // Initialize the static instance pointer
        if (Client_instance != nullptr) {
            ESP_LOGE(TAG_CLIENT, "BLE_Client instance already exists!");
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
        
        //Clean up resources if needed
        Client_instance = nullptr;
        esp_bluedroid_disable();
        esp_bluedroid_deinit();
        esp_bt_controller_disable();
        esp_bt_controller_deinit();
        nvs_flash_deinit();
        ESP_LOGI(TAG_CLIENT, "BLE Client Deinitialized");
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

 void BLE_Client:: connSetup() {
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gattc_register_callback(gattc_event_handler);
    esp_ble_gattc_app_register(PROFILE_APP_ID);
    esp_ble_gatt_set_local_mtu(_gattc_profile_inst.local_mtu);
    ESP_LOGI(TAG_CLIENT, "BLE Client initialized and callbacks registered");
 }

 void BLE_Client::handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {

    esp_err_t ret = ESP_OK;

    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
    esp_ble_gatt_creat_conn_params_t creat_conn_params = {0};

    uint8_t uuid_len = 0;
    const uint8_t *uuid_data = NULL;

    // Log the event type
    ESP_LOGW(TAG_CLIENT, "GAP event: %s", get_gap_event_name(event));
    switch (event) {
        //---------------------------------------------------------------------------------------------------------
        // GAP event for starting scan complete
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            esp_ble_gap_start_scanning(SCAN_DURATION);
            ESP_LOGI(TAG_GAP, "Scan parameters set successfully, starting scan...");
        break;
        
        //---------------------------------------------------------------------------------------------------------
        // GAP event for starting scan complete
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            //scan start complete event to indicate scan start successfully or failed
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG_GAP, "Scanning start failed, status %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(TAG_GAP, "Scanning start successfully");
        break;

        //---------------------------------------------------------------------------------------------------------
        // GAP event for scan result
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            
            if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                            scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                            ESP_BLE_AD_TYPE_NAME_CMPL,
                                                            &adv_name_len);

                ESP_LOGI(TAG_GAP, "Scan result, device "ESP_BD_ADDR_STR", name len %d", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
                ESP_LOG_BUFFER_CHAR(TAG_GAP, adv_name, adv_name_len);

               
                uuid_data = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                                            scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                                            ESP_BLE_AD_TYPE_128SRV_CMPL, // Change for other UUID types if needed
                                                                            &uuid_len);

                

                if (uuid_data && uuid_len > 0) {
                    ESP_LOGI(TAG_GAP, "UUID Found:");
                    ESP_LOG_BUFFER_HEX(TAG_GAP, uuid_data, uuid_len);
                } else {
                    ESP_LOGI(TAG_GAP, "No UUID found in advertisement.");
                }

                // Check if adv data and scan resp data are present and log them
                if (scan_result->scan_rst.adv_data_len > 0) {
                    ESP_LOGI(TAG_GAP, "adv data:");
                    ESP_LOG_BUFFER_HEX(TAG_GAP, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
                }
                if (scan_result->scan_rst.scan_rsp_len > 0) {
                    ESP_LOGI(TAG_GAP, "scan resp:");
                    ESP_LOG_BUFFER_HEX(TAG_GAP, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
                }


                if (strlen(DEVICE_NAME_SERVER) == adv_name_len && strncmp((char *)adv_name, DEVICE_NAME_SERVER, adv_name_len) == 0) {
                    ESP_LOGI(TAG_GAP, "Device found %s", DEVICE_NAME_SERVER);
                    if (_is_connected == false) {
                        esp_ble_gap_stop_scanning();
                        memcpy(&creat_conn_params.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                        creat_conn_params.remote_addr_type = scan_result->scan_rst.ble_addr_type;
                        creat_conn_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
                        creat_conn_params.is_direct = true;
                        creat_conn_params.is_aux = false;
                        creat_conn_params.phy_mask = 0x0;
                        //ret = esp_ble_gattc_enh_open(_gattc_profile_inst.gattc_if, &creat_conn_params);
                        ret = esp_ble_gattc_open(_gattc_profile_inst.gattc_if,scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                        if (ret != ESP_OK) {
                            ESP_LOGE(TAG_GAP, "Connection failed, error code = %d", ret);
                            _is_connected = false;
                        }
                        else {
                            ESP_LOGI(TAG_GAP, "Connection successfull");
                            _is_connected = true;
                        }
                    }                  
                }
            }
            //ke plan wieso das gmacht wird
           //else if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {}
           //else {
        break;

        //---------------------------------------------------------------------------------------------------------
        // GAP event for stopping scan complete
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
                
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG_GAP, "Scanning stop failed, status %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(TAG_GAP, "Scanning stop successfully");

            break;

        //---------------------------------------------------------------------------------------------------------
        // GAP advertising stop complete event
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG_GAP, "Advertising stop failed, status %x", param->adv_stop_cmpl.status);
                break;
            }
            ESP_LOGI(TAG_GAP, "Advertising stop successfully");
        break;

        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG_GAP, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
                param->update_conn_params.status,
                param->update_conn_params.conn_int,
                param->update_conn_params.latency,
                param->update_conn_params.timeout);
            ESP_LOGI(TAG_GAP, "Connection params update");
        break;

        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
           //ESP_LOGI(TAG_GAP, "Packet length update, status %d, rx %d, tx %d",
           //    param->pkt_data_length_cmpl.status,
           //    param->pkt_data_length_cmpl.params.rx_len,
           //    param->pkt_data_length_cmpl.params.tx_len);
              ESP_LOGI(TAG_GAP, "Packet length update");
        break;

        //---------------------------------------------------------------------------------------------------------
        // Default case for unhandled events
        //---------------------------------------------------------------------------------------------------------
        default:
            ESP_LOGE(TAG_GAP, "Unhandled GAP event: %d", event);
        break;
    }
 }

 void BLE_Client::handle_event_gattc(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){

    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    esp_gatt_status_t status = ESP_GATT_OK;
    esp_err_t ret = ESP_OK;

    uint16_t char_count =0;
    uint8_t cccd_value[2] = {0x02,0x00};
    esp_gattc_char_elem_t *char_elem_result   = NULL;
    esp_gattc_descr_elem_t *descr_elem_result = NULL;

    esp_bd_addr_t bda;

    ESP_LOGW(TAG_CLIENT, "GATT event: %s", get_gattc_event_name(event));
    switch (event) {
            //----------------------------------------------------------------------------------------------------------
            // GATT client registered successfully
            //----------------------------------------------------------------------------------------------------------
            case ESP_GATTC_REG_EVT:
                ESP_LOGI(TAG_GATTS, "GATT client register, status %d, app_id %d, gattc_if %d", param->reg.status, param->reg.app_id, gattc_if);
                _gattc_profile_inst.gattc_if = gattc_if;
                ret = esp_ble_gap_set_scan_params(&_scan_params);
                if (ret){
                    ESP_LOGE(TAG_GATTS, "set scan params error, error code = %d", ret);
                }
            break;
            
            //----------------------------------------------------------------------------------------------------------
            // GATT Connect event
            //----------------------------------------------------------------------------------------------------------
            case ESP_GATTC_CONNECT_EVT:
                //ESP_LOGI(TAG_GATTS, "Connected, conn_id %d, remote "ESP_BD_ADDR_STR"", p_data->connect.conn_id, ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
                ESP_LOGI(TAG_GATTS, "Connected, conn_id %d", p_data->connect.conn_id);
                _gattc_profile_inst.conn_id = p_data->connect.conn_id;
                memcpy(_gattc_profile_inst.remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
                ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
                if (ret){
                    ESP_LOGE(TAG_GATTS, "Config MTU error, error code = %d", ret);
                }
            break;
            
            //----------------------------------------------------------------------------------------------------------
            // GATT Open event
            //----------------------------------------------------------------------------------------------------------
            case ESP_GATTC_OPEN_EVT:
                if (param->open.status != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "Open failed, status %d", p_data->open.status);
                    break;
                }
                ESP_LOGI(TAG_GATTS, "Open successfully");
            break;

            //------------------------------------------------------------------------------------------------------------
            // GATT discover service event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_DIS_SRVC_CMPL_EVT:
                if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "Service discover failed, status %d", param->dis_srvc_cmpl.status);
                    break;
                }
                ESP_LOGI(TAG_GATTS, "Service discover complete, conn_id %d", param->dis_srvc_cmpl.conn_id);
                ret = esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &_remote_service_uuid);
                if (ret != ESP_OK){
                    ESP_LOGE(TAG_GATTS, "Search service failed, error code = %d", ret);
                }
                else {
                    ESP_LOGI(TAG_GATTS, "Search service started");
                }
                
            break;

            //------------------------------------------------------------------------------------------------------------
            // GATT config MTU event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_CFG_MTU_EVT:
                ESP_LOGI(TAG_GATTS, "MTU exchange, status %d, MTU %d", param->cfg_mtu.status, param->cfg_mtu.mtu);
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT service serch result event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_SEARCH_RES_EVT:
                ESP_LOGI(TAG_GATTS, "Service search result, conn_id = %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
                ESP_LOGI(TAG_GATTS, "start handle %d, end handle %d, current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);

                

                //check if the service UUID matches the remote service UUID
                if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128 &&
                    memcmp(p_data->search_res.srvc_id.uuid.uuid.uuid128, _remote_service_uuid.uuid.uuid128, _remote_service_uuid.len) == 0){
                    ESP_LOGI(TAG_GATTS, "Service found by uuid");
                    _get_server = true;
                    memcpy(_gattc_profile_inst.remote_bda, param->open.remote_bda, sizeof(esp_bd_addr_t));
                    _gattc_profile_inst.service_start_handle = p_data->search_res.start_handle;
                    _gattc_profile_inst.service_end_handle = p_data->search_res.end_handle;
                }
                else {
                    ESP_LOGI(TAG_GATTS, "Service found but not right uuid");
                }
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT service search complete event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_SEARCH_CMPL_EVT:
                if (p_data->search_cmpl.status != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "Service search failed, status %x", p_data->search_cmpl.status);
                    break;
                }
                if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
                    ESP_LOGI(TAG_GATTS, "Get service information from remote device");
                } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
                    ESP_LOGI(TAG_GATTS, "Get service information from flash");
                } else {
                    ESP_LOGI(TAG_GATTS, "Unknown service source");
                }
                ESP_LOGI(TAG_GATTS, "Service search complete");
              
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t));
                if (!char_elem_result){
                    ESP_LOGE(TAG_GATTS, "gattc no mem");
                    break;
                }
                if (_get_server) {
                    status = esp_ble_gattc_get_attr_count( gattc_if,
                                                            _gattc_profile_inst.conn_id,
                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                            _gattc_profile_inst.service_start_handle,
                                                            _gattc_profile_inst.service_end_handle,
                                                            0,
                                                            &char_count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(TAG_GATTS, "esp_ble_gattc_get_attr_count error, error code = %d", status);
                        free(char_elem_result);
                        char_elem_result = nullptr;
                        break;
                    }
                }

                ESP_LOGI(TAG_GATTS, "char count %d", char_count);
                if (char_count > 0) {
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            _gattc_profile_inst.service_start_handle,
                                                            _gattc_profile_inst.service_end_handle,
                                                            _remote_char_uuid,
                                                            char_elem_result,
                                                            &char_count);

                    if (status != ESP_GATT_OK){
                        ESP_LOGE(TAG_GATTS, "char not found by uuid, error %d", status);
                        free(char_elem_result);
                        char_elem_result = nullptr;
                        break;
                    }
                    
                    /*  The service has only one characteristic in our application , so we used first 'char_elem_result' */
                    if (char_count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_INDICATE)){
                        ESP_LOGI(TAG_GATTS, "char found by uuid");
                        ESP_LOGI(TAG_GATTS, "char handle %d", char_elem_result[0].char_handle);
                        _gattc_profile_inst.char_handle = char_elem_result[0].char_handle;
                        ret = esp_ble_gattc_register_for_notify( gattc_if,
                                                                _gattc_profile_inst.remote_bda,
                                                                _gattc_profile_inst.char_handle);

                        if (ret != ESP_OK){
                            ESP_LOGE(TAG_GATTS, "Register for notify error, error code = %d", ret);
                        }
                        else {
                            ESP_LOGI(TAG_GATTS, "Register for notify successfully");
                        }
                    }
                    else {
                        ESP_LOGE(TAG_GATTS, "char not found by uuid or no property indicate");
                        free(char_elem_result);
                        char_elem_result = nullptr;
                        break;
                    }
                }
                else {
                    ESP_LOGE(TAG_GATTS, "no char found");
                    free(char_elem_result);
                    char_elem_result = nullptr;
                    break;
                }    
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT register for notification event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_REG_FOR_NOTIFY_EVT:

                if (p_data->reg_for_notify.status != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "Notification register failed");
                }
                else {
                    ESP_LOGI(TAG_GATTS, "Notification register successfully");
                    ret = esp_ble_gattc_get_attr_count( gattc_if,
                                                        _gattc_profile_inst.conn_id,
                                                        ESP_GATT_DB_DESCRIPTOR,
                                                        _gattc_profile_inst.service_start_handle,
                                                        _gattc_profile_inst.service_end_handle, 
                                                        _gattc_profile_inst.char_handle,
                                                        &char_count);
                    if (ret != ESP_GATT_OK){
                        ESP_LOGE(TAG_GATTS, "esp_ble_gattc_get_attr_count error");
                        break;
                    }

                    if (char_count > 0) {
                        ESP_LOGI(TAG_GATTS, "Descriptor count %d", char_count);
                        descr_elem_result = (esp_gattc_descr_elem_t *) malloc(sizeof(esp_gattc_descr_elem_t) * char_count);
                        if (!descr_elem_result){
                            ESP_LOGE(TAG_GATTS, "malloc error, gattc no mem");
                            break;
                        }
                        else {
                            ret = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                        _gattc_profile_inst.conn_id,
                                                                        p_data->reg_for_notify.handle,
                                                                        _remote_descr_uuid,
                                                                        descr_elem_result,
                                                                        &char_count);

                            if (ret != ESP_GATT_OK){
                                ESP_LOGE(TAG_GATTS, "esp_ble_gattc_get_descr_by_char_handle error");
                                free(descr_elem_result);
                                descr_elem_result = nullptr;
                                break;
                            }

                            /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                            if (char_count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_128 &&
                                memcmp(descr_elem_result[0].uuid.uuid.uuid128, _remote_descr_uuid.uuid.uuid128, sizeof(_remote_descr_uuid.uuid.uuid128)) == 0){
                                    uint16_t indication_en = 2;
                                    ret = esp_ble_gattc_write_char_descr( gattc_if,
                                                                    _gattc_profile_inst.conn_id,
                                                                    descr_elem_result[0].handle,
                                                                    sizeof(indication_en),
                                                                    (uint8_t *)&indication_en,
                                                                    ESP_GATT_WRITE_TYPE_RSP,
                                                                    ESP_GATT_AUTH_REQ_NONE);
                                if (ret != ESP_GATT_OK){
                                    ESP_LOGE(TAG_GATTS, "esp_ble_gattc_write_char_descr error");
                                }
                                else {
                                    ESP_LOGI(TAG_GATTS, "Write descriptor successfully");
                                }
                            }

                            /* free descr_elem_result */
                            free(descr_elem_result);
                            descr_elem_result = nullptr;
                        }
                    }
                    else{
                        ESP_LOGE(TAG_GATTS, "decsr not found");
                    }
                }  
            break;

            //------------------------------------------------------------------------------------------------------------
            // GATT notify event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_NOTIFY_EVT:
                if (p_data->notify.is_notify){
                    ESP_LOGI(TAG_GATTS, "Notification received");
                }else{
                    ESP_LOGI(TAG_GATTS, "Indication received");
                }
                //ESP_LOG_BUFFER_HEX(TAG_GATTS, p_data->notify.value, p_data->notify.value_len);
                ESP_LOGI(TAG_GATTS, "Characteristic value received:");
                memcpy(_char_recv_buffer, p_data->notify.value, p_data->notify.value_len);
                ESP_LOGI(TAG_GATTS, "Received value: %.*s", p_data->notify.value_len, _char_recv_buffer);
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT write descriptor event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_WRITE_DESCR_EVT:
                if (p_data->write.status != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "Descriptor write failed, status %x", p_data->write.status);
                    break;
                }
                ESP_LOGI(TAG_GATTS, "Descriptor write successfully");
            
                ret = esp_ble_gattc_write_char( gattc_if,
                                                _gattc_profile_inst.conn_id,
                                                _gattc_profile_inst.char_handle,
                                                sizeof(_char_send_buffer),
                                                _char_send_buffer,
                                                ESP_GATT_WRITE_TYPE_RSP,
                                                ESP_GATT_AUTH_REQ_NONE);
                if (ret != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "esp_ble_gattc_write_char error, error code = %d", ret);
                    break;
                }
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT service changed event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_SRVC_CHG_EVT:
                ESP_LOGI(TAG_GATTS, "Service change!");
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT write characteristic event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_WRITE_CHAR_EVT:
                if (p_data->write.status != ESP_GATT_OK){
                    ESP_LOGE(TAG_GATTS, "Caracteristic write failed, status");
                    break;
                }
                ESP_LOGI(TAG_GATTS, "Characteristic write successfully");
                //ESP_LOGI(TAG_GATTS, "Write value: ");
                //// Print out the written data byte by byte
                //for (int i = 0; i < write_param->length; i++) {
                //    ESP_LOGI(TAG, "0x%02X", write_param->value[i]);
                //}
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT Disconnect event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_DISCONNECT_EVT:
                ESP_LOGI(TAG_GATTS, "Disconnected, conn_id %d, reason %d", p_data->disconnect.conn_id, p_data->disconnect.reason);
                _is_connected = false;
                _get_server = false;
                _gattc_profile_inst.conn_id = 0;
                _gattc_profile_inst.service_start_handle = 0;
                _gattc_profile_inst.char_handle = 0;
                _gattc_profile_inst.service_end_handle = 0;

                ret = esp_ble_gap_start_scanning(SCAN_DURATION);
                if(ret != ESP_OK){
                    ESP_LOGE(TAG_GATTS, "start scan failed, error code = %x", ret);
                }
                else {
                    ESP_LOGI(TAG_GATTS, "start scan successfully");
                }
            break;
           
            //------------------------------------------------------------------------------------------------------------
            // default case for unhandled events
            //------------------------------------------------------------------------------------------------------------
            default:
                ESP_LOGE(TAG_GATTS, "Unhandled GATTC event: %s", get_gattc_event_name(event));
                break;
        }
    }

 void BLE_Client::send(const std::string &data) {
     std::cout << "BLE Client Sending: " << data << std::endl;
 }
 
 std::string BLE_Client::receive() {
     
     return "Received Data from BLE Server";
 }

 const char* BLE_Client::get_gattc_event_name(esp_gattc_cb_event_t event) {
    switch (event) {
        case ESP_GATTC_REG_EVT: return "ESP_GATTC_REG_EVT";
        case ESP_GATTC_UNREG_EVT: return "ESP_GATTC_UNREG_EVT";
        case ESP_GATTC_OPEN_EVT: return "ESP_GATTC_OPEN_EVT";
        case ESP_GATTC_READ_CHAR_EVT: return "ESP_GATTC_READ_CHAR_EVT";
        case ESP_GATTC_WRITE_CHAR_EVT: return "ESP_GATTC_WRITE_CHAR_EVT";
        case ESP_GATTC_CLOSE_EVT: return "ESP_GATTC_CLOSE_EVT";
        case ESP_GATTC_SEARCH_CMPL_EVT: return "ESP_GATTC_SEARCH_CMPL_EVT";
        case ESP_GATTC_SEARCH_RES_EVT: return "ESP_GATTC_SEARCH_RES_EVT";
        case ESP_GATTC_READ_DESCR_EVT: return "ESP_GATTC_READ_DESCR_EVT";
        case ESP_GATTC_WRITE_DESCR_EVT: return "ESP_GATTC_WRITE_DESCR_EVT";
        case ESP_GATTC_NOTIFY_EVT: return "ESP_GATTC_NOTIFY_EVT";
        case ESP_GATTC_PREP_WRITE_EVT: return "ESP_GATTC_PREP_WRITE_EVT";
        case ESP_GATTC_EXEC_EVT: return "ESP_GATTC_EXEC_EVT";
        case ESP_GATTC_ACL_EVT: return "ESP_GATTC_ACL_EVT";
        case ESP_GATTC_CANCEL_OPEN_EVT: return "ESP_GATTC_CANCEL_OPEN_EVT";
        case ESP_GATTC_SRVC_CHG_EVT: return "ESP_GATTC_SRVC_CHG_EVT";
        case ESP_GATTC_ENC_CMPL_CB_EVT: return "ESP_GATTC_ENC_CMPL_CB_EVT";
        case ESP_GATTC_CFG_MTU_EVT: return "ESP_GATTC_CFG_MTU_EVT";
        case ESP_GATTC_MULT_ADV_ENB_EVT: return "ESP_GATTC_MULT_ADV_ENB_EVT";
        case ESP_GATTC_MULT_ADV_UPD_EVT: return "ESP_GATTC_MULT_ADV_UPD_EVT";
        case ESP_GATTC_MULT_ADV_DATA_EVT: return "ESP_GATTC_MULT_ADV_DATA_EVT";
        case ESP_GATTC_MULT_ADV_DIS_EVT: return "ESP_GATTC_MULT_ADV_DIS_EVT";
        case ESP_GATTC_CONGEST_EVT: return "ESP_GATTC_CONGEST_EVT";
        case ESP_GATTC_BTH_SCAN_ENB_EVT: return "ESP_GATTC_BTH_SCAN_ENB_EVT";
        case ESP_GATTC_BTH_SCAN_CFG_EVT: return "ESP_GATTC_BTH_SCAN_CFG_EVT";
        case ESP_GATTC_BTH_SCAN_RD_EVT: return "ESP_GATTC_BTH_SCAN_RD_EVT";
        case ESP_GATTC_BTH_SCAN_THR_EVT: return "ESP_GATTC_BTH_SCAN_THR_EVT";
        case ESP_GATTC_BTH_SCAN_PARAM_EVT: return "ESP_GATTC_BTH_SCAN_PARAM_EVT";
        case ESP_GATTC_BTH_SCAN_DIS_EVT: return "ESP_GATTC_BTH_SCAN_DIS_EVT";
        case ESP_GATTC_SCAN_FLT_CFG_EVT: return "ESP_GATTC_SCAN_FLT_CFG_EVT";
        case ESP_GATTC_SCAN_FLT_PARAM_EVT: return "ESP_GATTC_SCAN_FLT_PARAM_EVT";
        case ESP_GATTC_SCAN_FLT_STATUS_EVT: return "ESP_GATTC_SCAN_FLT_STATUS_EVT";
        case ESP_GATTC_ADV_VSC_EVT: return "ESP_GATTC_ADV_VSC_EVT";
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: return "ESP_GATTC_REG_FOR_NOTIFY_EVT";
        case ESP_GATTC_UNREG_FOR_NOTIFY_EVT: return "ESP_GATTC_UNREG_FOR_NOTIFY_EVT";
        case ESP_GATTC_CONNECT_EVT: return "ESP_GATTC_CONNECT_EVT";
        case ESP_GATTC_DISCONNECT_EVT: return "ESP_GATTC_DISCONNECT_EVT";
        case ESP_GATTC_READ_MULTIPLE_EVT: return "ESP_GATTC_READ_MULTIPLE_EVT";
        case ESP_GATTC_QUEUE_FULL_EVT: return "ESP_GATTC_QUEUE_FULL_EVT";
        case ESP_GATTC_SET_ASSOC_EVT: return "ESP_GATTC_SET_ASSOC_EVT";
        case ESP_GATTC_GET_ADDR_LIST_EVT: return "ESP_GATTC_GET_ADDR_LIST_EVT";
        case ESP_GATTC_DIS_SRVC_CMPL_EVT: return "ESP_GATTC_DIS_SRVC_CMPL_EVT";
        default: return "UNKNOWN_GATTC_EVENT";
    }
}
 
 const char* BLE_Client::get_gap_event_name(esp_gap_ble_cb_event_t event) {
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

 #endif