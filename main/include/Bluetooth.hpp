/**
 * @file Bluetooth.hpp
 * @brief Base class for Bluetooth communication.
 */

 #ifndef BLUETOOTH_HPP
 #define BLUETOOTH_HPP
 
 #include <string>
 /**
  * @class Bluetooth
  * @brief Abstract base class for BLE communication.
  */
 class Bluetooth {
 public:
     Bluetooth();
     virtual ~Bluetooth();
     
     virtual void Init() = 0;
     virtual void Send(const std::string &data) = 0;
     virtual std::string Receive() = 0;
 };
 
 #endif // BLUETOOTH_HPP