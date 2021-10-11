#include "djmWifi.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_http_client.h"

#define NETWORK_READY_BIT 1
#define CASE(x) case x: printf(#x); break;

static void
network_event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data)
{
    //printf("Event:: %s\t", event_base);
    if (event_base == WIFI_EVENT) {
        switch(event_id) {
            CASE(WIFI_EVENT_WIFI_READY)
            CASE(WIFI_EVENT_SCAN_DONE)
            CASE(WIFI_EVENT_STA_START)
            CASE(WIFI_EVENT_STA_STOP)
            CASE(WIFI_EVENT_STA_CONNECTED)
            CASE(WIFI_EVENT_STA_DISCONNECTED)
            CASE(WIFI_EVENT_STA_AUTHMODE_CHANGE)

            CASE(WIFI_EVENT_STA_WPS_ER_SUCCESS)
            CASE(WIFI_EVENT_STA_WPS_ER_FAILED)
            CASE(WIFI_EVENT_STA_WPS_ER_TIMEOUT)
            CASE(WIFI_EVENT_STA_WPS_ER_PIN)
            CASE(WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP)

            CASE(WIFI_EVENT_AP_START)
            CASE(WIFI_EVENT_AP_STOP)
            CASE(WIFI_EVENT_AP_STACONNECTED)
            CASE(WIFI_EVENT_AP_STADISCONNECTED)
            CASE(WIFI_EVENT_AP_PROBEREQRECVED)
            default: printf("UNKNOWN");
        }
    }
    else if (event_base == IP_EVENT) {
        switch(event_id) {
            case IP_EVENT_STA_GOT_IP: {
                // ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                // TODO: log/print ip address
                printf("Got ip address\n");
                xEventGroupSetBits(arg, NETWORK_READY_BIT);
                break;
            }
            CASE(IP_EVENT_STA_LOST_IP)
            CASE(IP_EVENT_AP_STAIPASSIGNED)
            CASE(IP_EVENT_GOT_IP6)
            CASE(IP_EVENT_ETH_GOT_IP)
            CASE(IP_EVENT_PPP_GOT_IP)
            CASE(IP_EVENT_PPP_LOST_IP)
        }
    }
    else printf("UNKNOWN");
    printf("\n");
}

int
djm_wifi_init(const char *ssid, const char *password)
{
    EventGroupHandle_t evgroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(
                ESP_EVENT_ANY_BASE,
                ESP_EVENT_ANY_ID,
                &network_event_handler,
                evgroup));


    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t* netif =  esp_netif_create_default_wifi_sta();
    assert(netif);

    nvs_flash_init();

    {
        esp_netif_dhcpc_stop(netif);
        esp_netif_ip_info_t ip_info;

        IP4_ADDR(&ip_info.ip, 192, 168, 1, 94);
        IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
        IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

        esp_netif_set_ip_info(netif, &ip_info);
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {0};
    // file max limits taken from esp_wifit_types.h
    strncpy((char*)wifi_config.sta.ssid, ssid, 32);
    strncpy((char*)wifi_config.sta.password, password, 64);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    printf("Starting wifi..\n");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    EventBits_t newbits = xEventGroupWaitBits(evgroup, NETWORK_READY_BIT, true, true, portMAX_DELAY);
    assert(newbits == NETWORK_READY_BIT);
    printf("Wifi Connected\n");
    vEventGroupDelete(evgroup);
    return 0;
}

struct httpArg {
    fileChunkHandler_f callback;
    void *user;
    int downloaded;
};

static esp_err_t
my_http_event_handler(esp_http_client_event_t *evt)
{
    //printf("HTTP EVENT:\t");
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:          printf("HTTP_EVENT_ERROR");        break;
        case HTTP_EVENT_ON_CONNECTED:   printf("HTTP_EVENT_ON_CONNECTED"); break;
        case HTTP_EVENT_HEADER_SENT:    printf("HTTP_EVENT_HEADER_SENT");  break;
        case HTTP_EVENT_ON_FINISH:      printf("HTTP_EVENT_ON_FINISH");    break;
        case HTTP_EVENT_DISCONNECTED:   printf("HTTP_EVENT_DISCONNECTED"); break;
        case HTTP_EVENT_ON_HEADER:
            printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            {
                struct httpArg *arg = (evt->user_data);
                arg->downloaded += evt->data_len;
                //printf("HTTP_EVENT_ON_DATA, len=%d (total: %d)", evt->data_len, arg->downloaded);
                if (arg->callback) arg->callback(evt->data_len, evt->data, arg->user);
                break;
            }
        default:
            printf("Unknown(%08x)", evt->event_id);
    }
    printf("\n");
    return ESP_OK;
}

int
djm_download(const char *url, fileChunkHandler_f callback, void *user)
{
    struct httpArg arg = { callback, user, 0 };
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = my_http_event_handler,
        .user_data = &arg,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    printf("WILL PEFORM\n");
    ESP_ERROR_CHECK(esp_http_client_perform(client));
    printf("DID PERFORM\n");

    esp_http_client_cleanup(client);
    return 0;
}
