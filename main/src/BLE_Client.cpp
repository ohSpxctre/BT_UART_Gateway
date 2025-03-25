/**
 * @file BLE_Client.cpp
 * @brief Implementation of BLE_Client class.
 */

 #include "BLE_Client.hpp"
 #include <iostream>
 
 BLE_Client::BLE_Client() {
     std::cout << "BLE Client Constructor" << std::endl;
 }
 
 BLE_Client::~BLE_Client() {
     std::cout << "BLE Client Destructor" << std::endl;
 }
 
 void BLE_Client::Init() {
     std::cout << "BLE Client Initialized" << std::endl;
 }
 
 void BLE_Client::Send(const std::string &data) {
     std::cout << "BLE Client Sending: " << data << std::endl;
 }
 
 std::string BLE_Client::Receive() {
     std::cout << "BLE Client Receiving Data" << std::endl;
     return "Received Data from BLE Server";
 }