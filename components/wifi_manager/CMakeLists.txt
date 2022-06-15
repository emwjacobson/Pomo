idf_component_register(SRCS "wifi_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES led_manager esp_timer freertos esp_system mdns lwip esp_http_client esp_netif esp_common esp_wifi log esp_http_server json)
