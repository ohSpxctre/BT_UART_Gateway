/**
 * @file BLE_Client.cpp
 * @brief Implementation of the BLE_Client class for managing BLE client functionality.
 *
 * This file contains the implementation of the BLE_Client class, which provides
 * functionality for initializing and managing a BLE client, handling GAP and GATT
 * events, and sending/receiving data over BLE. The class uses the ESP-IDF framework
 * for BLE operations and integrates with a MessageHandler for communication with
 * other system components.
 *
 * Usage:
 * - Instantiate the BLE_Client class with appropriate parameters.
 * - Call `connSetup()` to initialize the BLE client and register callbacks.
 * - Use `send()` to send data to the connected BLE server.
 * - Use `sendTask()` to process messages from the BLE queue and send them to the server.
 * 
 * @note This implementation is designed for use with the ESP-IDF framework.
 * @note The class manages initialization and deinitialization of BLE resources.
 * @note The class handles GAP and GATT events and provides a message queue interface for communication.
 * @note The class handles BLE connection and disconnection events, including MTU size exchange.
 * @note The class manages data transmission and reception over BLE characteristics.
 *
 * @author hoyed1
 * @date 09.04.2025
 */

 #include "BLE_Client.hpp"
 #include <iostream>

 // Define the static variable outside the class
BLE_Client* BLE_Client::Client_instance = nullptr;
 
BLE_Client::BLE_Client(esp_ble_scan_params_t scan_params, esp_bt_uuid_t remote_service_uuid, esp_bt_uuid_t remote_char_uuid, esp_bt_uuid_t remote_descr_uuid, MessageHandler* msgHandler) :
    _scan_params(scan_params),
    _remote_service_uuid(remote_service_uuid),
    _remote_char_uuid(remote_char_uuid),
    _remote_descr_uuid(remote_descr_uuid)
{
    // Set the message handler
    _msgHandler = msgHandler;
    
    // Initialize the static instance pointer
    if (Client_instance != nullptr) {
        ESP_LOGE(BLE_TAGS::TAG_CLIENT, "BLE_Client instance already exists!");
        return;
    }
    // save the pointer to the instantiated object
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
    // Remove the message handler reference
    _msgHandler = nullptr;
    ESP_LOGI(BLE_TAGS::TAG_CLIENT, "BLE Client Deinitialized");
}

void BLE_Client::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (Client_instance) {
        // Call the static event handle method for GAP events
        Client_instance->handle_event_gap(event, param);
    }
}

void BLE_Client::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gattc_cb_param_t *param) {
    if (Client_instance) {
        // Call the static event handle method for GATT events
        Client_instance->handle_event_gattc(event, gatts_if, param);
    }
}

void BLE_Client:: connSetup() {
    // Send not connected message to UART interface
    MessageHandler::Message msg;
    std::fill(msg.begin(), msg.end(), '\0');
    std::copy(BLE_Defaults::BT_status_NC.begin(), BLE_Defaults::BT_status_NC.end(), msg.begin());
    msg[BLE_Defaults::BT_status_NC.length()] = '\n';
    _msgHandler->send(MessageHandler::QueueType::UART_QUEUE, msg, MessageHandler::ParserMessageID::MSG_ID_BLE);
    // Register callback functions
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gattc_register_callback(gattc_event_handler);
    // Register the GATT server application
    esp_ble_gattc_app_register(BLE_Defaults::PROFILE_APP_ID);
    // Set the local MTU size
    esp_ble_gatt_set_local_mtu(_gattc_profile_inst.local_mtu);
    ESP_LOGI(BLE_TAGS::TAG_CLIENT, "BLE Client initialized and callbacks registered");
}

void BLE_Client::handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {

    esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
    esp_err_t ret = ESP_OK;

    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    uint8_t uuid_len = 0;
    const uint8_t *uuid_data = NULL;

    // Log the event type
    ESP_LOGW(BLE_TAGS::TAG_CLIENT, "GAP event: %s", get_gap_event_name(event));
    switch (event) {
        //---------------------------------------------------------------------------------------------------------
        // GAP event for starting scan complete
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            //set scan parameters successfully, start scanning
            esp_ble_gap_start_scanning(BLE_Defaults::SCAN_DURATION);
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Scan parameters set successfully, starting scan...");
        break;
        
        //---------------------------------------------------------------------------------------------------------
        // GAP event for starting scan complete
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            //scan start complete event to indicate scan start successfully or failed
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLE_TAGS::TAG_GAP, "Scanning start failed, status %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Scanning start successfully");
        break;

        //---------------------------------------------------------------------------------------------------------
        // GAP event for scan result
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
            // scan result event, found a device
            if (scan_result->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
                // reading device name from advertisement data
                adv_name = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                            scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                            ESP_BLE_AD_TYPE_NAME_CMPL,
                                                            &adv_name_len);
                
                // log the device name and address
                ESP_LOGI(BLE_TAGS::TAG_GAP, "Scan result, device "ESP_BD_ADDR_STR", name len %d", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), adv_name_len);
                ESP_LOG_BUFFER_CHAR(BLE_TAGS::TAG_GAP, adv_name, adv_name_len);
                // read uuid from advertisement data
                uuid_data = esp_ble_resolve_adv_data_by_type(scan_result->scan_rst.ble_adv,
                                                                            scan_result->scan_rst.adv_data_len + scan_result->scan_rst.scan_rsp_len,
                                                                            ESP_BLE_AD_TYPE_128SRV_CMPL, // Change for other UUID types if needed
                                                                            &uuid_len);
                // log the uuid data if found
                if (uuid_data && uuid_len > 0) {
                    ESP_LOGI(BLE_TAGS::TAG_GAP, "UUID Found:");
                    ESP_LOG_BUFFER_HEX(BLE_TAGS::TAG_GAP, uuid_data, uuid_len);
                } else {
                    ESP_LOGI(BLE_TAGS::TAG_GAP, "No UUID found in advertisement.");
                }

                // Check if adv data and scan resp data are present and log them
                if (scan_result->scan_rst.adv_data_len > 0) {
                    ESP_LOGI(BLE_TAGS::TAG_GAP, "adv data:");
                    ESP_LOG_BUFFER_HEX(BLE_TAGS::TAG_GAP, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
                }
                if (scan_result->scan_rst.scan_rsp_len > 0) {
                    ESP_LOGI(BLE_TAGS::TAG_GAP, "scan resp:");
                    ESP_LOG_BUFFER_HEX(BLE_TAGS::TAG_GAP, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
                }

                // Check if the device name matches the expected server name
                if (strlen(BLE_Defaults::DEVICE_NAME_SERVER) == adv_name_len && strncmp((char *)adv_name, BLE_Defaults::DEVICE_NAME_SERVER, adv_name_len) == 0) {
                    ESP_LOGI(BLE_TAGS::TAG_GAP, "Device found %s", BLE_Defaults::DEVICE_NAME_SERVER);
                    if (_is_connected == false) {
                        // stop scanning
                        esp_ble_gap_stop_scanning();
                        // connect to the device
                        ret = esp_ble_gattc_open(_gattc_profile_inst.gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                        if (ret != ESP_OK) {
                            ESP_LOGE(BLE_TAGS::TAG_GAP, "Connection failed, error code = %d", ret);
                            _is_connected = false;
                        }
                        else {
                            ESP_LOGI(BLE_TAGS::TAG_GAP, "Connection successfull");
                            _is_connected = true;
                        }
                    }                  
                }
                else {
                    // if the device name does not match, continue scanning
                    esp_ble_gap_start_scanning(BLE_Defaults::SCAN_DURATION);
                }
            }
        break;

        //---------------------------------------------------------------------------------------------------------
        // GAP event for stopping scan complete
        //---------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            // check if scan stopped successfully
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(BLE_TAGS::TAG_GAP, "Scanning stop failed, status %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Scanning stop successfully");
            if (!_is_connected) {
                // if not connected, start scanning again
                ret = esp_ble_gap_start_scanning(BLE_Defaults::SCAN_DURATION);
                if (ret){
                    ESP_LOGE(BLE_TAGS::TAG_GAP, "Restart scanning failed, error code = %d", ret);
                }
            }
        break;
        
        //--------------------------------------------------------------------------------------------------------
        // GAP event for connection parameters update
        //--------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Connection parameters updated");                
        break;
        
        //--------------------------------------------------------------------------------------------------------
        // GAP event for packet length update
        //--------------------------------------------------------------------------------------------------------
        case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Packet length updated");
        break;

        //---------------------------------------------------------------------------------------------------------
        // Default case for unhandled events
        //---------------------------------------------------------------------------------------------------------
        default:
            ESP_LOGE(BLE_TAGS::TAG_GAP, "Unhandled GAP event: %d", event);
        break;
    }
}

void BLE_Client::handle_event_gattc(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){

    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;
    esp_gatt_status_t status = ESP_GATT_OK;
    esp_err_t ret = ESP_OK;

    // Message object to send bluetooth status to UART interface
    MessageHandler::Message msg;

    // temporary variable to count the number of attributes
    uint16_t count = 0;
    
    esp_gattc_char_elem_t *char_elem_result   = NULL;
    esp_gattc_descr_elem_t *descr_elem_result = NULL;

    ESP_LOGW(BLE_TAGS::TAG_CLIENT, "GATT event: %s", get_gattc_event_name(event));
    switch (event) {
        //----------------------------------------------------------------------------------------------------------
        // GATT client registered successfully
        //----------------------------------------------------------------------------------------------------------
        case ESP_GATTC_REG_EVT:
            // save the bluetooth interface after app registration
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "GATT client register, status %d, app_id %d, gattc_if %d", param->reg.status, param->reg.app_id, gattc_if);
            _gattc_profile_inst.gattc_if = gattc_if;
            // set the scan parameters
            ret = esp_ble_gap_set_scan_params(&_scan_params);
            if (ret){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "set scan params error, error code = %d", ret);
            }
        break;
        
        //----------------------------------------------------------------------------------------------------------
        // GATT Connect event
        //----------------------------------------------------------------------------------------------------------
        case ESP_GATTC_CONNECT_EVT:
            // check if the connection was successful
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Connected, conn_id %d", p_data->connect.conn_id);
            // save the connection ID and remote address
            _gattc_profile_inst.conn_id = p_data->connect.conn_id;
            memcpy(_gattc_profile_inst.remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
            // request mtu size
            ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
            if (ret){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Config MTU error, error code = %d", ret);
            }
        break;
        
        //----------------------------------------------------------------------------------------------------------
        // GATT Open event
        //----------------------------------------------------------------------------------------------------------
        case ESP_GATTC_OPEN_EVT:
            // check if the opening connection was successful
            if (param->open.status != ESP_GATT_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Open failed, status %d", p_data->open.status);
                break;
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Open successfully");
            // send conncetion message to the UART interface
            std::fill(msg.begin(), msg.end(), '\0');
            std::copy(BLE_Defaults::BT_status_C.begin(), BLE_Defaults::BT_status_C.end(), msg.begin());
            msg[BLE_Defaults::BT_status_C.length()] = '\n';
            _msgHandler->send(MessageHandler::QueueType::UART_QUEUE, msg, MessageHandler::ParserMessageID::MSG_ID_BLE);
        break;

        //------------------------------------------------------------------------------------------------------------
        // GATT discover service event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            // check if the service discovery was successful
            if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Service discover failed, status %d", param->dis_srvc_cmpl.status);
                break;
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service discover complete, conn_id %d", param->dis_srvc_cmpl.conn_id);
            // start searching for the service
            ret = esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &_remote_service_uuid);
            if (ret != ESP_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Search service failed, error code = %d", ret);
            }
            else {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Search service started");
            }    
        break;

        //------------------------------------------------------------------------------------------------------------
        // GATT config MTU event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_CFG_MTU_EVT:
            // log mtu size after configuration
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "MTU exchange, status %d, MTU %d", param->cfg_mtu.status, param->cfg_mtu.mtu);
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT service serch result event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_SEARCH_RES_EVT:
            // log the service search result
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service search result, conn_id = %x, is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "start handle %d, end handle %d, current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);

            //check if the service UUID matches the remote service UUID
            if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128 &&
                memcmp(p_data->search_res.srvc_id.uuid.uuid.uuid128, _remote_service_uuid.uuid.uuid128, _remote_service_uuid.len) == 0){
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service found by uuid");
                // set server flag to ture and save service handles
                _get_server = true;
                _gattc_profile_inst.service_start_handle = p_data->search_res.start_handle;
                _gattc_profile_inst.service_end_handle = p_data->search_res.end_handle;
            }
            else {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service found but not right uuid");
            }
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT service search complete event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_SEARCH_CMPL_EVT:
            
            // check if the service search was successful
            if (p_data->search_cmpl.status != ESP_GATT_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Service search failed, status %x", p_data->search_cmpl.status);
                break;
            }
            // log the service source
            if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Get service information from remote device");
            } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Get service information from flash");
            } else {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Unknown service source");
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service search complete");
            // allocate memory for characteristic element result
            char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t));
            if (!char_elem_result){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "gattc no mem");
                break;
            }
            // check if the server was found
            if (_get_server) {
                count =0;
                // get count for attributes
                status = esp_ble_gattc_get_attr_count( gattc_if,
                                                        _gattc_profile_inst.conn_id,
                                                        ESP_GATT_DB_CHARACTERISTIC,
                                                        _gattc_profile_inst.service_start_handle,
                                                        _gattc_profile_inst.service_end_handle,
                                                        0,
                                                        &count);
                if (status != ESP_GATT_OK){
                    ESP_LOGE(BLE_TAGS::TAG_GATTS, "esp_ble_gattc_get_attr_count error, error code = %d", status);
                    free(char_elem_result);
                    char_elem_result = nullptr;
                    break;
                }
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "char count %d", count);
            if (count > 0) {
                // get the characteristic by UUID
                status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                        p_data->search_cmpl.conn_id,
                                                        _gattc_profile_inst.service_start_handle,
                                                        _gattc_profile_inst.service_end_handle,
                                                        _remote_char_uuid,
                                                        char_elem_result,
                                                        &count);
                // check if the characteristic was found
                if (status != ESP_GATT_OK){
                    ESP_LOGE(BLE_TAGS::TAG_GATTS, "char not found by uuid, error %d", status);
                    free(char_elem_result);
                    char_elem_result = nullptr;
                    break;
                }
                // server has only one characteristic, so we used first 'char_elem_result'
                if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_INDICATE)){
                    ESP_LOGI(BLE_TAGS::TAG_GATTS, "char found by uuid");
                    // save the characteristic handle
                    _gattc_profile_inst.char_handle = char_elem_result[0].char_handle;
                    ESP_LOGI(BLE_TAGS::TAG_GATTS, "char handle %d", _gattc_profile_inst.char_handle);
                    // register for notification
                    ret = esp_ble_gattc_register_for_notify( gattc_if,
                                                            _gattc_profile_inst.remote_bda,
                                                            _gattc_profile_inst.char_handle);
                    // check if the registration was successful
                    if (ret != ESP_OK){
                        ESP_LOGE(BLE_TAGS::TAG_GATTS, "Register for notify error, error code = %d", ret);
                    }
                    else {
                        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Register for notify successfully");
                    }
                }
                else {
                    // if no characteristic was found
                    ESP_LOGE(BLE_TAGS::TAG_GATTS, "char not found by uuid or no property indicate");
                    // free the allocated memory for char_elem_result
                    free(char_elem_result);
                    char_elem_result = nullptr;
                    break;
                }
            }
            else {
                // if no attributes are found, log the error
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "no char found");
                // free the allocated memory for char_elem_result
                free(char_elem_result);
                char_elem_result = nullptr;
                break;
            }    
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT register for notification event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_REG_FOR_NOTIFY_EVT:
            //check if the registration was successful or not
            if (p_data->reg_for_notify.status != ESP_GATT_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Notification register failed");
            }
            else {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Notification register successfully");
                // temporary variable to count the number of attributes
                uint16_t count = 0;
                // get number of attributes in service
                ret = esp_ble_gattc_get_attr_count( gattc_if,
                                                    _gattc_profile_inst.conn_id,
                                                    ESP_GATT_DB_DESCRIPTOR,
                                                    _gattc_profile_inst.service_start_handle,
                                                    _gattc_profile_inst.service_end_handle, 
                                                    _gattc_profile_inst.char_handle,
                                                    &count);
                // check if the attribute count was successful
                if (ret != ESP_GATT_OK){
                    ESP_LOGE(BLE_TAGS::TAG_GATTS, "esp_ble_gattc_get_attr_count error");
                    break;
                }
                if (count > 0) {
                    ESP_LOGI(BLE_TAGS::TAG_GATTS, "Descriptor count %d", count);
                    // allocate memory for descriptor element result
                    descr_elem_result = (esp_gattc_descr_elem_t *) malloc(sizeof(esp_gattc_descr_elem_t) * count);
                    if (!descr_elem_result){
                        ESP_LOGE(BLE_TAGS::TAG_GATTS, "malloc error, gattc no mem");
                        break;
                    }
                    else {
                        // get the descriptor by characteristic handle
                        ret = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                    _gattc_profile_inst.conn_id,
                                                                    p_data->reg_for_notify.handle,
                                                                    _remote_descr_uuid,
                                                                    descr_elem_result,
                                                                    &count);
                        // check if the descriptor was found
                        if (ret != ESP_GATT_OK){
                            // if not found, log error and free the allocated memory
                            ESP_LOGE(BLE_TAGS::TAG_GATTS, "esp_ble_gattc_get_descr_by_char_handle error");
                            free(descr_elem_result);
                            descr_elem_result = nullptr;
                            break;
                        }

                        // check if the descriptor was found by uuid
                        if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_128 &&
                            memcmp(descr_elem_result[0].uuid.uuid.uuid128, _remote_descr_uuid.uuid.uuid128, sizeof(_remote_descr_uuid.uuid.uuid128)) == 0){
                            // write the descriptor value to enable indication
                            ret = esp_ble_gattc_write_char_descr( gattc_if,
                                                            _gattc_profile_inst.conn_id,
                                                            descr_elem_result[0].handle,
                                                            sizeof(BLE_Defaults::INDICATION),
                                                            (uint8_t *)&BLE_Defaults::INDICATION,
                                                            ESP_GATT_WRITE_TYPE_NO_RSP,
                                                            ESP_GATT_AUTH_REQ_NONE);
                            // check if the write was successful
                            if (ret != ESP_GATT_OK){
                                ESP_LOGE(BLE_TAGS::TAG_GATTS, "esp_ble_gattc_write_char_descr error");
                            }
                            else {
                                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Write descriptor successfully");
                            }
                        }

                        // free the temporary allocated memory
                        free(descr_elem_result);
                        descr_elem_result = nullptr;
                    }
                }
                else{
                    ESP_LOGE(BLE_TAGS::TAG_GATTS, "decsr not found");
                }
            }  
        break;

        //------------------------------------------------------------------------------------------------------------
        // GATT notify event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_NOTIFY_EVT:
            // check if the notification was successful
            if (p_data->notify.is_notify){
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Notification received");
            }else{
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Indication received");
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Received value: %.*s", p_data->notify.value_len, _char_recv_buffer);

            // Clear message buffer
            MessageHandler::Message msg;
            std::fill(msg.begin(), msg.end(), '\0');

            if (std::all_of(p_data->notify.value, p_data->notify.value + p_data->notify.value_len, [](uint8_t x) { return x == 0; })) {
                //receiving empty message to check timeout
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Received empty message, ignoring");
                break;
            }
            else {
                 // Copy the received data into the message buffer
                std::copy(p_data->notify.value, p_data->notify.value + p_data->notify.value_len, msg.begin());
                // send received message to message queue for data parser
                _msgHandler->send(MessageHandler::QueueType::DATA_PARSER_QUEUE, msg, MessageHandler::ParserMessageID::MSG_ID_BLE);
            }
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT write descriptor event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_WRITE_DESCR_EVT:
            // check if the write descriptor was successful
            if (p_data->write.status != ESP_GATT_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Descriptor write failed, status %x", p_data->write.status);
                break;
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Descriptor write successfully");
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT service changed event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_SRVC_CHG_EVT:
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service change!");
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT write characteristic event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_WRITE_CHAR_EVT:
            // check if the write characteristic was successful
            if (p_data->write.status != ESP_GATT_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "Caracteristic write failed, status");
                break;
            }
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Characteristic write successfully");
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // GATT Disconnect event
        //------------------------------------------------------------------------------------------------------------
        case ESP_GATTC_DISCONNECT_EVT:
            // log reason for disconnection
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Disconnected, conn_id %d, reason %d", p_data->disconnect.conn_id, p_data->disconnect.reason);
            // set connected & server flags to false
            _is_connected = false;
            _get_server = false;
            // delete the connection ID service handle and characteristic handle
            _gattc_profile_inst.conn_id = 0;
            _gattc_profile_inst.service_start_handle = 0;
            _gattc_profile_inst.char_handle = 0;
            _gattc_profile_inst.service_end_handle = 0;

            // start scanning again after disconnection
            ret = esp_ble_gap_start_scanning(BLE_Defaults::SCAN_DURATION);
            if(ret != ESP_OK){
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "start scan failed, error code = %x", ret);
            }
            else {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "start scan successfully");
            }
            // send disconnection message to UART interface
            std::fill(msg.begin(), msg.end(), '\0');
            std::copy(BLE_Defaults::BT_status_DC.begin(), BLE_Defaults::BT_status_DC.end(), msg.begin());
            msg[BLE_Defaults::BT_status_DC.length()] = '\n';
            _msgHandler->send(MessageHandler::QueueType::UART_QUEUE, msg, MessageHandler::ParserMessageID::MSG_ID_BLE);
        break;
        
        //------------------------------------------------------------------------------------------------------------
        // default case for unhandled events
        //------------------------------------------------------------------------------------------------------------
        default:
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "Unhandled GATTC event: %s", get_gattc_event_name(event));
        break;
    }
}

void BLE_Client::send(const char *data) {
    // Check if the client is connected to the server
    if (_is_connected) {
        // Send data to the BLE server
        esp_err_t ret = esp_ble_gattc_write_char(_gattc_profile_inst.gattc_if,
                                                _gattc_profile_inst.conn_id,
                                                _gattc_profile_inst.char_handle,
                                                strlen(data),
                                                (uint8_t *)data,
                                                ESP_GATT_WRITE_TYPE_NO_RSP,
                                                ESP_GATT_AUTH_REQ_NONE);
        if (ret != ESP_OK) {
            ESP_LOGE(BLE_TAGS::TAG_CLIENT, "Failed to send data, error code = %d", ret);
        } else {
            ESP_LOGI(BLE_TAGS::TAG_CLIENT, "Data sent successfully: %s", data);
        } 
    }
    else {
        ESP_LOGE(BLE_TAGS::TAG_CLIENT, "Client is not connected to the server, cannot send data");
    }
}

void BLE_Client::sendTask(MessageHandler* msgHandler) {
    // Get message from the queue and send it to the BLE server
    MessageHandler::Message msg;

    if (msgHandler->receive(MessageHandler::QueueType::BLE_QUEUE, msg)) {
        this->send(msg.data());
    }
    else {
        ESP_LOGW(BLE_TAGS::TAG_CLIENT, "Failed to receive message from queue");
       // send empty message to the client
       const char data[20] = {0};
       this->send(data);
    }
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