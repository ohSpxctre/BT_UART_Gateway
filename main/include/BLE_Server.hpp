/**
* @file BLE_Server.hpp
* @brief BLE Server class inheriting from Bluetooth.
*/

#ifndef BLE_SERVER_HPP
#define BLE_SERVER_HPP

#include "Bluetooth.hpp"


/**
* @class BLE_Server
* @brief BLE Server implementation.
*/

constexpr uint16_t PROFILE_APP_ID = 0x00;
constexpr uint16_t PROFILE_NUM = 1;

static char test_device_name[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server";

constexpr uint8_t SERVICE_UUID128[16] = {
  0xed, 0x48, 0x46, 0x52,  // First 4 bytes (Little-Endian)
  0x49, 0x07,              // Next 2 bytes (Little-Endian)
  0xd4, 0x4e,              // Next 2 bytes (Little-Endian)
  0x8d, 0x61,              // Next 2 bytes (Little-Endian)
  0x4a, 0x07, 0xa0, 0x09, 0xe4, 0x9d // Last 6 bytes (Little-Endian)
};

//constexpr uint16_t GATTS_SERVICE_UUID_TEST_A  = 0x00FF;
//constexpr uint16_t GATTS_CHAR_UUID_TEST_A     = 0xFF01;
//constexpr uint16_t GATTS_DESCR_UUID_TEST_A    = 0x3333;
//constexpr uint16_t GATTS_NUM_HANDLE_TEST_A    = 4;
//
//constexpr uint16_t TEST_MANUFACTURER_DATA_LEN  = 17;
//
constexpr uint16_t GATT_CHAR_VAL_LEN_MAX = 0x40;
//
//constexpr uint16_t PREPARE_BUF_MAX_SIZE = 1024;

static uint8_t char1_str[] = {0x11,0x22,0x33};

static esp_gatt_char_prop_t gatt_property = 0;

static esp_attr_value_t gatt_char_value = 
{
    .attr_max_len = GATT_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};


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


class BLE_Server : public Bluetooth {

private:
  static void gatts_event_handler (esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t *);
  static void gap_event_handler(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

  struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];

  uint8_t adv_config_done = 0;
  uint8_t scan_rsp_config_done = 0;
  uint8_t adv_service_uuid128[32];

public:
    BLE_Server();
    ~BLE_Server();

    void connSetup(void) override;
    void send(const std::string &data) override;
    std::string receive() override;
};

#endif // BLE_SERVER_HPP