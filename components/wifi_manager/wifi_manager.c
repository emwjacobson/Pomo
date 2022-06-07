#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <mdns.h>
#include <esp_vfs.h>
#include <sdkconfig.h>

#include "wifi_manager.h"

#define PATH_MAX_LENGTH ESP_VFS_PATH_MAX+128
#define SCRATCH_BUFSIZE (10240)

static const char *TAG = "Wifi Manager";

esp_err_t wifi_init(void) {
    esp_err_t err;
    
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_netif_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_event_loop_create_default`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `mdns_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = mdns_hostname_set("pomo");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error setting mdns hostname. Error %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

bool wifi_is_configured(void) {
    return false;
}

esp_err_t wifi_start_ap(void) {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t err;
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_wifi_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_WIFI_CONFIG_SSID,
            .ssid_len = strlen(CONFIG_WIFI_CONFIG_SSID),
            .channel = 6,
            .password = CONFIG_WIFI_CONFIG_PASSWORD,
            .max_connection = ESP_WIFI_MAX_CONN_NUM,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = false
            }
        }
    };

    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_mode`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_config`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_start`. Error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Finished setting up wifi.");
    return ESP_OK;
}

esp_err_t wifi_start_station(void) {
    return ESP_ERR_NOT_FINISHED;
}

esp_err_t send_page(httpd_req_t* req, int fd, char* filepath, int status) {
    httpd_resp_set_type(req, "text/html");

    char* chunk = calloc(1, SCRATCH_BUFSIZE);
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                ESP_LOGE(TAG, "File sending failed!");
                close(fd);
                free(chunk);
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    free(chunk);
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    if (status != 200) httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, NULL);
    return ESP_OK;
}

static esp_err_t config_get_handler(httpd_req_t* req) {
    char filepath[PATH_MAX_LENGTH];
    strncpy(filepath, CONFIG_SETUP_FS_BASE, PATH_MAX_LENGTH);
    strncat(filepath, req->uri, PATH_MAX_LENGTH - strlen(CONFIG_SETUP_FS_BASE));

    if (strncmp(req->uri, "/", PATH_MAX_LENGTH) == 0) {
        // The '/' is already included from the req->uri strncat call
        strncat(filepath, "index.html", PATH_MAX_LENGTH - strlen(CONFIG_SETUP_FS_BASE) - 1);
    }

    esp_err_t err;

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        // ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        // /* Respond with 500 Internal Server Error */
        // httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        // return ESP_FAIL;
        
        // File does not exist, send 404
        strncpy(filepath, CONFIG_SETUP_FS_BASE "/404.html", PATH_MAX_LENGTH);
        fd = open(filepath, O_RDONLY, 0);
        err = send_page(req, fd, filepath, 200);
    } else {
        // File does exist, so send the page!

        err = send_page(req, fd, filepath, 200);
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error sending page request! Error: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

static const httpd_uri_t config_index = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = config_get_handler,
    .user_ctx = NULL
};

esp_err_t wifi_start_config_server(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.lru_purge_enable = true;

    esp_err_t err;
    err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting httpd server. Error: %s", esp_err_to_name(err));
        return err;
    }

    httpd_register_uri_handler(server, &config_index);

    return ESP_OK;
}
