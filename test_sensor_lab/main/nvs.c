#include "mySetup.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "mqtt.h"

// constants for nvs.c
#define NVS_STORAGE_NAME "storage"
#define NUM_KEY_WORDS 7

nvs_handle_t my_nvs_handle;

// every key can have only 4000 character == 4000 bytes. NO BULLSHIT sometimes just 1500
// therefore put an 0 to the define NAME_SENSOR_ARRAY
// and open a new key in the nvs
uint8_t nvs_index;

char key[15];                        // this constains "sensors_key"+(string)nvs_index as string...
const char *sensors_key = "sensors"; // this is the major key for the jasonfile

// one keyWords saves sizeBuffer many characters
const char *keyWords[NUM_KEY_WORDS] = {"s0", "s1", "s2", "s3", "s4", "s5", "s6"};

const char *values_key = "values";
const char *name_key = "name";
const uint16_t sizeBuffer = 1500; // for moving data from or to the nvs

void addEventToStorage(char *json_str, const char *sensorName,
                       uint8_t peopleCount, int8_t state, time_t time);

void initMY_nvs(void)
{
    ESP_LOGI("PROGRESS", "Initializing NVS");
    nvs_index = 0;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    initNVS_json(NO_OPEN_NVS);
}

/**
 * init the json file as string in the nvs
 * this function deletes the current storaged values
 *
 * bool_openHandle === 1 means there is an open handle
 */
void initNVS_json(uint8_t bool_openHandle)
{
    esp_err_t err = ESP_OK;
    if (bool_openHandle != OPEN_NVS)
    {
        err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    }
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error initNVS_json opening NVS handle %s!\n", esp_err_to_name(err));
    }
    cJSON *root;
    cJSON *sensor = NULL;
    root = cJSON_CreateObject();
    cJSON *sensors_array = cJSON_AddArrayToObject(root, sensors_key);
    // set sensor 'counter'

    // create the three sensors
    uint8_t lengthArray;

#ifdef SEND_EVERY_EVENT
    lengthArray = 3;
    const char *sensorNames[3] = {"counter", "indoor_barrier", "outdoor_barrier"};
#else
    lengthArray = 1;
    const char *sensorNames[1] = {"counter"};
#endif
    for (int i = 0; i < lengthArray; i++)
    {
        sensor = cJSON_CreateObject();

        cJSON_AddStringToObject(sensor, name_key, sensorNames[i]);

        cJSON_AddArrayToObject(sensor, values_key);

        cJSON_AddItemToArray(sensors_array, sensor);
    }

    // parse to string
    char *my_json_string = cJSON_Print(root);
    // here combine storage name with the index due to restriciton
    // ESP_LOGI("APP","initialized string: %s",my_json_string);
    // ESP_LOGI("APP","test getkeyForNVS: %s",keyWords[nvs_index]);

    err = nvs_set_str(my_nvs_handle, keyWords[nvs_index], my_json_string);

    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error initNVS_json saving NVS handle %s!\n", esp_err_to_name(err));
    }
    // cJSON_Delete(root);
    nvs_commit(my_nvs_handle);
    cJSON_Delete(root);
    if (bool_openHandle != OPEN_NVS)
    {
        nvs_close(my_nvs_handle);
    }
}

void sendDataFromJSON_toDB(uint8_t bool_openHandle)
{

    // check all keys and send if data is non-zero
    esp_err_t err = ESP_OK;
    if (bool_openHandle != OPEN_NVS)
    {
        err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    }
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error initNVS_json opening NVS handle %s!\n", esp_err_to_name(err));
    }
    // iterate through all keys
    for (uint8_t i = 0; i <= nvs_index; i++)
    {
        if (err != ESP_OK)
        {
            ESP_LOGE("NVS", "Error sendDataFromJSON_toDB opening NVS handle %s!\n", esp_err_to_name(err));
        }

        size_t size = 0;
        err = nvs_get_str(my_nvs_handle, keyWords[i], NULL, &size);
        char *json_str = malloc(size);

        // generate current key
        err = nvs_get_str(my_nvs_handle, keyWords[i], json_str, &size);
        if (err == ESP_OK)
        {
            sendToMQTT(json_str, QOS_SAFE);
            // ESP_LOGI("NVS", "sent data to MQTT\n");
            err = nvs_erase_key(my_nvs_handle, keyWords[i]);
            if (err != ESP_OK)
            {
                ESP_LOGE("NVS", "Error sendDataFromJSON_toDB earse NVS key %s!\n", esp_err_to_name(err));
            }
            err = nvs_commit(my_nvs_handle);
            if (err != ESP_OK)
            {
                ESP_LOGE("NVS", "Error sendDataFromJSON_toDB commiting NVS handle %s!\n", esp_err_to_name(err));
            }
        }
        else
        {
            ESP_LOGE("NVS", "Error sendDataFromJSON_toDB nvs_get_str %s!\n", esp_err_to_name(err));
        }
        // ESP_LOGI("APP","output string from nvs string: %s and length %d",outpu_str,size);
        free(json_str);
    }

    nvs_index = 0;
    initNVS_json(OPEN_NVS);

    if (bool_openHandle != OPEN_NVS)
    {
        nvs_close(my_nvs_handle);
    }
    // empty nvs totally
}

/**
 * this function writes an event to the NVM
 * state might be -1 which means, no state...
 */
void writeToNVM(const char *sensorName, uint8_t peopleCount, int8_t state, time_t time)
{

    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error writeToNVM nvs_get_str %s!\n", esp_err_to_name(err));
    }
    else
    {
        // get string value out of NVS
        // char json_str[sizeBuffer];
        size_t size = 0;
        err = nvs_get_str(my_nvs_handle, keyWords[nvs_index], NULL, &size);
        if (err != ESP_OK)
        {
            ESP_LOGE("NVS", "Error writeToNVM could not get size %s!\n", esp_err_to_name(err));
        }
        // ESP_LOGI("NVS", "current size %d of storage key: %s", size, keyWords[nvs_index]);

        // check if at least 100 bytes free space, otherwise use next key..
        if (size + NEEDED_SPACE_NVS > sizeBuffer)
        {
            // space for this key in nvs is filled -> open a new key in nvs
            nvs_index++;
            if (nvs_index >= NUM_KEY_WORDS)
            {
                nvs_index--;
                sendDataFromJSON_toDB(OPEN_NVS);
                ESP_LOGI("NVS", "reset index: %s", keyWords[nvs_index]);
            }
            else
            {
                // create new JSON in values
                initNVS_json(OPEN_NVS);
                ESP_LOGI("NVS", "increased index: %s", keyWords[nvs_index]);
            }

            err = nvs_get_str(my_nvs_handle, keyWords[nvs_index], NULL, &size);
            if (err != ESP_OK)
            {
                ESP_LOGE("NVS", "Error writeToNVM could get str second time size %s!\n", esp_err_to_name(err));
            }
        }
        char *json_str = malloc(size);
        err = nvs_get_str(my_nvs_handle, keyWords[nvs_index], json_str, &size);
        if (err != ESP_OK)
        {
            ESP_LOGE("NVS", "Error writeToNVM could get str %s!\n", esp_err_to_name(err));
        }
        addEventToStorage(json_str, sensorName, peopleCount, state, time);

        free(json_str);
    }

    nvs_close(my_nvs_handle);

    // test_access();
}

/**
 * create JSON event and saves it to NVS
 */
void addEventToStorage(char *json_str, const char *sensorName,
                       uint8_t peopleCount, int8_t state, time_t time)
{
    // get sensor
    esp_err_t err;
    cJSON *root = cJSON_Parse(json_str);

    cJSON *sensor_array = cJSON_GetObjectItem(root, sensors_key);
    // get correct sensor:
    cJSON *sensor = NULL;
    cJSON *current_element = NULL;
    cJSON_ArrayForEach(current_element, sensor_array)
    {
        char *curName = cJSON_GetObjectItem(current_element, name_key)->valuestring;
        if (strcmp(sensorName, curName) == 0)
        {
            // strings are equal
            sensor = current_element;
            break;
        }
    }
    cJSON *values_array = cJSON_GetObjectItem(sensor, values_key);

    // we dont have 64 bit so do some shitty trick...
    char *newEvent_str = malloc(NEEDED_SPACE_NVS);
    sprintf(newEvent_str, "{\"timestamp\":%lld000}", (long long)time);

    cJSON *newEvent = cJSON_Parse(newEvent_str);
    // if it's NOT a system report

    // state can be optional
    if (state == 0 || state == 1)
    {
        cJSON_AddNumberToObject(newEvent, "state", state);
    }
    cJSON_AddNumberToObject(newEvent, "countPeople", peopleCount);

    cJSON_AddItemToArray(values_array, newEvent);

    char *my_json_string = cJSON_Print(root);

    err = nvs_set_str(my_nvs_handle, keyWords[nvs_index], my_json_string);
    if (err != ESP_OK)
    {
        ESP_LOGE("NVS", "Error addEventToStorage saving in NVS %s!\n", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(my_json_string);
    free(newEvent_str);

    nvs_commit(my_nvs_handle);
}

void test_access(void)
{
    esp_err_t err = nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI("NVS", "Error test_access(%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    // char outpu_str[sizeBuffer];
    // size_t size=sizeof(outpu_str);
    nvs_stats_t nvs_stats;
    err = nvs_get_stats(NULL, &nvs_stats);
    ESP_LOGI("NVS", "Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n", nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);

    nvs_close(my_nvs_handle);
}