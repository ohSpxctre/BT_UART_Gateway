#include <thread>
#include <chrono>
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "uart.hpp"
#include "MessageHandler.hpp"
#include "DataParser.hpp"
#include "CommandHandler.hpp"
#include "Bluetooth.hpp"
#include "BLE_Client.hpp"
#include "BLE_Server.hpp"

#define BLE_SERVER 0

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
    }
}



extern "C" void app_main(void) {
    /* Set log level*/
    esp_log_level_set("*", ESP_LOG_INFO);

    /* Create instances */
    MessageHandler msgHandler;
    CommandHandler cmdHandler;
    DataParser dataParser(cmdHandler);
    Uart uart0;

#if BLE_SERVER
    BLE_Server bleInterface;
#else
    BLE_Client bleInterface;
#endif
    /* Initialize the Bluetooth interface */
    bleInterface.setMessageHandler(&msgHandler);
    bleInterface.connSetup();

    /* Initialize the UART interface for UART port 0 (default) */
    uart0.init(msgHandler.getUartEventQueue());
    
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
    std::thread bleSendThread(bleSendTask, std::ref(bleInterface), std::ref(msgHandler));

    while (true)
    {
        // Main loop to keep the internal tasks running
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
}
