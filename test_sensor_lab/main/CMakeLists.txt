idf_component_register(SRCS "webFunctions.c" "nvs.c" "countingAlgo.c" "mySetup.c" "main.c" "wifi.c" "timeMgmt.c" "mqtt.c"
                    INCLUDE_DIRS "." "./include"
                    REQUIRES json mqtt nvs_flash ssd1306 caps_ota rest_client)
