/**
 * @file BLE_Server.cpp
 * @brief Implementation of the BLE_Server class for managing BLE server functionality.
 *
 * This file contains the implementation of the BLE_Server class, which provides
 * functionality for initializing and managing a BLE server, handling GAP and GATT
 * events, and sending/receiving data over BLE. The class uses the ESP-IDF framework
 * for BLE operations and integrates with a MessageHandler for communication with
 * other system components.
 *
 * Features:
 * - BLE initialization and configuration
 * - GAP and GATT event handling
 * - Advertising and connection management
 * - Sending and receiving data via BLE characteristics
 * - Integration with a message queue for inter-component communication
 *
 * Usage:
 * - Instantiate the BLE_Server class with appropriate parameters.
 * - Call `connSetup()` to initialize the BLE server and register callbacks.
 * - Use `send()` to send data to connected BLE clients.
 * - Use `sendTask()` to process messages from the BLE queue and send them to clients.
 *
 * @author hoyed1
 * @date 09.04.2025
 */

#include "BLE_Server.hpp"
#include <iostream>

#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Define the static instance pointer
BLE_Server* BLE_Server::Server_instance = nullptr;

BLE_Server::BLE_Server(esp_ble_adv_params_t adv_params, esp_ble_adv_data_t adv_data, MessageHandler* msgHandler) :
    _adv_params(adv_params),
    _adv_data(adv_data)
{
    // Set the message handler
    _msgHandler = msgHandler;
    
    // Initialize the static instance pointer
    if (Server_instance != nullptr) {
        ESP_LOGE(BLE_TAGS::TAG_SERVER, "BLE_Server instance already exists!");
        return;
    }
    //save the pointer to the insantiated object
    Server_instance = this;

    // Set UUID in advertising data
    _adv_data.service_uuid_len = _gatts_profile_inst.service_id.id.uuid.len;
    _adv_data.p_service_uuid = (uint8_t *)_gatts_profile_inst.service_id.id.uuid.uuid.uuid128;

    //Set data for advertising data
    _adv_data.service_data_len = sizeof(_adv_data_buffer);
    _adv_data.p_service_data = _adv_data_buffer;

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
    ESP_LOGI(BLE_TAGS::TAG_SERVER, "BLE Server Deinitialized");
    _msgHandler = nullptr;
}


void BLE_Server::connSetup() {
    // Temporary Message to send bluetooth status to UART interface
    //MessageHandler::Message msg;
    //// fill message with 0
    //std::fill(msg.begin(), msg.end(), '\0');
    //// copy not connected message
    //std::copy(BLE_Defaults::BT_status_NC.begin(), BLE_Defaults::BT_status_NC.end(), msg.begin());
    //msg[BLE_Defaults::BT_status_NC.length()] = '\n';
    //// Send not connected message to UART interface
    //_msgHandler->send(MessageHandler::QueueType::UART_QUEUE, msg, MessageHandler::ParserMessageID::MSG_ID_BLE);
    
    // Register callback functions
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    // Register the GATT server application
    esp_ble_gatts_app_register(BLE_Defaults::PROFILE_APP_ID);
    // Set the local MTU size
    esp_ble_gatt_set_local_mtu(_gatts_profile_inst.local_mtu);
    ESP_LOGI(BLE_TAGS::TAG_SERVER, "BLE Server initialized and callbacks registered");
}

 void BLE_Server::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (Server_instance) {
        // Call the static event handle method for GAP events
        Server_instance->handle_event_gap(event, param);
    }
}

void BLE_Server::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (Server_instance) {
        // Call the static event handle method for GATT events
        Server_instance->handle_event_gatts(event, gatts_if, param);
    }
}


void BLE_Server::handle_event_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGW(BLE_TAGS::TAG_SERVER, "GAP event: %s", get_gap_event_name(event));
    switch (event)
    {
    //--------------------------------------------------------------------------------------------------------
    // GAP event for setting advertising data
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        // start advertising after setting adv data successfully
        esp_ble_gap_start_advertising(&_adv_params);
    break;

    //--------------------------------------------------------------------------------------------------------
    // GAP event for starting advertising complete
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        // advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(BLE_TAGS::TAG_GAP, "Advertising start failed, status %d", param->adv_start_cmpl.status);
        } else {
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Advertising start successfully");
        }
        break;
    //--------------------------------------------------------------------------------------------------------
    // GAP event for stopping advertising complete
    //--------------------------------------------------------------------------------------------------------
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        //advertising stop complete event to indicate advertising stop successfully or failed
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            //if advertising stop failed, set the advertising flag to true
            _is_advertising = true;
            ESP_LOGE(BLE_TAGS::TAG_GAP, "Advertising stop failed: %s", esp_err_to_name(param->adv_stop_cmpl.status));
        } else {
            // if advertising stop successfully, set the advertising flag to false
            _is_advertising = false;
            ESP_LOGI(BLE_TAGS::TAG_GAP, "Advertising stopped successfully");
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
        ESP_LOGI(BLE_TAGS::TAG_GAP, "Packet length set successfully");
    break;
    
    //--------------------------------------------------------------------------------------------------------
    // GAP default case
    //--------------------------------------------------------------------------------------------------------
    default:
        ESP_LOGE(BLE_TAGS::TAG_GAP, "Unhandled GAP event: %s", get_gap_event_name(event));
    break;
    }
}


void BLE_Server::handle_event_gatts(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    
    // temporary variable to save connection parameters before updating connection parameters
    esp_ble_conn_update_params_t conn_params = {0};

    // temporary variable for storing return values of function calls
    esp_err_t ret = ESP_OK;

    // temporary variables
    const uint8_t *prf_char;
    uint16_t length = 0;
    
    //Loging event type
    ESP_LOGW(BLE_TAGS::TAG_SERVER, "GATT event: %s", get_gatts_event_name(event));

    switch (event)
    {
    //--------------------------------------------------------------------------------------------------------
    // GATT Server reg event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_REG_EVT:
        // save the bluetooth interfache after app registration
        _gatts_profile_inst.gatts_if = gatts_if;        
        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Device Name: %s", BLE_Defaults::DEVICE_NAME_SERVER);
        // set device name
        ret = esp_ble_gap_set_device_name(BLE_Defaults::DEVICE_NAME_SERVER);

        //check if the device name was set successfully
        if (ret){
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "set device name failed, error code = %x", ret);
        }
        
        // set advertising data
        ret = esp_ble_gap_config_adv_data(&_adv_data);
        // Check if the advertising data was set successfully
        if (ret) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "set adv data failed, error code = %x", ret);
        }

        // creating the gatt service
        ret = esp_ble_gatts_create_service(gatts_if, &_gatts_profile_inst.service_id, BLE_Defaults::HANDLE_NUM);
        // Check if the service was created successfully
        if(ret) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "create service failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server create service event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service create, status %d, service_handle %d", param->create.status, param->create.service_handle);
        
        //saving the service handle
        _gatts_profile_inst.service_handle = param->create.service_handle;
        //starting the service
        ret = esp_ble_gatts_start_service(_gatts_profile_inst.service_handle);
        if (ret)
        {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "start service failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Write event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Characteristic write, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        // if not prepare write event
        if (!param->write.is_prep) {

            ESP_LOGI(BLE_TAGS::TAG_GATTS, "value len %d, value ", param->write.len);
            ESP_LOG_BUFFER_HEX(BLE_TAGS::TAG_GATTS, param->write.value, param->write.len);

            // check if write handle & value is for descriptor
            if (_gatts_profile_inst.descr_handle == param->write.handle && param->write.len == 2) {
                _gatts_profile_inst.descr_value = param->write.value[1]<<8 | param->write.value[0];
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Descriptor value: %d", _gatts_profile_inst.descr_value);
                // decide if the write event is for notify or indication
                switch (_gatts_profile_inst.descr_value) {
                    case 0x0001: // Notification enable
                        if (_gatts_profile_inst.property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Notification enabled should not happen!");
                            }
                        break;
                    case 0x0002: // Indication enable
                        if (_gatts_profile_inst.property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Indication enabled");
                            }
                        break;
                    case 0x0000: // both disable
                        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Notification/Indication disabled");
                        break;
                    default:
                        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Unknown descriptor value: %d", _gatts_profile_inst.descr_value);
                        break;
                }
            }
            // check if write handle is charakteristic handle
            else if (_gatts_profile_inst.char_handle == param->write.handle) {
                std::fill(_char_data_buffer, _char_data_buffer + sizeof(_char_data_buffer), 0);
                std::copy(param->write.value, param->write.value + param->write.len, _char_data_buffer);
                if (std::all_of(_char_data_buffer, _char_data_buffer + sizeof(_char_data_buffer), [](uint8_t x) { return x == 0; })) {
                    //receiving empty message to check timeout
                    ESP_LOGI(BLE_TAGS::TAG_GATTS, "Received empty message, ignoring");
                    break;
                }
                else {
                
                    _char_data_buffer[param->write.len] = '\0';
                    _new_data_rcv = true;
                }
            }
            else {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Unknown handle: %d", param->write.handle);
            }
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT MTU exchange event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_MTU_EVT:
        // Exchange of MTU size between server and client
        ESP_LOGI(BLE_TAGS::TAG_GATTS, "MTU exchange, MTU %d", param->mtu.mtu);
        // save the MTU size
        _gatts_profile_inst.local_mtu = param->mtu.mtu;
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server start service event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_START_EVT:
        // check if start service was successful
        if (param->start.status != ESP_GATT_OK) {    
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service start failed: %s", esp_err_to_name(param->start.status));
        } else {
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service started successfully");     
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service handle: %d", param->start.service_handle);
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Service UUID: %s", _gatts_profile_inst.service_id.id.uuid.uuid.uuid128);
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Characteristic UUID: %s", _gatts_profile_inst.char_uuid.uuid.uuid128);

            // add characteristic to the service
            ret = esp_ble_gatts_add_char(_gatts_profile_inst.service_handle,
                                            &_gatts_profile_inst.char_uuid,
                                            _gatts_profile_inst.perm,
                                            _gatts_profile_inst.property,
                                            &_char_value,
                                            NULL);
            
            // check if the characteristic was added successfully
            if (ret == ESP_OK) {
                ESP_LOGI(BLE_TAGS::TAG_GATTS, "Char add okey, result: %d", ret);
            }
            else {
                ESP_LOGE(BLE_TAGS::TAG_GATTS, "add char failed, error code = %x", ret);
            }
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server add char event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_ADD_CHAR_EVT:
        // check if the characteristic was added successfully
        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Characteristic add, status %d, attr_handle %d, service_handle %d",
            param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        // save the charakteristic handle
        _gatts_profile_inst.char_handle = param->add_char.attr_handle;
        // check if the characteristic handle is valid
        ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (ret == ESP_FAIL){
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "ILLEGAL HANDLE");
        }
        else{
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Char handle: %d", param->add_char.attr_handle);
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Char value length: %d", length);
        }
        // add charakteristic descriptor to the service
        ret = esp_ble_gatts_add_char_descr(_gatts_profile_inst.service_handle,
                                            &_gatts_profile_inst.descr_uuid,
                                            _gatts_profile_inst.perm,
                                            &_descr_value,
                                            NULL);
        // check if the descriptor was added successfully
        if (ret) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "Add descriptor failed, error code = %x", ret);
        }
        else {
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Descriptor added successfully, handle: %d", param->add_char.attr_handle);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Server add char descriptor event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        // check if the descriptor was added successfully
        _gatts_profile_inst.descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(BLE_TAGS::TAG_GATTS, "Descriptor add, status %d, attr_handle %d, service_handle %d",
                param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Connect event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CONNECT_EVT:
        // set connected flag to true
        _is_connected = true;
        _conn_state_change = true;
        _conn_state = BLE_Defaults::ConnectionState::CONNECTED;

        // save the connection id
        _gatts_profile_inst.conn_id = param->connect.conn_id;
        // send mtu request to the client
        esp_ble_gattc_send_mtu_req(gatts_if, _gatts_profile_inst.local_mtu);
        // save remote_bda
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));

        // set connection parameters
        conn_params.latency = 0;
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.timeout = BLE_Defaults::TIMEOUT_DEFAULT;    // timeout = 400*10ms = 4000ms
        // update the connection parameters
        ret = esp_ble_gap_update_conn_params(&conn_params);
        // check if the connection parameters were updated successfully
        if (ret) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "Update connection params failed, error code = %x", ret);
        }
        // stop advertising after connection is established
        ret = esp_ble_gap_stop_advertising();
        if (ret) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "Stop advertising failed, error code = %x", ret);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT Disconnect event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_DISCONNECT_EVT:
        // set connected flag to false
        _is_connected = false;
        // start advertising after disconnection
        ret = esp_ble_gap_start_advertising(&_adv_params);
        // check if the advertising was started successfully
        if (ret) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "Start advertising failed");
        }
        // set advertising flag to true
        _is_advertising = true;
        _conn_state_change = true;
        _conn_state = BLE_Defaults::ConnectionState::DISCONNECTED;
    break;

    //--------------------------------------------------------------------------------------------------------
    // GATT confirmation event
    //--------------------------------------------------------------------------------------------------------
    case ESP_GATTS_CONF_EVT:
        // check if the status after confirmation event
        if (param->conf.status != ESP_GATT_OK) {
            ESP_LOGE(BLE_TAGS::TAG_GATTS, "Confirm failed, status %d", param->conf.status);
        } else {
            ESP_LOGI(BLE_TAGS::TAG_GATTS, "Confirm success, handle %d", param->conf.handle);
        }
    break;

    //--------------------------------------------------------------------------------------------------------
    // default case
    //--------------------------------------------------------------------------------------------------------
    default:
        ESP_LOGE(BLE_TAGS::TAG_GATTS, "Unhandled GATTS event: %s", get_gatts_event_name(event));
        break;
    }
}

void BLE_Server::send(const char * data) {
    // save data to the buffer
    std::fill(_char_value.attr_value, _char_value.attr_value + _char_value.attr_len, 0);
    _char_value.attr_value = (uint8_t *)data;

    if (_is_connected) {
        // if indications are enabled
        if (_gatts_profile_inst.descr_value == BLE_Defaults::INDICATION){
            // Send indication to the client
            esp_ble_gatts_send_indicate(_gatts_profile_inst.gatts_if, _gatts_profile_inst.conn_id, _gatts_profile_inst.char_handle, _char_value.attr_len, _char_value.attr_value, true);
        }
        // if notifications are enabled
        else if (_gatts_profile_inst.descr_value == BLE_Defaults::NOTIFICATION){
            //send notification to the client
            esp_ble_gatts_send_indicate(_gatts_profile_inst.gatts_if, _gatts_profile_inst.conn_id, _gatts_profile_inst.char_handle, _char_value.attr_len, _char_value.attr_value, false);
        }
        else{
            // if notifications and indications are disabled, do not send data
            ESP_LOGI(BLE_TAGS::TAG_SERVER, "Notification/Indication disabled, not sending data");
        }
        
    } else {
        // if not connected to a client, do not send data
        ESP_LOGI(BLE_TAGS::TAG_SERVER, "Cannot send data, not connected to a client");
    }
}


void BLE_Server::logBleState (MessageHandler * msgHandler) {
    // create message
    MessageHandler::Message message;
    // fill message with 0
    std::fill(message.begin(), message.end(), '\0');
    
    if (_conn_state_change) {
        // reset the flag
        _conn_state_change = false;
        switch (_conn_state) {
            case BLE_Defaults::ConnectionState::DISCONNECTED:
                std::fill(message.begin(), message.end(), '\0');
                // copy disconnected message
                std::copy(BLE_Defaults::BT_status_DC.begin(), BLE_Defaults::BT_status_DC.end(), message.begin());
                message[BLE_Defaults::BT_status_DC.length()] = '\n';
                // send message to the UART interface
                msgHandler->send(MessageHandler::QueueType::UART_QUEUE, message, MessageHandler::ParserMessageID::MSG_ID_BLE);
                break;
            case BLE_Defaults::ConnectionState::CONNECTED:
                // fill message with 0
                std::fill(message.begin(), message.end(), '\0');
                // copy connected message
                std::copy(BLE_Defaults::BT_status_C.begin(), BLE_Defaults::BT_status_C.end(), message.begin());
                message[BLE_Defaults::BT_status_C.length()] = '\n';
                // send message to the UART interface
                msgHandler->send(MessageHandler::QueueType::UART_QUEUE, message, MessageHandler::ParserMessageID::MSG_ID_BLE);
            break;
            case BLE_Defaults::ConnectionState::NOT_CONNECTED:
                // fill message with 0
                std::fill(message.begin(), message.end(), '\0');
                // copy not connected message
                std::copy(BLE_Defaults::BT_status_NC.begin(), BLE_Defaults::BT_status_NC.end(), message.begin());
                message[BLE_Defaults::BT_status_NC.length()] = '\n';
                // send message to the UART interface
                msgHandler->send(MessageHandler::QueueType::UART_QUEUE, message, MessageHandler::ParserMessageID::MSG_ID_BLE);
            break;
        }
    }
}


bool BLE_Server::receive(uint8_t *data) {
    // Check if new data is received
    if (_new_data_rcv) {
        // Clear the new data received flag
        _new_data_rcv = false;
        std::copy(_char_data_buffer, _char_data_buffer + sizeof(_char_data_buffer), data);
        return true;
    } else {
        // If no new data is received, return false
        return false;
    }
}

void BLE_Server::receiveTask(MessageHandler* msgHandler) {

    uint8_t rcv_buffer[BLE_Defaults::MTU_DEFAULT-3] = {0};
    MessageHandler::Message msg;    
   
    if (this->receive(rcv_buffer)) {
        // fill message with 0
        std::fill(msg.begin(), msg.end(), '\0');
        // copy received message
        std::copy(rcv_buffer, rcv_buffer + sizeof(rcv_buffer), msg.begin());
        //Send received message to message  queue for data parser 
        msgHandler->send(MessageHandler::QueueType::DATA_PARSER_QUEUE, msg, MessageHandler::ParserMessageID::MSG_ID_BLE);
    }

    this->logBleState(msgHandler);

}

void BLE_Server::sendTask(MessageHandler* msgHandler) {
    // create message
    MessageHandler::Message message;

    if (msgHandler->receive(MessageHandler::QueueType::BLE_QUEUE, message)) {
        // Process the received message
        const char* data(message.data());
        ESP_LOGI(BLE_TAGS::TAG_SERVER, "Sending data: %s", data);
        this->send(data);
    } else {
        ESP_LOGI(BLE_TAGS::TAG_SERVER, "No message to send");
        // send empty message to the client
        const char data[20] = {0};
        this->send(data);
    }
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
