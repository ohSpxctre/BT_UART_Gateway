#include "util/Core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

namespace rtos {

bool isInterruptContextAcive()
{
    return xPortInIsrContext();
}

}
