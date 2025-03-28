#ifndef BLE_CONFIG_HPP
#define BLE_CONFIG_HPP


#include "Bluetooth.hpp"

#define TAG "BLE_Server"
constexpr char device_name[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server";

constexpr uint16_t PROFILE_APP_ID = 0x00;
constexpr uint16_t PROFILE_NUM = 1;

static uint8_t SERVICE_UUID[16] = {
  0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

static uint8_t CHAR_UUID[16] = {
  0xF1, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

static uint8_t DESCR_UUID[16] = {
  0xF2, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

static uint16_t service_handle;
static uint16_t char_handle;
static uint16_t descr_handle;

static uint8_t char_str[] = {0x11,0x22,0x33};

esp_attr_value_t gatts_char_value = {
  .attr_max_len = sizeof(char_str),
  .attr_len = sizeof(char_str),
  .attr_value = char_str
};

uint8_t adv_config_done = 0;
uint8_t scan_rsp_config_done = 0;


/*
static esp_gatt_char_prop_t gatt_property = 0;

static esp_attr_value_t gatt_char_value = 
{
    .attr_max_len = GATT_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};
*/


/* only necessary if you want to have multiple profiles with different services
struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};
*/

esp_ble_adv_params_t adv_params = {
   .adv_int_min = 0x190,                                   //minimum intervall is 100ms
   .adv_int_max = 0x320,                                   //max interval is 200ms
   .adv_type = ADV_TYPE_IND,                               //Connectable undirected advertisement (any device can connect)
   .own_addr_type = BLE_ADDR_TYPE_PUBLIC,                  //own address is public
   //.peer_addr = ;                                        //not used because any device can connect
   //.peer_addr_type = ;                                   //not used ...
   .channel_map = ADV_CHNL_ALL,
   .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY //Allow scan and connect requests from any device.
};


// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data = {
  .set_scan_rsp = false,
  .include_name = true,
  .include_txpower = true,
  .min_interval = 0x190, //slave connection min interval, Time = min_interval * 1.25 msec
  .max_interval = 0x320, //slave connection max interval, Time = max_interval * 1.25 msec
  .appearance = 0x00,
  .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
  .p_manufacturer_data =  NULL, //&test_manufacturer[0],
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = sizeof(SERVICE_UUID),
  .p_service_uuid = SERVICE_UUID,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// optional!! is sent upon scan request
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
  .set_scan_rsp = true,
  .include_name = true,
  .include_txpower = true,
  .appearance = 0x00,
  .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
  .p_manufacturer_data =  NULL, //&test_manufacturer[0],
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = sizeof(SERVICE_UUID),
  .p_service_uuid = SERVICE_UUID,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};


#endif // BLE_CONFIG_HPP