/**
 * @file BLE_Client.hpp
 * @brief BLE Client class inheriting from Bluetooth.
 */

 #ifndef BLE_CLIENT_HPP
 #define BLE_CLIENT_HPP
 
 #include "Bluetooth.hpp"

 
 
 /**
  * @class BLE_Client
  * @brief BLE Client implementation.
  */
 class BLE_Client : public Bluetooth {
 public:
     BLE_Client();
     ~BLE_Client();
     
     void connSetup() override;
     void send(const std::string &data) override;
     std::string receive() override;
 };
 
 #endif // BLE_CLIENT_HPP