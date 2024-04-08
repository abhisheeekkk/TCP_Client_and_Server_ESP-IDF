/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "addr_from_stdin.h"
#include "lwip/err.h"
#include "lwip/sockets.h"


#if defined(CONFIG_EXAMPLE_IPV4)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#elif defined(CONFIG_EXAMPLE_IPV6)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#else
#define HOST_IP_ADDR "192.168.1.19"
#endif

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "example";
static const char *payload = "Within the realm of this digital communication, 'Message from ESP32' serves as a humble yet significant token of the vast capabilities of modern technology. It represents more than just a string of characters; it encapsulates the essence of connectivity, innovation, and the boundless potential of human ingenuity. As it traverses through the intricate network of circuits, routers, and servers, it carries with it the aspirations, intentions, and commands of its creators. Each byte encoded within this message is a testament to the tireless efforts of engineers, developers, and innovators striving to push the boundaries of what's possible in the digital age. 'Message from ESP32' embodies the spirit of collaboration, bridging gaps, and forging connections across distances physical and metaphorical. It symbolizes the relentless pursuit of progress, the quest for knowledge, and the pursuit of excellence in the ever-evolving landscape of technology. With each transmission, it sparks a chain reaction of events,he remarkable journey of humanity's exploration into the realm of bits and bytes.";
//"Message from ESP32 ";



wifi_config_t wifi_sta_config;
wifi_config_t wifi_ap_config;

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                //ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                //ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
                esp_wifi_connect();
                break;
        }
    }
    else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t* event = event_data;
                ESP_LOGI(TAG, "Station connected with IP: "IPSTR", GW: "IPSTR", Mask: "IPSTR".",
                    IP2STR(&event->ip_info.ip),
                    IP2STR(&event->ip_info.gw),
                    IP2STR(&event->ip_info.netmask));
                break;
            }
        }
    }
}

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch (event_id) {
        case IP_EVENT_AP_STAIPASSIGNED: {
            ip_event_ap_staipassigned_t* event = event_data;
            ESP_LOGI(TAG, "SoftAP client connected with IP: "IPSTR".",
                IP2STR(&event->ip));
            break;
        }
    }
}

void WIFI_INIT() {
    // NVS: Required by WiFi Driver
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init()); 
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void AP_INIT_IP(char *ssid, char *password, char *ip, char *gw, char *nmask) {
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();
    
    esp_netif_ip_info_t IP_settings_ap;
    IP_settings_ap.ip.addr=ipaddr_addr(ip);
    IP_settings_ap.netmask.addr=ipaddr_addr(nmask);
    IP_settings_ap.gw.addr=ipaddr_addr(gw);
    esp_netif_dhcps_stop(esp_netif_ap);
    esp_netif_set_ip_info(esp_netif_ap, &IP_settings_ap);
    esp_netif_dhcps_start(esp_netif_ap);
 
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_ap_event_handler, NULL, NULL));

    strcpy((char *)wifi_ap_config.ap.ssid, ssid); 
    strcpy((char *)wifi_ap_config.ap.password, password);
    wifi_ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_ap_config.ap.max_connection = 4;
}

void AP_INIT(char *ssid, char *password) {
    esp_netif_create_default_wifi_ap();
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_ap_event_handler, NULL, NULL));

    strcpy((char *)wifi_ap_config.ap.ssid, ssid); 
    strcpy((char *)wifi_ap_config.ap.password, password);
    wifi_ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_ap_config.ap.max_connection = 4;
}

void STA_INIT_IP(char *ssid, char *password, char *ip, char *gw, char *nmask) {
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    esp_err_t ret = esp_netif_dhcpc_stop(esp_netif_sta);
    if(ret == ESP_OK) ESP_LOGI(TAG, "esp_netif_dhcpc_stop OK");
    else ESP_LOGI(TAG, "esp_netif_dhcpc_stop ERROR");

    esp_netif_ip_info_t IP_settings_sta;
    IP_settings_sta.ip.addr=ipaddr_addr(ip);
    IP_settings_sta.netmask.addr=ipaddr_addr(nmask);
    IP_settings_sta.gw.addr=ipaddr_addr(gw);
    esp_netif_set_ip_info(esp_netif_sta, &IP_settings_sta);
  
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, NULL));
    
    strcpy((char *)wifi_sta_config.sta.ssid, ssid); 
    strcpy((char *)wifi_sta_config.sta.password, password);
}

void STA_INIT(char *ssid, char *password) {
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, NULL));
        
    strcpy((char *)wifi_sta_config.sta.ssid, ssid);
    strcpy((char *)wifi_sta_config.sta.password, password);
}

void WIFI_START(wifi_mode_t mode) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
   
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
  
    if (mode==WIFI_MODE_APSTA || mode==WIFI_MODE_STA) ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    if (mode==WIFI_MODE_APSTA || mode==WIFI_MODE_AP) ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    
    ESP_ERROR_CHECK(esp_wifi_start());
}

void setup_sys(void) {
    ///*
    WIFI_INIT();
    // STA_INIT_IP("wifi_ssid","password","192.168.1.19","192.168.1.1","255.255.255.0");
    AP_INIT_IP("esp32_AP","password","192.168.254.15","192.168.254.1","255.255.255.0");
    WIFI_START(WIFI_MODE_AP); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP

    // WIFI_INIT();
    // STA_INIT_IP("wifi_ssid","password","192.168.1.19","192.168.1.1","255.255.255.0");
    // AP_INIT_IP("esp32_AP","password","192.168.254.1","192.168.254.1","255.255.255.0");
    // WIFI_START(WIFI_MODE_AP); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP
    //*/

    /*
    WIFI_INIT();
    STA_INIT_IP("wifi_ssid","password","192.168.1.19","192.168.1.1","255.255.255.0");
    AP_INIT("esp32_AP","password");
    WIFI_START(WIFI_MODE_APSTA); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP
    */

    /*
    WIFI_INIT();
    STA_INIT("wifi_ssid","password");
    AP_INIT("esp32_AP","password");
    WIFI_START(WIFI_MODE_APSTA); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP
    */

    /*
    WIFI_INIT();
    STA_INIT_IP("wifi_ssid","password","192.168.1.19","192.168.1.1","255.255.255.0");
    WIFI_START(WIFI_MODE_STA); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP
    */

    /*
    WIFI_INIT();
    AP_INIT("wifi_ssid","password");
    WIFI_START(WIFI_MODE_AP); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP
    */
}

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = "192.168.254.16";
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
#if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr("192.168.254.16");
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
        struct sockaddr_in6 dest_addr = { 0 };
        inet6_aton(host_ip, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_storage dest_addr = { 0 };
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket createdddd, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    setup_sys();

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}
