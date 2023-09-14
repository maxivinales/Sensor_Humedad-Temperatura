#include "http_client.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "freertos/projdefs.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define LEN_MAX_PAY_HTTP 1024

const char *TAG_http_client = "HTTTP client";

bool http_receiving = false;
char *recv_http = NULL; // Inicializa a NULL

char _recv_http[LEN_MAX_PAY_HTTP];
size_t cursor;

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %d, %s\n", evt->data_len, (char *)evt->data);
        if (evt->data_len > 0) {
            ESP_LOGW("debug Cukla http_client", "linea 25");
            if (http_receiving == false) {
                http_receiving = true;
                ESP_LOGW("debug Cukla http_client", "linea 28");
                cursor = evt->data_len;
                ESP_LOGW("debug Cukla http_client", "linea 30");

                char *aux_s1, *aux_s2;
                aux_s1 = (char *)evt->data;
                aux_s2 = &_recv_http[0];
                for(int i = 0; i < evt->data_len; i++){
                    *(aux_s2 + i) = *(aux_s1 + i);
                }
                ESP_LOGW("debug Cukla http_client", "%s",aux_s2);
                
                // strcpy(&recv_http[0], (char *)evt->data);
                // strcpy_s(&recv_http[0], evt->data_len, (char *)evt->data);
                // strncpy(&recv_http[0], (char *)evt->data, evt->data_len);
                // snprintf(&recv_http[0], evt->data_len, "%s", (char *)evt->data);
                ESP_LOGW("debug Cukla http_client", "linea 34");

                // recv_http = malloc((evt->data_len + 1) * sizeof(char));
                // if (recv_http == NULL) {
                //     ESP_LOGE(TAG_http_client, "Error de asignación de memoria\n");
                //     return ESP_FAIL;
                // }
                // *(recv_http + evt->data_len) = (char)(0);
                // ESP_LOGW(TAG_http_client, "recv_http -> %s", recv_http);
                // strcpy(recv_http, (char *)evt->data);
            } else {
                ESP_LOGW("debug Cukla http_client", "linea 45");
                // strcpy(&recv_http[cursor], (char *)evt->data);
                char *aux_s1, *aux_s2;
                aux_s1 = (char *)evt->data;
                aux_s2 = &_recv_http[cursor];
                for(int i = 0; i < evt->data_len; i++){
                    *(aux_s2 + i) = *(aux_s1 + i);
                }
                ESP_LOGW("debug Cukla http_client", "%s",aux_s2);
                // strcpy_s(&recv_http[0], evt->data_len, (char *)evt->data);
                // strncpy(&recv_http[cursor], (char *)evt->data, evt->data_len);
                // snprintf(&recv_http[cursor], evt->data_len, "%s", (char *)evt->data);
                ESP_LOGW("debug Cukla http_client", "linea 49");
                cursor += evt->data_len;
                ESP_LOGW("debug Cukla http_client", "linea 51");
                // char *aux_S1;
                // size_t len_string = strlen(recv_http) + 1 + evt->data_len + 1;
                // aux_S1 = malloc(len_string);
                // if (aux_S1 == NULL) {
                //     ESP_LOGE(TAG_http_client, "Error de asignación de memoria\n");
                //     free(recv_http); // Liberar recv_http antes de salir
                //     return ESP_FAIL;
                // }
                // strcpy(aux_S1, recv_http);
                // strcat(aux_S1, (char *)evt->data);
                // free(recv_http); // Liberar recv_http antes de asignar el nuevo valor
                // recv_http = aux_S1;
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        printf("HTTP_EVENT_ON_FINISH: %s\n", _recv_http);
        if (http_receiving == true) {
            http_receiving = false;
            ESP_LOGW("debug Cukla http_client", "linea 67");
            cJSON *Jsonsito = cJSON_Parse(_recv_http);
            ESP_LOGW("debug Cukla http_client", "linea 69");
            if (Jsonsito == NULL) {
                ESP_LOGW(TAG_http_client, "Error al analizar el JSON\n");
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    printf("Error: %s\n", error_ptr);
                }
                // free(recv_http); // Liberar recv_http en caso de error
                // recv_http = NULL;
                return ESP_FAIL;
            }
            ESP_LOGW("debug Cukla http_client", "linea 80");
            cJSON *firmware_version = cJSON_GetObjectItemCaseSensitive(Jsonsito, "firmware_version");
            if (firmware_version != NULL) {
                printf("firmware_version: %s\n", firmware_version->valuestring);
                strcpy(new_firmware_version.value_str, firmware_version->valuestring);
                new_firmware_version.value = 1;
                // cJSON_Delete(firmware_version);
                firmware_version = NULL;
            } else {
                ESP_LOGW(TAG_http_client, "Objeto firmware_version no encontrado o no válido\n");
            }
            ESP_LOGW("debug Cukla http_client", "linea 90");
            cJSON *datetime = cJSON_GetObjectItemCaseSensitive(Jsonsito, "datetime");
            if (datetime != NULL) {
                printf("datetime: %s\n", datetime->valuestring);
                strcpy(fecha_y_hora.value_str, datetime->valuestring);
                // cJSON_Delete(datetime);
                datetime = NULL;
            } else {
                ESP_LOGW(TAG_http_client, "Objeto datetime no encontrado o no válido\n");
            }
            ESP_LOGW("debug Cukla http_client", "linea 121");

            // free(recv_http); // Liberar recv_http después de analizar el JSON
            // recv_http = NULL; // Establecer el puntero a NULL para evitar errores futuros
            // free(Jsonsito);
            if(Jsonsito != NULL){
                cJSON_Delete(Jsonsito);
                Jsonsito = NULL;
            }

            if(firmware_version != NULL){
                cJSON_Delete(Jsonsito);
                Jsonsito = NULL;
            }

            if(datetime != NULL){
                cJSON_Delete(Jsonsito);
                Jsonsito = NULL;
            }
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void rest_get()
{
    esp_http_client_config_t config_get = {
        .url = "http://fabrica.faniot.ar:1880/ota/firmwareversion?chip_id=C44F33605219",//"http://worldclockapi.com/api/json/utc/now",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        // .buffer_size = 1024,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    // vTaskDelay(pdMS_TO_TICKS(500));
    // esp_http_client_close(client);
}

void get_data_time(char* _URL){
    esp_http_client_config_t config_get = {
        .url = _URL,
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .buffer_size = 1024,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    // vTaskDelay(pdMS_TO_TICKS(500));
    // esp_http_client_close(client);
}