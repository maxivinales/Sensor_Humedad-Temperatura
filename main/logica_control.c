#include "logica_control.h"

#include "Leq_task.c"
#include "audio_task.c"
#include "WiFi_manager.c"
#include "config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/projdefs.h"
#include "ota.c"
#include "http_client.c"
#include "mqtt.c"
#include <stdio.h>

// #include "config.h"
// #include "Leq_task.c"
// #include "audio_task.c"
// #include "WiFi_manager.c"
// #include "ota.c"

static const char *TAG_CONTROL = "Logica de control";

TaskHandle_t TaskHandle_control;
QueueHandle_t msg_queue_toControl = NULL;

void control_task(void *parameter){
    struct data_t msd_control_buffer;
    printf("Iniciando control_task\n");

    loadConfig();                                   // traigo la configuración guardada en ROM
    vTaskDelay(pdMS_TO_TICKS(200));

    if(mode_WiFi_manager.value == 0){
        ESP_LOGI(TAG_CONTROL, "ESP_WIFI_MODE_AP");          // mensaje de debug por serial
        wifi_init_softap();                         // inicia el WiFi en Modo AP
        ESP_LOGW(TAG_CONTROL, "Luego del WiFi ->[APP] Free memory: %lu bytes", esp_get_free_heap_size());
        if(server == NULL){
            server = start_webserver();
        }
        while(1){
            // esp_err_t _err;
            // wifi_ap_record_t _ap_info;
            // _err = esp_wifi_sta_get_ap_info(&_ap_info);
            // if(_err == ESP_OK){
            //     if(strcmp((char*)(&_ap_info.ssid[0]), "") != 0){
            //         init_OTA();
            //         update_firmware(CHIPID.value_str);
            //     }
            // }
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }else{
        // mode_WiFi_manager.value = 0;
        wifi_init_sta(data_WiFi_SC);
        // vTaskDelay(pdMS_TO_TICKS(5000));
        // init_OTA();
        // saveConfig();
        // update_firmware(CHIPID.value_str);
        uint8_t cont_timeout = 0;
        while(wifi_connection_status.value == 0){
            vTaskDelay(pdMS_TO_TICKS(100));
            cont_timeout++;
            if(cont_timeout >= 100){
                mode_WiFi_manager.value = 0;
                saveConfig();
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
        }
        
        if(wifi_connection_status.value == 1){
            get_firmware_version(CHIPID.value_str);
            mqtt_launch();
            int cont_mqtt_start = 0;
            ESP_LOGI(TAG_CONTROL, "Intentando conectarse a broker MQTT %s, user = %s , pass = %s", MQTT_URL_FANIOT, data_MQTT_SC.User, data_MQTT_SC.pass);
            while (cont_mqtt_start < 600 && mqtt_connected == false) {
                vTaskDelay(pdMS_TO_TICKS(100));
                cont_mqtt_start++;
                printf("cont_mqtt  = %d\n", cont_mqtt_start);
            }

            if(!mqtt_connected){
                mode_WiFi_manager.value = 0;
                // ver de poner una alarma o algo
                saveConfig();
                vTaskDelay(pdMS_TO_TICKS(100));
                esp_restart();
            }
        }
        // rest_get();
        
    }

    aux_launch();   // lanzo mi tarea auxiliar, TENGO QUE CAMBIARLE EL NOMBRE

    TickType_t xPeriod = portMAX_DELAY;//pdMS_TO_TICKS(30000);

    struct data_control_t *msj_control = malloc(sizeof(struct data_control_t));
    while (1)
    {
        if (xQueueReceive(msg_queue_toControl, (void*)msj_control, xPeriod) == pdTRUE){
            switch (msj_control->cmd) {
                case NADA:
                    // code
                    ESP_LOGI(TAG_CONTROL, "acá no hace nada");
                break;

                case SAVE:
                    // code
                    ESP_LOGI(TAG_CONTROL, "supuestamente es para guardar los datos");
                break;

                case ERROR_RECEPCION:
                    // code
                    ESP_LOGI(TAG_CONTROL, "ERROR -> %s", msj_control->value_str);
                break;

                case TEST:
                    // code
                    ESP_LOGI(TAG_CONTROL, "Testeo -> %d\n%f\n%s", msj_control->value, msj_control->value_f, msj_control->value_str);
                    ESP_LOGW(TAG_CONTROL, "Free memory: %lu bytes", esp_get_free_heap_size());

                    char json1[] = "{\"datetime\":{\"date\":\"21-09-2023\",\"time\":\"08:20:55:2353\"}}";
                    char json2[] = "{\"Temperatura\":{\"Mod\":36,\"ud\":\"°C\"},\"HR\":{\"Mod\":95,\"ud\":\"%\"}}";//"{\"data\":{\"Temperatura\":{\"Mod\":36,\"ud\":\"°C\"},\"HR\":{\"Mod\":95,\"ud\":\"%%\"}}}";
                    char *json3, *json4;
                    json3 = embedJsonInObject(&json2, "data");
                    json4 = mergeJsons(&json1, json3);
                    ESP_LOGI(TAG_CONTROL, "json1:\n\n%s\n\n", json1);
                    ESP_LOGI(TAG_CONTROL, "json2:\n\n%s\n\n", json2);
                    ESP_LOGI(TAG_CONTROL, "json3:\n\n%s\n\n", json3);
                    ESP_LOGI(TAG_CONTROL, "json4:\n\n%s\n\n", json4);
                    free(json3);
                    free(json4);
                break;

                case WIFI_MANAGER_START:
                    // code
                    ESP_LOGI(TAG_CONTROL, "Arranca el WiFi Manager");
                    mode_WiFi_manager.value = 0;
                    saveConfig();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    esp_restart();
                break;

                default:
                    // coso
                break;
            }
        }
        // free(msj_control);
        // vTaskDelay(pdMS_TO_TICKS(1000));        // miro cada 10 ms
    }
}

esp_err_t control_launch(){
    msg_queue_toControl = xQueueCreate(MSG_QUEUE_LENGTH, sizeof(struct data_control_t));

    xTaskCreatePinnedToCore(             // Use xTaskCreate() in vanilla FreeRTOS
        control_task,        // Function to be called
        "control_task",          // Name of task
        10000,             // Stack size (bytes in ESP32, words in FreeRTOS)
        NULL,              // Parameter to pass
        10,                   // Task priority
        &TaskHandle_control, // Task handle
        APP_CORE);              // Run on one core for demo purposes (ESP32 only)

    return(ESP_OK);                     // con este tipo de comandos indico si algo no sale bien
}

esp_err_t control_kill(){
    if (TaskHandle_control != NULL)
    {
        vTaskDelete(TaskHandle_control);
        TaskHandle_control = NULL;
    }

    if (msg_queue_toControl != NULL)
    {
        vQueueDelete(msg_queue_toControl);
        msg_queue_toControl = NULL;
    }

    return(ESP_OK);                     // con este tipo de comandos indico si algo no sale bien
}