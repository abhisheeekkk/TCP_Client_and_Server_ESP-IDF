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
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#define PORT                        3333
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

static const char *TAG = "example";



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
    STA_INIT_IP("esp32_AP","password","192.168.254.16","192.168.254.1","255.255.255.0");
    // AP_INIT_IP("esp32_AP","password","192.168.254.1","192.168.254.1","255.255.255.0");
    WIFI_START(WIFI_MODE_STA); // WIFI_MODE_APSTA | WIFI_MODE_STA | WIFI_MODE_AP

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

static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[1280];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s\n", len, rx_buffer);

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation.
            static const char* payloadd = "This is to let you know i have received the data--->>>> Abhishek here!!";
             int sizee = strlen(payloadd);
             int to_write = sizee;
            
            while (to_write > 0) {
                int written = send(sock, payloadd, strlen(payloadd), 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }
        }
    } while (len > 0);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#ifdef CONFIG_EXAMPLE_IPV6
    else if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#ifdef CONFIG_EXAMPLE_IPV6
        else if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
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

#ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, NULL);
#endif
}
