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
    //QueueHandle_t uart_queue;
    char data[uart::UART_BUFFER_SIZE];

    MessageHandler msgHandler;

    MessageHandler::Message message{};
    std::string msg = "Hello from UART!";
    std::copy(msg.begin(), msg.end(), message.begin());

    msgHandler.send(MessageHandler::QueueType::UART_QUEUE, message);
    msgHandler.receive(MessageHandler::QueueType::UART_QUEUE, message);
    ESP_LOGI(pcTaskGetName(nullptr), "Received message: %s", message.data());

    uart.init(msgHandler.getUartEventQueue());
    
    while (true)
    {
        int bytesReceived = uart.receive(data);

        // Log the received data
        ESP_LOGI(pcTaskGetName(nullptr), "Received data: %s", data);

        // Re-send the received data back to the sender
        uart.send(data, bytesReceived);
    }
    
}


extern "C" void app_main(void) {
    // Create a thread for the UART task
    esp_pthread_cfg_t cfg = create_config("uartTest_task", 4096, 5);
    esp_pthread_set_cfg(&cfg);
    std::thread testThread(uartTest_task);

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
