// this file contains the whole setup process
#include "myESP_setup.h"

// function declarations
void setup_display(void);
static void event_handler(void* arg, esp_event_base_t event_base,
                           int32_t event_id, void* event_data);
void wifi_init_sta(void);
// void test_wifi(void);



void setup_myESP(void){
    // set up log levels:
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("test_mqtt", ESP_LOG_VERBOSE);

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
    
	// start task, for analyzing the 
	// xTaskCreate(analyzer, "analizer", 4096, NULL, PRIO_ANALIZER, NULL);
	// xTaskCreate(showRoomState, "show count", 2048, NULL, PRIO_SHOW_COUNT, NULL);
	// xTaskCreate(pushInBuffer, "push in Buffer", 2048, NULL, PRIO_IN_BUFFER, NULL);
}



void setup_display(void){
    ssd1306_128x64_i2c_init();
	ssd1306_setFixedFont(ssd1306xled_font6x8);
	ssd1306_clearScreen();
	ssd1306_printFixed(0,0,"People count:",STYLE_NORMAL);
	ssd1306_printFixedN(0,16,"0",STYLE_BOLD,2);
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
    
    /**
    Creates an event group with 24 bits and it stores
    the flags in individual bits.
    The tasks can wait for the flag to be raised
    **/
    my_wifi_event_group = xEventGroupCreate();
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

int64_t get_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv sec) * 1000LL + (tv.tv usec/1000LL):
}