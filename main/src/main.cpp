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

esp_pthread_cfg_t create_config(const char *name, int core_id, int stack, int prio)
{
    auto cfg = esp_pthread_get_default_config();
    cfg.thread_name = name;
    //cfg.pin_to_core = core_id;
    cfg.stack_size = stack;
    cfg.prio = prio;
    return cfg;
}

void* task_fn() {
    while (true) {
        ESP_LOGI("task_fn", "Hello from task");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return NULL;
}

extern "C" void app_main(void) {
    std::thread testThread(task_fn);

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
