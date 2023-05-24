#include "mySetup.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"


// constants for nvs.c
#define NVS_STORAGE_NAME "storage"

nvs_handle_t my_nvs_handle;

// every key can have only 4000 character == 4000 bytes
// therefore put an 0 to the define NAME_SENSOR_ARRAY 
// and open a new key in the nvs
uint8_t nvs_index = 0;

char key[10]; // this constains "sensors_key"+(string)nvs_index as string...
const char* sensors_key = "sensors"; // this is the major key for the jasonfile
const char* values_key = "values"; 
const char* name_key = "name"; 
const uint16_t sizeBuffer = 1000; // for moving data from or to the nvs

char* getKeyForNVS(void);
void addEventToStorage(char* json_str, const char* sensorName, 
                         uint8_t peopleCount, int8_t state, time_t time);

void initMY_nvs(void){
    esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}
/**
 * init the json file as string in the nvs
 * this function deletes the current storaged values
*/
void initNVS_json(void){    
    
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if(err != ESP_OK) {
        ESP_LOGI("APP","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    cJSON *root;
	root = cJSON_CreateObject();
	cJSON *sensors_array = cJSON_AddArrayToObject(root, sensors_key);
    // set sensor 'counter'
    
    // create the three sensors
    const char* sensorNames[3] = {"counter", "indoor_barrier", "outdoor_barrier"};

    for(int i = 0; i < 3; i++){
        cJSON *sensor = cJSON_CreateObject();

        cJSON_AddStringToObject(sensor, name_key, sensorNames[i]);
        cJSON_AddArrayToObject(sensor, values_key);
        cJSON_AddItemToArray(sensors_array, sensor);
    }

    // parse to string
    char *my_json_string = cJSON_Print(root);
    // here combine storage name with the index due to restriciton
    // ESP_LOGI("APP","initialized string: %s",my_json_string);
    // ESP_LOGI("APP","test getkeyForNVS: %s",getKeyForNVS());


    err = nvs_set_str(my_nvs_handle, getKeyForNVS(), my_json_string);
    if(err != ESP_OK) {
        ESP_LOGI("APP","Error (%s) saving in NVS!\n", esp_err_to_name(err));
    }
    nvs_close(my_nvs_handle);

}


void sendDataFromJSON_toDB(void){
    // check all keys and send if data is non-zero
    for(int i = 0; i < sizeBuffer; i++){
        
    }
    // empty nvs totally
}
/**
 * this function writes an event to the NVM 
 * state might be -1 which means, no state...
*/
void writeToNVM(const char* sensorName, uint8_t peopleCount, int8_t state, time_t time){
    
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI("APP","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else {
        // get string value out of NVS
        char json_str[sizeBuffer];
        size_t size=sizeof(json_str);
        err =  nvs_get_str(my_nvs_handle, getKeyForNVS(), json_str, &size);
        if(err != ESP_OK) {
            ESP_LOGI("APP","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        // check if at least 100 bytes free space, otherwise use next key..
        if(size + 100 > sizeBuffer){
            // space for this key in nvs is filled -> open a new key in nvs
            nvs_index++;
            // create new JSON in values
            initNVS_json();
        }
        addEventToStorage(json_str, sensorName, peopleCount, state, time);
    } 
    
    nvs_close(my_nvs_handle); 
}

/**
 * create JSON event and saves it to NVS
*/
void addEventToStorage(char* json_str, const char* sensorName, 
                         uint8_t peopleCount, int8_t state, time_t time){
    // get sensor
    esp_err_t err;
    cJSON *root = cJSON_Parse(json_str);

    cJSON *sensor_array = cJSON_GetObjectItem(root, sensors_key);
    // get correct sensor:
    cJSON *sensor = NULL;
    cJSON *current_element = NULL;
    cJSON_ArrayForEach(current_element, sensor_array) {
        char* curName = cJSON_GetObjectItem(current_element, name_key)->valuestring;
        if(strcmp(sensorName,curName) == 0){
            // strings are equal
            sensor = current_element;
            break;
        }
    }
    cJSON *values_array = cJSON_GetObjectItem(sensor, values_key);
    
    cJSON *newEvent = cJSON_CreateObject();
    cJSON_AddNumberToObject(newEvent, "timestamp", time);
    // state can be optional
    if(state == 0 || state == 1){
        cJSON_AddNumberToObject(newEvent, "state", state);
    }
    cJSON_AddNumberToObject(newEvent, "countPeople", peopleCount);

    cJSON_AddItemToArray(values_array,newEvent);
    
    char *my_json_string = cJSON_Print(root);

    err = nvs_set_str(my_nvs_handle, getKeyForNVS(), my_json_string);
    if(err != ESP_OK) {
        ESP_LOGI("APP","Error (%s) saving in NVS!\n", esp_err_to_name(err));
    }
    nvs_commit(my_nvs_handle);
}

char* getKeyForNVS(void){
    sprintf(key,"%s%d",sensors_key,nvs_index);
    return key;
}




















void test_access(void){
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if(err != ESP_OK) {
        ESP_LOGI("APP","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    char outpu_str[sizeBuffer];
    size_t size=sizeof(outpu_str);
    err =  nvs_get_str(my_nvs_handle, getKeyForNVS(), outpu_str, &size);
    
    if(err != ESP_OK) {
        ESP_LOGI("APP","Error (%s) nvs_get_str!\n", esp_err_to_name(err));
        
    }
    // ESP_LOGI("APP","output string from nvs string: %s and length %d",outpu_str,size);

    cJSON *root = cJSON_Parse(outpu_str);
    char* secOutput = cJSON_Print(root);
    ESP_LOGI("APP","access success: %s",secOutput);
    nvs_close(my_nvs_handle);

}