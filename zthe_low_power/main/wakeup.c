#include "mySetup.h"
#include "wakeup.h"
#include "esp32/rom/rtc.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "soc/rtc.h"
#include "soc/uart_reg.h"
#include "soc/timer_group_reg.h"

// Function which runs after exit from deep sleep
void RTC_IRAM_ATTR wakeup_routine(void);

// Comment out this line if you're using the internal RTC RC (150KHz) oscillator.
#define USE_EXTERNAL_RTC_CRYSTAL
#ifdef USE_EXTERNAL_RTC_CRYSTAL
#define DEEP_SLEEP_TIME_OVERHEAD_US (650 + 100 * 240 / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ)
#else
#define DEEP_SLEEP_TIME_OVERHEAD_US (250 + 100 * 240 / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ)
#endif // USE_EXTERNAL_RTC_CRYSTAL

RTC_IRAM_ATTR void deepsleep_for_us(uint64_t duration_us)
{
    // Feed watchdog
    REG_WRITE(TIMG_WDTFEED_REG(0), 1);
    // Get RTC calibration
    uint32_t period = REG_READ(RTC_SLOW_CLK_CAL_REG);
    // Calculate sleep duration in microseconds
    int64_t sleep_duration = (int64_t)duration_us - (int64_t)DEEP_SLEEP_TIME_OVERHEAD_US;
    if (sleep_duration < 0)
    {
        sleep_duration = 0;
    }
    // Convert microseconds to RTC clock cycles
    int64_t rtc_count_delta = (sleep_duration << RTC_CLK_CAL_FRACT) / period;
    // Feed watchdog
    REG_WRITE(TIMG_WDTFEED_REG(0), 1);
    // Get current RTC time
    SET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_UPDATE);
    while (GET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_VALID) == 0)
    {
        ets_delay_us(1);
    }
    SET_PERI_REG_MASK(RTC_CNTL_INT_CLR_REG, RTC_CNTL_TIME_VALID_INT_CLR);
    uint64_t now = READ_PERI_REG(RTC_CNTL_TIME0_REG);
    now |= ((uint64_t)READ_PERI_REG(RTC_CNTL_TIME1_REG)) << 32;
    // Set wakeup time
    uint64_t future = now + rtc_count_delta;
    WRITE_PERI_REG(RTC_CNTL_SLP_TIMER0_REG, future & UINT32_MAX);
    WRITE_PERI_REG(RTC_CNTL_SLP_TIMER1_REG, future >> 32);
    // Start RTC deepsleep timer
    REG_SET_FIELD(RTC_CNTL_WAKEUP_STATE_REG, RTC_CNTL_WAKEUP_ENA, RTC_TIMER_TRIG_EN); // Wake up on timer
    WRITE_PERI_REG(RTC_CNTL_SLP_REJECT_CONF_REG, 0);                                  // Clear sleep rejection cause
    // Go to sleep
    CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
    SET_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
}

RTC_IRAM_ATTR uint64_t my_rtc_time_get_us(void)
{
    SET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_UPDATE);
    while (GET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_VALID) == 0)
    {
        ets_delay_us(1); // might take 1 RTC slowclk period, don't flood RTC bus
    }
    SET_PERI_REG_MASK(RTC_CNTL_INT_CLR_REG, RTC_CNTL_TIME_VALID_INT_CLR);
    uint64_t t = READ_PERI_REG(RTC_CNTL_TIME0_REG);
    t |= ((uint64_t)READ_PERI_REG(RTC_CNTL_TIME1_REG)) << 32;

    uint32_t period = REG_READ(RTC_SLOW_CLK_CAL_REG);

    // Convert microseconds to RTC clock cycles
    int64_t now1 = ((t * period) >> RTC_CLK_CAL_FRACT);

    return now1;
}

const char RTC_RODATA_ATTR time_fmt_str[] = "Time: %llu\n";
const char RTC_RODATA_ATTR statusFmtStr[] = "Status: %x\n";
static const char RTC_RODATA_ATTR outerFmtStr[] = "Woken up by outer barrier\n";
static const char RTC_RODATA_ATTR innerFmtStr[] = "Woken up by inner barrier\n";
// static const char RTC_RODATA_ATTR info[] = "fillsize: %d, head: %d, sizeBuffer: %d\n";

void RTC_IRAM_ATTR wakeup_routine(void)
{

    REG_WRITE(TIMG_WDTFEED_REG(0), 1);
    uint64_t rtc_time_sec = (my_rtc_time_get_us() / 1000000);
    uint64_t now = rtc_time_sec + timeOffset;
    // ets_printf(time_fmt_str, now);
    // ets_printf(info, fillSize, head, SIZE_BUFFER);
    // REG_WRITE(TIMG_WDTFEED_REG(0), 1);

    // Retrieve mask specifying which RTC pin triggered the wakeup
    uint32_t pinMaskS = REG_GET_FIELD(RTC_CNTL_EXT_WAKEUP1_STATUS_REG, RTC_CNTL_EXT_WAKEUP1_STATUS);

    uint8_t restart_flag = 1; // it restarts if flag is 0
    // ets_printf(statusFmtStr, pinMaskS);
    if (fillSize < SIZE_BUFFER)
    {
        if (pinMaskS & (uint32_t)1 << INDOOR_BARRIER_RTC)
        {
            Barrier_data data = {INDOOR_BARRIER, now};
            buffer[fillSize] = data;
            fillSize++;
            ets_printf(innerFmtStr);
        }
        REG_WRITE(TIMG_WDTFEED_REG(0), 1);

        if (pinMaskS & (uint32_t)1 << OUTDOOR_BARRIER_RTC)
        {

            Barrier_data data = {OUTDOOR_BARRIER, now};
            buffer[fillSize] = data;
            fillSize++;
            ets_printf(outerFmtStr);
        }
        REG_WRITE(TIMG_WDTFEED_REG(0), 1);
    }
    if (pinMaskS & (uint32_t)1 << WAKE_UP_BUTTON_RTC)
    {
        restart_flag = 0; // it restarts if flag is 0
    }
    REG_WRITE(TIMG_WDTFEED_REG(0), 1);

    REG_SET_BIT(RTC_CNTL_EXT_WAKEUP1_REG, RTC_CNTL_EXT_WAKEUP1_STATUS_CLR);

    while (REG_GET_FIELD(UART_STATUS_REG(0), UART_ST_UTX_OUT))
    {
        ;
    }
    // Set the pointer of the wake stub function.
    REG_WRITE(RTC_ENTRY_ADDR_REG, (uint32_t)&wakeup_routine);
    // deepsleep_for_us(20000000LL);

    if (restart_flag && (rtc_time_sec < WAKEUP_AFTER) && (fillSize < SIZE_BUFFER))
    {
        // continue sleeping
        CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
        SET_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
    }

    return;
}