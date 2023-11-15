#include "config.c"
#include "logica_control.c"
#include "sht40.c"
// #include "WiFi_manager.c"

// recursos RTOS
// SemaphoreHandle_t mutex_handles;
// SemaphoreHandle_t mutex_i2c;


void app_main(void)
{
sht40_launch();
control_launch();
}
