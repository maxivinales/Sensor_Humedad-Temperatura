#include "config.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG_CONFIG = "Configuracion";


esp_err_t loadConfig(){
    ESP_LOGW(TAG_CONFIG, "Inicio ->[APP] Free memory: %lu bytes", esp_get_free_heap_size());
    esp_err_t ret = nvs_flash_init();           // inicializa el manejo de memoria flash (creo que no es la spiffs)
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { // si tira uno de esos errores
      ESP_ERROR_CHECK(nvs_flash_erase());       // limpia la memoria
      ret = nvs_flash_init();                   // e inicializa nuevamente
    }
    ESP_ERROR_CHECK(ret);                       // comprueba nuevamente los errores

    printf("Cargando configuración\n");

    ret = nvs_open("storage", NVS_READWRITE, &handle_NVS);
    if (ret != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        get_chipid();
        return(ret);
    }else{
        // Read
        printf("Reading data from NVS ... ");
        uint32_t _wifimode = 0;
        ret = nvs_get_u32(handle_NVS, "WifiMode", &_wifimode);

        size_t size_aux;
        size_aux = sizeof(data_WiFi_SC.SSID);
        ret = nvs_get_str(handle_NVS, "SSID", &data_WiFi_SC.SSID[0], &size_aux);        // recupero SSID de WiFi
        printf((ret != ESP_OK) ? "Get SSID failed!\n" : "Get SSID done\n");

        size_aux = sizeof(data_WiFi_SC.pass);
        ret = nvs_get_str(handle_NVS, "pass", &data_WiFi_SC.pass[0], &size_aux);        // recupero pass de WiFi
        printf((ret != ESP_OK) ? "Get pass failed!\n" : "Get pass done\n");

        ret = nvs_get_u32(handle_NVS, "authmode", (uint32_t*)(&data_WiFi_SC.authmode));         // recupero modo de autentificación de WiFi
        printf((ret != ESP_OK) ? "Get authmode failed!\n" : "Get authmode done\n");

        ret = nvs_get_i32(handle_NVS, "WifiMode", (int32_t*)(&mode_WiFi_manager.value));         // recupero modo de autentificación de WiFi
        printf((ret != ESP_OK) ? "Get WifiMode failed!\n" : "Get WifiMode done\n");

        //data_MQTT_SC
        size_aux = sizeof(data_MQTT_SC.User);
        ret = nvs_get_str(handle_NVS, "user_mqtt", &data_MQTT_SC.User[0], &size_aux);        // recupero SSID de WiFi
        printf((ret != ESP_OK) ? "Get user_mqtt failed!\n" : "Get user_mqtt done\n");

        size_aux = sizeof(data_MQTT_SC.pass);
        ret = nvs_get_str(handle_NVS, "pass_mqtt", &data_MQTT_SC.pass[0], &size_aux);        // recupero pass de WiFi
        printf((ret != ESP_OK) ? "Get pass_mqtt failed!\n" : "Get pass_mqtt done\n");
        
        nvs_commit(handle_NVS);
    }
    nvs_close(handle_NVS);

    get_chipid();

    // construyo el nombre de la red WiFi
    char* aux_S1;
    aux_S1 = malloc(2*sizeof(CHIPID.value_str)+1);
    snprintf(aux_S1, 2*sizeof(CHIPID.value_str)+1,  "%s (%s)", NOMBRE_PRODUCTO, CHIPID.value_str);
    strcpy(SSID_WiFi_Manager.value_str, aux_S1);
    free(aux_S1);
    // aux_S1 = &SSID_WiFi_Manager.value_str[0];
    // aux_S2 = &CHIPID.value_str[0];
    // strncat()
    // snprintf(&SSID_WiFi_Manager.value_str[0], sizeof(SSID_WiFi_Manager.value_str), "Sonometro (%s)", aux_S);

    if(topic_control == NULL){
        size_t len_topic_control;
        len_topic_control = strlen(NOMBRE_PRODUCTO) + strlen(CHIPID.value_str) + strlen("UserControl") + 2*strlen("/") + 1;
        topic_control = malloc(len_topic_control);
        snprintf(topic_control, len_topic_control, "%s/%s/UserControl", NOMBRE_PRODUCTO, CHIPID.value_str);
    }

    return(ESP_OK);
}

esp_err_t saveConfig(){
    printf("Guardando configuración\n");
    get_chipid();

    // guardo datos de la red
    esp_err_t _err;
    // storage se llama el lugar donde guardo las variables
    _err = nvs_open("storage", NVS_READWRITE, &handle_NVS);
    if (_err != ESP_OK) {
        printf("Error (%s) abriendo NVS handle! No se pueden guardar los datos\n", esp_err_to_name(_err));
        return(_err);
    } else {
        printf("Done\n");
        // Write
        printf("Updating WiFi data in NVS ... ");

        _err = nvs_set_str(handle_NVS, "SSID", data_WiFi_SC.SSID);          // guardo SSID WiFi
        printf((_err != ESP_OK) ? "save SSID failed!\n" : "save SSID done\n");

        _err = nvs_set_str(handle_NVS, "pass", data_WiFi_SC.pass);          // guardo contraseña WiFi 
        printf((_err != ESP_OK) ? "save pass failed!\n" : "save pass done\n");

        _err =nvs_set_u32(handle_NVS, "authmode" ,(uint32_t)data_WiFi_SC.authmode);// guardo modo de autentificación WiFi
        printf((_err != ESP_OK) ? "save authmode failed!\n" : "save authmode done\n");

        _err = nvs_set_str(handle_NVS, "CHIPID", CHIPID.value_str);         // guardo CHIPID
        printf((_err != ESP_OK) ? "save CHIPID failed!\n" : "save CHIPID done\n");

        _err = nvs_set_str(handle_NVS, "MAC", MAC.value_str);               // guardo MAC (chipid en uint8_t)
        printf((_err != ESP_OK) ? "save MAC failed!\n" : "save MAC done\n");

        _err =nvs_set_i32(handle_NVS, "WifiMode", mode_WiFi_manager.value); // guardo mestado del WiFi manager
        printf((_err != ESP_OK) ? "save WifiMode failed!\n" : "save WifiMode done\n");

        _err = nvs_set_str(handle_NVS, "user_mqtt", data_MQTT_SC.User);          // guardo user mqtt
        printf((_err != ESP_OK) ? "save user_mqtt failed!\n" : "save user_mqtt done\n");

        _err = nvs_set_str(handle_NVS, "pass_mqtt", data_MQTT_SC.pass);          // guardo contraseña mqtt 
        printf((_err != ESP_OK) ? "save pass_mqtt failed!\n" : "save pass_mqtt done\n");

        printf("Committing updates in NVS ... ");
        _err = nvs_commit(handle_NVS);
        printf((_err != ESP_OK) ? "Committing failed!\n" : "Committing done\n");
    }

    // Close
    nvs_close(handle_NVS);
    
    return(ESP_OK);
}

esp_err_t get_chipid(){
    esp_err_t _err;
    
    uint8_t *aux_MAC;
    char *aux_MAC_ch;
    aux_MAC = malloc(13*sizeof(char));
    aux_MAC_ch = (char*)aux_MAC;
    // aux_MAC_ch = &MAC.value_str[0];
    // aux_MAC = (uint8_t)aux_MAC_ch;
    _err = esp_efuse_mac_get_default(aux_MAC);
    strcpy(MAC.value_str, aux_MAC_ch);
    free(aux_MAC_ch);
    
    if(_err != ESP_OK){
        printf("error solicitando la MAC/ChipID. err = %d\n", (int)_err);
    }else{
        snprintf(&CHIPID.value_str[0], 13, "%02X%02X%02X%02X%02X%02X", MAC.value_str[0], MAC.value_str[1], MAC.value_str[2], MAC.value_str[3], MAC.value_str[4], MAC.value_str[5]);
        printf("ChipID / MAC = %s\n", CHIPID.value_str);

    }
    return(_err);
}