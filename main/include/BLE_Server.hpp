/**
* @file BLE_Server.hpp
* @brief BLE Server class inheriting from Bluetooth.
*/

//idf.py flash wenn error weben odb server

//-----------------------------------------------------
// idea to maybe use constexpr helper functions to generate all the structs?
//------------------------------------------------------

#ifndef BLE_SERVER_HPP
#define BLE_SERVER_HPP

#include "Bluetooth.hpp"
#include <cstdint> // Include for fixed-width integer types

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)


constexpr char CHAR_VALUE_DEFAULT [ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server_Default";

constexpr esp_gatt_srvc_id_t SERVICE_ID_DEFAULT = {
  .id = {
      .uuid = SERVICE_UUID_DEFAULT,
      .inst_id = SERVICE_INST_ID,  // Usually set to 0 unless multiple instances are needed
  },
  .is_primary = true // Define it as a primary service
};

constexpr esp_attr_value_t GATTS_CHAR_VALUE_DEFAULT = {
  .attr_max_len = ESP_GATT_MAX_ATTR_LEN,
  .attr_len = 0,
  .attr_value = nullptr
};

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

// optional!! is sent upon scan request
// scan response data
constexpr esp_ble_adv_data_t SCAN_RSP_DATA_DEFAULT = {
  .set_scan_rsp = true,
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
  .p_service_uuid =  NULL,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
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
  esp_attr_control_t char_resp_ctrl;

  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  uint16_t descr_value;
  esp_bt_uuid_t descr_uuid;
  uint16_t local_mtu;
};

typedef struct {
  uint8_t *prepare_buf;
  uint16_t prepare_len;
} prepare_type_env_t;

/**
* @class BLE_Server
* @brief BLE Server implementation.
*/

class BLE_Server : public Bluetooth {

private:

  static BLE_Server* Server_instance; // Static instance pointer

  uint8_t _adv_data_buffer[ESP_BLE_ADV_DATA_LEN_MAX] = "Hello World!";
  uint8_t _char_data_buffer[20] = "Hello Server!";
  uint8_t _descr_data_buffer[128] = "Characteristic Descriptor Data";
  uint8_t _char_rcv_buffer[MTU_DEFAULT-3] = {0};


  gatts_profile_inst _gatts_profile_inst = {
    .gatts_cb = gatts_event_handler,
    .gatts_if = ESP_GATT_IF_NONE,
    .app_id = PROFILE_APP_ID,
    .conn_id = 0,
    .service_handle = 0,
    .service_id = SERVICE_ID_DEFAULT,
    .char_handle = 0,
    .char_uuid = CHAR_UUID_DEFAULT,
    .char_resp_ctrl = ESP_GATT_AUTO_RSP,
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    .property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_INDICATE, 
    .descr_handle = 0,
    .descr_value = 0,
    .descr_uuid = DESCR_UUID_DEFAULT,
    .local_mtu = MTU_DEFAULT,
  };

  esp_ble_adv_params_t _adv_params;
  esp_ble_adv_data_t _adv_data;
  esp_ble_adv_data_t _scan_rsp_data;

  bool _is_advertising = false;
  bool _is_connected = false;

  uint8_t _adv_config_done = 0;

  esp_attr_value_t _char_value = {
    .attr_max_len = ESP_GATT_MAX_ATTR_LEN,
    .attr_len = sizeof(_char_data_buffer),
    .attr_value = _char_data_buffer
  };

  esp_attr_value_t _descr_value = {
    .attr_max_len = sizeof(_descr_data_buffer),
    .attr_len = sizeof(_descr_data_buffer),
    .attr_value = _descr_data_buffer
  };

  prepare_type_env_t _prepare_write_env = {
    .prepare_buf = NULL,
    .prepare_len = 0,
  };
  
  const char* get_gatts_event_name(esp_gatts_cb_event_t);
  const char* get_gap_event_name(esp_gap_ble_cb_event_t);

  static void gatts_event_handler (esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t *);
  static void gap_event_handler(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

  void handle_event_gatts(esp_gatts_cb_event_t, esp_gatt_if_t, esp_ble_gatts_cb_param_t *);
  void handle_event_gap(esp_gap_ble_cb_event_t, esp_ble_gap_cb_param_t *);

  void handle_write_event (esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
  void handle_exec_write_event (prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

  void print_adv_data(const uint8_t *adv_data, uint8_t adv_data_len);

public:
    BLE_Server( esp_ble_adv_params_t adv_params = ADV_PARAMS_DEFAULT,
                esp_ble_adv_data_t adv_data = ADV_DATA_DEFAULT,
                esp_ble_adv_data_t scan_rsp_data = SCAN_RSP_DATA_DEFAULT
              );

    ~BLE_Server();

    void connSetup(void) override;
    void send(const std::string &data) override;
    std::string receive() override;
};



#endif // BLE_SERVER_HPP