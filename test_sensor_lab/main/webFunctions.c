#include "mySetup.h"
#include "webFunctions.h"
#include "caps_ota.h"
#include "rest_client.h"
#include "cJSON.h"

void updateOTA(void *args);

void init_web_functions(void)
{
    //  rest_client_init("http://caps-platform.live:3000/api/users/34/config?configId=9");
    rest_client_init("http://caps-platform.live:3000/api/users/34/config?configId=9");
    char *output = malloc(sizeof(char) * 10000);

    esp_err_t err;
    const char *token = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2ODU5OTY2MDUsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiMzQifQ.iVbL1seLo5zAyfbuVtjfS-KQ-yZaZ1Gq9T3abJq1baZST3znvoI79_WKwKaU9atpgk9NWQ8DfoxHnsL6j-U7m-BtcCjzr7_O7OOUAH4Oo4gPJIaicdlxfV55atTP9ybP9TKpsOlPIElSjSK2JCS8YqcFG0qdIKeGXYcv3q0YKT7W_HrQTsI3g3XF39p54E7prYBv_y4rtMQxwfIcEi0GyHCg_K9m0FybRVYq3FNIhn42_nmhwuzB2JWSa2790iyiIwj3ZCxmAJqDDRCVPLlIO3sPiHfczY4bchjQu0bJpltb721GaKQAw16iWcf_uSsZJT1FtTsMXZpu5K-_LVZvEuBZ6wxGJqpHN06Wk87xuUGn4oQeLNNdt1J2GQD8f7r8Nd1XZ0QrszP73lpfK63Y1GU4HxTyUVHuJF1nZjOC3g7QZxkJjaln_Q_9lieVNU_KkhEPES_oMbvPBHkxUrAIlJtzDNkVViTZK-Oiz71Dpj65ERxmGeN9aW-WuuG9agUGDslALqZ-fC2TKETpw1pz0nWNC-AhLfXIpJJKE5LgJR-g0flT4wU9kRNWM-8pXkWRaEb60D9yjsZmJ5qv05LrT4C_5yU69jiYT5q-wd3xEy0JFZJ47uf5RcpG-1ZmePC_FIwCf0wGa1GOB8AURNJXvcaB1yN8BxAYeSZMrthz6-A";
    err = rest_client_set_token(token);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error init_web_functions(%s) could not set token.\n", esp_err_to_name(err));
    }
    // rest_client_fetch_set_key("leider", "geil");
    err = rest_client_perform(NULL);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error init_web_functions(%s) could not perform.\n", esp_err_to_name(err));
    }
    // output fetch: {"results":{"id":9,"settings_json":"\"{\\\"test\\\":\\\"helloe\\\", \\\"leider\\\":42}\"","createdAt":"2023-06-05T20:32:48.000Z","updatedAt":"2023-06-06T08:11:27.000Z","userId":34}}
    cJSON *results = (cJSON *)rest_client_fetch_key("results", OBJECT, true, &err);
    if (err != ESP_OK)
    {
        ESP_LOGI("BUG", "Error init_web_functions(%s) could not fetch key.\n", esp_err_to_name(err));
    }
    printf("output fetch: %s\n", cJSON_Print(results));

    cJSON *settings = cJSON_Parse(cJSON_GetObjectItem(results, "settings_json")->valuestring);
    printf("settings fetch: %s\n", cJSON_Print(settings));

    rest_client_set_method(HTTP_METHOD_PUT);
    settings = cJSON_GetObjectItem(settings, "test");

    // printf("output fetch: %s\n", output);

    // err = rest_client_perform(NULL);
    free(output);

    // xTaskCreate(updateOTA, "update OTA", 4000, NULL, PRIO_OTA_TASK, &xOTA);
    // in teest mode activate xOTA again
}

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
    }
}

uint8_t getCount_backup(void)
{
    return 0;
}

void setCount_backup(uint8_t count)
{
}