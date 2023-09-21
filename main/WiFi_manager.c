#include "WiFi_manager.h"
#include "config.h"
#include "esp_err.h"

#include "esp_netif_types.h"
#include "freertos/projdefs.h"
#include "mqtt.h"
// #include "ota.c"

static const char *TAG_WiFi_Manager = "prueba WiFi manager";

extern SemaphoreHandle_t mutex_handles;

void buildJsonNets(char* _Json, wifi_ap_record_t* _redes, uint32_t _size) {
    // Calcula el tamaño total necesario para almacenar el JSON
    // size_t jsonSize = strlen("{\"Nets\":[") + (_size - 1) * (strlen("{\"SSID\":\"\",\"authmode\":,\"rssi\":},") + 20) + strlen("]}") + 1;
    // char* _Json = (char*)malloc(jsonSize);

    if (_Json) {
        strcpy(_Json, "{\"Nets\":[");

        for (int i = 0; i < _size; i++) {
            int _len_rssi = snprintf(NULL, 0, "%d", _redes->rssi);
            int _len_authmode = snprintf(NULL, 0, "%d", _redes->authmode);

            // Asegura espacio suficiente para el nuevo elemento JSON
            size_t entrySize = strlen("{\"SSID\":\"\",\"authmode\":,\"rssi\":},") + strlen(&_redes->ssid) + _len_rssi + _len_authmode;
            char* entry = (char*)malloc(entrySize);
            // if (!entry) {
            //     // Manejar el error de asignación de memoria
            //     free(_Json);
            //     return NULL;
            // }

            if (i > 0) {
                strcat(_Json, ",");
            }

            snprintf(entry, entrySize, "{\"SSID\":\"%s\",\"authmode\":%d,\"rssi\":%d}", _redes->ssid, _redes->authmode, _redes->rssi);
            strcat(_Json, entry);

            free(entry);
            _redes++;
        }

        strcat(_Json, "]}");
    }

    // return _Json;
}


uint32_t decode_Json(const char* _Json, struct WiFi_data_t *WiFi_data, struct MQTT_user_data_t *MQTT_data){
    ESP_LOGW(TAG_WiFi_Manager, "decode_Json\n");

    uint32_t ret = 0;
    
    if (_Json == NULL) {
        ESP_LOGW(TAG_WiFi_Manager, "No se proporcionó un JSON válido\n");
        return(0);
    }

    cJSON *Jsonsito = cJSON_Parse(_Json);
    if(Jsonsito == NULL) {
        ESP_LOGW(TAG_WiFi_Manager, "Error al analizar el JSON\n");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        return(0);
    }

    cJSON *get_chipid = cJSON_GetObjectItemCaseSensitive(Jsonsito, "get_chipid");
    if (get_chipid != NULL) {
        printf("get_chipid: %d\n", get_chipid->valueint);
        if(get_chipid->valueint == 1){
            ret = ret|1;            // escribo 1 bit
        }
    } else {
        ESP_LOGW(TAG_WiFi_Manager, "Objeto get_chipid no encontrado o no válido\n");
    }

    ret = ret<<1;           // desplazo todos los bits hacia la izquierda

    cJSON *off_WM_mode = cJSON_GetObjectItemCaseSensitive(Jsonsito, "off_WM_mode");
    if (off_WM_mode != NULL) {
        printf("off_WM_mode: %d\n", off_WM_mode->valueint);
        if(off_WM_mode->valueint == 1){
            ret = ret|1;            // escribo 1 bit
        }
    } else {
        ESP_LOGW(TAG_WiFi_Manager, "Objeto off_WM_mode no encontrado o no válido\n");
    }

    ret = ret<<1;           // desplazo todos los bits hacia la izquierda
    cJSON *MQTT = cJSON_GetObjectItemCaseSensitive(Jsonsito, "MQTT");
    if (MQTT != NULL && cJSON_IsObject(MQTT)) {
        cJSON *MQTTUserItem = cJSON_GetObjectItemCaseSensitive(MQTT, "User");
        cJSON *MQTTpassItem = cJSON_GetObjectItemCaseSensitive(MQTT, "Pass");

        if (cJSON_IsString(MQTTUserItem) && cJSON_IsString(MQTTpassItem)) {
            printf("MQTT User: %s\n", MQTTUserItem->valuestring);
            printf("MQTT Pass: %s\n", MQTTpassItem->valuestring);

            // Guardo credenciales MQTT
            // MQTT_data->User = MQTTUserItem->valuestring;
            strcpy(MQTT_data->User, MQTTUserItem->valuestring);
            // MQTT_data->pass = MQTTpassItem->valuestring;
            strcpy(MQTT_data->pass, MQTTpassItem->valuestring);

            ret = ret|1;            // escribo 1 bit
        } else {
            ESP_LOGW(TAG_WiFi_Manager, "Valores inválidos para MQTT User o Pass\n");
            MQTT_data = NULL;
        }
    } else {
        ESP_LOGW(TAG_WiFi_Manager, "Objeto MQTT no encontrado o no válido\n");
    }


    ret = ret<<1;           // desplazo todos los bits hacia la izquierda
    cJSON *WiFi_conn_req = cJSON_GetObjectItemCaseSensitive(Jsonsito, "WiFi_connection_request");
    if (WiFi_conn_req != NULL) {
        printf("WiFi_connection_request: %d\n", WiFi_conn_req->valueint);
        if(WiFi_conn_req->valueint == 1){
            ret = ret|1;            // escribo 1 bit
        }
    } else {
        ESP_LOGW(TAG_WiFi_Manager, "Objeto WiFi_connection_request no encontrado o no válido\n");
    }

    ret = ret<<1;           // desplazo todos los bits hacia la izquierda
    cJSON *WiFi = cJSON_GetObjectItemCaseSensitive(Jsonsito, "WiFi");
    if (WiFi != NULL && cJSON_IsObject(WiFi)) {
        cJSON *WiFissidItem = cJSON_GetObjectItemCaseSensitive(WiFi, "SSID");
        cJSON *WiFipassItem = cJSON_GetObjectItemCaseSensitive(WiFi, "Pass");
        cJSON *WiFiauthmodeItem = cJSON_GetObjectItemCaseSensitive(WiFi, "authmode");

        if (cJSON_IsString(WiFissidItem) && cJSON_IsString(WiFipassItem)) {
            printf("WiFi SSID: %s\n", WiFissidItem->valuestring);
            printf("WiFi Pass: %s\n", WiFipassItem->valuestring);
            printf("WiFi authmode: %d\n", WiFiauthmodeItem->valueint);

            // guardo credenciales de WiFi
            strcpy(WiFi_data->SSID, WiFissidItem->valuestring);
            // WiFi_data->SSID = WiFissidItem->valuestring;
            strcpy(WiFi_data->pass, WiFipassItem->valuestring);
            // WiFi_data->pass = WiFipassItem->valuestring;
            WiFi_data->authmode = (wifi_auth_mode_t)(WiFiauthmodeItem->valueint);
            
            ret = ret|1;            // escribo 1 bit
        } else {
            ESP_LOGW(TAG_WiFi_Manager, "Valores inválidos para WiFi SSID o Pass\n");
            WiFi_data = NULL;
        }
    } else {
        ESP_LOGW(TAG_WiFi_Manager, "Objeto WiFi no encontrado o no válido\n");
    }

    cJSON_Delete(Jsonsito);
    ESP_LOGW(TAG_WiFi_Manager, "Fin decode_Json\n");
    printf("ret -> %lu\n", ret);
    return(ret);
}

//// TCP/IP ////
esp_err_t tcpip_init_AP(void){
    esp_err_t _err = ESP_OK;
    _err = esp_netif_init();                  // inicia TCP/IP
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 171, Error: %d", _err);
        return _err;
    }

    if(netif_wifi_AP == NULL){
        netif_wifi_AP = esp_netif_create_default_wifi_ap();                 // inicia la netif para el AP
    }

    return _err;
}

esp_err_t tcpip_deinit_AP(void){
    esp_err_t _err = ESP_OK;
    esp_netif_destroy_default_wifi(netif_wifi_AP);// destruyo el objeto que en teoria no uso mas
    _err = esp_netif_deinit();
    netif_wifi_AP = NULL;
    return _err;
}

esp_err_t tcpip_init_STA(void){
    esp_err_t _err = ESP_OK;
    _err = esp_netif_init();                  // inicia TCP/IP

    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 195, Error: %d", _err);
        return _err;
    }

    if(netif_wifi_STA == NULL){
        netif_wifi_STA = esp_netif_create_default_wifi_sta();                 // inicia la netif para el AP
    }

    return _err;
}

esp_err_t tcpip_deinit_STA(void){
    esp_err_t _err = ESP_OK;
    esp_netif_destroy_default_wifi(netif_wifi_STA);// destruyo el objeto que en teoria no uso mas
    _err = esp_netif_deinit();
    netif_wifi_STA = NULL;
    return _err;
}

esp_err_t init_loop_default(void){
    esp_err_t _err = ESP_OK;
    
    if(loop_flag == false){
        _err = esp_event_loop_create_default();   // crea el loop para la tarea
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 220, Error: %d", _err);
            return _err;
        }
        loop_flag = true;
    }
    return _err;
}

void deinit_loop_default(void){
    esp_event_loop_delete_default();
    loop_flag = false;
}

////    WiFi    ////

void wifi_init_softap(void)
{

    // ESP_ERROR_CHECK(esp_netif_init());                  // inicia TCP/IP

    if(loop_flag == false){
        ESP_ERROR_CHECK(init_loop_default());   // crea un event loop
    }

    if(netif_wifi_AP == NULL){
        ESP_ERROR_CHECK(tcpip_init_AP());
        // netif_wifi_AP = esp_netif_create_default_wifi_ap();                 // inicia la netif para el AP
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();    // inicializa las configuraciones WiFi por defecto
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if(instance_any_id == NULL){
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,     // crea un evento
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        instance_any_id));
    }


    wifi_config_t wifi_config = {                       // configuraciones del AP
        .ap = {
            //.ssid = WIFI_SSID,
            .ssid_len = strlen(SSID_WiFi_Manager.value_str),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };

    uint8_t* aux_u8;
    aux_u8 = &wifi_config.ap.ssid[0]; 
    char* aux_S;
    aux_S = (char*)aux_u8;
    strcpy(aux_S, SSID_WiFi_Manager.value_str);
    
    // wifi_config_SC = &wifi_config;

    if (strlen(WIFI_PASS) == 0) {       // si no le pongo nada en la contraseña es WiFi abierto
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));                              // seteo el modo del WiFi
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));     // guardo la configuracion
    ESP_ERROR_CHECK(esp_wifi_start());                                                  // pongo en marcha el AP

    ESP_LOGI(TAG_WiFi_Manager, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             SSID_WiFi_Manager.value_str, WIFI_PASS, WIFI_CHANNEL);
}

void wifi_deinit_softap(void){
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,     // elimina un evento
    //                                                     ESP_EVENT_ANY_ID,
    //                                                     instance_any_id));
    if(netif_wifi_AP != NULL){
        tcpip_deinit_AP();
    }

    if(loop_flag == true){
        esp_event_loop_delete_default();
    }
    
    esp_wifi_stop();
}

esp_err_t wifi_init_sta(struct WiFi_data_t _net){
    ESP_LOGE(TAG_WiFi_Manager, "Net -> SSID =  %s\tpass: %s", _net.SSID, _net.pass);
    esp_err_t _err = ESP_OK;

    if(loop_flag == false){
        _err = init_loop_default();
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 317, Error: %d", _err);
            return _err;
        }
    }

    if(netif_wifi_STA == NULL){
        _err = tcpip_init_STA();
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 325, Error: %d", _err);
            return _err;
        }
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();    // inicializa las configuraciones WiFi por defecto
    _err = (esp_wifi_init(&cfg));
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 333, Error: %d", _err);
        return _err;
    }


   if(instance_any_id == NULL){
        _err = esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &wifi_event_handler,
                                                    NULL,
                                                    &instance_any_id);
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 351, Error: %d", _err);
            return _err;
        }
    }

    if(instance_got_ip == NULL){
        _err = esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &wifi_event_handler,
                                                    NULL,
                                                    &instance_got_ip);
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 363, Error: %d", _err);
            return _err;
        }
    }
    
    
    wifi_config_t wifi_config_sta = {
        .sta = {
            // .ssid = _ssid,
            // .password = _password,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = _net.authmode,        // aca iria la configuracion de WiFi
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    uint8_t *aux1;

    aux1 = (uint8_t*)_net.SSID;    
    for(int i = 0; i<strlen(_net.SSID); i++){
        wifi_config_sta.sta.ssid[i] = *aux1;
        aux1++;
        // if(i == strlen(_net.SSID)){
        //     wifi_config.sta.ssid[i] = (uint8_t)("\0");
        // }
    }
    ESP_LOGW(TAG_WiFi_Manager, "SSID -> %s", wifi_config_sta.sta.ssid);

    aux1 = (uint8_t*)_net.pass;    
    for(int i = 0; i<strlen(_net.pass); i++){
        wifi_config_sta.sta.password[i] = *aux1;
        aux1++;
        // if(i == strlen(_net.pass)){
        //     wifi_config.sta.password[i] = (uint8_t)("\0");
        // }
    }
    ESP_LOGW(TAG_WiFi_Manager, "pass -> %s", wifi_config_sta.sta.password);

    

    _err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 403, Error: %d", _err);
        return _err;
    }


    _err = (esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 410, Error: %d", _err);
        return _err;
    }

    _err = (esp_wifi_start() );
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 416, Error: %d", _err);
        return _err;
    }

   return _err;
}

void wifi_deinit_sta(void){
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,     // elimina un evento
    //                                                     ESP_EVENT_ANY_ID,
    //                                                     instance_any_id));
    
    // ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,     // elimina un evento
    //                                                     IP_EVENT_STA_GOT_IP,
    //                                                     instance_got_ip));
    if(netif_wifi_STA != NULL){
        tcpip_deinit_STA();
    }

    if(loop_flag == true){
        esp_event_loop_delete_default();
    }
    esp_wifi_stop();
}

esp_err_t wifi_init_apsta(struct WiFi_data_t _net){
    esp_err_t _err = ESP_OK;

    if(loop_flag == false){
        ESP_ERROR_CHECK(init_loop_default());   // crea un event loop
    }

    if(netif_wifi_AP == NULL){
        _err = (tcpip_init_AP());
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 451, Error: %d", _err);
            return _err;
        }
    }

     if(netif_wifi_STA == NULL){
        _err = tcpip_init_STA();
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 459, Error: %d", _err);
            return _err;
        }
    }

    
    if(instance_any_id == NULL){
        _err = esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &wifi_event_handler,
                                                    NULL,
                                                    &instance_any_id);
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 472, Error: %d", _err);
            return _err;
        }
    }

    if(instance_got_ip == NULL){
        _err = esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &wifi_event_handler,
                                                    NULL,
                                                    &instance_got_ip);
        if(_err != ESP_OK){
            ESP_LOGE(TAG_WiFi_Manager, "LINEA 484, Error: %d", _err);
            return _err;
        }
    }

    
    wifi_config_t wifi_config_sta = {
        .sta = {
            // .ssid = _ssid,
            // .password = _password,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = _net.authmode,        // aca iria la configuracion de WiFi
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    uint8_t *aux1;

    aux1 = (uint8_t*)_net.SSID;    
    for(int i = 0; i<strlen(_net.SSID); i++){
        wifi_config_sta.sta.ssid[i] = *aux1;
        aux1++;
    }
    ESP_LOGW(TAG_WiFi_Manager, "SSID -> %s", wifi_config_sta.sta.ssid);

    aux1 = (uint8_t*)_net.pass;    
    for(int i = 0; i<strlen(_net.pass); i++){
        wifi_config_sta.sta.password[i] = *aux1;
        aux1++;
    }
    ESP_LOGW(TAG_WiFi_Manager, "pass -> %s", wifi_config_sta.sta.password);

    

    _err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 524, Error: %d", _err);
        return _err;
    }

    _err = (esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 530, Error: %d", _err);
        return _err;
    }

    _err = (esp_wifi_start() );
    if(_err != ESP_OK){
        ESP_LOGE(TAG_WiFi_Manager, "LINEA 536, Error: %d", _err);
        return _err;
    }

    ESP_LOGI(TAG_WiFi_Manager, "wifi_init_apsta finished.");

    
    return _err;
}

void wifi_deinit_apsta(void){

    if(netif_wifi_AP != NULL){
        tcpip_deinit_AP();
    }

    if(netif_wifi_STA != NULL){
        tcpip_deinit_STA();
    }

    if(loop_flag == true){
        esp_event_loop_delete_default();
    }
    esp_wifi_stop();
}

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    xSemaphoreTake(mutex_handles, portMAX_DELAY);   // tomo el semaforo

    if(event_base == WIFI_EVENT){
        ESP_LOGW(TAG_WiFi_Manager,"Evento -> WIFI_EVENT");
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG_WiFi_Manager, "WiFi STA started");
            esp_wifi_connect();
            break;
        
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG_WiFi_Manager, "WiFi STA Connected to %s", data_WiFi_SC.SSID);
            wifi_connection_status.value = 1;   // levanto mi bandera de que WiFi está conectado
            break;
        
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG_WiFi_Manager, "WiFi STA disconnected");
            wifi_connection_status.value = 0;   // bajo mi bandera de que WiFi está conectado
            break;
        
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG_WiFi_Manager, "WiFi AP started");
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG_WiFi_Manager, "WiFi AP stopped");
            break;
        
        case WIFI_EVENT_AP_STACONNECTED:
            wifi_event_ap_staconnected_t* event_apstaconnnected = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG_WiFi_Manager, "station "MACSTR" join, AID=%d",                                          // imprime el dispositivo que se conecto
                    MAC2STR(event_apstaconnnected->mac), event_apstaconnnected->aid);
            
            wifi_connection_status.value = 1;   // levanto mi bandera de que WiFi está conectado
            
            break;
        
        case WIFI_EVENT_AP_STADISCONNECTED:
            wifi_event_ap_stadisconnected_t* event_ap_sta_disconnected = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG_WiFi_Manager, "station "MACSTR" leave, AID=%d",
                    MAC2STR(event_ap_sta_disconnected->mac), event_ap_sta_disconnected->aid);                                              // imprime el dispositivo que se desconecto
            
            wifi_connection_status.value = 0;   // bajo mi bandera de que WiFi está conectado

            break;
        
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t *event_stagotip = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG_WiFi_Manager, "Got IP:" IPSTR, IP2STR(&event_stagotip->ip_info.ip));

            wifi_connection_status.value = 1;   // levanto mi bandera de que WiFi está conectado
            ip_status.value = 1;                // levanto mi bandera de que tengo direccion IP

            break;
        
        default:
            break;
        }
    }else if(event_base == IP_EVENT){
        ESP_LOGW(TAG_WiFi_Manager,"Evento -> IP_EVENT");
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t *event_stagotip = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG_WiFi_Manager, "Got IP:" IPSTR, IP2STR(&event_stagotip->ip_info.ip));
            ip_status.value = 1;                // levanto mi bandera de que tengo direccion IP
            snprintf(ip_status.value_str, sizeof(ip_status.value_str), IPSTR, IP2STR(&event_stagotip->ip_info.ip));
            break;
        case IP_EVENT_STA_LOST_IP:
            ip_status.value = 0;                // bajo mi bandera de que tengo direccion IP
            break;
        default:
            break;
        }
    }

    xSemaphoreGive(mutex_handles);   // suelto el semaforo
}

///////////////////

////    HTTP    ////
/* An HTTP POST handler */
static esp_err_t connect_post_handler(httpd_req_t *req)
{
    char buf[req->content_len + 1];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        uint32_t requested_actions;
        requested_actions = decode_Json(&buf[0], &data_WiFi_SC, &data_MQTT_SC);
        /*
        requested_actions contiene informacion bit a bit, a continuacuión se
        describe que hace cada bit:
        0   ->  si está seteado quiere decir que se tienen datos para conectarnos a una red WiFi
        1   ->  si está seteado quiere decir que tenemos que pasarle la red a la cual estamos conectados
                al cliente
        2   ->  si está seteado quiere decir que tenemos datos para conectarnos al broker MQTT
        3   ->  Si está seteado quiere decir que deberíamos apagar el WiFi manager
        4   ->  Solicitud del chipid
        LOS DEMAS BITS POR AHORA ESTAN RESERVADOS
        */
        if(requested_actions == 0){ // si es cero se produjo un error
            httpd_resp_send_404(req);   // devuelvo un 404
        }else{
            if(requested_actions&1){    // bit 0
                ESP_LOGW(TAG_WiFi_Manager, "bit 0: me conecto a una red WiFi\n");// debug Cukla
                char *aux_s = "Datos recibidos";
                httpd_resp_send(req, aux_s, strlen(aux_s));  // supuestamente es 200
                vTaskDelay(pdMS_TO_TICKS(250));
                
                
                // guardo datos de la red
                esp_err_t _err;
                // storage se llama el lugar donde guardo las variables
                _err = nvs_open("storage", NVS_READWRITE, &handle_NVS);
                if (_err != ESP_OK) {
                    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(_err));
                } else {
                    printf("Done\n");
                    // Write
                    printf("Updating WiFi data in NVS ... ");

                    _err = nvs_set_str(handle_NVS, "SSID", data_WiFi_SC.SSID);
                    printf((_err != ESP_OK) ? "save SSID failed!\n" : "save SSID done\n");

                    _err = nvs_set_str(handle_NVS, "pass", data_WiFi_SC.pass);
                    printf((_err != ESP_OK) ? "save pass failed!\n" : "save pass done\n");

                    _err =nvs_set_u32(handle_NVS, "authmode", (uint32_t)data_WiFi_SC.authmode);

                    printf("Committing updates in NVS ... ");
                    _err = nvs_commit(handle_NVS);
                    printf((_err != ESP_OK) ? "Committing failed!\n" : "Committing done\n");
                    // Close
                    nvs_close(handle_NVS);
                }
                esp_wifi_stop();
                wifi_init_apsta(data_WiFi_SC);                
            }
            requested_actions = requested_actions>>1;   // corro 1 bit a la derecha
            if(requested_actions&1){    // bit 1
                ESP_LOGW(TAG_WiFi_Manager, "bit 1: me piden la red WiFi a la que estoy conectado\n");// debug Cukla
                wifi_ap_record_t AP_data;
                esp_err_t err_conn = ESP_OK;
                err_conn = esp_wifi_sta_get_ap_info(&AP_data);
                ESP_LOGW(TAG_WiFi_Manager, "WiFi conectado -> %s\n", AP_data.ssid);
                char* aux;
                uint8_t *aux2;
                aux2 = &AP_data.ssid[0];
                aux = (char*)aux2;

                char response[128];
                if(mqtt_connected){ // VER SI PONER sizeof(response) en lugar del 128
                    snprintf(response, 128, "{\"WiFi SSID\":\"%s\",\"MQTT user\":\"%s\"}", aux, data_MQTT_SC.User);
                }else{
                    snprintf(response, 128, "{\"WiFi SSID\":\"%s\",\"MQTT user\":\"%s\"}", aux, "none");
                }
                aux = &response[0];

                httpd_resp_set_status(req, http_200_hdr);
                httpd_resp_set_type(req, http_content_type_json);
                httpd_resp_send(req, aux, strlen(aux));
            }   
            requested_actions = requested_actions>>1;   // corro 1 bit a la derecha
            if(requested_actions&1){    // bit 2
                ESP_LOGW(TAG_WiFi_Manager, "bit 2: pendiente aún, acá debería conectarse al broker mqtt\n");// debug Cukla
                saveConfig();
                httpd_resp_send(req, "Intentando conectar a MQTT...", strlen("Intentando conectar a MQTT..."));
                int cont_mqtt_launch = 0;
                while(ip_status.value != 1 && cont_mqtt_launch < 50){
                    vTaskDelay(pdMS_TO_TICKS(100));
                    cont_mqtt_launch++;
                    ESP_LOGI(TAG_WiFi_Manager, "cont_mqtt_launch -> %d", cont_mqtt_launch);
                }

                if(ip_status.value == 1){
                    mqtt_launch();
                }else{
                    ESP_LOGE(TAG_WiFi_Manager, "No está conectado a WiFi, no se puede conectar a MQTT");
                }

                
            }
            requested_actions = requested_actions>>1;   // corro 1 bit a la derecha
            if(requested_actions&1){    // bit 3
                ESP_LOGW(TAG_WiFi_Manager, "bit 3: acá debería pasar todalmente a modo STA\n");// debug Cukla

                esp_err_t _err;
                // storage se llama el lugar donde guardo las variables
                wifi_ap_record_t AP_data;
                esp_err_t err_conn = ESP_OK;
                err_conn = esp_wifi_sta_get_ap_info(&AP_data);

                char* aux;
                uint8_t *aux2;
                aux2 = &AP_data.ssid[0];
                aux = (char*)aux2;
                if(strcmp(aux, "") == 0){
                    httpd_resp_set_status(req, http_400_hdr);
                    httpd_resp_set_type(req, http_content_type_json);
                    httpd_resp_send(req, "No conectado a WiFi, conectese a una red SSID y contraseña validos", strlen("No conectado a WiFi, conectese a una red SSID y contraseña validos"));
                }else if(mqtt_connected == false){
                    httpd_resp_set_status(req, http_400_hdr);
                    httpd_resp_set_type(req, http_content_type_json);
                    httpd_resp_send(req, "Falla en conexión a MQTT, escriba un usuario y contraseña validos", strlen("Falla en conexión a MQTT, escriba un usuario y contraseña validos"));
                }
                else{
                    httpd_resp_set_status(req, http_200_hdr);
                    httpd_resp_set_type(req, http_content_type_json);
                    httpd_resp_send(req, "Pasando a modo de trabajo STA, el WiFi manager se apagará", strlen("Pasando a modo de trabajo STA, el WiFi manager se apagará"));
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    // wifi_deinit_softap();
                    // corroboro que esté guardado ok la red
                    _err = nvs_open("storage", NVS_READWRITE, &handle_NVS);
                    if (_err != ESP_OK) {
                        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(_err));
                    }else{
                        
                        //
                        if(strcmp(aux, data_WiFi_SC.SSID) == 0){
                            // Write
                            printf("guardo dato que indica que la ESP debe iniciar en modo STA");

                            mode_WiFi_manager.value = 1;                                            // guardo el valor en la configuración

                            _err = nvs_set_u32(handle_NVS, "WifiMode", 1);       // 1 = Wifi manager apagado, 0 = encendido
                            printf((_err != ESP_OK) ? "save WifiMode failed!\n" : "save SSID done\n");
                            nvs_commit(handle_NVS);
                            nvs_close(handle_NVS);
                            ESP_LOGW(TAG_WiFi_Manager, "Reinicio la ESP...");
                            saveConfig();
                            esp_restart();
                        }
                    }

                    nvs_close(handle_NVS);
                }

            }

            requested_actions = requested_actions>>1;   // corro 1 bit a la derecha
            if(requested_actions&1){    // bit 4
                ESP_LOGW(TAG_WiFi_Manager, "bit 4: Se envía \n");// debug Cukla
                char *aux_s;
                esp_err_t _err;
                _err = esp_efuse_mac_get_default(&mac_address_or_chipid[0]);
                if(_err != ESP_OK){
                    ESP_LOGI(TAG_WiFi_Manager, "error solicitando la MAC/ChipID. err = %d\n", (int)_err);
                }else{
                    snprintf(&ChipId[0], 13, "%02X%02X%02X%02X%02X%02X", mac_address_or_chipid[0], mac_address_or_chipid[1], mac_address_or_chipid[2], mac_address_or_chipid[3], mac_address_or_chipid[4], mac_address_or_chipid[5]);
                    ESP_LOGI(TAG_WiFi_Manager, "ChipID / MAC = %s\n", ChipId);
                    aux_s = &ChipId[0];
                    httpd_resp_send(req, aux_s, strlen(aux_s));  // supuestamente es 200
                }
            }
        }

    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    ESP_LOGW(TAG_WiFi_Manager, "linea 398");// debug Cukla
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        // Return ESP_OK to keep underlying socket open
        ESP_LOGW(TAG_WiFi_Manager, "linea 402");// debug Cukla
        return ESP_OK;
    } else if (strcmp("/connect", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/connect URI is not available");
        ESP_LOGW(TAG_WiFi_Manager, "linea 406");// debug Cukla
        // Return ESP_FAIL to close underlying socket
        return ESP_FAIL;
    }
    // For any other URI send 404 and close socket
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

// paso inicial para el WiFi Manager
static esp_err_t WiFi_Manager_handler(httpd_req_t *req)
{
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    // httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, http_200_hdr);
    httpd_resp_set_type(req, http_content_type_html);
    httpd_resp_send(req, (char*)index_html_start, index_html_end - index_html_start);
    // httpd_resp_set_type(req, http_content_type_js);
	// httpd_resp_send(req, (char*)code_js_start, code_js_end - code_js_start);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG_WiFi_Manager, "Request headers lost");
    }
    return ESP_OK;
}

static esp_err_t JS_handler(httpd_req_t *req)
{

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    // httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, http_200_hdr);
    httpd_resp_set_type(req, http_content_type_js);
	httpd_resp_send(req, (char*)code_js_start, code_js_end - code_js_start);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG_WiFi_Manager, "Request headers lost");
    }
    return ESP_OK;
}

static esp_err_t CSS_handler(httpd_req_t *req)
{
    
    const char* resp_str = (const char*) req->user_ctx;
    // httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, http_200_hdr);
    httpd_resp_set_type(req, http_content_type_css);
    httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_cache);
    httpd_resp_send(req, (char*)style_css_start, style_css_end - style_css_start);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG_WiFi_Manager, "Request headers lost");
    }

    return ESP_OK;
}

static esp_err_t WiFi_PullNets(httpd_req_t *req)
{

    //  traigo las redes WiFi
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));        // guardo las redes WiFi en una lista
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));                               // toma la cantidad de redes que leyó
    ESP_LOGI(TAG_WiFi_Manager, "Total APs scanned = %u", ap_count);

    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG_WiFi_Manager, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    
    int num_redes = 0;
    for(int i = 0; i < DEFAULT_SCAN_LIST_SIZE; i++){
        if(i > 0){
            if(strcmp(&ap_info[num_redes].ssid, &ap_info[i].ssid) != 0){
                // aux_nets = ap_info[i];
                num_redes++;
                ap_info[num_redes] = ap_info[i];

                if(strcmp(&ap_info[num_redes].ssid, "") == 0){
                    num_redes--;
                    i = DEFAULT_SCAN_LIST_SIZE - 1;
                }
            }
        }else{
            if(strcmp(&ap_info[num_redes].ssid, "") == 0){
                num_redes--;
                i = DEFAULT_SCAN_LIST_SIZE - 1;
            }else{
                ap_info[num_redes] = ap_info[i];
            }
        }
    }

    char Json[JSON_BUFFER_SIZE];
    char* resp_str = &Json[0];
    buildJsonNets(resp_str, &ap_info[0], num_redes+1);       // genero el json

    // Agregar los encabezados CORS necesarios
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    httpd_resp_set_status(req, http_200_hdr);
    httpd_resp_set_type(req, http_content_type_json);
    ESP_LOGW(TAG_WiFi_Manager, "Envio de Json ->[APP] Free memory: %lu bytes", esp_get_free_heap_size());
	httpd_resp_send(req, resp_str, (int)strlen(resp_str));
    ESP_LOGW(TAG_WiFi_Manager, "longitud del JSON de redes -> %d\n", (int)strlen(resp_str));
    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG_WiFi_Manager, "Request headers lost");
        // free(buf);
    }
    return ESP_OK;
}

static esp_err_t cors_options_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);  // Responder sin contenido
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    ESP_LOGW(TAG_WiFi_Manager, "linea 552");// debug Cukla
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server);

    httpd_handle_t server = NULL;                   // creo el puntero que maneja el servidor
    httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // cargo la configuracion por defecto
    config.stack_size = 10*config.stack_size;        // a ver si asi no se me cuelga
    
    // config.lru_purge_enable = true;                 // permite que ciere el socket si es necesario
    

    // Start the httpd server
    ESP_LOGI(TAG_WiFi_Manager, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {  // inicia el servidor
        // Set URI handlers
        ESP_LOGI(TAG_WiFi_Manager, "Registering URI handlers");
        // registra los URI para cada caso
        // httpd_register_uri_handler(server, &hello); // get
        httpd_register_uri_handler(server, &connect_WM);  // post
        // httpd_register_uri_handler(server, &ctrl);  // put
        httpd_register_uri_handler(server, &wifimanager);  // pseudo wifi manager
        httpd_register_uri_handler(server, &jswifimanager); 
        httpd_register_uri_handler(server, &csswifimanager); 
        httpd_register_uri_handler(server, &PullNets); 
        httpd_register_uri_handler(server, &cors_uri);

        #if CONFIG_EXAMPLE_BASIC_AUTH
        httpd_register_basic_auth(server);  // supongo que carga las credenciales con respecto al servidor
        #endif
        return server;                      // si llegó hasta acá está todo ok
    }

    ESP_LOGI(TAG_WiFi_Manager, "Error starting server!");// si llegó hasta acá está todo mal xD
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)  // para detener el servidor
{
    // Stop the httpd server
    // httpd_unregister_uri_handler()
    return httpd_stop(server);
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG_WiFi_Manager, "Starting webserver");
        *server = start_webserver();
    }
}

// funcion para detener la pila de eventos relacionada con http
static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG_WiFi_Manager, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG_WiFi_Manager, "Failed to stop http server");
        }
    }
}