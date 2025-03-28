#ifndef BLE_CONFIG_HPP
#define BLE_CONFIG_HPP


#include "Bluetooth.hpp"

#if 0

static uint8_t CHAR_UUID[16] = {
  0xF1, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

static uint8_t DESCR_UUID[16] = {
  0xF2, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

//atic uint16_t service_handle;
static uint16_t char_handle;
static uint16_t descr_handle;



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


// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data

#endif

#endif // BLE_CONFIG_HPP