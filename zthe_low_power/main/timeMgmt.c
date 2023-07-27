
#include "mySetup.h"

#include "esp_sntp.h"
#include "timeMgmt.h"
#include "main.h"

static const char *TAG = "time management";
char strftime_buf[64];

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void initSNTP(void)
{
    ESP_LOGI("PROGRESS", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, SNTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif

    sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    if (retry == retry_count)
    {
        ESP_LOGE("PROGRESS", "Could not retrieve time.!\n");
    }
    time(&now);

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("PROGRESS", "The current date/time in Germany is: %s", strftime_buf);
}
/**
 * returns the timestamp in seconds
 */
time_t get_timestamp(void)
{
#ifdef USE_WIFI
    return time(NULL);
#else
    return 0;
#endif
}
/**
 * returns the time as string
 * used in error_message()
 */
char *getDate(void)
{
    time_t now = time(&now);
    // char *strftime_buf = calloc(64, sizeof(char));
    struct tm timeinfo = {0};

    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    return strftime_buf;
}