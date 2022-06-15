#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <esp_log.h>
#include <esp_ping.h>
#include <ping/ping_sock.h>
#include <esp_http_server.h>
#include <mdns.h>
#include <esp_vfs.h>
#include <sdkconfig.h>
#include <cJSON.h>

#include <led_manager.h>
#include <led_strip.h>

#include <esp_http_client.h>
#include "wifi_manager.h"

static const char *TAG = "Wifi Manager";
static httpd_handle_t server = NULL;
static esp_netif_t* cfg_netif_ap;
static esp_netif_t* cfg_netif_sta;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        led_fade_in(COLOR_GREEN);
    }
}

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

    cfg_netif_ap = esp_netif_create_default_wifi_ap();
    cfg_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_wifi_init`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_mode`. Error: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_start`. Error: %s", esp_err_to_name(err));
        return err;
    }

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    // server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    config.uri_match_fn = httpd_uri_match_wildcard;
    // config.lru_purge_enable = true;
    
    err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting httpd server. Error: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

bool wifi_is_configured(void) {
    return false;
}

esp_err_t wifi_start_ap(void) {
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
            },
        }
    };

    esp_err_t err;

    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_config`. Error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Finished setting up wifi.");
    return ESP_OK;
}

esp_err_t wifi_connect_to_ap(const char ssid[32], const char password[32]) {
    esp_err_t err;

    esp_wifi_disconnect();

    wifi_config_t ap_config = {
        .sta = {
            .threshold = {
                // .authmode = WIFI_AUTH_WPA2_WPA3_PSK
            },
        },
    };
    strncpy((char*)ap_config.sta.ssid, ssid, 32);
    strncpy((char*)ap_config.sta.password, password, 32);

    err = esp_wifi_set_config(WIFI_IF_STA, &ap_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_set_config`. Error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Attempting to connect to SSID: %s Password: %s", ap_config.sta.ssid, ap_config.sta.password);

    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error running `esp_wifi_connect`. Error: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

static esp_err_t api_get_ssids_handler(httpd_req_t* req) {
    esp_err_t err;

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    err = esp_wifi_scan_start(NULL, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error executing `esp_wifi_scan_start`. Error %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return err;
    }

    err = esp_wifi_scan_get_ap_records(&number, ap_info);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error executing `esp_wifi_scan_get_ap_records`. Error %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return err;
    }

    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error executing `esp_wifi_scan_get_ap_num`. Error %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return err;
    }

    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    cJSON* json = cJSON_CreateObject();
    cJSON* ssid_arr = cJSON_AddArrayToObject(json, "ssids");
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        cJSON* ap = cJSON_CreateString((char*)(ap_info[i].ssid));
        cJSON_AddItemToArray(ssid_arr, ap);
    }

    ESP_LOGI(TAG, "APs converted to cJSON object");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, cJSON_PrintUnformatted(json), HTTPD_RESP_USE_STRLEN);
    cJSON_Delete(json);
    ESP_LOGI(TAG, "API Request complete");
    return ESP_OK;
}

static esp_err_t api_post_connect_to_ap(httpd_req_t* req) {
    ESP_LOGI(TAG, "Got request to /connect endpoint");

    char* content = calloc(req->content_len + 1, sizeof(char));
    
    // NOTE: `httpd_req_recv` does not add zero-termination
    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0) {
        ESP_LOGW(TAG, "Error getting HTTP request content. Content length: %i", req->content_len);
        free(content);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }

    // httpd_req_recv DOES NOT add zero-termination, so we need to do it.
    // Technically calloc should zero the memory so any bytes not written
    // should still be 0s, but I'm still doing it...
    content[ret] = 0x00;

    ESP_LOGD(TAG, "Connect endpoint content: %s", content);

    cJSON* json = cJSON_Parse(content);
    if (json == NULL) {
        const char* err_ptr = cJSON_GetErrorPtr();
        if (err_ptr != NULL) {
            ESP_LOGW(TAG, "Error parsing JSON. Error before: %s", err_ptr);
            free(content);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
            return ESP_FAIL;
        }
    }

    free(content);

    // If the json is correctly formatted, it should be in the following form:
    /*
        {
            "ssid": "TheSSIDHere",
            "password": "ThePasswordHere"
        }
    */

    char ssid[32];
    char password[32];

    cJSON* ssid_json = cJSON_GetObjectItem(json, "ssid");
    if (!cJSON_IsString(ssid_json) || ssid_json->valuestring == NULL) {
        ESP_LOGW(TAG, "Error getting SSID from JSON");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }
    snprintf(ssid, 32, ssid_json->valuestring);

    cJSON* password_json = cJSON_GetObjectItem(json, "password");
    if (!cJSON_IsString(password_json) || password_json->valuestring == NULL) {
        ESP_LOGW(TAG, "Error getting password from JSON");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }
    snprintf(password, 32, password_json->valuestring);

    cJSON_Delete(json);

    esp_err_t err = wifi_connect_to_ap(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error connecting to wifi.");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static bool ping_finished = false;
static bool ping_successful = false;

static void test_on_ping_end(esp_ping_handle_t hdl, void *args) {
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    printf("%d packets transmitted, %d received, time %dms\n", transmitted, received, total_time_ms);
    ping_finished = true;
    ping_successful = (received > 0);

    // esp_ping_stop(hdl);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "Error executing `esp_ping_stop`. Error: %s", esp_err_to_name(err));
    //     httpd_resp_send_500(req);
    //     return err;
    // }
    esp_ping_delete_session(hdl);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "Error executing `esp_ping_delete_session`. Error: %s", esp_err_to_name(err));
    //     httpd_resp_send_500(req);
    //     return err;
    // }
}

static esp_err_t api_get_check_connection(httpd_req_t* req) {
    esp_err_t err;

    ESP_LOGI(TAG, "Got request to check connection endpoint");

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(cfg_netif_sta, &ip_info);

    ESP_LOGI(TAG, "Got IP info");

    ip_addr_t target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.type = IPADDR_TYPE_V4;
    target_addr.u_addr.ip4.addr = ip_info.gw.addr;
    
    static char str[IP4ADDR_STRLEN_MAX];
    ip4addr_ntoa_r(&target_addr.u_addr.ip4, str, IP4ADDR_STRLEN_MAX);
    printf("IP ADDRESS: %s\n", str);

    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;
    ping_config.timeout_ms = 500;

    /* set callback functions */
    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = NULL;
    cbs.on_ping_success = NULL;
    cbs.on_ping_end = test_on_ping_end;

    ping_finished = false;
    ping_successful = false;

    ESP_LOGI(TAG, "Configured ping client");

    esp_ping_handle_t ping;
    err = esp_ping_new_session(&ping_config, &cbs, &ping);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_ping_new_session`. Error: %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return err;
    }
    err = esp_ping_start(ping);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error executing `esp_ping_start`. Error: %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return err;
    }

    ESP_LOGI(TAG, "Started ping");

    int i = 0;
    while (!ping_finished) {
        if (i >= (1000/PING_WAIT_DELAY_MS) * 6) {
            ESP_LOGI(TAG, "Timed out waiting for ping results");
            esp_ping_stop(ping);
            esp_ping_delete_session(ping);
            break;
        }
        ESP_LOGI(TAG, "Waiting for ping to finish...");
        vTaskDelay(pdMS_TO_TICKS(PING_WAIT_DELAY_MS));
        i++;
    }

    ESP_LOGI(TAG, "IP check finished");

    cJSON* resp_json = cJSON_CreateObject();
    cJSON_AddItemToObject(resp_json, "success", cJSON_CreateBool(ping_successful));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, cJSON_PrintUnformatted(resp_json), HTTPD_RESP_USE_STRLEN);
    cJSON_Delete(resp_json);
    ESP_LOGI(TAG, "Request handling finished");
    return ESP_OK;
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

esp_err_t wifi_start_config_server(void) {
    if (server == NULL) {
        ESP_LOGW(TAG, "`wifi_start_config_server` called before HTTP server setup!");
        return ESP_FAIL;
    }

    static const httpd_uri_t api_get_ssids_config = {
        .uri = "/api/get_ssids",
        .method = HTTP_GET,
        .handler = api_get_ssids_handler,
        .user_ctx = NULL
    };

    static const httpd_uri_t api_connect_config = {
        .uri = "/api/connect",
        .method = HTTP_POST,
        .handler = api_post_connect_to_ap,
        .user_ctx = NULL
    };

    static const httpd_uri_t api_check_connection_config = {
        .uri = "/api/check_connection",
        .method = HTTP_GET,
        .handler = api_get_check_connection,
        .user_ctx = NULL
    };

    static const httpd_uri_t config_get_config = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = config_get_handler,
        .user_ctx = NULL
    };
    
    httpd_register_uri_handler(server, &api_get_ssids_config);
    httpd_register_uri_handler(server, &api_connect_config);
    httpd_register_uri_handler(server, &api_check_connection_config);
    httpd_register_uri_handler(server, &config_get_config);

    return ESP_OK;
}
