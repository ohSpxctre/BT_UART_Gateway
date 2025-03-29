/**
* @file BLE_Server.hpp
* @brief BLE Server class inheriting from Bluetooth.
*/

//idf.py flash wenn error weben odb server

#ifndef BLE_SERVER_HPP
#define BLE_SERVER_HPP

#include "Bluetooth.hpp"
#include <cstdint> // Include for fixed-width integer types


constexpr char device_name[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server";

constexpr uint16_t PROFILE_APP_ID = 0x00;
constexpr uint16_t PROFILE_NUM = 1;
constexpr uint8_t SERVICE_INST_ID = 0;

constexpr uint8_t SERVICE_UUID[16] = {
  0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
  0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
};

//-----------------------------------------------------
// idea to maybe use constexpr helper functions to generate all the structs?
//------------------------------------------------------

constexpr esp_gatt_srvc_id_t service_id_config = {
  .id = {
      .uuid = {
          .len = ESP_UUID_LEN_128,   // Specify that this is a 128-bit UUID
          .uuid = { .uuid128 = {     // Copy your 128-bit UUID
              0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
              0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
          }}
      },
      .inst_id = SERVICE_INST_ID,  // Usually set to 0 unless multiple instances are needed
  },
  .is_primary = true // Define it as a primary service
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


constexpr esp_ble_adv_data_t adv_data_config = {
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
  .p_service_uuid = const_cast<uint8_t*>(SERVICE_UUID),
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// optional!! is sent upon scan request
// scan response data
constexpr esp_ble_adv_data_t scan_rsp_data_config = {
  .set_scan_rsp = true,
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
  .p_service_uuid =  const_cast<uint8_t*>(SERVICE_UUID),
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

/**
* @class BLE_Server
* @brief BLE Server implementation.
*/

class BLE_Server : public Bluetooth {

private:

  static BLE_Server* instance; // Static instance pointer

  esp_ble_adv_params_t _adv_params;
  esp_ble_adv_data_t _adv_data;
  esp_ble_adv_data_t _scan_rsp_data;

  uint8_t _adv_config_done;
  uint8_t _scan_rsp_config_done;

  esp_gatt_srvc_id_t _service_id;

  uint16_t _service_handle;

  static void gatts_event_handler (esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t *);
  static void gap_event_handler(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

  void handle_event_gatts(esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t *);
  void handle_event_gap(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

public:
    BLE_Server( esp_ble_adv_params_t adv_params = adv_params_config,
                esp_ble_adv_data_t adv_data = adv_data_config,
                esp_ble_adv_data_t scan_rsp_data = scan_rsp_data_config,

                uint8_t adv_config_done = 0,
                uint8_t scan_rsp_config_done = 0,
                esp_gatt_srvc_id_t service_id = service_id_config,
                uint16_t service_handle = 0
              );

    ~BLE_Server();

    void connSetup(void) override;
    void send(const std::string &data) override;
    std::string receive() override;
};



#endif // BLE_SERVER_HPP