// this file contains the whole setup process
#include "myESP_setup.h"

// function declarations
void setup_display(void);
static void event_handler(void* arg, esp_event_base_t event_base,
                           int32_t event_id, void* event_data);
void wifi_init_sta(void);
void test_mqtt(void);
void setup_mqtt(void);// has to be setup after wifi

// gloabal vars
int wifi_conn_retry_num = 0, wifi_conn_max_retry = 10;
esp_mqtt_client_config_t my_mqtt_client_cfg = {
    .event_handle = my_mqtt_event_handler,
    .host = "mqtt.caps-platform.live",
    .port = 1883,
    .username = "JWT",
    .password = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE2ODQzMzY3MjYsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiMzQvODIifQ.x1Vv2qN0NfV2TTx9HGjTv1Qh8yxZfIJAvDPXK1R7dcRgsKMMqOS_NL7skBFqmEyhPe30Tnu4UQeH0OdXl7lOCGIkya-SKi4K2oVZYvByGQHoHToWuOpE-e62ohW0QCXMLq2Np5qV9zkaUb5BibQOt8qE7gzZKFAbjvYRPq9xyJAFeLVCii5160CevLP5ZOCcEN_6iOaq9kDyY-JaIHKssNMWuyo-X_j9Jz53-FzgLgB-omAjd1TP5DRQQS7RdgWDmd0BlfeNvPg-1rCt7gCDNo3uA_tpoUaNVPWYOe4jF5BFph6f_ZToQwEGGk_y5OTz9Y9c9Jw823ZHjm3FJ3PtcrhvdErJJkh8d2NausVIwCZ9llWUdFCT7-buMUF2Li-TRoG0OXvb2232n5BCv6Vc26kNhrJcBvD9gmjfyNVYso1r0EjsoMLSEmcRH8RbscPUcb5-p0rCUkCZNM9xF2-fH5Okq25wZQCsZpqUMugive6lYyZKGQ7e2cZerYEyiSfE70XvVpF5vsF83AYHFlWBS82gTZfAp_Dw-9Um3PoOzdEZmkhmD5csoAiX169acQat_rD2hZh8It4f7oE2KTouBSQUFrnBhDJKrWZO0HE7Dvhu9GwYbw-CvyE8x1UBokxVpY-Id1jyAki3Vm4ClUk2rTN15sayVbA_koVTJTkVtNA",
};
EventGroupHandle_t my_wifi_event_group;
esp_mqtt_client_handle_t my_client;


void setup_myESP(void){
    // set up log levels:
    // esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("test_mqtt", ESP_LOG_VERBOSE);
    // init global variables:
    
    setup_display();
    //init light barrier 1
	gpio_pad_select_gpio(PIN_DETECT_1);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_1, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_1)); 
    // make pin active for ISR
	gpio_set_intr_type(PIN_DETECT_1, GPIO_INTR_POSEDGE);
	
	//init light barrier 2
	gpio_pad_select_gpio(PIN_DETECT_2);
	ESP_ERROR_CHECK(gpio_set_direction(PIN_DETECT_2, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_en(PIN_DETECT_2));
    // make pin active for ISR
	gpio_set_intr_type(PIN_DETECT_2, GPIO_INTR_POSEDGE);
    	
	// init isr
	gpio_install_isr_service(0);

    //(void*)PIN_DETECT_1
    gpio_isr_handler_add(PIN_DETECT_1, isr_barrier1, NULL);
    gpio_isr_handler_add(PIN_DETECT_2, isr_barrier2, NULL);
 
    wifi_init_sta();
    EventBits_t bits = xEventGroupWaitBits(my_wifi_event_group, MY_WIFI_CONN_BIT | MY_WIFI_FAIL_BIT,
										   pdFALSE, pdFALSE, portMAX_DELAY);
	if(bits & MY_WIFI_CONN_BIT) {
		ESP_LOGI("MAIN", "WiFi connected!");
		setup_mqtt();
        test_mqtt();
	}
	else if(bits & MY_WIFI_FAIL_BIT) {
		ESP_LOGE("MAIN", "WiFi failed to connect");
	} 
	else {
		ESP_LOGW("MAIN", "Unexpected event");
	}
	// start task, for analyzing the 
	// xTaskCreate(analyzer, "analizer", 4096, NULL, PRIO_ANALIZER, NULL);
	// xTaskCreate(showRoomState, "show count", 2048, NULL, PRIO_SHOW_COUNT, NULL);
	// xTaskCreate(pushInBuffer, "push in Buffer", 2048, NULL, PRIO_IN_BUFFER, NULL);
}


void setup_mqtt(void){
    my_client = esp_mqtt_client_init(&my_mqtt_client_cfg);
    esp_mqtt_client_register_event(my_client, ESP_EVENT_ANY_ID, my_mqtt_event_handler, my_client);
    esp_mqtt_client_start(my_client);

}
void test_mqtt(void){
	char* mqtt_msg;
	// always send 42 
	const char* my_user_id_mqtt = "34";
	const char* my_device_id_mqtt = "82";


	int bytes = asprintf(&mqtt_msg,
				"{\"sensors\":[{\"name\": \"light_barrier\", \"values\":[{\"timestamp\":%lld, \"countPeople\": %d}]}]}", 
				(long long)1684394386976, 42);
	ESP_LOGI("test_mqtt", "The message is %s", mqtt_msg);
	
	char topic[50];
	sprintf(topic,"%s/%s/data",my_user_id_mqtt,my_device_id_mqtt);
	ESP_LOGI("test_mqtt", "The topic is %s", topic);
	int msg_id = esp_mqtt_client_publish(my_client, topic, mqtt_msg, bytes, Q0S, 0) ;
	// We only support Q05 0 or 1
	if(msg_id < 0) {
		ESP_LOGE("test_mqtt", "MQTT could not publish your message");
	}
}

void wifi_init_sta(void) {
    // for avoiding bug: "wifi:wifi osi_nvs_open fail ret=4353"
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    my_wifi_event_group = xEventGroupCreate();
    /**
    Creates an event group with 24 bits and it stores
    the flags in individual bits.
    The tasks can wait for the flag to be raised
    **/
    
    // my_wifi_event_group = &my_wifi;

    // Init Network Interface
    ESP_ERROR_CHECK(esp_netif_init());
    /**
    Create system event loop for all network events
    **/
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Create a station network interface object
    esp_netif_create_default_wifi_sta();
    // Initialize with default WiFi config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // Create the WiFi task and init the driver
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id, instance_got_ip;
    /**
    Register an event handler to the default event loop
    The event handler instance is required to deregister the
    handler
    **/
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,
                                                        &event_handler, NULL,
                                                        &instance_got_ip));
    /**
    Creates the WiFi cfg object
    pmf_cfg defines extended privacy protection
    to network management frames.
    **/
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    // Determines the station mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // Assigns the cfq to the station
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    // Starts the station
    ESP_ERROR_CHECK(esp_wifi_start());    
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data){
    if(event_base == WIFI_EVENT){
        switch(event_id){
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    if(wifi_conn_retry_num < wifi_conn_max_retry){
                        esp_wifi_connect();
                        wifi_conn_retry_num++;
                        ESP_LOGW( "WIFI", "Retrying connection...");
                    } 
                    else {
                        xEventGroupSetBits(my_wifi_event_group,MY_WIFI_FAIL_BIT);
                    }
                }
                break;
            default:
                break;
        }
    } 
    else if(event_base == IP_EVENT){
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                {
                    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
                    ESP_LOGI("WIFI", "Received IP: " IPSTR, IP2STR(&event->ip_info.ip));
                    // wifi_sniffer_retry_index = 0;
                    xEventGroupSetBits(my_wifi_event_group, MY_WIFI_CONN_BIT);
                }
                break;
            default:
                break;
        }   
    }
}


void setup_display(void){
    ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
	ssd1306_clearScreen();
	ssd1306_printFixed(0,0,"People count:",STYLE_NORMAL);
	ssd1306_printFixedN(0,16,"0",STYLE_BOLD,2);
}

uint64_t get_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec)*(uint64_t)1000 + (tv.tv_usec/(uint64_t)1000);
}
