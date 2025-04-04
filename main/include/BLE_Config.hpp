#ifndef BLE_CONFIG_HPP
#define BLE_CONFIG_HPP


#include "Bluetooth.hpp"

constexpr const char* TAG_SERVER = "BLE_Server";
constexpr const char* TAG_CLIENT = "BLE_Client";
constexpr const char* TAG_GAP = "GAP";
constexpr const char* TAG_GATTS = "GATT";

constexpr char DEVICE_NAME_SERVER[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Server";
constexpr char DEVICE_NAME_CLIENT[ESP_BLE_ADV_NAME_LEN_MAX] = "ESP_GATT_Client";

constexpr char CHAR_ADV_DATA[ESP_BLE_ADV_NAME_LEN_MAX] = "ADV ESP DATA";

constexpr uint16_t PROFILE_APP_ID = 0x00;
constexpr uint16_t PROFILE_NUM = 1;
constexpr uint8_t SERVICE_INST_ID = 0;
constexpr uint16_t CHAR_INST_ID = 0;

constexpr uint16_t MTU_DEFAULT = 23; // Default MTU size

constexpr esp_bt_uuid_t CHAR_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
    0xF1, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
  }}
};

constexpr esp_bt_uuid_t SERVICE_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
    0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
  }}
};

constexpr esp_bt_uuid_t DESCR_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
    0xF2, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x78, 0x56, 0x78, 0x56, 0x34, 0x12
  }}
};

constexpr esp_bt_uuid_t NOTIFY_DESCR_UUID_DEFAULT = {
  .len = ESP_UUID_LEN_128,
  .uuid = {.uuid128 = {
      0x70, 0x42, 0x4B, 0xDE, 0xE1, 0x77, 0xD6, 0x41,
      0x46, 0x84, 0xF1, 0x9D, 0x42, 0x9B, 0x6D, 0x9F
      }
  },
};

#endif // BLE_CONFIG_HPP