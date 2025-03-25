/**
 * @file BLE_Server.hpp
 * @brief BLE Server class inheriting from Bluetooth.
 */

 #ifndef BLE_SERVER_HPP
 #define BLE_SERVER_HPP
 
 #include "Bluetooth.hpp"

 #include "esp_gap_ble_api.h"
 #include "esp_gatt_defs.h"
 #include "esp_gatts_api.h"
 #include "esp_gattc_api.h"
 
 
 /**
  * @class BLE_Server
  * @brief BLE Server implementation.
  */
 class BLE_Server : public Bluetooth {
 public:
     BLE_Server();
     ~BLE_Server();
     
     void Init() override;
     void Send(const std::string &data) override;
     std::string Receive() override;
 };
 
 #endif // BLE_SERVER_HPP