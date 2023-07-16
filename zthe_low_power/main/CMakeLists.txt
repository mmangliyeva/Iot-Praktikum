idf_component_register(SRCS "powerMesurment.c" "epaperInterface.cpp" "wakeup.c" "webFunctions.c" "countingAlgo.c" "mySetup.c" "main.c" "wifi.c" "timeMgmt.c" "mqtt.c"
                    INCLUDE_DIRS "." "./include"
                    REQUIRES json nvs_flash mqtt platform_client epaper)
set(COMPONENT_REQUIRES "epaper")
