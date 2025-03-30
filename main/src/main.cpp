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
#include "BLE_Server.hpp"

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

void bluetoothTest_task()
{
    // Create a BLE_Server object
    BLE_Server bleServer;
    bleServer.connSetup();
    //bleServer.send("Hello from ESP32 BLE Server");
    
    while (true)
    {
        // Log BLE server status
        ESP_LOGI(pcTaskGetName(nullptr), "BLE Server running...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


extern "C" void app_main(void) {
    // Create a thread for the UART task
    esp_pthread_cfg_t cfg = create_config("uartTest_task", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread testThread(uartTest_task);

    esp_pthread_cfg_t cfg2 = create_config("bluetoothTest_task", 4096, 5);
    esp_pthread_set_cfg(&cfg2);
    std::thread testThread2(bluetoothTest_task);

     // Let the main task do something too
     while (true) {
        std::stringstream ss;
        ss << "core id: " << xPortGetCoreID()
           << ", prio: " << uxTaskPriorityGet(nullptr)
           << ", minimum free stack: " << uxTaskGetStackHighWaterMark(nullptr) << " bytes.";
        ESP_LOGI(pcTaskGetName(nullptr), "%s", ss.str().c_str());
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
