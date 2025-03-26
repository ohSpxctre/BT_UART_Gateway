/**
 * @file BLE_Server.cpp
 * @brief Implementation of BLE_Server class.
 */

 #include "BLE_Server.hpp"
 #include <iostream>
 
 BLE_Server::BLE_Server() {
     std::cout << "BLE Server Constructor" << std::endl;
 }
 
 BLE_Server::~BLE_Server() {
     std::cout << "BLE Server Destructor" << std::endl;
 }
 
 void BLE_Server::Init() {

    
     std::cout << "BLE Server Initialized" << std::endl;
 }
 
 void BLE_Server::Send(const std::string &data) {
     std::cout << "BLE Server Sending: " << data << std::endl;
 }
 
 std::string BLE_Server::Receive() {
     std::cout << "BLE Server Receiving Data" << std::endl;
     return "Received Data from BLE Client";
 }
 