idf_component_register(SRCS "rest_client.c"
                      INCLUDE_DIRS "include/"
                      PRIV_INCLUDE_DIRS "./"
                      REQUIRES json esp_http_client)

set_target_properties(${COMPONENT_LIB} PROPERTIES LINK_FLAGS_DEBUG -s)