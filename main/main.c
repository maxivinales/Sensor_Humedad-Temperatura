#include "config.c"
#include "logica_control.c"
// #include "WiFi_manager.c"

// recursos RTOS
SemaphoreHandle_t mutex_handles;
SemaphoreHandle_t mutex_i2c;

void app_main(void)
{  
    mutex_handles = xSemaphoreCreateMutex();
    mutex_i2c = xSemaphoreCreateMutex();
    control_launch();

    // while (1)
    // {
        
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
   
}
