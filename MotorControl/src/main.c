#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

// Defines the number of steps per rotation
static const int steps_per_revolution = 2038;

// Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
#define MOTOR_PIN_1 GPIO_NUM_26 // IN1
#define MOTOR_PIN_2 GPIO_NUM_27 // IN3
#define MOTOR_PIN_3 GPIO_NUM_14 // IN2
#define MOTOR_PIN_4 GPIO_NUM_12 // IN4

static int step_number = 0;
static uint32_t step_delay_us;

static void stepper_set_speed(int rpm)
{
    step_delay_us = 60UL * 1000UL * 1000UL / steps_per_revolution / rpm;
}

static void stepper_step_motor(int this_step)
{
    switch (this_step) {
    case 0: // 1010
        gpio_set_level(MOTOR_PIN_1, 1);
        gpio_set_level(MOTOR_PIN_2, 0);
        gpio_set_level(MOTOR_PIN_3, 1);
        gpio_set_level(MOTOR_PIN_4, 0);
        break;
    case 1: // 0110
        gpio_set_level(MOTOR_PIN_1, 0);
        gpio_set_level(MOTOR_PIN_2, 1);
        gpio_set_level(MOTOR_PIN_3, 1);
        gpio_set_level(MOTOR_PIN_4, 0);
        break;
    case 2: // 0101
        gpio_set_level(MOTOR_PIN_1, 0);
        gpio_set_level(MOTOR_PIN_2, 1);
        gpio_set_level(MOTOR_PIN_3, 0);
        gpio_set_level(MOTOR_PIN_4, 1);
        break;
    case 3: // 1001
        gpio_set_level(MOTOR_PIN_1, 1);
        gpio_set_level(MOTOR_PIN_2, 0);
        gpio_set_level(MOTOR_PIN_3, 0);
        gpio_set_level(MOTOR_PIN_4, 1);
        break;
    }
}

static void stepper_step(int steps_to_move)
{
    int steps_left = steps_to_move > 0 ? steps_to_move : -steps_to_move;
    int direction = steps_to_move > 0 ? 1 : -1;

    for (int i = 0; i < steps_left; i++) {
        step_number += direction;
        if (step_number == 4) {
            step_number = 0;
        } else if (step_number < 0) {
            step_number = 3;
        }
        stepper_step_motor(step_number);
        esp_rom_delay_us(step_delay_us);

        // Yield periodically so the idle task can run and feed the watchdog
        if (i % 100 == 0) {
            vTaskDelay(1);
        }
    }
}

void app_main(void)
{
    gpio_set_direction(MOTOR_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_PIN_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_PIN_4, GPIO_MODE_OUTPUT);

    while (1) {
        // Rotate CW slowly at 5 RPM
        stepper_set_speed(5);
        stepper_step(steps_per_revolution);
        vTaskDelay(pdMS_TO_TICKS(1000));

        /*// Rotate CCW quickly at 10 RPM
        stepper_set_speed(10);
        stepper_step(-steps_per_revolution);
        vTaskDelay(pdMS_TO_TICKS(1000));*/
    }
}
