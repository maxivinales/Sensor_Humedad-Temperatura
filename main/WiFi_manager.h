#include "config.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "esp_err.h"
// #include "esp_wifi_types.h"
// #include "esp_wifi_default.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/projdefs.h"
// #include "freertos/task.h"
// #include "esp_mac.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"

// #include "lwip/err.h"
// #include "lwip/sys.h"

// #include <esp_system.h>
// #include <sys/param.h>
// #include "esp_netif.h"
// #include "esp_tls_crypto.h"
// #include <esp_http_server.h>
// #include "freertos/event_groups.h"

// #include <cJSON.h>      // librerpia para manejar Json

#include "ota.h"

// #define WIFI_SSID      "Test SC "
#define WIFI_PASS      ""
#define WIFI_CHANNEL   1
#define MAX_STA_CONN       4

#define SCAN_METHOD WIFI_FAST_SCAN
#define SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define RSSI -127
#define AUTHMODE WIFI_AUTH_OPEN

#define DEFAULT_SCAN_LIST_SIZE 16   // coincide con la maxima cantidad de resultados que se puede tener

#define JSON_BUFFER_SIZE 1200

// crea instancias para eventos referidos al WiFi
esp_event_handler_instance_t instance_any_id = NULL;
esp_event_handler_instance_t instance_got_ip = NULL;
// nvs_handle_t handle_wifi;   // para guardar SSID y pass

typedef enum{
    OK_WME,         // sin errores, WiFi Manager sigue encendido
    OK_WMD,         // sin errores, WiFi Manager sigue encendido
    ERR_WIFI,       // error en obtención de los parámetros del WiFi
    ERR_MQTT,       // error en la obtención de los parámetros MQTT
    ERR_WM,         // error en la obtención de los parámetros del WiFI Manager
    ERR_GRAL        // no funciona el Json en gral
}DATA_JSON_OUTPUT;

//// Archivos externos  ////
extern const uint8_t style_css_start[] asm("_binary_styles_css_start");
extern const uint8_t style_css_end[]   asm("_binary_styles_css_end");
extern const uint8_t code_js_start[] asm("_binary_script_js_start");
extern const uint8_t code_js_end[] asm("_binary_script_js_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/* const httpd related values stored in ROM */
const static char http_200_hdr[] = "200 OK";
// const static char http_302_hdr[] = "302 Found";
const static char http_400_hdr[] = "400 Bad Request";
// const static char http_404_hdr[] = "404 Not Found";
// const static char http_503_hdr[] = "503 Service Unavailable";
// const static char http_location_hdr[] = "Location";
const static char http_content_type_html[] = "text/html";
const static char http_content_type_js[] = "text/javascript";
const static char http_content_type_css[] = "text/css";
const static char http_content_type_json[] = "application/json";
const static char http_cache_control_hdr[] = "Cache-Control";
// const static char http_cache_control_no_cache[] = "no-store, no-cache, must-revalidate, max-age=0";
const static char http_cache_control_cache[] = "public, max-age=31536000";
// const static char http_pragma_hdr[] = "Pragma";
// const static char http_pragma_no_cache[] = "no-cache";

static httpd_handle_t server = NULL;        // handler para el servidor HTTP

int num_clientes_TCPIP = 0;

wifi_config_t *wifi_config_SC;

// chip id
uint8_t mac_address_or_chipid[6];   // MAC o CHIP ID
char ChipId[13];                    // ChipID en char

void buildJsonNets(char* jsonBuffer, wifi_ap_record_t* _redes, uint32_t _size);
uint32_t decode_Json(const char* _Json, struct WiFi_data_t *WiFi_data, struct MQTT_user_data_t *MQTT_data);
////    TCP/IP  ////
esp_netif_t *netif_wifi_AP = NULL;
esp_netif_t *netif_wifi_STA = NULL;
bool loop_flag = false;

esp_err_t tcpip_init_AP(void);
esp_err_t tcpip_deinit_AP(void);
esp_err_t tcpip_init_STA(void);
esp_err_t tcpip_deinit_STA(void);
esp_err_t init_loop_default(void);
void deinit_loop_default(void);

//// WiFi   ////
static void wifi_event_handler(void* arg, esp_event_base_t event_base,                          // este maneja los eventos relacionados a WiFi
                                    int32_t event_id, void* event_data);

void wifi_init_softap(void);
void wifi_deinit_softap(void);
esp_err_t wifi_init_sta(struct WiFi_data_t _net);
void wifi_deinit_sta(void);
esp_err_t wifi_init_apsta(struct WiFi_data_t _net);     // inicia 
void wifi_deinit_apsta(void);
//// http   ////

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);


// función para inicializar la pila de eventos relacionada con http
typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;

#define HTTPD_401      "401 UNAUTHORIZED"           /*!< HTTP Response 401 */

/* An HTTP POST handler */
static esp_err_t connect_post_handler(httpd_req_t *req);

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);

// paso inicial para el WiFi Manager
static esp_err_t WiFi_Manager_handler(httpd_req_t *req);

static esp_err_t JS_handler(httpd_req_t *req);

static esp_err_t CSS_handler(httpd_req_t *req);

static esp_err_t WiFi_PullNets(httpd_req_t *req);

static esp_err_t cors_options_handler(httpd_req_t *req);

static httpd_handle_t start_webserver(void);

static esp_err_t stop_webserver(httpd_handle_t server);  // para detener el servidor


// funcion para detener la pila de eventos relacionada con http
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

// URIS
static const httpd_uri_t connect_WM = {
    .uri       = "/connect",
    .method    = HTTP_POST,
    .handler   = connect_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t wifimanager = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = WiFi_Manager_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static const httpd_uri_t jswifimanager = {
    .uri       = "/script.js",
    .method    = HTTP_GET,
    .handler   = JS_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static const httpd_uri_t csswifimanager = {
    .uri       = "/styles.css",
    .method    = HTTP_GET,
    .handler   = CSS_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static const httpd_uri_t PullNets = {
    .uri       = "/index.js/PullNets",
    .method    = HTTP_GET,
    .handler   = WiFi_PullNets,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static const httpd_uri_t cors_uri = {
        .uri       = "*",  // Esto permitirá CORS para todas las rutas
        .method    = HTTP_OPTIONS,
        .handler   = cors_options_handler,
        .user_ctx  = NULL
};