#include "mqtt.h"
#include "mqtt_client.h"
#include <string.h>

TaskHandle_t TaskHandle_mqtt;
// QueueHandle_t msg_queue_toControl = NULL;

static const char *TAG_MQTT = "MQTT_EXAMPLE";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG_MQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;  // Configura la bandera como verdadera
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        // ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "teste", 0);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG_MQTT, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;  // Configura la bandera como falsa cuando se desconecta
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG_MQTT, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG_MQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
        break;
    }
}


void mqtt_task(void *parameter)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URL_FANIOT,
        .credentials = {
            .username = "test_SC",
            .authentication = {
                .password = "faniot123",
            },
        },
        // .credentials = {
        //     .username = "username",
        //     .authentication = {
        //         .key_password = "pass",
        //         .certificate = "certificate",
        //         .certificate_len = strlen("certificate"),
        //         .key = "key",
        //         .key_len = NULL,
        //         .key_password_len = NULL,
        //         .use_secure_element = false,
        //         .ds_data = NULL,
        //     },
        //     .client_id = NULL,
        //     .set_null_client_id = true,
        // },
        // .broker.verification = {}
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    #if CONFIG_BROKER_URL_FROM_STDIN
        char line[128];

        if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
            int count = 0;
            printf("Please enter url of mqtt broker\n");
            while (count < 128) {
                int c = fgetc(stdin);
                if (c == '\n') {
                    line[count] = '\0';
                    break;
                } else if (c > 0 && c < 127) {
                    line[count] = c;
                    ++count;
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            mqtt_cfg.broker.address.uri = line;
            printf("Broker url: %s\n", line);
        } else {
            ESP_LOGE(TAG_MQTT, "Configuration mismatch: wrong broker url");
            abort();
        }
    #endif /* CONFIG_BROKER_URL_FROM_STDIN */

    while (1)
    {        
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_mqtt_client_publish(client, "teste", "1234", 0, 1, 0);
        // printf("xD hasta aqui llego");
        ESP_LOGI(TAG_MQTT, "mqtt task\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGW(TAG_MQTT, "Free memory: %lu bytes", esp_get_free_heap_size());
        // get_data_time("http://worldtimeapi.org/api/timezone/America/Argentina/Cordoba");
    }
}
esp_err_t mqtt_launch(){
    //msg_queue_toControl = xQueueCreate(MSG_QUEUE_LENGTH, sizeof(struct data_t));
    xTaskCreatePinnedToCore(             // Use xTaskCreate() in vanilla FreeRTOS
        mqtt_task,        // Function to be called
        "mqtt_task",          // Name of task
        10000,             // Stack size (bytes in ESP32, words in FreeRTOS)
        NULL,              // Parameter to pass
        3,                   // Task priority
        &TaskHandle_mqtt, // Task handle
        APP_CORE);              // Run on one core for demo purposes (ESP32 only)
    return(ESP_OK);                     // con este tipo de comandos indico si algo no sale bien
}
/*
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
*/