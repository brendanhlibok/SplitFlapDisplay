#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define pin1 GPIO_NUM_35 // IN1

void app_main(){
    while(true){
        gpio_set_direction(pin1, GPIO_MODE_INPUT);

        int trans_val = gpio_get_level(pin1);

        printf("Value: %d\n", trans_val);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    
}