idf_component_register(SRCS "wifi_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash led_manager esp_timer freertos esp_system mdns lwip esp_http_client esp_netif esp_common esp_wifi log esp_http_server json)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/www)
    spiffs_create_partition_image(www ${CMAKE_CURRENT_SOURCE_DIR}/www FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${CMAKE_CURRENT_SOURCE_DIR}/www doesn't exit. Please run 'npm run build' in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()
