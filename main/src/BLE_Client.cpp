/**
 * @file BLE_Client.cpp
 * @brief Implementation of BLE_Client class.
 */

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
        //esp_err_t ret = nvs_flash_init();
        //if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        //    ESP_ERROR_CHECK(nvs_flash_erase());
        //    ESP_ERROR_CHECK(nvs_flash_init());
        //}
    //
        //// Initialize Bluetooth controller and enable BLE mode
        //esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        //esp_bt_controller_init(&bt_cfg);
        //esp_bt_controller_enable(ESP_BT_MODE_BLE);
        //esp_bluedroid_init();
        //esp_bluedroid_enable();
 }
 
 BLE_Client::~BLE_Client() {
        // Clean up resources if needed
        //Client_instance = nullptr;
        //esp_bluedroid_disable();
        //esp_bluedroid_deinit();
        //esp_bt_controller_disable();
        //esp_bt_controller_deinit();
        //nvs_flash_deinit();
        //ESP_LOGI(TAG, "BLE Client Deinitialized");
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
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
    esp_ble_gatt_creat_conn_params_t creat_conn_params = {0};

    // Log the event type
    ESP_LOGW(TAG_CLIENT, "GAP event: %d", event);
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

                // Check if adv data and scan resp data are present and log them
                if (scan_result->scan_rst.adv_data_len > 0) {
                    ESP_LOGI(TAG_GAP, "adv data:");
                    ESP_LOG_BUFFER_HEX(TAG_GAP, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
                }
                if (scan_result->scan_rst.scan_rsp_len > 0) {
                    ESP_LOGI(TAG_GAP, "scan resp:");
                    ESP_LOG_BUFFER_HEX(TAG_GAP, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
                }

                if (adv_name != NULL) {
                    ESP_LOGI(TAG_GAP, "Device name: %.*s", adv_name_len, adv_name);
                } else {
                    ESP_LOGI(TAG_GAP, "Device name not found in advertisement data");
                }

                if (strlen(DEVICE_NAME_SERVER) == adv_name_len && strncmp((char *)adv_name, DEVICE_NAME_SERVER, adv_name_len) == 0) {
                    ESP_LOGI(TAG_GAP, "Device found %s", DEVICE_NAME_SERVER);
                    if (_is_connected == false) {
                        _is_connected = true;
                        ESP_LOGI(TAG_GAP, "Connect to the remote device");
                        esp_ble_gap_stop_scanning();
                        memcpy(&creat_conn_params.remote_bda, scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
                        creat_conn_params.remote_addr_type = scan_result->scan_rst.ble_addr_type;
                        creat_conn_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
                        creat_conn_params.is_direct = true;
                        creat_conn_params.is_aux = false;
                        creat_conn_params.phy_mask = 0x0;
                        esp_ble_gattc_enh_open(_gattc_profile_inst.gattc_if,
                            &creat_conn_params);
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
            //ESP_LOGI(TAG_GAP, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
            //    param->update_conn_params.status,
            //    param->update_conn_params.conn_int,
            //    param->update_conn_params.latency,
            //    param->update_conn_params.timeout);
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
    uint16_t notify_en =1;
    esp_gattc_char_elem_t *char_elem_result   = NULL;
    esp_gattc_descr_elem_t *descr_elem_result = NULL;

    esp_bd_addr_t bda;

    ESP_LOGW(TAG_CLIENT, "GATT event: %d", event);
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
                ESP_LOGI(TAG_GATTS, "Open successfully, MTU %u", p_data->open.mtu);
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
                esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &_remote_service_uuid);
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
                    memcmp(p_data->search_res.srvc_id.uuid.uuid.uuid128, _remote_service_uuid.uuid.uuid128, ESP_UUID_LEN_128) == 0) {
                    ESP_LOGI(TAG_GATTS, "Service found");
                    _get_server = true;
                    _gattc_profile_inst.service_start_handle = p_data->search_res.start_handle;
                    _gattc_profile_inst.service_end_handle = p_data->search_res.end_handle;
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

                if (_get_server) {
                    status = esp_ble_gattc_get_attr_count( gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                            _gattc_profile_inst.service_start_handle,
                                                            _gattc_profile_inst.service_end_handle,
                                                            0,
                                                            &char_count);
                
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(TAG_GATTS, "esp_ble_gattc_get_attr_count error");
                        break;
                    }

                    if (char_count > 0) {
                        char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * char_count);
                        if (!char_elem_result){
                            ESP_LOGE(TAG_GATTS, "gattc no mem");
                            break;
                        }
                        else {
                            status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                                    p_data->search_cmpl.conn_id,
                                                                    _gattc_profile_inst.service_start_handle,
                                                                    _gattc_profile_inst.service_end_handle,
                                                                    CHAR_UUID_DEFAULT,
                                                                    char_elem_result,
                                                                    &char_count);

                            //check if the char UUID matches the remote char UUID
                            if (status != ESP_GATT_OK){
                                ESP_LOGE(TAG_GATTS, "esp_ble_gattc_get_char_by_uuid error");
                                free(char_elem_result);
                                char_elem_result = nullptr;
                                break;
                            }

                            /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                            if (char_count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                                _gattc_profile_inst.char_handle = char_elem_result[0].char_handle;
                                esp_ble_gattc_register_for_notify (gattc_if, _gattc_profile_inst.remote_bda, char_elem_result[0].char_handle);
                            }
                            /* free char_elem_result */
                            free(char_elem_result);
                            char_elem_result = nullptr;
                        }
                    }
                    else {
                        ESP_LOGE(TAG_GATTS, "no char found");
                    }
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
                                ret = esp_ble_gattc_write_char_descr( gattc_if,
                                                                    _gattc_profile_inst.conn_id,
                                                                    descr_elem_result[0].handle,
                                                                    sizeof(notify_en),
                                                                    (uint8_t *)&notify_en,
                                                                    ESP_GATT_WRITE_TYPE_RSP,
                                                                    ESP_GATT_AUTH_REQ_NONE);
                                if (ret != ESP_GATT_OK){
                                    ESP_LOGE(TAG_GATTS, "esp_ble_gattc_write_char_descr error");
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
                    ESP_LOGE(TAG_GATTS, "Characteristic write failed, status");
                    break;
                }
                ESP_LOGI(TAG_GATTS, "Characteristic write successfully");
            break;
            
            //------------------------------------------------------------------------------------------------------------
            // GATT Disconnect event
            //------------------------------------------------------------------------------------------------------------
            case ESP_GATTC_DISCONNECT_EVT:
                _is_connected = false;
                _get_server = false;
                //ESP_LOGI(TAG_GATTS, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
                //    ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.reason);
            break;
           
            //------------------------------------------------------------------------------------------------------------
            // default case for unhandled events
            //------------------------------------------------------------------------------------------------------------
            default:
                ESP_LOGE(TAG_GATTS, "Unhandled GATTC event: %d", event);
                break;
        }
    }


 
 void BLE_Client:: connSetup() {
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gattc_register_callback(gattc_event_handler);
    esp_ble_gattc_app_register(PROFILE_APP_ID);
    esp_ble_gatt_set_local_mtu(_gattc_profile_inst.local_mtu);
    ESP_LOGI(TAG_CLIENT, "BLE Client initialized and callbacks registered");
 }
 
 void BLE_Client::send(const std::string &data) {
     std::cout << "BLE Client Sending: " << data << std::endl;
 }
 
 std::string BLE_Client::receive() {
     std::cout << "BLE Client Receiving Data" << std::endl;
     return "Received Data from BLE Server";
 }