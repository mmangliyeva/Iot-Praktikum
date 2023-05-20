#include "mySetup.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

nvs_handle_t my_nvs_handle;

// every key can have only 4000 character == 4000 bytes
// that is why every sensor has 3 keys == 12000 bytes storage in NVS
char** key_counter = {"counter1","counter2","counter3"};
char** key_indoor_bar = {"key_indoor_bar1","key_indoor_bar2","key_indoor_bar3"};
char** key_outdoor_bar = {"key_outdoor_bar1","key_outdoor_bar2","key_outdoor_bar3"};


void sendDataFromJSON_toDB(void){
    // check all keys and send if data is non-zero

    // empty nvs totally
}

void writeCounterToNVM(uint8_t peopleCount){
    const char* nameSensor = "counter";
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else {
        // get string value out of NVS
        char* json_value;
        size_t size_json;
        err = nvs_get_str(my_nvs_handle,nameSensor,json_value,&size_json)
        // check if at least 100 bytes free space, otherwise use next key..

        printf((err != ESP_OK) ? "Failed load json!\n" : "Done\n");
        // parse string to JSON-object
        cJSON *root = cJSON_Parse(json_value);
        // append count, time 
        
        // parse JSON-object back to string
        err = nvs_set_str(my_nvs_handle,nameSensor);

        // save sting in NVS check if too big
    }
}