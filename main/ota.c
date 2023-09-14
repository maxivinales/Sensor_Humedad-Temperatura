#include "ota.h"
#include "config.h"
#include "esp_err.h"
#include "freertos/projdefs.h"

static const char *TAG_ota = "OTA tool";

// extern void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

// recursos RTOS
extern SemaphoreHandle_t mutex_handles;

static void event_handler_OTA(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    xSemaphoreTake(mutex_handles, portMAX_DELAY);   // tomo el semaforo

    if (event_base == ESP_HTTPS_OTA_EVENT) {
        switch (event_id) {
            case ESP_HTTPS_OTA_START:
                ESP_LOGI(TAG_ota, "OTA started");
                break;
            case ESP_HTTPS_OTA_CONNECTED:
                ESP_LOGI(TAG_ota, "Connected to server");
                break;
            case ESP_HTTPS_OTA_GET_IMG_DESC:
                ESP_LOGI(TAG_ota, "Reading Image Description");
                break;
            case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
                ESP_LOGI(TAG_ota, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
                break;
            case ESP_HTTPS_OTA_DECRYPT_CB:
                ESP_LOGI(TAG_ota, "Callback to decrypt function");
                break;
            case ESP_HTTPS_OTA_WRITE_FLASH:
                ESP_LOGD(TAG_ota, "Writing to flash: %d written", *(int *)event_data);
                break;
            case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                ESP_LOGI(TAG_ota, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
                break;
            case ESP_HTTPS_OTA_FINISH:
                ESP_LOGI(TAG_ota, "OTA finish");
                break;
            case ESP_HTTPS_OTA_ABORT:
                ESP_LOGI(TAG_ota, "OTA abort");
                break;
        }
    }

    xSemaphoreGive(mutex_handles);   // suelto el semaforo
}


static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG_ota, "Running firmware version: %s", running_app_info.version);
    }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG_ota, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
#endif

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version) {
        ESP_LOGW(TAG_ota, "New firmware security version is less than eFuse programmed, %"PRIu32" < %"PRIu32, new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}

esp_err_t init_OTA(void){
    ESP_LOGI(TAG_ota, "OTA tool start");
    // a esta altura netif debe estar iniciada
    esp_err_t errorcito = ESP_OK;

    // _err = (esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    // if(_err != ESP_OK){
    //     ESP_LOGE(TAG_ota, "Error iniciando OTA :(");
    // }

    if(instance_OTA == NULL){
        errorcito = esp_event_handler_instance_register(ESP_HTTPS_OTA_EVENT,
                                                    NULL,//IP_EVENT_STA_GOT_IP,
                                                    event_handler_OTA,//&wifi_event_handler,
                                                    NULL,
                                                    &instance_OTA);
        if(errorcito != ESP_OK){
            ESP_LOGE(TAG_ota, "LINEA 114, Error: %d", errorcito);
            return errorcito;
        }
    }
    
    return errorcito;
}

esp_err_t update_firmware(char* _chipid){
    ESP_LOGI(TAG_ota, "Starting Advanced OTA example");

    char url_ota[OTA_URL_SIZE]; // genero la url
    snprintf(url_ota, OTA_URL_SIZE, "%s/ota/update?chip_id=%s", OTA_URL_FANIOT, _chipid);
    ESP_LOGI(TAG_ota, "URL OTA -> %s\n", url_ota);

    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = url_ota,
        // .url = "http://fabrica.faniot.ar:1880/ota/update?chip_id=C44F33605219",     // este tiene que ser por parámetro
        .cert_pem = (char *)server_cert_pem_start,                                  // ver si es necesario
        .timeout_ms = CONFIG_OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
    };

    ESP_LOGI(TAG_ota, "URL cfg -> %s\n", config.url);

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
#ifdef CONFIG_EXAMPLE_ENABLE_PARTIAL_HTTP_DOWNLOAD
        .partial_http_download = true,
        .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
#endif
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_ota, "ESP HTTPS OTA Begin failed");
        // vTaskDelete(NULL);
        return(err);
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_ota, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_ota, "image header verification failed");
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
        ESP_LOGD(TAG_ota, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG_ota, "Complete data was not received.");
    } else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(TAG_ota, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG_ota, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG_ota, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            // vTaskDelete(NULL);
            return(err);
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG_ota, "ESP_HTTPS_OTA upgrade failed");
    // vTaskDelete(NULL);
    return(err);
}

esp_err_t get_firmware_version(void){
    // esp_http_client_config_t config = {
    //     .url = "http://fabrica.faniot.ar:1880/ota/firmwareversion?chip_id=C44F33605219",
    // };

    //  // Inicializa el cliente HTTP
    // esp_http_client_handle_t client = esp_http_client_init(&config);

    // // Realiza la solicitud HTTP GET
    // esp_err_t err = esp_http_client_perform(client);

    //  if (err == ESP_OK) {
    //     ESP_LOGI("HTTP_CLIENT", "Solicitud HTTP completada con éxito");

    //     // Lee y procesa la respuesta JSON
    //     int content_length = esp_http_client_get_content_length(client);
    //     char *response_buffer = (char *)malloc(content_length + 1);
    //     int read_len = esp_http_client_read_response(client, response_buffer, content_length);
        
    //     if (read_len >= 0) {
    //         response_buffer[read_len-1] = '\0'; // Agrega el terminador nulo al final de la cadena
    //         ESP_LOGI(TAG_ota, "response -> %s", response_buffer);
    //         process_json_response(response_buffer);
    //     } else {
    //         ESP_LOGE("HTTP_CLIENT", "Error al leer la respuesta HTTP");
    //     }

    //     free(response_buffer);
    // } else {
    //     ESP_LOGE("HTTP_CLIENT", "Error durante la solicitud HTTP");
    // }

    // // Libera los recursos del cliente HTTP
    // esp_http_client_cleanup(client);

    uint8_t timer_timeout_wifi_connection = 0;

    while(wifi_connection_status.value == 0 && timer_timeout_wifi_connection >= 100){
        vTaskDelay(1);
        timer_timeout_wifi_connection++;
        ESP_LOGW("HTTP_CLIENT", "timer_timeout_wifi_connection = %u", timer_timeout_wifi_connection);
    }

    if(wifi_connection_status.value == 0 ){
        ESP_LOGE("HTTP_CLIENT", "No conectado a WiFi, no se puede obtener ultima version de firmware");
        return(ESP_FAIL);
    }else{
        timer_timeout_wifi_connection = 0;
        rest_get();
        while(new_firmware_version.value == 0 && timer_timeout_wifi_connection >= 100){
            vTaskDelay(1);
            timer_timeout_wifi_connection++;
        }

        if(new_firmware_version.value == 0){
            ESP_LOGE("HTTP_CLIENT", "Error durante la solicitud HTTP");
            return(ESP_FAIL);
        }else{
            ESP_LOGI("HTTP_CLIENT", "Nuevo firmware disponible: %s\n", new_firmware_version.value_str);
            return(ESP_OK);
        }
    }
}

void process_json_response(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE("JSON", "Error al analizar JSON");
        return;
    }

    cJSON *value = cJSON_GetObjectItem(root, "campo_json"); // Reemplaza "campo_json" con el nombre del campo que deseas obtener

    if (value != NULL) {
        ESP_LOGI("JSON", "Valor del campo JSON: %s", value->valuestring);
    } else {
        ESP_LOGE("JSON", "Campo JSON no encontrado");
    }

    if(root != NULL){
        cJSON_Delete(root);
        root = NULL;
    }
    
    if(value != NULL){
        cJSON_Delete(value);
        value = NULL;
    }
}