#include <assert.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "wifi_connection.h"

#include "wifi/wifi_config_build.h"

static const char *TAG = "wifi";
static esp_timer_handle_t reconnect_timer;

static void connect_to_access_point(void *arg)
{
    const esp_err_t result = esp_wifi_connect();
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Connection request failed: %s; retrying in 5 seconds",
                 esp_err_to_name(result));
        ESP_ERROR_CHECK(esp_timer_start_once(reconnect_timer, 5000000));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        connect_to_access_point(NULL);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        const wifi_event_sta_connected_t *event = event_data;
        ESP_LOGI(TAG, "Connected to %02x:%02x:%02x:%02x:%02x:%02x on channel %u",
                 event->bssid[0], event->bssid[1], event->bssid[2],
                 event->bssid[3], event->bssid[4], event->bssid[5], event->channel);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        const wifi_event_sta_disconnected_t *event = event_data;
        ESP_LOGW(TAG, "Disconnected (reason %u); retrying in 5 seconds", event->reason);
        ESP_ERROR_CHECK(esp_timer_start_once(reconnect_timer, 5000000));
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *event = event_data;
        ESP_LOGI(TAG, "Address: " IPSTR "; netmask: " IPSTR "; gateway: " IPSTR,
                 IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.netmask),
                 IP2STR(&event->ip_info.gw));

        wifi_ap_record_t access_point;
        if (esp_wifi_sta_get_ap_info(&access_point) == ESP_OK)
        {
            ESP_LOGI(TAG, "Signal strength: %d dBm", access_point.rssi);
        }
    }
}

void wifi_connection_start(void)
{
    if (strlen(WIFI_PASSWORD) == 0)
    {
        ESP_LOGW(TAG, "Wi-Fi disabled: copy wifi_config.example.h to wifi_config.h and set the password");
        return;
    }

    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *station = esp_netif_create_default_wifi_sta();
    assert(station != NULL);

    const esp_timer_create_args_t timer_args = {
        .callback = connect_to_access_point,
        .name = "wifi-reconnect",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &reconnect_timer));

    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .bssid_set = true,
            .bssid = WIFI_BSSID,
            .channel = 0,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_LOGI(TAG, "Joining %s using the configured access-point radio", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
}
