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
#include "MessageHandler.hpp"
#include "DataParser.hpp"
#include "CommandHandler.hpp"

// Create a MessageHandler instance
MessageHandler msgHandler;

// Create a CommandHandler and DataParser instance
CommandHandler cmdHandler;
DataParser dataParser(cmdHandler);

// Create a UART instance
uart::Uart uart0;

esp_pthread_cfg_t create_config(const char *name, int stack, int prio)
{
    auto cfg = esp_pthread_get_default_config();
    cfg.thread_name = name;
    //cfg.pin_to_core = core_id;
    cfg.stack_size = stack;
    cfg.prio = prio;
    return cfg;
}

void uartReceiveTest_task()
{
    while (true) {
        uart0.receiveTask(&msgHandler);
    }
}


void uartSendTest_task(){
    // Get message from the UART queue and send it to the UART interface
    while (true) {
        uart0.sendTask(&msgHandler);
    }
}

void dataParserTestTask() {
    while(true) {
        dataParser.dataParserTask(&msgHandler);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


extern "C" void app_main(void) {
    // Initialize the UART interface
    uart0.init(msgHandler.getUartEventQueue());

    // Create a thread for the UART task
    esp_pthread_cfg_t cfg = create_config("uartReceiveTest_task", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread testThread(uartReceiveTest_task);

    // Create a thread for the UART send task
    cfg = create_config("uartSendTest_task", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread sendThread(uartSendTest_task);

    // Create a thread for the DataParser task
    cfg = create_config("dataParserTestTask", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread parserThread(dataParserTestTask);

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
