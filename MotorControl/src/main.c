#include <stdio.h>
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp32/rom/ets_sys.h>
#include <sys/param.h>
#include <freertos/task.h>

#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "cJSON.h"

//WIFI Station

#define ESP_WIFI_SSID      "bdan"
#define ESP_WIFI_PASS      "CharizardEX14"   
#define ESP_MAXIMUM_RETRY   5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *WIFITAG = "wifi station";
static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

//MDNS

#define MDNS_INSTANCE "splitflapdisplay"

//#define EXAMPLE_BUTTON_GPIO   CONFIG_MDNS_BUTTON_GPIO

static const char *MDNSTAG = "mdns-test";

//HTTP Server

static const char *HTTPTAG = "Basic HTTP Server";

//JSON 

static const char *JSONTAG = "JSON_PARSE";

//HARDWARE SETUP

#define pin1 GPIO_NUM_27 // IN1
#define pin2 GPIO_NUM_26 // IN2
#define pin3 GPIO_NUM_25 // IN3
#define pin4 GPIO_NUM_33 // IN4

static const int steps_per_revolution = 2038;

static int step_case = 0;
static uint32_t step_delay_us;

// Data type for motor movement
struct motorChange{
    int steps;
    char direction[4];
};

// Sets the motor pins based on the step case.
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

// Takes in the number of steps needed to move with the direction and commands the motor
// Notes: Positive value is CW, Negative value is CCW, 1 Full rotation is 2038 steps
void spinSteps(struct motorChange change) {

    int direction = 1;

    // Set the increment for the direction
    if (strcmp(change.direction, "cw") == 0) {
        direction = -1;
    } else {
        direction = 1;
    }

    // Run through the steps
    for (int i=0; i<change.steps; i++) {
        step_case += direction;

        if (step_case == 4) {
            step_case = 0;
        } else if (step_case < 0) {
            step_case = 3;
        }

        stepperStep(step_case);
        esp_rom_delay_us(step_delay_us);

        // Delays 1 tick every 100 steps
        if (i % 100 == 0) {
                vTaskDelay(1);
            }
    }

}

// Takes in the desired speed in RPM and sets the delay time to achieve this speed.
void setSpeed(int rpm) {
    step_delay_us = 60UL * 1000UL * 1000UL / steps_per_revolution / rpm;
}

// Handles wifi connection and IP setting events
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    // Handle wifi connection
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFITAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFITAG,"connect to the AP fail");

    // Handle IP setting once connected
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFITAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initializes the station mode and attempts connection to the network
void wifi_init_sta(void){

    s_wifi_event_group = xEventGroupCreate();

    //Initialize the networking stack
    esp_netif_init();

    //Creates dispatcher that matches incoming events
    esp_event_loop_create_default();

    //Creates network interface for station mode
    esp_netif_create_default_wifi_sta();

    //Initializes wifi driver with default settings
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Registers the wifi_event handler for WiFi connection, and IP
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

    // Builds the config struct with my SSID/password
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start()); // Starts the driver, and triggers the handler to call esp_wifi_connect()
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); //Turn off power save mode
    ESP_LOGI(WIFITAG, "wifi_init_sta finished.");

    // Waits for connection or failure, outputs result
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFITAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFITAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(WIFITAG, "UNEXPECTED EVENT");
    }

}

// Initializes mdns
static void initialize_mdns(void)
{
    char *hostname = MDNS_INSTANCE;

    //initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_LOGI(MDNSTAG, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK(mdns_instance_name_set(MDNS_INSTANCE));

}

// Parses json for direction and speed
struct motorChange parse_json(const char *json_string) {

    struct motorChange result;

    cJSON *root = cJSON_Parse(json_string);

    //Split the steps value
    cJSON *steps_item = cJSON_GetObjectItemCaseSensitive(root, "steps");
    
    int steps = 0;
    
    if (cJSON_IsString(steps_item)){
        steps = atoi(steps_item->valuestring);
        ESP_LOGI(JSONTAG, "Steps (string): %s, As Integer: %d", steps_item->valuestring, steps);
    } else {
        ESP_LOGI(JSONTAG, "Invalid step format");
    }

    result.steps = steps;
    
    // Split the direction value
    cJSON *direction_item = cJSON_GetObjectItemCaseSensitive(root, "direction");

    ESP_LOGI(JSONTAG, "Direction: %s", direction_item->valuestring);

    strncpy(result.direction, direction_item->valuestring, sizeof(result.direction) - 1);

    printf("steps: %d, direction: %s\n", result.steps, result.direction);

    // Frees up heap memory allocated for the tree
    cJSON_Delete(root);
    
    return result;

}

// Handles motor spin inputs from website
static esp_err_t motor_command_handler(httpd_req_t *req) {
    char buf[128];
    int ret, remaining = req->content_len;

    // If buffer is empty, send error
    if (remaining <= 0) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) -1));
    
        if (ret <=0) {
            return ESP_FAIL;
        }

        buf [ret] = '\0';
        printf("received chunk %s\n", buf);

        remaining -= ret;
    }

    struct motorChange change = parse_json(buf);

    setSpeed(14);
    spinSteps(change);

    const char* resp_str = "<h1>TESTING</h1>";
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(HTTPTAG, "success");

    return ESP_OK;
}

// Sets uri for the motor command handler
static const httpd_uri_t motor_command_uri= {
    .uri       = "/motor_command",               // the address at which the resource can be found
    .method    = HTTP_POST,          // The HTTP method (HTTP_GET, HTTP_POST, ...)
    .handler   = motor_command_handler, // The function which process the request
    .user_ctx  = NULL               // Additional user data for context
};

// Starts webserver
httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(HTTPTAG, "Server started successfully, registering URI handlers...");
        httpd_register_uri_handler(server, &motor_command_uri);
        return server;
    }

    ESP_LOGE(HTTPTAG, "Failed to start server");
    return NULL;
}


void app_main(void){
    //Initialize Motor Pins
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

     //Start station mode
    ESP_LOGI(WIFITAG, "Setting up Station Mode");
    wifi_init_sta();

    initialize_mdns();

    start_webserver();

}
