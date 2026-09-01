#include <stdio.h>
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp32/rom/ets_sys.h>

#define pin1 GPIO_NUM_12
#define pin2 GPIO_NUM_14
#define pin3 GPIO_NUM_27
#define pin4 GPIO_NUM_26

static const int steps_per_revolution = 2038;

static int step_case = 0;
static uint32_t step_delay_us;

void stepperPulse(bool dir){
    switch(step_case){
        case 0:
            gpio_set_level(pin1, 1);
            gpio_set_level(pin2, 0);
            gpio_set_level(pin3, 1);
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
            gpio_set_level(pin2, 1);
            gpio_set_level(pin3, 0);
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

        stepperPulse(step_case);
        esp_rom_delay_us(step_delay_us);

        if (i % 100 == 0) {
                vTaskDelay(1);
            }
    }

}

void setSpeed(int rpm) {
    step_delay_us = 60UL * 1000UL * 1000UL / steps_per_revolution / rpm;
}



void app_main(void)
{
    gpio_set_direction(pin1, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin2, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin3, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin4, GPIO_MODE_OUTPUT);

    while(true){
        setSpeed(5);
        spinSteps(steps_per_revolution);
        vTaskDelay(pdMS_TO_TICKS(1000));

    }


    }
