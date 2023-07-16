#include "mySetup.h"
#include "powerMesurment.h"
#include "referenceWriting.h"

#ifdef WITH_CALCULATION

int index1 = 0;
int minERR1 = INT_MAX;

int index2 = 0;
int minERR2 = INT_MAX;

SemaphoreHandle_t xCore1 = NULL;
SemaphoreHandle_t xCore2 = NULL;

void core1(void *args)
{
    // Template matching for every text
    int writingTemplateWidth = 200;
    int writingTemplateHeight = 20;
    int writingFieldWidth = 280;
    int writingFieldHeight = 32;
    unsigned char *writing = (uint8_t *)calloc(writingFieldWidth * writingFieldHeight, sizeof(uint8_t));

    for (int t = 0; t < 8; t++)
    {
        for (int y = 0; y <= writingFieldHeight - writingTemplateHeight; y++)
        {
            // y -> 12
            // task 1 von 0-5
            // task 2 von 6-12
            for (int x = 0; x <= writingFieldWidth - writingTemplateWidth; x++)
            {
                // x -> 80
                int ERR = 0;
                for (int i = 0; i < writingTemplateHeight; i++)
                {
                    // i -> 20
                    for (int j = 0; j < writingTemplateWidth; j++)
                    {
                        // i -> 200
                        int searchPix = writing[y * writingFieldWidth + i * writingFieldWidth + j + x];
                        int templatePix = references[t][i * writingTemplateWidth + j];
                        ERR += abs(searchPix - templatePix);
                    }
                }

                if (minERR1 > ERR)
                {
                    minERR1 = ERR;
                    index1 = t;
                }
            }
            vTaskDelay(10);
        }
    }
    ESP_LOGI("PROGRESS", "Best match core1: %d", index1);
    xSemaphoreGive(xCore1);
    vTaskSuspend(NULL);
}

void core2(void *args)
{
    // Template matching for every text
    int writingTemplateWidth = 200;
    int writingTemplateHeight = 20;
    int writingFieldWidth = 280;
    int writingFieldHeight = 32;
    unsigned char *writing = (uint8_t *)calloc(writingFieldWidth * writingFieldHeight, sizeof(uint8_t));

    for (int t = 8; t < 16; t++)
    {
        for (int y = 0; y <= writingFieldHeight - writingTemplateHeight; y++)
        {
            // y -> 12
            // task 1 von 0-5
            // task 2 von 6-12
            for (int x = 0; x <= writingFieldWidth - writingTemplateWidth; x++)
            {
                // x -> 80
                int ERR = 0;
                for (int i = 0; i < writingTemplateHeight; i++)
                {
                    // i -> 20
                    for (int j = 0; j < writingTemplateWidth; j++)
                    {
                        // i -> 200
                        int searchPix = writing[y * writingFieldWidth + i * writingFieldWidth + j + x];
                        int templatePix = references[t][i * writingTemplateWidth + j];
                        ERR += abs(searchPix - templatePix);
                    }
                }

                if (minERR2 > ERR)
                {
                    minERR2 = ERR;
                    index2 = t;
                }
            }
            vTaskDelay(10);
        }
    }
    ESP_LOGI("PROGRESS", "Best match core2: %d", index2);
    xSemaphoreGive(xCore2);
    vTaskSuspend(NULL);
}

void calculation(void)
{

    xTaskCreatePinnedToCore(core1, "core1", 5000, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(core2, "core2", 5000, NULL, 10, NULL, 1);
    // wait until wifi is ready56
    xCore1 = xSemaphoreCreateBinary();
    xCore2 = xSemaphoreCreateBinary();

    if (xSemaphoreTake(xCore1, portMAX_DELAY) == pdTRUE)
    {
        xSemaphoreGive(xCore1);
    }
    // wait until wifi is ready56
    if (xSemaphoreTake(xCore2, portMAX_DELAY) == pdTRUE)
    {
        xSemaphoreGive(xCore2);
    }
    if (minERR1 > minERR2)
    {
        ESP_LOGI("PROGRESS", "Best match: %d", index2);
    }
    else
    {
        ESP_LOGI("PROGRESS", "Best match: %d", index1);
    }
}

void measureCalculation(void)
{
    startMeasure();
    calculation();
    stopMeasure();
}
#endif

void resetEnergyMeasurement(void)
{
    ESP_ERROR_CHECK(gpio_set_level(REST_ENERGY_PIN, 1));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(REST_ENERGY_PIN, 0));
}
void toggleEnergyMeasurement(void)
{
    ESP_ERROR_CHECK(gpio_set_level(TOGGL_ENERGY_PIN, 1));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(TOGGL_ENERGY_PIN, 0));
}
void startMeasure(void)
{
    ESP_LOGI("PROGRESS", "Starting measurments");
    ESP_ERROR_CHECK(gpio_set_level(LED_PIN, 0));
    toggleEnergyMeasurement(); // start measurement
}

void stopMeasure(void)
{
    toggleEnergyMeasurement();
    // stop measurement
    resetEnergyMeasurement();
    ESP_ERROR_CHECK(gpio_set_level(LED_PIN, 1));
    ESP_LOGI("PROGRESS", "Measurment done");
}
