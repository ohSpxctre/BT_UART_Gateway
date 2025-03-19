#include "rtos.h"

#include "freertos/task.h"

namespace rtos {

void startScheduler()
{
    vTaskStartScheduler();

    assert(true);
    while(true);
}

}
