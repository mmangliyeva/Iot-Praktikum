#include "stdio.h"

#include "esp_err.h"

#include "rest_client_helper.h"

#define MAX_HTTP_RESPONSE_BUFFER 2048

// const char *"REST_CLIENT" = "REST_CLIENT";
const uint8_t length_of_base_url = 70;

char *token_client = NULL;
// static const char* token_client = "";
esp_http_client_config_t http_client_cfg;
esp_http_client_handle_t http_client;
esp_http_client_method_t http_method = HTTP_METHOD_GET;

cJSON *cjson_response = NULL;

char *response_buffer;
size_t response_len = 0;
char *host_url = NULL;

char *query_keys = NULL;
size_t query_length = 0;

esp_err_t http_event_handler(esp_http_client_event_t *http_client_event)
{
  // static char* response_buffer;
  // static int response_len;

  switch (http_client_event->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGE("REST_CLIENT", "HTTP Error...");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD("REST_CLIENT", "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI("REST_CLIENT", "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD("REST_CLIENT", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", http_client_event->header_key, http_client_event->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD("REST_CLIENT", "HTTP_EVENT_ON_DATA, len=%d", http_client_event->data_len);
    if (!esp_http_client_is_chunked_response(http_client_event->client))
    {
      if (!response_buffer)
      {
        response_buffer = (char *)malloc(esp_http_client_get_content_length(http_client_event->client) + 1);
        memset(response_buffer, 0, esp_http_client_get_content_length(http_client_event->client) + 1);
        response_len = 0;
        if (!response_buffer)
        {
          ESP_LOGE("REST_CLIENT", "Response buffer could not be allocated...");
          return ESP_FAIL;
        }
      }
      memcpy(response_buffer + response_len, http_client_event->data, http_client_event->data_len);

      response_len += esp_http_client_get_content_length(http_client_event->client) + 1;
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI("REST_CLIENT", "HTTP_EVENT_ON_FINISH");
    if (response_buffer != NULL)
    {
      // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
      response_buffer[response_len - 1] = '\0';
      ESP_LOGD("REST_CLIENT", "The buffer has the message %s", response_buffer);
    }
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI("REST_CLIENT", "HTTP_EVENT_DISCONNECTED");
    break;
  default:
    ESP_LOGW("REST_CLIENT", "Event %d cannot be recognized...", http_client_event->event_id);
    break;
  }
  return ESP_OK;
}

esp_err_t rest_client_fetch(const char *name)
{
  size_t len_query = strlen(name) + 2;
  char *query;
  asprintf(&query, "keys=%s", name);

  if (query_keys == NULL)
  {
    query_keys = query;
    query_length = len_query;
  }
  else
  {
    size_t size = len_query + query_length + 2;
    char *extended;
    asprintf(&extended, "%s,%s", query_keys, name);
    query_length += size;

    free(query_keys);
    free(query);
    query_keys = extended;
  }

  return ESP_OK;
}

esp_err_t rest_client_set_key(const char *name, const char *value)
{
  size_t len_query = strlen(name) + strlen(value) + 2;
  char *query;
  asprintf(&query, "%s=%s", name, value);

  if (query_keys == NULL)
  {
    query_keys = query;
    query_length = len_query;
  }
  else
  {
    size_t size = len_query + query_length + 2;
    char *extended;
    asprintf(&extended, "%s&%s", query_keys, query);
    query_length += size;

    free(query_keys);
    free(query);
    query_keys = extended;
  }

  return ESP_OK;
}

esp_err_t rest_client_set_token(const char *token)
{

  if (token == NULL)
  {
    printf("token == null, token: %s!\n", token);
    ESP_LOGE("REST_CLIENT", "Please provide a valid token string");
    return ESP_FAIL;
  }
  // token_client = malloc(sizeof(char) * 800);
  if (token_client != NULL)
  {
    free(token_client);
  }
  int bytes = asprintf(&token_client, "Bearer %s", token);
  if (bytes <= 0)
  {
    ESP_LOGE("PROGRESS", "Could not allocate memory for token");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t rest_client_perform(cJSON *http_op_data)
{
  esp_err_t ret = ESP_OK;

  char *tmp_url = NULL;

  if (token_client == NULL)
  {
    ESP_LOGE("REST_CLIENT", "Please provide a valid token string");
    return ESP_FAIL;
  }
  // printf("test quey length: %d\n", query_length);
  // printf("test str length: %d\n", strlen(host_url));
  // size_t length = strlen(host_url) + query_length + 2;

  // HARDCODED because strlen() crashes for me...
  // 70 stands for the length of this url: "http://caps-platform.live:3000/api/users/34/config/device/fetch"
  size_t length = length_of_base_url + query_length + 2;

  if (query_keys && length < esp_get_free_heap_size())
  {

    asprintf(&tmp_url, "%s&%s", host_url, query_keys);

    http_client_cfg.url = tmp_url;
  }
  else
  {
    http_client_cfg.url = host_url;
  }

  // printf("url: %s\n", http_client_cfg.url);

  http_client = esp_http_client_init(&http_client_cfg);
  ret = esp_http_client_set_method(http_client, http_method);
  if (ret)
  {
    return ret;
  }

  ret = rest_client_set_header("Content-Type", "application/json");
  if (ret)
  {
    return ret;
  }

  ret = rest_client_set_header("authorization", token_client);
  if (ret)
  {
    return ret;
  }

  if (http_method != HTTP_METHOD_GET && !http_op_data)
  {
    ESP_LOGE("REST_CLIENT", "This HTTP Method requires data to be passed on");
    return ESP_FAIL;
  }
  else if (http_method != HTTP_METHOD_GET && http_op_data)
  {
    char *data = cJSON_PrintUnformatted(http_op_data);
    printf("unter cJSON_PrintUnformatted\n%s\n\n", data);
    esp_http_client_set_post_field(http_client, data, strlen(data));

    free(data);
  }

  ret = esp_http_client_perform(http_client);
  if (ret)
  {
    ESP_LOGE("REST_CLIENT", "HTTP Request failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI("REST_CLIENT", "HTTP Request finished successfully with status %d", esp_http_client_get_status_code(http_client));
  ret = esp_http_client_cleanup(http_client);
  if (ret)
  {
    return ret;
  }
  // free heap
  free(host_url);
  host_url = NULL;
  free(query_keys);
  query_keys = NULL;
  if (tmp_url)
  {
    free(tmp_url);
    tmp_url = NULL;
  }
  query_length = 0;
  return ret;
}

void *rest_client_get_fetched_value(const char *key, cjson_types_t value_type, bool is_json, esp_err_t *ret)
{
  if (!response_buffer)
  {
    *ret = ESP_FAIL;
    return NULL;
  }

  if (!is_json)
  {
    *ret = ESP_OK;
    return (void *)strdup(response_buffer);
  }

  if (!cjson_response)
  {
    cjson_response = cJSON_Parse(response_buffer);
    if (cjson_response == NULL)
    {
      *ret = ESP_FAIL;
      return NULL;
    }
  }

  void *data;
  switch (value_type)
  {
  case INT:
    data = malloc(sizeof(int));
    memcpy(data, &cJSON_GetObjectItem(cjson_response, key)->valueint, sizeof(int));
    *ret = ESP_OK;
    return data;
  case STRING:
  {
    char *val_string = cJSON_GetObjectItem(cjson_response, key)->valuestring;
    data = malloc(sizeof(char) * strlen(val_string) + 1);
    sprintf(data, "%s", val_string);
    *ret = ESP_OK;
    return data;
  }
  break;
  case OBJECT:
    *ret = ESP_OK;
    return (void *)cJSON_GetObjectItem(cjson_response, key);
  default:
    ESP_LOGE("REST_CLIENT", "This data type is not supported");
    *ret = ESP_FAIL;
    return NULL;
  }
  *ret = ESP_FAIL;
  // free(token_client);
  return NULL;
}

void rest_client_set_method(esp_http_client_method_t method)
{
  http_method = method;
}

esp_err_t rest_client_set_header(const char *header, const char *value)
{
  return esp_http_client_set_header(http_client, header, value);
}

/**
 * type is either: "type=global" OR "type=device&deviceId=<here you deviceID>"
 * url should be of the form: "http://caps-platform.live:3000/api/users/<groupID>/config/device/<fetch OR update>"
 */
void rest_client_init(const char *url, const char *type)
{

  esp_log_level_set("REST_CLIENT", ESP_LOG_NONE);

  if (query_keys != NULL)
  {
    free(query_keys);
    query_length = 0;
  }
  if (host_url != NULL)
  {
    free(host_url);
    host_url = NULL;
  }
  char *tmp_host_url;
  asprintf(&tmp_host_url, "%s?%s", url, type);

  host_url = strdup(tmp_host_url);

  free(tmp_host_url);
  if (response_buffer)
  {
    free(response_buffer);
    response_buffer = NULL;
    response_len = 0;
  }

  http_client_cfg.disable_auto_redirect = false;
  http_client_cfg.max_redirection_count = 5;
  http_client_cfg.event_handler = http_event_handler;
  http_client_cfg.buffer_size_tx = 4096;

  if (cjson_response != NULL)
  {
    cJSON_free(cjson_response);
    cjson_response = NULL;
  }
}
