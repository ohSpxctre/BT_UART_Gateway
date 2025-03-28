/**
* @file BLE_Server.hpp
* @brief BLE Server class inheriting from Bluetooth.
*/

#ifndef BLE_SERVER_HPP
#define BLE_SERVER_HPP

#include "Bluetooth.hpp"
#include "BLE_Config.hpp"
#include <cstdint> // Include for fixed-width integer types


#define TAG "BLE_Server"

constexpr char device_name[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server";

constexpr uint16_t PROFILE_APP_ID = 0x00;
constexpr uint16_t PROFILE_NUM = 1;

uint8_t SERVICE_UUID[16] = {
  0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

static uint8_t char_str[] = {0x11,0x22,0x33};

esp_attr_value_t gatts_char_value = {
  .attr_max_len = sizeof(char_str),
  .attr_len = sizeof(char_str),
  .attr_value = char_str
};



constexpr esp_ble_adv_params_t adv_params_config = {
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
* @class BLE_Server
* @brief BLE Server implementation.
*/

class BLE_Server : public Bluetooth {

private:

  uint8_t  _service_uuid128[16];
  esp_ble_adv_params_t _adv_params;
  uint8_t _adv_config_done;
  uint8_t _scan_rsp_config_done;
  uint16_t _service_handle;

  static void gatts_event_handler (esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t *);
  static void gap_event_handler(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

public:
    BLE_Server(uint8_t *service_uuid128 = SERVICE_UUID,
                esp_ble_adv_params_t adv_params = adv_params_config,
                uint8_t adv_config_done = 0,
                uint8_t scan_rsp_config_done = 0);

    ~BLE_Server();

    void connSetup(void) override;
    void send(const std::string &data) override;
    std::string receive() override;
};



#endif // BLE_SERVER_HPP