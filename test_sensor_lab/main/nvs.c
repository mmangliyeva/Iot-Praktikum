#include "mySetup.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "mqtt.h"

// constants for nvs.c
#define NVS_STORAGE_NAME "storage"

nvs_handle_t my_nvs_handle;

// every key can have only 4000 character == 4000 bytes
// therefore put an 0 to the define NAME_SENSOR_ARRAY 
// and open a new key in the nvs
uint8_t nvs_index;

char key[12]; // this constains "sensors_key"+(string)nvs_index as string...
const char* sensors_key = "sensors"; // this is the major key for the jasonfile
const char* values_key = "values"; 
const char* name_key = "name"; 
const char* count_backup_key = "count_backup"; 
const uint16_t sizeBuffer = 2000;    // for moving data from or to the nvs

char* getKeyForNVS(void);
void addEventToStorage(char* json_str, const char* sensorName, 
                         uint8_t peopleCount, int8_t state, time_t time);

void initMY_nvs(void){
    ESP_LOGI("PROGRESS", "Initializing NVS");
    nvs_index = 1;
    esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
    initNVS_json(NO_OPEN_NVS);
}
/**
 * resets the count if we having between 5-6am
 * I assume no one is at that time in the room...
*/
void resetCount(uint8_t bool_openHandle){
    time_t now = time(NULL);
	struct tm *now_tm = localtime(&now);
    if(now_tm->tm_hour == RESET_COUNT_HOUR && now_tm->tm_min < RESET_COUNT_MIN){
        esp_err_t err = ESP_OK;
        if( bool_openHandle != OPEN_NVS){
            err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
        }
        if(err != ESP_OK) {
            ESP_LOGI("NVS","Error resetCount (%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        // here reset of count
        err = nvs_set_u8(my_nvs_handle, count_backup_key, 0);
        if(err != ESP_OK) {
            ESP_LOGI("NVS","Error resetCount(%s) could not reset count.\n", esp_err_to_name(err));
        }
        else{
            ESP_LOGI("NVS","RESET COUNT -> restart!\n");
        }
        nvs_commit(my_nvs_handle);

        if( bool_openHandle != OPEN_NVS){
            nvs_close(my_nvs_handle);
        }
        esp_restart();
    }
}
/**
 * save as backup the poeple count in nvs, in the case that the esp restarts
*/
uint8_t restoreCount(uint8_t bool_openHandle){    
    esp_err_t err = ESP_OK;
    if( bool_openHandle != OPEN_NVS){
        err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    }
    if(err != ESP_OK) {
        ESP_LOGI("NVS","Error restoreCount (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    uint8_t count = 0;
    err = nvs_get_u8(my_nvs_handle,count_backup_key,&count);
    if(err != ESP_OK) {
        ESP_LOGI("NVS","Error restoreCount (%s) could not restore uint8!\n", esp_err_to_name(err));
    }

    if( bool_openHandle != OPEN_NVS){
        nvs_close(my_nvs_handle);
    }
    return count;
}

/**
 * init the json file as string in the nvs
 * this function deletes the current storaged values
 * 
 * bool_openHandle === 1 means there is an open handle
*/
void initNVS_json(uint8_t bool_openHandle){    
    esp_err_t err = ESP_OK;
    if( bool_openHandle != OPEN_NVS){
        err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);

    }
    if(err != ESP_OK) {
        ESP_LOGI("NVS","Error initNVS_json (%s) opening NVS handle!\n", esp_err_to_name(err));
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
        ESP_LOGI("NVS","Error initNVS_json (%s) saving in NVS!\n", esp_err_to_name(err));
    }
    nvs_commit(my_nvs_handle);
    if( bool_openHandle != OPEN_NVS){
        nvs_close(my_nvs_handle);
    }
}


void sendDataFromJSON_toDB(void *args){

    // check all keys and send if data is non-zero
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    // iterate through all keys
    for(uint8_t i = 1; i <= nvs_index; i++){
        if(err != ESP_OK) {
            ESP_LOGI("NVS","Error sendDataFromJSON_toDB(%s) opening NVS handle!\n", esp_err_to_name(err));
        }
        char json_str[sizeBuffer];
        size_t size=sizeof(json_str);
        // generate current key
        sprintf(key,"%s%d",sensors_key,i);
        err =  nvs_get_str(my_nvs_handle, key, json_str, &size);
        if(err == ESP_OK){
            sendToMQTT(json_str, QOS_SAFE);
            err = nvs_erase_key(my_nvs_handle,key);
            if(err != ESP_OK) {
                ESP_LOGI("NVS","Error sendDataFromJSON_toDB(%s) opening NVS handle!\n", esp_err_to_name(err));
            }
            err = nvs_commit(my_nvs_handle);
            if(err != ESP_OK) {
                ESP_LOGI("NVS","Error sendDataFromJSON_toDB(%s) opening NVS handle!\n", esp_err_to_name(err));
            }
        }
        else{
            ESP_LOGI("NVS","Error sendDataFromJSON_toDB (%s) nvs_get_str!\n", esp_err_to_name(err));            
        }
        // ESP_LOGI("APP","output string from nvs string: %s and length %d",outpu_str,size);
    }

    nvs_index = 1;
    initNVS_json(OPEN_NVS);

    nvs_close(my_nvs_handle);
    // empty nvs totally
}


/**
 * this function writes an event to the NVM 
 * state might be -1 which means, no state...
*/
void writeToNVM(const char* sensorName, uint8_t peopleCount, int8_t state, time_t time){
    
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI("NVS","Error writeToNVM(%s) opening NVS handle!\n", esp_err_to_name(err));
    } 
    else {
        // get string value out of NVS
        char json_str[sizeBuffer];
        size_t size=sizeof(json_str);
        err =  nvs_get_str(my_nvs_handle, getKeyForNVS(), json_str, &size);
        if(err != ESP_OK) {
            ESP_LOGI("NVS","Error writeToNVM(%s) get str!\n", esp_err_to_name(err));
        }
        // check if at least 100 bytes free space, otherwise use next key..
        if(size + 200 > sizeBuffer){
            // space for this key in nvs is filled -> open a new key in nvs
            nvs_index++;
            // create new JSON in values
            initNVS_json(OPEN_NVS);
            ESP_LOGI("NVS","increased index: %s", getKeyForNVS());
        }
        addEventToStorage(json_str, sensorName, peopleCount, state, time);
    }

    nvs_close(my_nvs_handle); 
    
    // test_access();
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
    
    // we dont have 64 bit so do some shitty trick...
    char newEvent_str[100];
    sprintf(newEvent_str, "{\"timestamp\":%lld000}",(long long)time);
    cJSON *newEvent = cJSON_Parse(newEvent_str);

    // state can be optional
    if(state == 0 || state == 1){
        cJSON_AddNumberToObject(newEvent, "state", state);
    }
    cJSON_AddNumberToObject(newEvent, "countPeople", peopleCount);

    cJSON_AddItemToArray(values_array,newEvent);
    
    char *my_json_string = cJSON_Print(root);

    err = nvs_set_str(my_nvs_handle, getKeyForNVS(), my_json_string);
    if(err != ESP_OK) {
        ESP_LOGI("NVS","Error addEventToStorage(%s) saving in NVS!\n", esp_err_to_name(err));
    }

    // backup count in nvs:
    err = nvs_set_u8(my_nvs_handle, count_backup_key, peopleCount);
    if(err != ESP_OK) {
        ESP_LOGI("NVS","Error addEventToStorage(%s) could not backup count.\n", esp_err_to_name(err));
    }

    nvs_commit(my_nvs_handle);
}
/**
 * returns the major key for finding the json-string in nvs
*/
char* getKeyForNVS(void){
    sprintf(key,"%s%d",sensors_key,nvs_index);
    return key;
}




















void test_access(void){
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if(err != ESP_OK) {
        ESP_LOGI("NVS","Error test_access(%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    // char outpu_str[sizeBuffer];
    // size_t size=sizeof(outpu_str);
    for(uint8_t i = 1; i <= nvs_index; i++){
        sprintf(key,"%s%d",sensors_key,i);
        // err =  nvs_get_str(my_nvs_handle, key, outpu_str, &size);
        nvs_stats_t nvs_stats;
        nvs_get_stats(key,&nvs_stats);
        if(err != ESP_OK) {
            ESP_LOGI("NVS","Error test_access(%s) nvs_get_str!\n", esp_err_to_name(err));
            
        }
        // ESP_LOGI("APP","output string from nvs string: %s and length %d",outpu_str,size);

        // cJSON *root = cJSON_Parse(outpu_str);
        // char* secOutput = cJSON_Print(root);    
        // ESP_LOGI("NVS","current nvs-storage: %s\n%s",key,outpu_str);
        ets_printf("Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n", (int)nvs_stats.used_entries,(int) nvs_stats.free_entries, (int)nvs_stats.total_entries);
    }
    nvs_close(my_nvs_handle);

}