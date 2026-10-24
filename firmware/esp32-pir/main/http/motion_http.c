#include <string.h>

#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi/wifi_connection.h"
#include "motion_http.h"

static const char *TAG = "motion-http";
static TaskHandle_t sender_task;

static void send_motion(void *arg)
{
    while (true)
    {
        // Coalesce notifications received while a request is running into
        // one pending request, rather than building an audio playback backlog.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!wifi_connection_is_ready())
        {
            ESP_LOGW(TAG, "Motion request skipped: Wi-Fi is not ready");
            continue;
        }

        const esp_http_client_config_t config = {
            .url = MOTION_SERVER_URL,
            .method = HTTP_METHOD_POST,
            // The server responds only after the audio sample finishes.
            .timeout_ms = 30000,
            .disable_auto_redirect = true,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL)
        {
            ESP_LOGE(TAG, "Could not initialize the HTTP client");
            continue;
        }

        ESP_LOGI(TAG, "Sending POST /motion");
        const esp_err_t result = esp_http_client_perform(client);
        if (result != ESP_OK)
        {
            ESP_LOGW(TAG, "Motion request failed: %s", esp_err_to_name(result));
        }
        else
        {
            const int status = esp_http_client_get_status_code(client);
            if (status == 202)
            {
                ESP_LOGI(TAG, "Motion accepted (HTTP 202)");
            }
            else
            {
                ESP_LOGW(TAG, "Motion request returned HTTP %d", status);
            }
        }

        esp_http_client_cleanup(client);
    }
}

void motion_http_start(void)
{
    if (strlen(MOTION_SERVER_URL) == 0)
    {
        ESP_LOGW(TAG, "Motion requests disabled: set MOTION_SERVER_URL at build time");
        return;
    }

    ESP_ERROR_CHECK(xTaskCreate(send_motion, "motion-http", 6144, NULL, 5,
                               &sender_task) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

bool motion_http_notify(void)
{
    if (sender_task == NULL)
    {
        return false;
    }
    if (!wifi_connection_is_ready())
    {
        ESP_LOGW(TAG, "Motion request skipped: Wi-Fi is not ready");
        return false;
    }

    return xTaskNotifyGive(sender_task) == pdPASS;
}
