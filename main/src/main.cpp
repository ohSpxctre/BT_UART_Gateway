#include <stdio.h>

#include "esp_log.h"  

#include "rtos.h"

#include "uart.hpp"

#if 0
extern "C" void app_main(void)
{
    auto testTask = rtos::Task::build()
        .name("testTask")
        .priority(1)
        .stackSize(16 * 1024)
        .spawn([]() {
            while (true) {
                rtos::Task::sleep(100); // give time for scheduler & locks
                ESP_LOGI("testTask", "Hello from testTask");
                //ESP_LOGI("debug", "testTask stack free: %u", uxTaskGetStackHighWaterMark(NULL));
                rtos::Task::sleep(1000);
            }
        });
        
    rtos::startScheduler();

}
#endif
extern "C" void task_fn(void* arg) {
    while (true) {
        ESP_LOGI("task_fn", "Hello from task");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
    xTaskCreate(task_fn, "task_fn", 4096, NULL, 5, NULL);
}
