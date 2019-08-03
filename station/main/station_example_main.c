/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_connect.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/ 

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data) 
{
    if (event_base == WIFI_EVENT){ 
        switch (event_id){
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
            break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY){
                    esp_wifi_connect();
                    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                    s_retry_num++;
                    ESP_LOGI(TAG, "Reconnect to WiFi");
                }
                ESP_LOGI(TAG,"Connect failed");
            break;
        }
    }else if (event_base == IP_EVENT){
        switch (event_id){
            case IP_EVENT_STA_GOT_IP:{
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "IP Address:%s",
                        ip4addr_ntoa(&event->ip_info.ip));
                s_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;}
        }
    }
} 

void app_main()
{
    s_wifi_event_group = xEventGroupCreate();
    wifi_connect(&event_handler, TAG);
}