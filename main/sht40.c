#include "sht40.h"
#include "esp_err.h"
#include "esp_log.h"
static sht4x_t dev;
static const char *TAG_SHT40 = "SHT40_EXAMPLE";
TaskHandle_t TaskHandle_sht40 = NULL;
#if defined(CONFIG_EXAMPLE_SHT4X_DEMO_LL)

void task(void *pvParameters)
{
    float temperature;
    float humidity;

    TickType_t last_wakeup = xTaskGetTickCount();

    // get the measurement duration for high repeatability;
    uint8_t duration = sht4x_get_measurement_duration(&dev);

    while (1)
    {
        // Trigger one measurement in single shot mode with high repeatability.
        ESP_ERROR_CHECK(sht4x_start_measurement(&dev));

        // Wait until measurement is ready (duration returned from *sht4x_get_measurement_duration*).
        vTaskDelay(duration);

        // retrieve the values and do something with them
        ESP_ERROR_CHECK(sht4x_get_results(&dev, &temperature, &humidity));
        printf("sht4x Sensor: %.2f °C, %.2f %%\n", temperature, humidity);

        // wait until 5 seconds are over
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(5000));
    }
}

#else
void task(void *pvParameters)
{
    float temperature;
    float humidity;

    TickType_t last_wakeup = xTaskGetTickCount();

    while (1)
    {
        // perform one measurement and do something with the results
        ESP_ERROR_CHECK(sht4x_measure(&dev, &temperature, &humidity));
        printf("sht4x Sensor: %.2f °C, %.2f %%\n", temperature, humidity);

        // wait until 5 seconds are over
        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(5000));
    }
}

#endif

void sht40_launch(){
   // esp_err_t _err = ESP_OK;
    //if (TaskHandle_sht40 == NULL)
    //{
    ESP_ERROR_CHECK(i2cdev_init());
    memset(&dev, 0, sizeof(sht4x_t));

    ESP_ERROR_CHECK(sht4x_init_desc(&dev, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
    ESP_ERROR_CHECK(sht4x_init(&dev));
    xTaskCreatePinnedToCore(
    task,
    "sht4x_test",
    configMINIMAL_STACK_SIZE * 8,
    NULL,
    5,
    &TaskHandle_sht40,
    APP_CPU_NUM);
    }
/* else
    {
    ESP_LOGE(TAG_SHT40, "Tarea ya creada");
    return (ESP_FAIL);
    }

}*/