<!-- | Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# _Sample project_

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This is the simplest buildable example. The example is used by command `idf.py create-project`
that copies the project to user specified path and set it's name. For more information follow the [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)



## How to use example
We encourage the users to use the example as a template for the new projects.
A recommended way is to follow the instructions on a [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project).

## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py. -->

# Sonometro IoT
***
Este es el firmware para uno de los sensores Smart City enmarcados en el ANR denominado ***Desarrollo De Sensores IOT-Ciudades Inteligentes para Implementación en el Municipio de Oberá-Misiones en áreas Urbanas, Rurales, Reservas Naturales e Industriales con investigación y análisis de aspectos integrales de Ciudad Inteligentes para mejora de procesos de Gobernanza y modernización tecnológica*** enmarcado en el PFIP2022.
La idea es obtener un sonómetro de clase 2 o superior que sea certificable por IRAM y el INTI.
Se decidió utilizar en este proyecto el IDE ***ESP IDF*** provisto por el mismo Espressif, el cual cuenta con muchos mas recursos para manejar el hardware del  los microcontroladores ESP32 y ESP8266, programandose en C y Assembler.
El microcontrolador a utilizarse es la ESP32, la hoja de datos del mismo está en (introducir enlace) y el set de introducciones en (introducir enlace).

(hablar algo de RTOS)
## Tabla de contenidos

- [**Especificaciones Técnicas según IRAM4074 y otras IRAM**](#especificaciones-técnicas-según-iram4074-y-otras-iram)
- [**Estructura del proyecto de firmware**](#estructura-del-proyecto-de-firmware)
    - [**RTOS**](#cosas-básicas-de-rtos)
    - [**WiFi Manager**](#wifi-manager)
        - [**Backend**](#backend)
            - [*Métodos*](#métodos)
            - [*Endpoints y "sub - endpoints"*](#enpoints-y-sub-endpoints)
        - [**Frontend**](#frontend)
    - [**main**](#main)
    - [**config**](#config)
    - [**logica_control**](#logica_control)
    - [**MQTT**](#mqtt)
        - [**Tarea MQTT en RTOS**](#tarea-mqtt-en-rtos)
        - [**Instrumentación de mensajes MQTT**](#instrumentación-de-mensajes-mqtt)
            - [*Publish*](#publish)
            - [*Suscribe*](#suscribe)
        - [**Tópics y formatos de datos**](#topics-y-formatos-de-datos)
    - [**audio_task**](#audio_task)

***
## Especificaciones Técnicas según IRAM4074 y otras IRAM

Debe ser de clase 2, para lo cual debe cumplir con las siguientes especificaciones:
1. Medir mínimamente con ponderaciones A y C (ubicaciones de polos y ceros en IRAM 4074 - 1 pág 12 y 13), y lineal (sin filtro).
1. Mínimamente ponderaciones tem
1. Rango de linealidad de 50 dB (Tabla 2 IRAM 4074 - 3)
1. Tolerancia ±1 dB (Tabla 2 IRAM 4074 - 3)
1. Demás valores de la Tabla 2 IRAM 4074 - 3 (ensayos de pulsos).
1. Sensibilidad entre 20 Hz y 20 kHz
1. Omnidireccional
1. Cambio máximo en medición durante 1 h de 0,5 dB (según Tabla 1 IRAM 4074 - 3)
1. Debe tener indicador de sobrecarga (overload)
1. Debe tener una tolerancia con respecto al factor de cresta menor a los de la Tabla 3 IRAM 4074 - 3
1. Frecuencia de muestreo de 48 kHz
1. Micrófono analógico, desmontable para sustituirlo por una señal eléctrica
1. Debe detectar picos (en dB)
1. Deberá tener una resolución de 0,1 dB o mayor
1. Para una variación de ±10% en la presión estática, debe variar como mucho ±0,5 dB en la medición.
1. Debe cumplir con el ensayo “Tone Burst”, que es detectar “trenes de pulsos” de ondas senoidales, es decir, una serie de periodos de ondas senoidales (que arrancan y terminan en 0). frecuencia de las ondas senoidales debe ser de 2000 Hz, y debe repetirse a 2 Hz. Por las dudas probemos a muchas frecuencias y períodos.
1. Se debe indicar en el manual com afectan las vibraciones al dispositivo. Para determinar esto se debe realizar un ensayo a aceleración de 1 m/s² entre 20 y 1000 Hz.
1. Deberá ser inmune a ruido electromagnético (ver punto 8.4 de IRAM 4074 - 1)
1. Se debe especificar el rango de temperatura de operación del equipo, según norma debe ser al menos de -10 a 50°C.
1. La frecuencia de referencia del dispositivo debe ser de 1 kHz (ver 3.7 de IRAM 4074 - 1)
1. Deberá tener la función de calibración a 94 y 114 dB.
1. Debe cumplir con el inciso 5 de la IRAM 4074 - 1, y las tablas II y III (direccionalidad).
1. Debe poder calibrarse para campo difuso y campo libre.
1. Antes de realizar la ponderación en frecuencia se debería corregir la curva del micrófono en frecuencia (si es necesario).
1. Debe ser de tipo integrador, con tiempo de integración seteable. Esto se puede hacer en el servidor. Se recomiendan las opciones: 10 s, 1 min, 5 min, 10 min, 1 h, 8 h, 24 h.
1. Tiempo de estabilización de la medición menor a 1 min.
1. Debe cumplir con el ensayo de promediación temporal definido en la 9.3.2 de la IRAM 4074 - 3.
1. Rango de linealidad y de pulsos (ver 9.3 de la IRAM 4074 - 3)
1. Opcional: análisis con filtros de octavas, adaptativo mediante FFT.

#### En el manual de instrucciones se debe indicar:
1. El tipo de micrófono y el método de montaje necesario para lograr las tolerancias requeridas para esa clase particular de medidor de nivel sonoro integrador
1. La dirección de incidencia y la respuesta en frecuencia a campo libre en esa dirección para instrumentos calibrados en campo difuso
1. Rango del instrumento para cada ponderación en frecuencia.
1. Precisión del instrumento
1. Nivel de presión de referencia según 3.8 de la IRAM 4074 - 1. Se entiende que sería de 94 dB para la mayoría de los calibradores.
1. Tipos de ponderación en frecuencia (A, C, Z)
1. Tipos de ponderación temporal (S, F, I), con descripción.
1. Efecto de las vibraciones sobre el dispositivo.
1. Efectos de los campos magnéticos sobre el dispositivo
1. Efectos de la temperatura sobre el dispositivo (8.5 IRAM 4074 - 1)
1. Efecto de la presencia del observador en una medición en campo libre
1. Efectos de la humedad sobre el dispositivo (8.6 IRAM 4074 - 1)
1. Límites de temperatura y humedad admisibles.
1. Efectos de utilizar accesorios tales como protectores de viento.
1. Procedimiento de calibración (4.2 IRAM 4074 - 1), para campo libre y campo difuso.
1. Ubicación del observador con respecto al dispositivo para minimizar los efectos del mismo sobre la medición.
1. Frecuencia de referencia (1 kHz) (3.9 IRAM 4074 - 1)
1. Tiempo de estabilización después del cual se pueden tomar lecturas válidad (4.9 IRAM 4074 - 1)la dirección de referencia. en función de la frecuencia. Según tabla IV IRAM 
1. Información para corregir sensibilidad en un campo difuso y la sensibilidad en 4074 - 1 y Apéndice B de la misma norma. Ver 9.1 IRAM 4074 - 3. Al menos hasta 8 kHz.
1. Si mide en campo libre y difuso. Creo que se especifica con R en el modelo si es campo difuso.
1. Respuesta direccional del dispositivo, a varias frecuencias. Se debe incluir 1 kHz.
1. Ancho de banda (donde la medición es lineal)
1. Tiempos prefijados de integracion (si los usamos).
1. Duración nominal de la batería.
1. Impedancia del micrófono
1. Forma de montaje del instrumento
1. Configuración para modo normal de funcionamiento
1. Efectos de recibir una descarga eléctrica
1. etc.

Si es posible:
1. Sensibilidad del dispositivo en función de la frecuencia
1. Comportamiento del medidor cuando se lo prueba con trenes de pulsos (“tone burst”), ver 7.2 y 7.3 de IRAM 4074 - 1

***
## Estructura del proyecto de firmware
En el presente proyecto se utiliza [Free RTOS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html), y se decide separar las tareas en archivos separados por una cuestión de buenas prácticas, para no tener demasiadas lineas en pocos scrips. Se presenta a continuación una descripción de cada archivo y sus funciones principales, así como el funcionamiento de RTOS.
### Cosas básicas de RTOS
Cada tarea debe tener al menos 2 métodos:
- <sub>task_launch()</sup>: No necesariamente debe llamarse así, toma el nombre dependiendo de la tarea que lancemos. Generalmente tiene la siguiente estructura:
```c
task_launch(){
    xTaskCreatePinnedToCore(    // comando que crea la tarea
        func_task,               // llama a la funcion que será el bucle de la tarea
        "func_task",             // nombre de la tarea dentro de RTOS a findes de debug
        100000,                 // tamaño del stack, depende de la cantidad de variables que maneja mi tarea
        NULL,                   // Parameter to pass (generalmente no se usa)
        2,                      // TPrioridad de la tarea (de menor a mayor, creo que va hasta 15)
        &TaskHandle_task,        // Task handle, es como un puntero que apunta a la tarea en memoria
        APP_CORE);              // Indica que nucleo usaremos
}
```
Depende de la tarea, también se pueden crear colas (qeue) en el task launch, en ese caso vayan a fijarse al script :)
- <sub>task_kill()</sup>: Es para matar la tarea, a fin de liberar memoria si la misma ya no se usa. También se pueden matar colas, eso fijense en el código, no acá.
```c
task_kill(){
    if (TaskHandle_task != NULL)
    {
        vTaskDelete(TaskHandle_task);
        TaskHandle_task = NULL;
    }
}
```
(ver si agregar algo de colas mas adelante).
Las tareas generalmente tienen 2 archivos:
- *task.h*: en el header se llaman a las dependencias de la tarea y se definen los métodos de la misma. También se definen constantes y lo que uno quiera definir y no tenga que estar dentro de una función de la tarea.
- *task.c*: aca está el cuerpo de los métodos y la tarea principal que tiene el bucle. Esta es la que se llama desde donde se quiera lanzar la tarea.
### ***WiFi Manager***
Es el servidor web que proporciona la interfáz inicial hombre-maquina para configurar el dispositivo por primera vez, y eventualmente al mudarlo de lugar. También proporciona la interfáz de conexión WiFi mientras el dispositivo está en funcionamiento. A continuación se describirán tanto el frontend como el backend (en caso de seguir desarrollando).
#### ***Backend***
Consta de una serie de funciones y métodos encargados de:
- En caso de necesitar configuraciones básicas (WiFi y MQTT), lanzar el servidor web con sus endpoints y el frontend.
- En caso de tener configurado correctamente WiFi y MQTT, lanzar el "*handler*" que maneja la conexión WiFi.
##### ***Métodos***
- Encargados de manejo de la pila TCP/IP:

**tcpip_init_AP**: Inicia la pila TCP/IP para que la ESP trabaje en modo AP (Access Point, generando una red WiFi).
```c
esp_err_t tcpip_init_AP(void);
```
**tcpip_deinit_AP**: Destructor de la pila TCP/IP para que la ESP trabaje en modo AP (Access Point, generando una red WiFi). **Usar con cuidado, no está 100% probado.**
```c
esp_err_t tcpip_deinit_AP(void);
```
**tcpip_init_STA**: Inicia la pila TCP/IP para que la ESP trabaje en modo STA (Estación WiFi, conectado a un access point, el modem de tu casa, etc.).
```c
esp_err_t tcpip_init_STA(void);
```
**tcpip_deinit_STA**: Destructor de la pila TCP/IP para que la ESP trabaje en modo STA. **Usar con cuidado, no está 100% probado.**
```c
esp_err_t tcpip_deinit_AP(void);
```
**init_loop_default**: Genera el loop que maneja TCP/IP.
```c
esp_err_t init_loop_default(void);
```
**deinit_loop_default**: Destructor del loop que maneja TCP/IP. **Usar con cuidado, no está 100% probado.**
```c
void deinit_loop_default(void);
```

**Se recomienda arrancar la ESP en un modo, y resetearla para cambiar de modo, debido a que al destruir una pila TCP/IP y luego crear otra pasan cosas extrañas que no se entienden del todo**🙊

- Encargados de manejar los modos WiFi:

**wifi_init_softap**: Pone a la ESP32 en modo AP (Access Point), lanzando una red WiFi desde la ESP.
```c
esp_err_t wifi_init_softap(void);
```
**wifi_deinit_softap**: Saca a la ESP de modo AP y desactiva el WiFi.
```c
esp_err_t wifi_deinit_softap(void);
```
**wifi_init_sta**: Inicia la ESP en modo STA (Estación), recibe como argumentos los datos para conectarse a una red.
```c
esp_err_t wifi_init_sta(struct WiFi_data_t _net);
```
**wifi_deinit_wifi_deinit_staapsta**: Saca a la ESP de modo STA y desactiva el WiFi.
```c
void wifi_deinit_sta(void);
```
**wifi_init_apsta**: Inicia la ESP en modo APSTA (Acces Point y Estación al mismo tiempo), recibe como argumentos los datos para conectarse a una red.
```c
esp_err_t wifi_init_apsta(struct WiFi_data_t _net);
```
**wifi_deinit_apsta**: Saca a la ESP de modo APSTA y desactiva el WiFi.
```c
void wifi_deinit_apsta(void);
```
- Funciones relacionadas con HTTP

**start_webserver**: Inicia el servidor web, lanzando todos los endpoints que se detallan en este documento.
```c
static httpd_handle_t start_webserver(void);
```

**stop_webserver**: Mata el servior web.
```c
static esp_err_t stop_webserver(httpd_handle_t server);
```

**connect_handler**: inicia el servidor web de manera segura, y maneja los eventos referidos a http.
```c
static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
```

**disconnect_handler**: mata el servidor web de manera segura.
```c
static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
```

**connect_post_handler**: maneja los post hechos al endpoint "/connect". Dentro tiene una lógica de corrimiento de bits para saber que evento se solicitó mediante post.
```c
static esp_err_t connect_post_handler(httpd_req_t *req);
```

**http_404_error_handler**: Entiendo que maneja las solicitudes erroneas, sin embargo se heredó del ejemplo de servidor http practicamente sin modificaciones.
```c
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);
```

**WiFi_Manager_handler**: maneja el endpoint "/", donde se entra al frontend del WiFi manager.
```c
static esp_err_t WiFi_Manager_handler(httpd_req_t *req);
```

**JS_handler**: maneja el endpoint "/script.js", donde se accede al código fuente de JavaScript para el frontend.
```c
static esp_err_t JS_handler(httpd_req_t *req);
```

**CSS_handler**: maneja el endpoint "/styles.css", donde se accede al código fuente de JavaScript para el frontend.
```c
static esp_err_t CSS_handler(httpd_req_t *req);
```

**WiFi_PullNets**: maneja el endpoint "/index.js/PullNets", donde se solicita a la ESP que devuelva las redes disponibles en formato JSON al frontend.
```c
static esp_err_t WiFi_PullNets(httpd_req_t *req);
```

**cors_options_handler**: maneja el endpoint "*", que maneja el CORS.
```c
static esp_err_t cors_options_handler(httpd_req_t *req);
```

- Funciones auxiliares

**buildJsonNets**: Genera el JSON con las redes disponibles.
```c
void buildJsonNets(char* _Json, wifi_ap_record_t* _redes, uint32_t _size);
```

**decode_Json**: Decodifica los mensajes enviados por post a "/connect", según un formato predefinido. 
```c
uint32_t decode_Json(const char* _Json, struct WiFi_data_t *WiFi_data, struct MQTT_user_data_t *MQTT_data);
```

##### ***Enpoints y "sub-endpoints"***
- **"/" (192.168.4.1)**: es la url donde se ingresa al frontend desde el navegador.
- **"/script.js" (192.168.4.1/script.js)**: es la dirección de donde se busca el código en javascript para el frontend.
- **"/styles.css" (192.168.4.1/styles.css)**: es la dirección de donde se buscan las instrucciones en css para el front.
- **"/index.js/PullNets" (192.168.4.1/index.js/PullNets)**: es la dirección donde se solicitan las redes disponibles a la ESP. ***se puede cambiar por "/script.js/PullNets" en el futuro para que tenga mas lógica***
- **"/connect" (192.168.4.1/connect)**: es el endpoint al cual se envian comandos mediante POST, a los que yo llamo *sub-endpoints*, a continuación se describen los comandos que se pueden enviar por POST:
    - **Setear SSID y password para el WiFi**: Para este caso se envía un JSON con el SSID y el pass de la red, de la siguiente forma, siendo el authmode un entero:
    ```JSON
    {
        "WiFi" : {
            "SSID" : "your SSID",
            "Pass" : "your_pass",
            "authmode" : authmode
        }
    }
    ```
    - **Setear usuario y password para el MQTT**: Para este caso se envía un JSON con el usuario y contraseña de MQTT previamente creados en el broker o DB encargada de la autentificación en el broker.
    ```JSON
    {
        "MQTT" : {
            "User" : "user_MQTT",
            "Pass" : "password"
        }
    }
    ```
    Automáticamente cuando le llegan estas instrucciones, la ESP intenta conectarse primeramente a WiFi y luego a MQTT. Se pueden enviar juntas de la siguiente manera:
    ```JSON
    {
        "WiFi" : {
            "SSID" : "FAN-IOT",
            "Pass" : "f4n10t2020",
            "authmode" : 3
        },
        "MQTT" : {
            "User" : "test_SC",
            "Pass" : "faniot123"
        }
    }
    ```
    - **Corroborar conexión WiFi y MQTT**: Para este caso se envía un JSON a la ESP32 de la siguiente manera:
    ```JSON
    {
        "WiFi_connection_request" : 1
    }
    ```
    Si el valor enviado es cero no responderá nada. La respuesta consta de la red WiFi a la que está conectada la ESP, respondiendo una cadena vacía si no está conectada a nada; y el usuario MQTT con el que está logueado, respondiendo none si no está logueado, ejemplo 1:
    ```JSON
    {
        "WiFi SSID": "SSID WiFi",
        "MQTT user": "user_MQTT"
    }
    ```
    Ejemplo 2:
    ```JSON
    {
        "WiFi SSID": "",
        "MQTT user": "none"
    }
    ```
    Si la respuesta consta de una conexión WiFI y una conexión MQTT ya estamos listos para apagar el WiFi Manager y pasar la ESP a modo STA.
    - **Apagar WiFi manager y pasar a modo STA**: Para este caso se envía un JSON a la ESP32 de la siguiente manera:
    ```JSON
    {
        "off_WM_mode" : 1
    }
    ```
    Si se envía un 0 no dará resultado. Recibe como respuesta texto plano que informa de éxito o de un problema.
    - **Solicitar Chip ID**: Fué hecho a modo de prueba, en este caso se envía un JSON a la ESP32 de la siguiente manera:
    ```JSON
    {
        "get_chipid" : 1
    }
    ```
    La respuesta es el chip id en texto plano.

#### ***Frontend***
**EXPLICAR EL FRONTEND.**

### ***main***
aca simplemente se debería llamar a *config.c* y *logica_control.c*, quedando sin loop infinito, ya que de esto se encargarán las tareas.

### ***config***
Esta no es una tarea de RTOS propiamente dicha, en ella están los métodos:
- *loadconfig()*: trae la información guardada en la ROM (dcredenciales, calibraciones, datos de versionado, etc.) y actualiza los datos en RAM.
- *saveconfig()*: guarda los datos importantes en la memoria ROM.
- *get_chipid()*: guarda el chipid en el elemento CHIPID.

También aquí se encuentra la definición de todas las veriables del sistema, así como las estructuras útiles y las definiciones de elementos numerados.


### ***logica_control***
Lanza las demás tareas, y se encarga de recibir las peticiones del usuario y hacer en consecuencia lo que se necesite. Recibe por cola las peticiones que el usuario envía a travéz de MQTT las ordenes.
Se lanza con la función control_launch() y se mata con la función control_kill();

### ***MQTT***
<!-- Primero va la parte de la tarea en si, luego la explicación de las tramas -->
Esta sección se separa en 2, en primer lugar la tarea de MQTT, y en segundo lugar la instrumentación de los mensajes (estructura del mensaje, topics, etc.).
#### ***Tarea MQTT en RTOS***
Se lanza mediante la función mqtt_launch(), y aún no está hecha una función para matar esta tarea debido a que es indispensable para el funcionamiento del dispositivo. Consta además de la cola llamada *msg_queue_to_mqtt_send* la cual se puede llamar desde cualquier tarea (que incluya dicha cola), para enviar mensajes al broker definido en el header de config.
#### ***Instrumentación de mensajes MQTT***
La cola *msg_queue_to_mqtt_send* recibe mensajes del tipo *data_mqtt_send_t*, que tiene la siguiente estructura
```c
struct data_mqtt_send_t{
    char topic[32];
    char payload[256];
    QOS_MQTT qos;
    bool retain;
};
```
donde:
- *topic*: es el topico del mensaje (ejemplo: Temperatura), el cual se incluirá en la parte final del tópico por defecto para este dispositivo (NombreDispositivo/ChipId/topic -> ejemplo: Sonometro/C44F33605219/Temperatura).
- *payload*: Es la carga útil del mensaje, en nuestro caso se explica el formato de la misma en la siguiente subsección.
- *qos*: es la calidad del servicio de MQTT, en este caso es del tipo elemento numerado *QOS_MQTT* definido en el header de config. Puede valer 0, 1 o 2, donde:
    - 0 (AT_MOST_ONCE)= le llega al cliente como máximo 1 mensaje, no se asegura de que le haya llegado. Es el modo con menor costo computacional.
    - 1 (ONE_OR_MORE)= le llega como mínimo 1 mensaje, asegurandose de que le lleguen si o si los mensajes al broker. Pueden llegarle mensajes repetidos.
    - 2 (ONE_TIME) = le llega solo 1 mensaje al broker, es el que tiene mayor costo computacional.
- *retain*: es *true* o *false*, es para que el broker retenga o no los mensajes. 
Se puede mandar mensajes a MQTT desde cualquier tarea siempre que se incluya a *msg_queue_to_mqtt_send* en la tarea.
##### ***Publish***
Para publicar mensajes MQTT desde cualquier tarea llamamos a la siguiente función:
```c
xQueueSend(msg_queue_to_mqtt_send, &teste, portMAX_DELAY);
```
donde *msg_queue_to_mqtt_send* es de tipo *data_mqtt_send_t* y el payload es un JSON. En dicho JSON se pueden incluir muchos datos:
- hora: se incluye según el siguiente formato:
 ```JSON
{
        "datetime" : {
            "date" : "DD-MM-AAAA",
            "time" : "hh:mm:ss,msms"
        } 
}
```
- variables (sensores, variables internas, etc):  
se incluye según el siguiente formato:
 ```JSON
{
        "Variable_medida" : {
            "magnitud" : 10.10,     // flotante
            "unidades" : "unidades"
        } 
}
```

Un ejemplo de dato enviado sería el siguiente:

 ```JSON
{
  "datetime" : {
      "date" : "20-09-2024",
      "time" : "12:24:15,2542"
  },
  "Temperatura" : {
      "magnitud" : 30.5,
      "unidades" : "°C"
  } ,
  "Humedad Relativa" : {
      "magnitud" : 80.4,
      "unidades" : "%"
  }
}
```
Tener en cuenta que usar espacios innesesarios en el JSON ocupan memoria.
##### ***Suscribe***
La ESP32 al iniciar en modo STA se suscribe al topic *(NombreDispositivo)/(ChipId)/UserControl* (ejemplo: Sonometro/C44F33605219/UserControl), a través del cual el usuario (a través de la página web correspondiente) envía instrucciones de configuración al dispositivo (ej.: actualizar firmware). Estos mensajes tienen la estructura de un JSON, según la siguiente:
 ```JSON
{
  "cmd" : 0,                    // entero
  "value" : 0,                  // entero
  "value_f" : 0.0,              // flotante
  "value_str": "texto a enviar" // string
}
```
donde:
- *cmd*: es el comando a ejecutar, es un numero entero que se define para cada orden a ejecutar (**UNA VEZ DEFINIDOS INCLUIR LA TABLA**). Siempre debe estar incluido en el JSON.
- *value*: es un numero entero a utilizarse o no, según la instrucción definida en *cmd*.
- *value_f*: es un flotante entero a utilizarse o no, según la instrucción definida en *cmd*.
- *value_str*: es una cadena de string de como máximo 256 caracteres (incluido el final del string), a utilizarse o no, según la instrucción definida en *cmd*.

Este JSON en la ESP se guarda en un objeto del tipo *data_control_t* según la siguiente estructura:
 ```c
struct data_control_t{
    cmd_control_t cmd;
    int value;
    float value_f;
    char  value_str[256];
};
```
Luego son enviados por cola a la lógica de control, donde se ejecuta la orden del usuario.
**MENSAJES QUE NO SIGAN DICHA ESTRUCTURA SERÁN IGNORADOS.**

#### ***Topics y formatos de datos***
Para estructurar los topics se piensa en el siguiente formato:

**(NOMBRE DE DISPOSITIVO)/(CHIP ID)/(SUPTOPICS)**

Donde el nombre del dispositivo puede ser por ejemplo sonómetro, el ChipId es un código único de cada ESP32 compuesto por 12 caracteres, y el Subtopic puede ser por ejemplo el nombre de una medición o una veriable interna de interés que se envía por MQTT. De este modo, un topic al que el usuario se suscribiría podría ser:

**Sonometro/C44F33605219/Leq**

Esto nos permite tener tópicos únicos por dispositivo, dandonos herramientas a la hora de filtrar la información para el cliente.

El formato de los datos se está discutiendo entre:
 ```JSON
{
  "datetime" : {
      "date" : "20-09-2024",
      "time" : "12:24:15,2542"
  },
  "Temperatura" : {
      "magnitud" : 30.5,
      "unidades" : "°C"
  } ,
  "Humedad Relativa" : {
      "magnitud" : 80.4,
      "unidades" : "%"
  }
}
```

y:

 ```JSON
{
  "datetime" : {
      "date" : "20-09-2024",
      "time" : "12:24:15,2542"
  },
  "data" : {
    "Temperatura" : {
        "magnitud" : 30.5,
        "unidades" : "°C"
    } ,
    "Humedad Relativa" : {
        "magnitud" : 80.4,
        "unidades" : "%"
    }
  }
}
```
### ***audio_task***
Se encarga de calcular el nivel sonoro equivalente. También tiene implementado el test unitario del filtro de audio según la norma IRAM4074-1.

### Archivos en Assembler
#### Cast_and_scale.S
Se encarga de recibir la muestra de audio en veces (numero entero sin signo generalmente) y pasarlo a unidades de presión. Para utilizarlo se lo encluye en el script.c, globalmente de la siguiente forma:
```c
extern void casting_y_escala(int muestra_cuentas, float* muestra_p, float* k_veces_to_p);
```
donde:
- *muestra_cuentas*: es la muestra de audio en veces
- *muestra_p*: puntero que apunta a la dirección de memoria donde se guardará la muestra en unidades de presión.
- *k_veces_to_p*: puntero que apunta a la dirección de memoria donde está la constante de conversión de veces a unidades de presion.

#### filtro.S
Contiene un algoritmo correspondiente a un filtro IIR de grado 2 según la forma directa tipo I. Para utilizarlo se lo encluye en el script.c, globalmente de la siguiente forma:
```c
extern void filtro_II_d_I(float* muestra_p, float* _x, float* _y, float* _SOS);
```
donde:
- *muestra_p*: puntero que apunta a la dirección de memoria donde se guardará la muestra en unidades de presión.
- *_x*: apunta al primer elemento de un stack de 3 elementos correspondientes a x[n], x[n-1] y x[n-2], dicho stack es una pila FIFO.
- *_y*: apunta al primer elemento de un stack de 3 elementos correspondientes a y[n], y[n-1] y y[n-2], dicho stack es una pila FIFO.
- *_SOS*: apunta al primer elemento de array que contiene los parámetros del filtro a implementar. La disposición de los parámetros es la siguiente:
    - *(_sos + 0) = b0
    - *(_sos + 1) = b1
    - *(_sos + 2) = b2
    - *(_sos + 3) = a1
    - *(_sos + 4) = a2
    
Si se pretende aplicar un filtro de grado mayor a 2 se debe implementar como una cascada de filtros de grado 2 según forma directa tipo I.

#### product_and_acu.S
Se encarga de realizar unqa escala a la salida de la etapa de filtrado y de realizar el acumulado del cuadrado de la presión. Para utilizarlo se lo encluye en el script.c, globalmente de la siguiente forma:
```c
extern void producto_y_acumulacion(float *_y, float *_acu, float *_k);
```
donde:
- *_y*: apunta a la salida de la etapa de filtrado.
- *_acu*: apunta a la direccion de memoria donde se guardará el acumulado de los cuadrados de la salida del filtro para calcular luego el Leq.
- *_k*: constante a la que se multiplica *_y antes de realizar el cuadrado. Generalmente se especifica en el diseño del filtro.