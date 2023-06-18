#include "mySetup.h"
#include "webFunctions.h"
#include "caps_ota.h"
#include "nvs.h"
#include "rest_client.h"
#include "cJSON.h"
#include <stdlib.h>

uint8_t fetchNumber(const char *key);
void updateOTA(void *args);
const char *fetch_url = "http://caps-platform.live:3000/api/users/34/config/device/fetch";
const char *update_url = "http://caps-platform.live:3000/api/users/34/config/device/update";
const char *token = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2ODQ1MTc5MjMsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiMzQvODIifQ.Sx1yIYFaF8hRFcRXFooq4jB3VG4dePK_KcGoRm5lAtUgSEHyH8TSWRGNhzInor7qY63ZF0eum7d_eFrLbuRCftyQ8Y6OSij7C0Ck8ZfZKwR-AnxKVXtbAscDIEjucP22CxFa1UQyqpoOIh21EqfqM-K_HtMG_rZvTguZO6cG5BmuDdC1IPrV8LDFnhA_HGN98HDLZw3Ss6A4oFbSFY7zjDhr5DSUEht-izLUNv3lEEJhk9XkyJsVs4XzHyMsDovG623PZTzJZTwrnk2A0eUGAG-QcgSTkeaB8URpiCWszHaq_stixaUZwHZNSKpkNOoKEQcqVJtuiJ-q5hLdy2SwxzC69N6HK2hWnKCpB5y9STh4Qb4rQTZwgIxszyW-JJ0TNKZgbOdMSVMbAHlHhd17S8U-u05voYArYh08hv5kk1VVnn6_kmAqNRqxrkH4oEX_IUhv-YyXopovZQvN2j0OIjIGXGtTVoH2YK43R81k7otupf-F9SnQnJEni4SAA8Ewm0Q-eXNH7OwZijhfJtXveDubt-RdgSvihC7MqDnj3PO026b2_r07YsSHIC2r3Pfupjp16aGzfxIK8oCpcYkifWu7BebfrNWAYZo79b6I70r5wzYRiTMnZbYt_-_vN_rPsFH8hLrg7y0MTRQ9S1KOIfjlWQrzW-NPz_6vmdaZ8ok";

void init_web_functions(void)
{
    // printf("count: %d\n", getCount_backup());
    // printf("set count to 69\n");
    // setCount_backup(69);
    // printf("updated count: %d\n", getCount_backup());
    // systemReport(msg);

    xTaskCreate(updateOTA, "update OTA", 4000, NULL, PRIO_OTA_TASK, &xOTA);
}
/**
 * TASK
 */
void updateOTA(void *args)
{
    while (1)
    {
        vTaskDelay((UPDATE_OTA * 1000) / portTICK_PERIOD_MS);
        if (ota_update() == ESP_OK)
        {
            ESP_LOGI("PROGRESS", "UPDATES ESP");

            esp_restart();
        }
        else
        {
            ESP_LOGI("BUG", "NO ESP UPDATE");
        }
        uint32_t heapSize = esp_get_free_heap_size();
        ESP_LOGI("PROGRESS", "[APP] Free memory: %d bytes", heapSize);
        if (heapSize < 8000)
        {
            error_message("PROGRESS", "You have a memory leak :(", "");
            sendDataFromJSON_toDB(NO_OPEN_NVS);
            esp_restart();
        }
        if (fetchNumber("restart_flag") == 1)
        {
            // ESP_LOGI("PROGRESS", "RESTART because flag in IoT-platform is 1.");
            error_message("PROGRESS", "RESTART because flag in IoT-platform is 1.", "");
            esp_restart();
        }
    }
}

/**
 * the key "sys_report" contains an array as string with system reports
 */
void systemReport(const char *msg)
{
    const char *sys_rep_key = "sys_report";

    // get current report:
    rest_client_init(fetch_url, "type=global");
    esp_err_t err = rest_client_set_token(token);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error getCount_backup(%s) could not set token.\n", esp_err_to_name(err));
    }
    rest_client_fetch(sys_rep_key);

    err = rest_client_perform(NULL);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error getCount_backup(%s) could not perform.\n", esp_err_to_name(err));
    }

    char *raw_data_str = (char *)rest_client_get_fetched_value("", INT, false, &err);
    // printf("raw string: %s\n", raw_data_str);
    cJSON *sys_report = cJSON_Parse(raw_data_str);
    // we save the array as a string
    cJSON *array = cJSON_Parse(cJSON_GetObjectItem(sys_report, sys_rep_key)->valuestring);
    cJSON_AddItemToArray(array, cJSON_CreateString(msg));

    char *arrayAsStr = cJSON_PrintUnformatted(array);
    // printf("raw system report: %s\n", arrayAsStr);

    // send array back to system report
    rest_client_init(update_url, "type=global");
    err = rest_client_set_token(token);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error getCount_backup(%s) could not set token.\n", esp_err_to_name(err));
    }

    rest_client_set_key(sys_rep_key, arrayAsStr);
    err = rest_client_perform(NULL);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error setCount_backup(%s) could not backup count.\n", esp_err_to_name(err));
    }
    free(raw_data_str);
    free(arrayAsStr);

    cJSON_Delete(sys_report);
    cJSON_Delete(array);
}

uint8_t getCount_backup(void)
{
    return fetchNumber("count");
}

uint8_t fetchNumber(const char *key)
{
    rest_client_init(fetch_url, "type=global");

    esp_err_t err = rest_client_set_token(token);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error fetchNumber(%s) could not set token.\n", esp_err_to_name(err));
    }
    // adds count to fetch request:
    rest_client_fetch(key);

    err = rest_client_perform(NULL);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error fetchNumber(%s) could not perform.\n", esp_err_to_name(err));
    }

    // char *raw_data_str = (char *)rest_client_get_fetched_value("", INT, false, &err);
    // printf("raw string: %s\n", raw_data_str);

    char *raw_data = (char *)rest_client_get_fetched_value(key, STRING, true, &err);
    // printf("raw fetched data: %d\n", raw_data);

    // convert str to int with atoi
    uint8_t count = atoi(raw_data);
    free(raw_data);

    return count;
}

void setCount_backup(uint8_t count)
{
    rest_client_init(update_url, "type=global");
    esp_err_t err = rest_client_set_token(token);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error getCount_backup(%s) could not set token.\n", esp_err_to_name(err));
    }

    char *tmp = malloc(sizeof(char) * 5);
    sprintf(tmp, "%d", count);

    rest_client_set_key("count", tmp);
    err = rest_client_perform(NULL);
    free(tmp);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error setCount_backup(%s) could not backup count.\n", esp_err_to_name(err));
    }
}