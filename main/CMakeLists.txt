idf_component_register(SRCS "sht40.c" "mqtt.c" "http_client.c" "ota.c" "config.c" "WiFi_manager.c" "Leq_task.c" "audio_task.c" "test.S" "product_and_acu.S" "cast_and_scale.S" "filtro.S" "logica_control.c" "main.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES  "web/index.html" "web/script.js" "web/styles.css" "server_certs/ca_cert.pem")
