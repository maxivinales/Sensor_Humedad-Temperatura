#ifndef MQTT_H_
#define MQTT_H_
#include "config.h"
#include "http_client.h"

bool mqtt_connected = false;  // Variable global para la bandera de conexión MQTT

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
esp_err_t mqtt_launch();
//esp_err_t aux_kill();
#endif