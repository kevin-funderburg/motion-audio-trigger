#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi/wifi_connection.h"
#include "http/motion_http.h"

#define PIR_GPIO GPIO_NUM_27
#define LED_GPIO GPIO_NUM_26

static const char *TAG = "motion";

void app_main(void)
{
    const gpio_config_t pir_config =
    {
        .pin_bit_mask = 1ULL << PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    const gpio_config_t led_config =
    {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&pir_config));
    ESP_ERROR_CHECK(gpio_config(&led_config));

    int motion_detected = gpio_get_level(PIR_GPIO);
    gpio_set_level(LED_GPIO, motion_detected);

    ESP_LOGI(TAG, "PIR input on GPIO%d; LED output on GPIO%d", PIR_GPIO, LED_GPIO);

    wifi_connection_start();
    motion_http_start();

    while (true)
    {
        const int current_motion = gpio_get_level(PIR_GPIO);

        if (current_motion != motion_detected)
        {
            motion_detected = current_motion;
            gpio_set_level(LED_GPIO, motion_detected);
            ESP_LOGI(TAG, "Motion %s", motion_detected ? "detected" : "ended");

            if (motion_detected)
            {
                motion_http_notify();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
