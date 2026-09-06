#include <stdio.h>
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp32/rom/ets_sys.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define ESP_WIFI_SSID      "bdan"
#define ESP_WIFI_PASS      "CharizardEX14"   
#define ESP_MAXIMUM_RETRY   5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

#define pin1 GPIO_NUM_27 // IN1
#define pin2 GPIO_NUM_26 // IN2
#define pin3 GPIO_NUM_25 // IN3
#define pin4 GPIO_NUM_33 // IN4

static const int steps_per_revolution = 2038;

static int step_case = 0;
static uint32_t step_delay_us;


// This function controls a single step of the motor
void stepperStep(int step_case){
    switch(step_case){
        case 0:
            gpio_set_level(pin1, 1);
            gpio_set_level(pin2, 1);
            gpio_set_level(pin3, 0);
            gpio_set_level(pin4, 0);
        break;
        case 1:
            gpio_set_level(pin1, 0);
            gpio_set_level(pin2, 1);
            gpio_set_level(pin3, 1);
            gpio_set_level(pin4, 0);
        break;
        case 2:
            gpio_set_level(pin1, 0);
            gpio_set_level(pin2, 0);
            gpio_set_level(pin3, 1);
            gpio_set_level(pin4, 1);
        break;
        case 3:
            gpio_set_level(pin1, 1);
            gpio_set_level(pin2, 0);
            gpio_set_level(pin3, 0);
            gpio_set_level(pin4, 1);
        break;
    }
}

// This function takes in the number of steps needed to move with the direction and commands the motor
// Notes: Negative value is CCW, positive value is CW, full rotation is 2038
void spinSteps(int steps_to_move) {
    int steps_left = steps_to_move > 0 ? steps_to_move : -steps_to_move;
    int direction = steps_to_move > 0 ? 1 : -1;

    for (int i=0; i<steps_left; i++) {
        step_case += direction;
        if (step_case == 4) {
            step_case = 0;
        } else if (step_case < 0) {
            step_case = 3;
        }

        stepperStep(step_case);
        esp_rom_delay_us(step_delay_us);

        if (i % 100 == 0) {
                vTaskDelay(1);
            }
    }

}

// This function takes in the desired speed in RPM and sets the delay time to achieve this speed.
void setSpeed(int rpm) {
    step_delay_us = 60UL * 1000UL * 1000UL / steps_per_revolution / rpm;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

#if 0
void wifi_init_softap(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
}
#endif

void wifi_init_sta(void){
    s_wifi_event_group = xEventGroupCreate();

    esp_netif_init();

    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,

        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

}

void app_main(void){
    gpio_set_direction(pin1, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin2, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin3, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin4, GPIO_MODE_OUTPUT);

   //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    while(true){
        setSpeed(5);
        spinSteps(-steps_per_revolution);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }


}
