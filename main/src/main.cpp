#include <stdio.h>

#include "rtos.h"

#include "uart.hpp"

extern "C" void app_main(void)
{
    printf("Hello World!\n");
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Hello World!\n");
    }

}