idf_component_register(SRCS "caps_ota.c"
                       INCLUDE_DIRS "./" 
                       REQUIRES app_update esp_https_ota json
                       EMBED_TXTFILES server_certs/ca_cert.pem)
