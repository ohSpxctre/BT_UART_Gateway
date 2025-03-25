/**
 * @file BLE_Client.hpp
 * @brief BLE Client class inheriting from Bluetooth.
 */

 #ifndef BLE_CLIENT_HPP
 #define BLE_CLIENT_HPP
 
 #include "Bluetooth.hpp"

 #include "esp_gap_ble_api.h"
 #include "esp_gatt_defs.h"
 #include "esp_gatts_api.h"
 #include "esp_gattc_api.h"
 
 
 /**
  * @class BLE_Client
  * @brief BLE Client implementation.
  */
 class BLE_Client : public Bluetooth {
 public:
     BLE_Client();
     ~BLE_Client();
     
     void Init() override;
     void Send(const std::string &data) override;
     std::string Receive() override;
 };
 
 #endif // BLE_CLIENT_HPP