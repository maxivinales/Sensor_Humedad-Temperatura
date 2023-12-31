#include "mqtt.h"
#include "cJSON.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stddef.h>
#include <string.h>

#define LEN_QUEUE_MQTT 10

TaskHandle_t TaskHandle_mqtt = NULL;
QueueHandle_t msg_queue_to_mqtt_send = NULL;

extern QueueHandle_t msg_queue_toControl;

esp_err_t sender_mqtt(esp_mqtt_client_handle_t _client , struct data_mqtt_send_t *msj);
struct data_control_t JsonDecodeMQTTControl(char* _json);

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

        ESP_LOGI(TAG_MQTT, "Me suscribo a: %s", topic_control);
        msg_id = esp_mqtt_client_subscribe(client, topic_control, ONE_TIME);
        ESP_LOGI(TAG_MQTT, "sent subscribe successful, msg_id=%d", msg_id);

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
    case MQTT_EVENT_DATA:   // aca tengo que tomar los datos del usuario
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if(strcmp(event->topic, topic_control) == 0 || strcmp(event->topic, topic_control) == 123){
            struct data_control_t cmd_received;
            cmd_received = JsonDecodeMQTTControl(event->data);
            xQueueSend(msg_queue_toControl, &cmd_received, portMAX_DELAY);
        }else{
            ESP_LOGW(TAG_MQTT, "strcmp(event->topic, topic_control) = %d", strcmp(event->topic, topic_control));
        }
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
            .username = data_MQTT_SC.User,//"test_SC",
            .authentication = {
                .password = data_MQTT_SC.pass,//"faniot123",
            },
        },
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
        void *_pv_buffer = malloc(sizeof(struct data_mqtt_send_t));
        if (xQueueReceive(msg_queue_to_mqtt_send, _pv_buffer, portMAX_DELAY)) {
            // printf("Consumidor recibió: %d\n", received_value);
            esp_err_t _err = ESP_OK;
            
            sender_mqtt(client, (struct data_mqtt_send_t*)_pv_buffer);
        }
        free(_pv_buffer);
    }
}
esp_err_t mqtt_launch(){
    esp_err_t _err = ESP_OK;
    if(TaskHandle_mqtt == NULL){
        //msg_queue_toControl = xQueueCreate(MSG_QUEUE_LENGTH, sizeof(struct data_t));
        xTaskCreatePinnedToCore(             // Use xTaskCreate() in vanilla FreeRTOS
            mqtt_task,        // Function to be called
            "mqtt_task",          // Name of task
            10000,             // Stack size (bytes in ESP32, words in FreeRTOS)
            NULL,              // Parameter to pass
            3,                   // Task priority
            &TaskHandle_mqtt, // Task handle
            APP_CORE);              // Run on one core for demo purposes (ESP32 only)
        // return(ESP_OK);                     // con este tipo de comandos indico si algo no sale bien
    }else{
        ESP_LOGE(TAG_MQTT, "Tarea ya creada");
        return(ESP_FAIL);
    }

    if(msg_queue_to_mqtt_send == NULL){
        msg_queue_to_mqtt_send = xQueueCreate(LEN_QUEUE_MQTT, sizeof(struct data_mqtt_send_t));
        if(msg_queue_to_mqtt_send == NULL){
            ESP_LOGE(TAG_MQTT, "error creando la cola");
            return(ESP_FAIL);
        }
    }else{
        ESP_LOGE(TAG_MQTT, "Cola ya creada");
        return(ESP_FAIL);
    }

    return(_err);
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



esp_err_t sender_mqtt(esp_mqtt_client_handle_t _client , struct data_mqtt_send_t *msj){
    ESP_LOGI(TAG_MQTT, "enviando msj mqtt a broker");
    int msj_id;
    char _topic[96];
    snprintf(_topic, 96, "%s/%s/%s", NOMBRE_PRODUCTO, CHIPID.value_str, msj->topic);
    msj_id = esp_mqtt_client_publish(_client, _topic, msj->payload, sizeof(msj->payload), msj->qos, msj->retain);
    if(msj->qos != AT_MOST_ONCE){
        if(msj_id != -1){
            ESP_LOGI(TAG_MQTT, "msj id = %d", msj_id);
            return(ESP_OK);
        }else{
            ESP_LOGE(TAG_MQTT, "error enviando msj mqtt a broker");
            return(ESP_FAIL);
        }
    }else{
        return(ESP_OK);
    }
}

struct data_control_t JsonDecodeMQTTControl(char* _Json){
    struct data_control_t _return;
    cJSON *Jsonsito = cJSON_Parse(_Json);
    if(Jsonsito == NULL) {
        ESP_LOGW(TAG_MQTT, "Error al analizar el JSON\n");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        _return.cmd = ERROR_RECEPCION;
        strcpy(_return.value_str, "Error al analizar el JSON recibido\n");
        return(_return);
    }else{
        cJSON *_cmd = cJSON_GetObjectItemCaseSensitive(Jsonsito, "cmd");
        if (_cmd != NULL) {
            printf("cmd: %d\n", _cmd->valueint);
            _return.cmd = (cmd_control_t)(_cmd->valueint);
        } else {
            _return.cmd = ERROR_RECEPCION;
            strcpy(_return.value_str, "Objeto cmd no encontrado o no válido\n");
            return(_return);
        }

        cJSON *_value = cJSON_GetObjectItemCaseSensitive(Jsonsito, "value");
        if (_value != NULL) {
            printf("value: %d\n", _value->valueint);
            _return.value = _value->valueint;
        } else {
            // _return.cmd = ERROR_RECEPCION;
            // strcpy(_return.value_str, "Objeto value no encontrado o no válido\n");
            ESP_LOGW(TAG_MQTT, "Objeto value no encontrado o no válido");
            // return(_return);
        }

        cJSON *_value_f = cJSON_GetObjectItemCaseSensitive(Jsonsito, "value_f");
        if (_value_f != NULL) {
            printf("value_f: %f\n", _value_f->valuedouble);
            _return.value_f = (float)_value_f->valuedouble;
        } else {
            // _return.cmd = ERROR_RECEPCION;
            // strcpy(_return.value_str, "Objeto value no encontrado o no válido\n");
            ESP_LOGW(TAG_MQTT, "Objeto value_f no encontrado o no válido");
            // return(_return);
        }

        cJSON *_value_str = cJSON_GetObjectItemCaseSensitive(Jsonsito, "value_str");
        if (_value_str != NULL) {
            printf("value_str: %s\n", _value_str->valuestring);
            strcpy(_return.value_str, _value_str->valuestring);
        } else {
            // _return.cmd = ERROR_RECEPCION;
            // strcpy(_return.value_str, "Objeto value no encontrado o no válido\n");
            ESP_LOGW(TAG_MQTT, "Objeto value_str no encontrado o no válido");
            // return(_return);
        }

        // if(_cmd != NULL){
        //     cJSON_Delete(_cmd);
        //     _cmd = NULL;
        // }

        // if(_value != NULL){
        //     cJSON_Delete(_value);
        //     _value = NULL;
        // }

        // if(_value_f != NULL){
        //     cJSON_Delete(_value_f);
        //     _value_f = NULL;
        // }

        // if(_value_str != NULL){
        //     cJSON_Delete(_value_str);
        //     _value_str = NULL;
        // }

        if(Jsonsito != NULL){
            cJSON_Delete(Jsonsito);
            Jsonsito = NULL;
        }
    }

    return(_return);
}