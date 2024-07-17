// Platform abstraction header for the HX711 driver

#include "em_gpio.h"
#include "sl_emlib_gpio_init_hx711_dt_config.h"
#include "sl_emlib_gpio_init_hx711_sck_config.h"
#include "cmsis_compiler.h"

#define clock_high() GPIO_PinOutSet(SL_EMLIB_GPIO_INIT_HX711_SCK_PORT, SL_EMLIB_GPIO_INIT_HX711_SCK_PIN)
#define clock_low()  GPIO_PinOutClear(SL_EMLIB_GPIO_INIT_HX711_SCK_PORT, SL_EMLIB_GPIO_INIT_HX711_SCK_PIN)
#define get_DOUT()   GPIO_PinInGet(SL_EMLIB_GPIO_INIT_HX711_DT_PORT, SL_EMLIB_GPIO_INIT_HX711_DT_PIN)

#define delay()      do {__NOP(); __NOP(); __NOP();} while(0)
