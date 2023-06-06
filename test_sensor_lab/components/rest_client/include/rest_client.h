#ifndef _REST_CLIENT_H_
#define _REST_CLIENT_H_

#include "esp_err.h"
#include "esp_http_client.h"
#include "stdbool.h"
#include "cJSON.h"

typedef enum
{
  INT,
  STRING,
  OBJECT,
} cjson_types_t;

/**
 * @brief Sets the token to be used in the Authorization header.
 *
 * @param token JWT from your device key
 * @return esp_err_t
 */
esp_err_t rest_client_set_token(const char *token);

/**
 * @brief It configures the library to use the given url as the base url for all requests.
 *
 * @param url Endopoint url for the IoT Platform
 */
void rest_client_init(const char *url, const char *type);

/**
 * @brief Sets the header to be used in the request.
 *        This function works with both the fetch and update endpoints.
 *
 * @param name Name of the key
 * @param value Value of the key
 * @return esp_err_t
 */
esp_err_t rest_client_fetch(const char *name);
esp_err_t rest_client_set_key(const char *name, const char *value);

/**
 * @brief It performs a REST request to the configured endpoint.
 *
 * @param http_op_data For the moment, pass NULL
 * @return esp_err_t
 */
esp_err_t rest_client_perform(cJSON *http_op_data);

/**
 * @brief It defines which REST operation to perform. By default, it uses GET
 *
 * @param method
 */
void rest_client_set_method(esp_http_client_method_t method);

/**
 * @brief It obtains the value from the endpoint's response.
 *
 * @param key The key name in your JSON settings in the IoT Platform
 * @param value_type The type of the expected value
 * @param is_json Indicates if the expected response from the IoT Platform is a JSON object
 * @param[out] ret The return value for a given key.
 * @return void*
 */
void *rest_client_get_fetched_value(const char *key, cjson_types_t value_type, bool is_json, esp_err_t *ret);

#endif