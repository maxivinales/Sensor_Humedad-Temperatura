#ifndef _CONFIG_H_
#define _CONFIG_H_

// Librerias externas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_wifi_types.h"
#include "esp_wifi_default.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>

#include <cJSON.h>      // librerpia para manejar Json

#include <inttypes.h>
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
// #include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "sdkconfig.h"

#include "xtensa/config/core-isa.h"

#include "cJSON.h" 

//// MQTT
// #include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
// #include <string.h>
// #include "esp_wifi.h"
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include "esp_event.h"
// #include "esp_netif.h"
// #include "protocol_examples_common.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "freertos/semphr.h"
// #include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
// #include "esp_log.h"
#include "mqtt_client.h"

#include "esp_sntp.h"

//// para JSON



// incluir todas las tareas aca
/*tareas*/

// pines
#define LED_ESP 2
#define SDA 21
#define SCL 22

// uC
#define APP_CORE 1
#define PRO_CORE 0

// constantes
#ifndef MPI         // numero pi
#define MPI 3.14159265358979323846
#endif

#define MSG_QUEUE_LENGTH 10
#define MSG_QUEUE_TOSENSOR_LENGTH 5

#define NOMBRE_PRODUCTO "Sonometro"
#define FIRMWARE_VERSION_DEFAULT "0.0.0"
#define OTA_URL_FANIOT  "http://fabrica.faniot.ar:1880"
#define MQTT_URL_FANIOT "mqtt://192.168.1.66:1883"

// Configuración SNTP
#define CONFIG_SNTP_SERVER1 "pool.ntp.org"
#define CONFIG_SNTP_SERVER2 "time.google.com"
#define CONFIG_SNTP_TIMEZONE "ART-3"        // esto hay que cambiar después
#define CONFIG_SNTP_UPDATE_DELAY 3600000 // Intervalo de actualización en milisegundos (1 hora)

//// Cuestiones referidas al WiFi
struct WiFi_data_t{
    char SSID[32];
    char pass[64];
    wifi_auth_mode_t authmode;
};

struct MQTT_user_data_t{
    char User[32];
    char pass[64];
};






// enum SOS_index{     // enumera los coeficientes del filtro
//     K_f = 0,        // constante proporcional del filtro
//     b_0 = 1,
//     b_1 = 2,
//     b_2 = 3,
//     a_0 = 4,        // a_0 debe ser siempre = 1
//     a_1 = 5,
//     a_2 = 6
// };

typedef enum    //definimos un tipo de datos categoricos, para trabajar mas comodamente
{               //puede tomar uno de los valores a continuacion
    NADA,
    SAVE,
    ERROR_RECEPCION,
    TEST,
    WIFI_MANAGER_START,
    UPDATE_TIME
} cmd_control_t;

typedef enum    //definimos un tipo de datos categoricos, para trabajar mas comodamente
{               //puede tomar uno de los valores a continuacion
    Z,
    A,
    C
} cmd_audio_weighting_t;

typedef enum{
    AT_MOST_ONCE,   // envia una vez, puede que llegue o no
    ONE_OR_MORE,    // envia y espera el recibido, puede llegar el mensaje mas de una vez
    ONE_TIME        // hace llegar exactamente un mensaje a destino, es el mas costoso computacionalmente
}QOS_MQTT;

struct data_t{
    // cmd_control_t cmd;  //mensaje de control
    int value;          //valor entero que se puede usar depende de lo que uno necesite
    float value_f;      //valor flotante que se puede usar depende de lo que uno necesite
    char  value_str[32]; //string que se puede usar depende de lo que uno necesite
};

struct data_control_t{      // datos que llegan desde MQTT al doi
    cmd_control_t cmd;
    int value;          //valor entero que se puede usar depende de lo que uno necesite
    float value_f;      //valor flotante que se puede usar depende de lo que uno necesite
    char  value_str[256]; //string que se puede usar depende de lo que uno necesite
};

struct data_mqtt_send_t{
    char topic[32];         // el topic se construye en el destino: NOMBRE_PRODUCTO/CHIPID/$topic <- esta ultima es el topic de la estructura
    char payload[256];
    QOS_MQTT qos;
    bool retain;
};

esp_err_t loadConfig();
esp_err_t saveConfig();

esp_err_t get_chipid();

esp_err_t init_sntp();
struct tm get_time_now();

char* mergeJsons(char* json1, char* json2);
char* embedJsonInObject(char* json, const char* objectName);

// Variables
struct WiFi_data_t data_WiFi_SC;
struct MQTT_user_data_t data_MQTT_SC;
char *topic_control = NULL;        // topico al que se suscribe para recibir comandos del usuario

nvs_handle_t handle_NVS;   // para guardar SSID y pass

struct data_t MAC;          // la diferencia entre CHIPID y MAC es que CHIPID está en ascii y MAC en hex
struct data_t CHIPID;
struct data_t mode_WiFi_manager = {.value = 0};    // .value = 0 -> WiFi manager encendido, .value = 1 -> WiFi manager apagado
struct data_t SSID_WiFi_Manager = {.value_str  = "Smart City"}; // nombre de la red WiFi, se cambia luego por el nombre del dispositivo y el chipid
struct data_t new_firmware_version = {.value_str = FIRMWARE_VERSION_DEFAULT};

struct data_t wifi_connection_status = {.value = 0};
struct data_t ip_status = {.value = 0};

struct tm datetime_SC;
struct data_t fecha_y_hora;
struct data_t fecha;
struct data_t hora;
struct data_t zona_horaria = {.value_str= CONFIG_SNTP_TIMEZONE};

#endif