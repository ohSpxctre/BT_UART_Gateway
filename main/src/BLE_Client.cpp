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
 
 void BLE_Client:: connSetup() {
     std::cout << "BLE Client Initialized" << std::endl;
 }
 
 void BLE_Client::send(const std::string &data) {
     std::cout << "BLE Client Sending: " << data << std::endl;
 }
 
 std::string BLE_Client::receive() {
     std::cout << "BLE Client Receiving Data" << std::endl;
     return "Received Data from BLE Server";
 }