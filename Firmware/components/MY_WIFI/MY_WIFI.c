#include <stdio.h>
#include "MY_WIFI.h"

//#include "WIFI.h"
///* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
//	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables globale :::::::::::::::::::::::::::::::::::::::::::::: */
//
//	    
//
///* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
//	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    
//	    
//	    
///* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
//	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions :::::::::::::::::::::::::::::::::::::::::::::: */
//
//
//// --------------------------------------------------------------------------------------------------------------------------------------
//	
//
//// ------------------------------------------------------------------------------------------------------------------------------
//// ESP32-S3 WiFi Example using esp_wifi API (ESP-IDF)
//// Features:
////  - Secure WPA2-PSK connection
////  - Auto-reconnect
////  - Periodic scan and display of available networks
////  - Event-driven handling via esp_event
//
//#include <string.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "esp_system.h"
//#include "esp_event.h"
//#include "esp_log.h"
//#include "nvs_flash.h"
//#include "esp_wifi.h"
//
//static const char *TAG = "wifi_espidfs3";
//
//// Replace with your network credentials
//#define WIFI_SSID      "YOUR_SSID"
//#define WIFI_PASS      "YOUR_PASSWORD"
//
//// Scan interval in seconds
//#define SCAN_INTERVAL_S 10
//
//// Event handler for WiFi events
//static void wifi_event_handler(void* arg, esp_event_base_t event_base,
//                               int32_t event_id, void* event_data)
//{
//    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//        ESP_LOGI(TAG, "Station started, connecting to SSID: %s", WIFI_SSID);
//        esp_wifi_connect();
//    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//        ESP_LOGW(TAG, "Disconnected, retrying...");
//        esp_wifi_connect();
//    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//        ESP_LOGI(TAG, "Got IP: %s", ip4addr_ntoa(&event->ip_info.ip));
//    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
//        uint16_t num_ap = 0;
//        esp_wifi_scan_get_ap_num(&num_ap);
//        wifi_ap_record_t *ap_records = malloc(num_ap * sizeof(wifi_ap_record_t));
//        esp_wifi_scan_get_ap_records(&num_ap, ap_records);
//        ESP_LOGI(TAG, "Scan done: %d APs found", num_ap);
//        for (int i = 0; i < num_ap; i++) {
//            ESP_LOGI(TAG, "%2d: SSID: %s, RSSI: %d, Security: %s",
//                     i + 1,
//                     ap_records[i].ssid,
//                     ap_records[i].rssi,
//                     (ap_records[i].authmode == WIFI_AUTH_OPEN) ? "Open" : "Secured");
//        }
//        free(ap_records);
//    }
//}
//
//void wifi_init_sta(void)
//{
//    // Initialize TCP/IP and event loop
//    ESP_ERROR_CHECK(esp_netif_init());
//    ESP_ERROR_CHECK(esp_event_loop_create_default());
//
//    esp_netif_create_default_wifi_sta();
//
//    // WiFi driver init
//    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//
//    // Register event handlers
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                        ESP_EVENT_ANY_ID,
//                                                        &wifi_event_handler,
//                                                        NULL,
//                                                        NULL));
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
//                                                        IP_EVENT_STA_GOT_IP,
//                                                        &wifi_event_handler,
//                                                        NULL,
//                                                        NULL));
//    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                        WIFI_EVENT_SCAN_DONE,
//                                                        &wifi_event_handler,
//                                                        NULL,
//                                                        NULL));
//
//    // WiFi configuration
//    wifi_config_t wifi_config = {
//        .sta = {
//            .ssid = "",
//            .password = "",
//            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
//            .pmf_cfg = {
//                .capable = true,
//                .required = false
//            },
//        },
//    };
//    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
//    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
//
//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//    ESP_ERROR_CHECK(esp_wifi_start());
//
//    ESP_LOGI(TAG, "wifi_init_sta finished.");
//}
//
//void scan_task(void *pvParameters)
//{
//    while (1) {
//        ESP_LOGI(TAG, "Starting WiFi scan...");
//        wifi_scan_config_t scan_conf = {
//            .ssid = 0,
//            .bssid = 0,
//            .channel = 0,
//            .show_hidden = true
//        };
//        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_conf, false)); // blocking scan
//        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_S * 1000));
//    }
//}
//
//extern "C" void app_main(void)
//{
//    // Initialize NVS
//    esp_err_t ret = nvs_flash_init();
//    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//        ESP_ERROR_CHECK(nvs_flash_erase());
//        ret = nvs_flash_init();
//    }
//    ESP_ERROR_CHECK(ret);
//
//    // Initialize WiFi in station mode
//    wifi_init_sta();
//
//    // Create scan task
//    xTaskCreate(scan_task, "wifi_scan", 4096, NULL, 5, NULL);
//}
//}
