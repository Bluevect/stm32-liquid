#include "bsp_led.h"

#include "main.h"
#include "stm32f1xx_hal.h"

void Led1_Toggle(void) { HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); }
