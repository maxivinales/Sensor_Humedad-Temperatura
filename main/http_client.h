#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include "config.h"
#include <cJSON.h>

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt);
static void rest_get(char* _url);
void get_data_time(char* _URL);
#endif