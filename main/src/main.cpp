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
    // Buffer to store the received data
    char data[uart::UART_BUFFER_SIZE];

    while (true)
    {
        // Buffer the received data till a carriage return or newline is received
        int bytesReceived = 0;
        bool isEndOfLine = false;
        // Read data from the UART interface
        while (!isEndOfLine && bytesReceived < uart::UART_BUFFER_SIZE - 1) {
            bytesReceived += uart0.receive(data + bytesReceived);
            // Re-send the received data back to the sender
            uart0.send(data + bytesReceived - 1, 1);
            // Check if the last character is a carriage return or newline
            if (data[bytesReceived - 1] == '\r' || data[bytesReceived - 1] == '\n') {
                isEndOfLine = true;
            }
        }           

        MessageHandler::Message message;
        std::copy(data, data + bytesReceived, message.begin());
        msgHandler.send(MessageHandler::QueueType::DATA_PARSER_QUEUE, message, MessageHandler::ParserMessageID::MSG_ID_UART);

        // Log the received data
        ESP_LOGI(pcTaskGetName(nullptr), "Received data: %s", data);

        
    }
    
}

void uartSendTest_task(){
    // Get message from the UART queue and send it to the UART interface
    MessageHandler::Message message;
    while (true) {
        if (msgHandler.receive(MessageHandler::QueueType::UART_QUEUE, message)) {
            uart0.send(message.data(), message.size());
            ESP_LOGI(pcTaskGetName(nullptr), "Sent data: %s", message.data());
        } else {
            ESP_LOGW(pcTaskGetName(nullptr), "Failed to receive message from UART_QUEUE");
        }
    }
}

void dataParserTestTask() {
    while(true){
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
