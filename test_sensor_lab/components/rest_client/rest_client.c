#include "stdio.h"

#include "esp_err.h"

#include "rest_client_helper.h"

#define MAX_HTTP_RESPONSE_BUFFER 2048

static const char *TAG = "rest_client";

char *token_client = NULL;
// static const char* token_client = "";
static esp_http_client_config_t http_client_cfg;
static esp_http_client_handle_t http_client;
static esp_http_client_method_t http_method = HTTP_METHOD_GET;

static cJSON *cjson_response = NULL;

char *response_buffer;
size_t response_len = 0;
static char *host_url = NULL;

char *query_keys;
static size_t query_length = 0;

esp_err_t http_event_handler(esp_http_client_event_t *http_client_event)
{
  // static char* response_buffer;
  // static int response_len;

  switch (http_client_event->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGE(TAG, "HTTP Error...");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", http_client_event->header_key, http_client_event->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", http_client_event->data_len);
    if (!esp_http_client_is_chunked_response(http_client_event->client))
    {
      if (!response_buffer)
      {
        response_buffer = (char *)malloc(esp_http_client_get_content_length(http_client_event->client) + 1);
        memset(response_buffer, 0, esp_http_client_get_content_length(http_client_event->client) + 1);
        response_len = 0;
        if (!response_buffer)
        {
          ESP_LOGE(TAG, "Response buffer could not be allocated...");
          return ESP_FAIL;
        }
      }
      memcpy(response_buffer + response_len, http_client_event->data, http_client_event->data_len);

      response_len += esp_http_client_get_content_length(http_client_event->client) + 1;
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    if (response_buffer != NULL)
    {
      // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
      response_buffer[response_len - 1] = '\0';
      ESP_LOGD(TAG, "The buffer has the message %s", response_buffer);
    }
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  default:
    ESP_LOGW(TAG, "Event %d cannot be recognized...", http_client_event->event_id);
    break;
  }
  return ESP_OK;
}

esp_err_t rest_client_fetch_set_key(const char *name, const char *value)
{
  size_t len_query = strlen(name) + strlen(value) + 2;
  char *query = malloc(sizeof(char) * len_query);
  sprintf(query, "%s=%s", name, value);

  if (query_keys == NULL)
  {
    query_keys = query;
    query_length = len_query;
  }
  else
  {
    size_t size = len_query + query_length + 2;
    char *extended = malloc(sizeof(char) * size);
    sprintf(extended, "%s&%s", query_keys, query);
    query_length += size;

    free(query_keys);
    query_keys = extended;
  }

  return ESP_OK;
}

esp_err_t rest_client_set_token(const char *token)
{

  if (token == NULL)
  {
    printf("token == null, token: %s!\n", token);
    ESP_LOGE(TAG, "Please provide a valid token string");
    return ESP_FAIL;
  }
  // token_client = malloc(sizeof(char) * 800);
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
    ESP_LOGE(TAG, "Please provide a valid token string");
    return ESP_FAIL;
  }

  size_t length = strlen(host_url) + query_length + 2;

  if (query_keys && length < esp_get_free_heap_size())
  {
    tmp_url = (char *)malloc(length);

    sprintf(tmp_url, "%s?%s", host_url, query_keys);

    http_client_cfg.url = tmp_url;
  }
  else
  {
    http_client_cfg.url = host_url;
  }

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
    ESP_LOGE(TAG, "This HTTP Method requires data to be passed on");
    return ESP_FAIL;
  }
  else if (http_method != HTTP_METHOD_GET && http_op_data)
  {
    char *data = cJSON_PrintUnformatted(http_op_data);
    esp_http_client_set_post_field(http_client, data, strlen(data));
    free(data);
  }

  ret = esp_http_client_perform(http_client);
  if (ret)
  {
    ESP_LOGE(TAG, "HTTP Request failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "HTTP Request finished successfully with status %d", esp_http_client_get_status_code(http_client));
  ret = esp_http_client_cleanup(http_client);
  if (ret)
  {
    return ret;
  }
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

void *rest_client_fetch_key(const char *key, cjson_types_t value_type, bool is_json, esp_err_t *ret)
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
    ESP_LOGE(TAG, "This data type is not supported");
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

void rest_client_init(const char *url)
{

  esp_log_level_set(TAG, ESP_LOG_NONE);

  host_url = strdup(url);

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
