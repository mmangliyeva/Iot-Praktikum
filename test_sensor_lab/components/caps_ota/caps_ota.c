#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "cJSON.h"

#include "caps_ota.h"
#define CONFIG_FIRMWARE_UPGRADE_URL CONFIG_FIRMWARE_SERVER_URL

const char* TAG = "caps_ota";
const char* TAG_EVENT = "caps_ota_event";

char *bearer_token = NULL;

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG_EVENT, "HTTP_EVENT_ERROR key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG_EVENT, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG_EVENT, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG_EVENT, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            {   
                int status = esp_http_client_get_status_code(evt->client);
                
                if(status > 200) {
                    char *tmp = (char *)evt->data;
                    tmp[evt->data_len] = '\000';
                    cJSON *obj = cJSON_Parse(tmp);
                    if(!obj) {
                        ESP_LOGE(TAG_EVENT, "cJSON could not parse the response. Showing it raw: %s", tmp);
                        break;
                    }
                    cJSON *err_name = cJSON_GetObjectItem(obj, "name");
                    if(!err_name) {

                        ESP_LOGW(TAG_EVENT, "Server error [%d] with response [%s]. This is not a IoT Platform error format. Please check again!", status, tmp);
                    }
                    cJSON *errors = cJSON_GetObjectItem(obj, "errors");
                    int array_size = cJSON_GetArraySize(errors);
                    ESP_LOGW(TAG_EVENT, "The server presented the following errors:");
                    for(int i = 0; i < array_size; ++i) {
                        cJSON *arr_obj = cJSON_GetArrayItem(errors, i);
                        cJSON *err_arr_obj = cJSON_GetObjectItem(arr_obj, "message");
                        if(!err_arr_obj) {
                            ESP_LOGW(TAG_EVENT, "The server registered an error but we could not parse it! The error code was [%d]", status);
                        }
                        ESP_LOGW(TAG_EVENT, "\t\t[%d] The server responded: %s (%d)", i, err_arr_obj->valuestring, status);
                    }
                }
                break;
            }
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG_EVENT, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_EVENT, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGI(TAG_EVENT, "The ID of this event is %d", evt->event_id);
    }
    return ESP_OK;
}

void set_bearer_token(const char* token, size_t size)
{
    if(bearer_token)
    {
        free(bearer_token);
    }

    bearer_token = (char *) malloc(sizeof(char)*(7+size+1));

    snprintf(bearer_token, 8+size, "Bearer %s\n", token);

}

void init(void) {

    // TODO: get a Queue from data processing team and trigger ota_update when an event arrived int the queue

}

esp_err_t validate_image_header(esp_app_desc_t *new_app_info) {
    if(new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        printf("Running firmware verion: %s, Firmware version to update: %s\n", running_app_info.version, new_app_info->version);
        if(atoi(running_app_info.version) >= atoi(new_app_info->version))
        {
            return ESP_FAIL;
        }
    }

    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        printf("Current running version is the same as a new. We will not continue the update.\n");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    err = esp_http_client_set_header(http_client, "authorization", bearer_token);
    return err;
}

int ota_update(void)
{   
    #if !CONFIG_APP_PROJECT_VER_FROM_CONFIG
        ESP_LOGE(TAG, "You need to specify the version through menu config \n\t Please enable CONFIG_APP_PROJECT_VER_FROM_CONFIG");
        ESP_ERROR_CHECK(ESP_FAIL);
    #endif

    if(strcmp(CONFIG_FIRMWARE_SERVER_URL, "http://caps-platform.live:3000/api/users/<userID>/ota/download/firmware/latest.bin") == 0) {
        ESP_LOGE(TAG, "You need to specify your userID through menuconfig");
        abort();
    }
    int length = strlen(CONFIG_APP_PROJECT_VER)+strlen(CONFIG_FIRMWARE_UPGRADE_URL)+2;
    char *ota_url = (char *) malloc(sizeof(char)*length);
    snprintf(ota_url, length, "%s/%s", CONFIG_FIRMWARE_UPGRADE_URL, CONFIG_APP_PROJECT_VER);
    set_bearer_token(CONFIG_OTA_TOKEN, strlen(CONFIG_OTA_TOKEN));
    esp_http_client_config_t config = {
        .url = ota_url,
        .keep_alive_enable = true,
        .buffer_size_tx = 2048,
        .event_handler = _http_event_handler,
        .max_redirection_count = 5
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);

    if (err != ESP_OK) {
        printf("ESP HTTPS OTA Begin failed\n");
        return ESP_FAIL;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        printf("esp_https_ota_read_img_desc failed\n");
        return ESP_FAIL;
    }

    if(validate_image_header(&app_desc) != ESP_OK) {
        printf("Validation failed\n");
        return ESP_ERR_OTA_VALIDATE_FAILED;
    }
    
    ESP_LOGI(TAG, "ESP HTTPS OTA in progress");
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        // esp_https_ota_perform returns after every read operation which gives user the ability to
        // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
        // data read so far.
    }
    ESP_LOGI(TAG, "ESP HTTPS OTA in finished");

    free(ota_url);

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        printf("Complete data was not received.\n");
        return ESP_ERR_HTTP_INVALID_TRANSPORT;
    } else {
        esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            printf("ESP_HTTPS_OTA upgrade successful. Rebooting ...\n");
            return ESP_OK;
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                printf("Image validation failed, image is corrupted\n");
                return ESP_ERR_OTA_VALIDATE_FAILED;
            }
            printf("ESP_HTTPS_OTA upgrade failed 0x%x\n", ota_finish_err);
            return ESP_ERR_OTA_BASE;
        }
    }
    return -1;
}

