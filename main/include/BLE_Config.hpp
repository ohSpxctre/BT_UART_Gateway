/**
 * @file BLE_Config.hpp
 * @brief Configuration file containing default values and structures for BLE server and client.
 *
 * This header file defines constants, default parameters, and structures used for configuring
 * the BLE (Bluetooth Low Energy) server and client. It includes settings for advertising,
 * GATT (Generic Attribute Profile) server and client, UUIDs, MTU size, and other BLE-related
 * parameters.
 *
 * The file is organized into the following sections:
 * - BLE Server Definitions: Contains default values and structures for the BLE server, including
 *   GATT service and characteristic configurations, advertising parameters, and server profile
 *   instance structure.
 * - BLE Client Definitions: Contains default values and structures for the BLE client, including
 *   scan parameters, remote service and characteristic UUIDs, and client profile instance structure.
 *
 * @note This file is designed for use with the ESP-IDF framework and relies on its BLE APIs.
 *
 * @author hoyed1
 * @date 09.04.2025
 */
#ifndef BLE_CONFIG_HPP
#define BLE_CONFIG_HPP


#include "Bluetooth.hpp"

/**
 * @brief Tag used for logging messages related to the BLE server.
 */
constexpr const char* TAG_SERVER = "BLE_Server";

/**
 * @brief Tag used for logging messages related to the BLE client.
 */
constexpr const char* TAG_CLIENT = "BLE_Client";

/**
 * @brief Tag used for logging messages related to GAP (Generic Access Profile) events.
 */
constexpr const char* TAG_GAP = "GAP";

/**
 * @brief Tag used for logging messages related to GATT (Generic Attribute Profile) server events.
 */
constexpr const char* TAG_GATTS = "GATT";

/**
 * @brief Default name for the BLE server device during advertising.
 */
constexpr char DEVICE_NAME_SERVER[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server";

/**
 * @brief Default name for the BLE client device during advertising.
 */
constexpr char DEVICE_NAME_CLIENT[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Client";

/**
 * @brief Application ID for the BLE profile.
 */
constexpr uint16_t PROFILE_APP_ID = 0x00;

/**
 * @brief Number of handles allocated for the GATT server.
 */
constexpr uint16_t HANDLE_NUM = 10;

/**
 * @brief Instance ID for the BLE service.
 */
constexpr uint8_t SERVICE_INST_ID = 0;

/**
 * @brief Instance ID for the BLE characteristic.
 */
constexpr uint16_t CHAR_INST_ID = 0;

/**
 * @brief Default Maximum Transmission Unit (MTU) size for BLE communication.
 */
constexpr uint16_t MTU_DEFAULT = 259;

/**
 * @brief Default timeout value for BLE operations, in milliseconds multiplied by 10.
 */
constexpr uint16_t TIMEOUT_DEFAULT = 200;

/**
 * @brief Bitmask value to enable BLE notifications.
 */
constexpr uint16_t NOTIFICATION = 0x0001;

/**
 * @brief Bitmask value to enable BLE indications.
 */
constexpr uint16_t INDICATION = 0x0002;

/**
 * @brief Status message indicating that Bluetooth is not connected.
 */
constexpr std::string_view BT_status_NC = "Bluetooth not connected";

/**
 * @brief Status message indicating that Bluetooth is connected.
 */
constexpr std::string_view BT_status_C = "Bluetooth connected";

/**
 * @brief Status message indicating that Bluetooth is disconnected.
 */
constexpr std::string_view BT_status_DC = "Bluetooth disconnected";

/**
 * @brief Default UUID for the BLE service.
 */
constexpr esp_bt_uuid_t SERVICE_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
    0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
  }}
};

/**
 * @brief Default UUID for the BLE characteristic.
 */
constexpr esp_bt_uuid_t CHAR_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
    0xF1, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
  }}
};

/**
 * @brief Default UUID for the BLE characteristic descriptor.
 */
constexpr esp_bt_uuid_t DESCR_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
    0xF2, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
  }}
};


//---------------------------------------------------------------------------------------------------------
// GATT Server specific definitions
//---------------------------------------------------------------------------------------------------------

/**
 * @brief Default GATT Service ID
 * This is the GATT interface ID for the server profile.
 */
constexpr esp_gatt_srvc_id_t SERVICE_ID_DEFAULT = {
  .id = {
      .uuid = SERVICE_UUID_DEFAULT,
      .inst_id = SERVICE_INST_ID,  // Usually set to 0 unless multiple instances are needed
  },
  .is_primary = true // Define it as a primary service
};

/**
 * @brief Default characteristic value.
 */
constexpr esp_attr_value_t GATTS_CHAR_VALUE_DEFAULT = {
  .attr_max_len = ESP_GATT_MAX_ATTR_LEN,
  .attr_len = 0,
  .attr_value = nullptr
};

/**
 * @brief Default advertising parameters.
 * This structure contains the parameters for BLE advertising, including interval, type, and filter policy.
 */

constexpr esp_ble_adv_params_t ADV_PARAMS_DEFAULT = {
  .adv_int_min = 0x190,                                   //minimum intervall is 100ms
  .adv_int_max = 0x320,                                   //max interval is 200ms
  .adv_type = ADV_TYPE_IND,                               //Connectable undirected advertisement (any device can connect)
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,                  //own address is public
  //.peer_addr = ;                                        //not used because any device can connect
  //.peer_addr_type = ;                                   //not used ...
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY //Allow scan and connect requests from any device.
};

/**
 * @brief Default advertising data structure.
 * This structure contains the advertising data that will be sent out during BLE advertising.
 */

constexpr esp_ble_adv_data_t ADV_DATA_DEFAULT = {
  .set_scan_rsp = false,
  .include_name = true,
  .include_txpower = false,
  .min_interval = 0x190, //slave connection min interval, Time = min_interval * 1.25 msec
  .max_interval = 0x320, //slave connection max interval, Time = max_interval * 1.25 msec
  .appearance = 0x00,
  .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
  .p_manufacturer_data =  NULL, //&test_manufacturer[0],
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = 0,
  .p_service_uuid = NULL,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

/**
 * @brief GATT Server profile instance structure.
 */

struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_attr_control_t char_resp_ctrl;

  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  uint16_t descr_value;
  esp_bt_uuid_t descr_uuid;
  uint16_t local_mtu;
};


//---------------------------------------------------------------------------------------------------------
// GATT Client specific definitions
//---------------------------------------------------------------------------------------------------------

/**
  * @brief UUID of the remote BLE service to filter for during scanning.
  */
 constexpr esp_bt_uuid_t REMOTE_FILTER_SERVICE_UUID = SERVICE_UUID_DEFAULT;
 
 /**
  * @brief UUID of the characteristic to interact with on the remote BLE device.
  */
 constexpr esp_bt_uuid_t REMOTE_FILTER_CHAR_UUID = CHAR_UUID_DEFAULT;
 
 /**
  * @brief UUID of the descriptor associated with the characteristic.
  */
 constexpr esp_bt_uuid_t REMOTE_DESCR_UUID_DEFAULT = DESCR_UUID_DEFAULT;
 
 /**
  * @brief BLE scan duration in seconds.
  */
 constexpr uint32_t SCAN_DURATION = 30; // seconds
 
 /**
  * @brief Default scan parameters for active BLE scanning.
  */
 constexpr esp_ble_scan_params_t BLE_SCAN_PARAMS_DEFAULT = {
     .scan_type              = BLE_SCAN_TYPE_ACTIVE,
     .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
     .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
     .scan_interval          = 0x50,
     .scan_window            = 0x30,
     .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
 };
 
 /**
  * @brief Structure to hold GATT client profile information.
  */
 struct gattc_profile_inst {
     esp_gattc_cb_t gattc_cb;            /**< Callback for GATT client events */
     uint16_t gattc_if;                  /**< GATT interface ID */
     uint16_t app_id;                    /**< Application ID */
     uint16_t conn_id;                   /**< Connection ID */
     uint16_t service_start_handle;     /**< Start handle of discovered service */
     uint16_t service_end_handle;       /**< End handle of discovered service */
     uint16_t char_handle;              /**< Handle of the target characteristic */
     esp_bd_addr_t remote_bda;          /**< Bluetooth address of the remote device */
     uint16_t local_mtu;                /**< Local MTU size */
 };

#endif // BLE_CONFIG_HPP