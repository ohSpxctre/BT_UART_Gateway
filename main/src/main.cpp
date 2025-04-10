#include <thread>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "uart.hpp"
#include "MessageHandler.hpp"
#include "DataParser.hpp"
#include "CommandHandler.hpp"
#include "Bluetooth.hpp"
#include "BLE_Client.hpp"
#include "BLE_Server.hpp"

/* For using statistics set in menuconfig:
    FREERTOS_USE_TRACE_FACILITY
    FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    FREERTOS_VTASKLIST_INCLUDE_COREID
*/
#define LOG_FREERTOS 0 // Set to 1 to enable FreeRTOS logging

bool BLE_SERVER = false; // Default to client mode

esp_pthread_cfg_t create_config(const char *name, int stack, int prio)
{
    auto cfg = esp_pthread_get_default_config();
    cfg.thread_name = name;
    //cfg.pin_to_core = core_id;
    cfg.stack_size = stack;
    cfg.prio = prio;
    return cfg;
}

void uartReceiveTask(Uart &uartX, MessageHandler &msgHandler)
{
    while (true) {
        uartX.receiveTask(&msgHandler);
    }
}

void uartSendTask(Uart &uartX, MessageHandler &msgHandler)
{
    // Print out information about the Bluetooth interface
    const char* bleInterface;
    if (BLE_SERVER) {
        bleInterface = "BLE Server mode\n";
    } else {
        bleInterface = "BLE Client mode\n";
    }
    uartX.send(bleInterface, strlen(bleInterface));

    // Get message from the UART queue and send it to the UART interface
    while (true) {
        uartX.sendTask(&msgHandler);
    }
}

void dataParserTask(DataParser &dataParser, MessageHandler &msgHandler) 
{
    // Parse the data from the UART interface and send it to the command handler
    while(true) {
        dataParser.dataParserTask(&msgHandler);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void bleSendTask(Bluetooth &bluetooth, MessageHandler &msgHandler) 
{
    // Send data to the Bluetooth interface from the command handler
    while (true) {
        bluetooth.sendTask(&msgHandler);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool getBleInterface(Uart &uartX, MessageHandler &msgHandler) {
    // Prompt the user to choose between Server and Client mode
    const char* prompt = "Choose BLE interface (Server or Client): \n";
    uartX.send(prompt, strlen(prompt));
    // Wait for the user to respond
    uartX.receiveTask(&msgHandler);
    MessageHandler::Message data;
    msgHandler.receive(MessageHandler::QueueType::DATA_PARSER_QUEUE, data);
    if (strncasecmp(data.data(), "Server", 6) == 0) {
        BLE_SERVER = true; // Set to server mode
        return true; // Server mode
    } 
    else if (strncasecmp(data.data(), "Client", 6) == 0) {
        BLE_SERVER = false; // Set to client mode
        return false; // Client mode
    }
    // If neither, return false (default to Client mode)
    return false;
}


extern "C" void app_main(void) {
    /* Set log level*/
    esp_log_level_set("*", ESP_LOG_INFO);

    /* Create instances */
    MessageHandler msgHandler;
    CommandHandler cmdHandler;
    DataParser dataParser(cmdHandler);
    Uart uart0;

    /* Initialize the UART interface for UART port 0 (default) */
    uart0.init(msgHandler.getUartEventQueue());

    /* Choose the BLE interface mode (Server or Client) */
    std::unique_ptr<Bluetooth> bleInterface;
    if (getBleInterface(uart0, msgHandler)) {
        bleInterface = std::make_unique<BLE_Server>();
    } else {
        bleInterface = std::make_unique<BLE_Client>();
    }

    /* Initialize the Bluetooth interface */
    bleInterface->setMessageHandler(&msgHandler);
    bleInterface->connSetup();

    /* Create threads for the UART receive task */
    esp_pthread_cfg_t cfg = create_config("uartReceiveTask", 4096, 4);
    esp_pthread_set_cfg(&cfg);
    std::thread uartReceiveThread(uartReceiveTask, std::ref(uart0), std::ref(msgHandler));

    /* Create a thread for the UART send task */
    cfg = create_config("uartSendTask", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread uartSendThread(uartSendTask, std::ref(uart0), std::ref(msgHandler));

    /* Create a thread for the DataParser task */
    cfg = create_config("dataParserTask", 4096, 3);
    esp_pthread_set_cfg(&cfg);
    std::thread dataParserThread(dataParserTask, std::ref(dataParser), std::ref(msgHandler));

    /* Create a thread for BLE send task */ 
    cfg = create_config("bleSendTask", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread bleSendThread(bleSendTask, std::ref(*bleInterface), std::ref(msgHandler));

    while (true)
    {
        
#if LOG_FREERTOS   
        // --- vTaskList() ---
        char taskList[1024];
        vTaskList(taskList);
        const char *taskListHeader = "Task List:\nTask Name\tState\tPrio\tStack\tNum\tCore\n";
        uart0.send(taskListHeader, strlen(taskListHeader));
        uart0.send(taskList, strlen(taskList));

        // --- Heap Info ---
        char buffer[64]; // temporary buffer for converting numbers to string

        uart0.send("Heap Info:\n", strlen("Heap Info:\n"));

        size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
        snprintf(buffer, sizeof(buffer), "Total Heap Size: %d bytes\n", (int)totalHeap);
        uart0.send(buffer, strlen(buffer));

        size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        snprintf(buffer, sizeof(buffer), "Free Heap Size: %d bytes\n", (int)freeHeap);
        uart0.send(buffer, strlen(buffer));

        size_t usedHeap = totalHeap - freeHeap;
        snprintf(buffer, sizeof(buffer), "Used Heap Size: %d bytes\n", (int)usedHeap);
        uart0.send(buffer, strlen(buffer));    
#endif
        // Main loop to keep the internal tasks running
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
}
