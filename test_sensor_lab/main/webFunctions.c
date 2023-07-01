#include "mySetup.h"
#include "webFunctions.h"
#include "caps_ota.h"
#include "nvs.h"
#include "platform_api.h"
#include "cJSON.h"
#include <stdlib.h>

uint8_t fetchNumber(const char *key); // more general way to fetch something from IoT-platform
void updateOTA(void *args);

const char *fetch_url = "http://caps-platform.live:3000/api/users/34/config/device/fetch";
const char *update_url = "http://caps-platform.live:3000/api/users/34/config/device/update";
// I know no safe implementation...
const char *token = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2ODQ1MTc5MjMsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiMzQvODIifQ.Sx1yIYFaF8hRFcRXFooq4jB3VG4dePK_KcGoRm5lAtUgSEHyH8TSWRGNhzInor7qY63ZF0eum7d_eFrLbuRCftyQ8Y6OSij7C0Ck8ZfZKwR-AnxKVXtbAscDIEjucP22CxFa1UQyqpoOIh21EqfqM-K_HtMG_rZvTguZO6cG5BmuDdC1IPrV8LDFnhA_HGN98HDLZw3Ss6A4oFbSFY7zjDhr5DSUEht-izLUNv3lEEJhk9XkyJsVs4XzHyMsDovG623PZTzJZTwrnk2A0eUGAG-QcgSTkeaB8URpiCWszHaq_stixaUZwHZNSKpkNOoKEQcqVJtuiJ-q5hLdy2SwxzC69N6HK2hWnKCpB5y9STh4Qb4rQTZwgIxszyW-JJ0TNKZgbOdMSVMbAHlHhd17S8U-u05voYArYh08hv5kk1VVnn6_kmAqNRqxrkH4oEX_IUhv-YyXopovZQvN2j0OIjIGXGtTVoH2YK43R81k7otupf-F9SnQnJEni4SAA8Ewm0Q-eXNH7OwZijhfJtXveDubt-RdgSvihC7MqDnj3PO026b2_r07YsSHIC2r3Pfupjp16aGzfxIK8oCpcYkifWu7BebfrNWAYZo79b6I70r5wzYRiTMnZbYt_-_vN_rPsFH8hLrg7y0MTRQ9S1KOIfjlWQrzW-NPz_6vmdaZ8ok";

void init_web_functions(void)
{
    xTaskCreate(updateOTA, "update OTA", 6000, NULL, PRIO_OTA_TASK, &xOTA);
}
/**
 * TASK
 * can update via OTA
 * checks for memory leaks
 * can resart device with restart_flag from the IoT-platform
 */
void updateOTA(void *args)
{
    while (1)
    {
        vTaskDelay((UPDATE_OTA * 1000) / portTICK_PERIOD_MS);
        // updates via OTA
        if (ota_update() == ESP_OK)
        {
            ESP_LOGI("PROGRESS", "UPDATES ESP");

            esp_restart();
        }
        else
        {
            ESP_LOGI("BUG", "NO ESP UPDATE");
        }
        // check for memory leaks
        uint32_t heapSize = esp_get_free_heap_size();
        ESP_LOGI("PROGRESS", "[APP] Free memory: %d bytes", heapSize);
        if (heapSize < 8000)
        {
            error_message("PROGRESS", "You have a memory leak :(", "");
            sendDataFromJSON_toDB(NO_OPEN_NVS);
            esp_restart();
        }

        uint32_t heapSize_begin = esp_get_free_heap_size();
        // restart the device over the air
        if (fetchNumber("restart_flag") == 1)
        {
            // ESP_LOGI("PROGRESS", "RESTART because flag in IoT-platform is 1.");
            error_message("PROGRESS", "RESTART because flag in IoT-platform is 1.", "");
            esp_restart();
        }
        uint32_t heapSize_end = esp_get_free_heap_size();
        if (heapSize_begin - heapSize_end != 0)
        {
            ESP_LOGE("PROGRESS", "Memory leaks %lu in fetch restart flag.", (long)(heapSize_begin - heapSize_end));
        }
    }
}

/**
 * the key "sys_report" contains an array as string with system reports
 * in the IoT-platform.
 * The array itself is a string, so we do quite messy parsing
 */
void systemReport(const char *msg)
{
    const char *sys_rep_key = "sys_report";

    platform_api_init(fetch_url);
    platform_api_set_token(token);
    platform_api_set_query_string("type", "global");
    platform_api_set_query_string("keys", sys_rep_key);
    platform_api_perform_request();
    char *raw_data_str;
    platform_api_retrieve_val(sys_rep_key, STRING, true, NULL, (void **)&raw_data_str);
    // printf("raw string: %s\n", raw_data_str);
    platform_api_cleanup();
    cJSON *array = cJSON_Parse(raw_data_str);
    cJSON_AddItemToArray(array, cJSON_CreateString(msg));
    // perfrom messy parsing because arrays are not supported thats why we safe a string
    cJSON *tmp = cJSON_Parse(cJSON_PrintUnformatted(array));
    char *arrayAsStr = cJSON_PrintUnformatted(tmp);
    // printf("raw system report: %s\n", arrayAsStr);

    // send array back to system report
    platform_api_init(update_url);
    platform_api_set_token(token);
    platform_api_set_query_string("type", "global");

    platform_api_set_query_string(sys_rep_key, arrayAsStr);
    platform_api_perform_request();
    platform_api_cleanup();

    free(raw_data_str);
    free(arrayAsStr);

    cJSON_Delete(tmp);
    cJSON_Delete(array);
}

uint8_t getCount_backup(void)
{
    return fetchNumber("count");
}

uint8_t getPrediction(void)
{
    return fetchNumber("prediction");
}

uint8_t fetchNumber(const char *key)
{
    platform_api_init(fetch_url);
    platform_api_set_token(token);
    platform_api_set_query_string("type", "global");
    platform_api_set_query_string("keys", key);
    platform_api_perform_request();
    char *raw_data;
    esp_err_t err = platform_api_retrieve_val(key, STRING, true, NULL, (void **)&raw_data);
    if (err != ESP_OK)
    {
        error_message("platform_api", "errors while fetching ", key);
    }
    // error_message("platform_api", "no errors...", key);
    // somehow the key is a string -> convert to integer
    uint8_t count = atoi(raw_data);
    platform_api_cleanup();
    if (err == ESP_OK)
    {
        free(raw_data);
    }

    return count;
}

void setCount_backup(uint8_t count)
{
    platform_api_init(update_url);
    platform_api_set_token(token);
    platform_api_set_query_string("type", "global");

    char *count_str = malloc(sizeof(char) * 5);
    sprintf(count_str, "%d", count);

    platform_api_set_query_string("count", count_str);
    platform_api_perform_request();
    platform_api_cleanup();

    free(count_str);
}