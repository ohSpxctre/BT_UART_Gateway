#include <thread>
#include <chrono>
#include <iostream>
#include <string>
#include <sstream>
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "uart.hpp"
#include "MessageQueue.h"
#include "Bluetooth.hpp"

#define IS_SERVER true     // Set to true for server, false for client

#if IS_SERVER
#include "BLE_Server.hpp"
#else
#include "BLE_Client.hpp"
#endif



  



esp_pthread_cfg_t create_config(const char *name, int stack, int prio)
{
    auto cfg = esp_pthread_get_default_config();
    cfg.thread_name = name;
    //cfg.pin_to_core = core_id;
    cfg.stack_size = stack;
    cfg.prio = prio;
    return cfg;
}

void uartTest_task()
{
    // Create a UART and MessageQueue object
    uart::Uart uart;
    QueueHandle_t uart_queue;
    char data[uart::UART_BUFFER_SIZE];

    uart.init(&uart_queue);
    
    while (true)
    {
        // Print the received data
        uart.receive(data);
        ESP_LOGI(pcTaskGetName(nullptr), "Received data: %s", data);

        // Send data over UART
        uart.send(data);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
}

#if IS_SERVER

void bluetoothServer_task()
{
    // Create a BLE_Server object
    BLE_Server bleServer;
    bleServer.connSetup();
    
    while (true)
    {
        // Log BLE server status
        //ESP_LOGI(pcTaskGetName(nullptr), "BLE Server running...");
        bleServer.send("Hello from ESP32 BLE Server");
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}
#else
void bluetoothClient_task()
{
    // Create a BLE_Server object
    BLE_Client bleClient;
    bleClient.connSetup();
    
    while (true)
    {
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}


#endif

extern "C" void app_main(void) {

    esp_log_level_set("*", ESP_LOG_INFO);  // Default: Show warnings and errors only
    esp_log_level_set("BLE", ESP_LOG_DEBUG);  // Enable detailed logs for your BLE module
    esp_log_level_set("GATTS", ESP_LOG_DEBUG);  // Enable logs for GATT Server
    esp_log_level_set("GAP", ESP_LOG_DEBUG);  // Enable logs for GAP events
    esp_log_level_set("BT", ESP_LOG_DEBUG);  // Enable logs for Bluetooth Stack

    // Create a thread for the UART task
    esp_pthread_cfg_t cfg = create_config("uartTest_task", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread testThread(uartTest_task);

#if IS_SERVER
    // Create a thread for the Bluetooth server task
    esp_pthread_cfg_t cfg2 = create_config("bluetoothServer_task", 8192 , 5);
    esp_pthread_set_cfg(&cfg2);
    std::thread testThread2(bluetoothServer_task);
#else
    // Create a thread for the Bluetooth client task
    esp_pthread_cfg_t cfg3 = create_config("bluetoothClient_task", 4096, 5);
    esp_pthread_set_cfg(&cfg3);
    std::thread testThread3(bluetoothClient_task);
#endif

    //Let the main task do something too
     while (true) {
        std::stringstream ss;
        ss << "core id: " << xPortGetCoreID()
           << ", prio: " << uxTaskPriorityGet(nullptr)
           << ", minimum free stack: " << uxTaskGetStackHighWaterMark(nullptr) << " bytes.";
        ESP_LOGI(pcTaskGetName(nullptr), "%s", ss.str().c_str());
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
