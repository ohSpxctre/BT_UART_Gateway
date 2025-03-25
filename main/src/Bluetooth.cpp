/**
 * @file Bluetooth.cpp
 * @brief Implementation of the Bluetooth base class.
 */

 #include "Bluetooth.hpp"
 #include <iostream>
 
 Bluetooth::Bluetooth() {
     std::cout << "Bluetooth Base Class Constructor" << std::endl;
 }
 
 Bluetooth::~Bluetooth() {
     std::cout << "Bluetooth Base Class Destructor" << std::endl;
 }
 
 /**
  * @file BLE_Server.hpp
  * @brief BLE Server class inheriting from Bluetooth.
  */
 