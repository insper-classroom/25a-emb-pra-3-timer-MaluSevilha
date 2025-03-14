/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

const int PIN_TRIGGER = 15;
const int PIN_ECHO = 14;

volatile bool timer_fired = false;
volatile uint32_t start_us, stop_us;

void pulso_trigger(int TRIGGER){
    gpio_put(PIN_TRIGGER, 1);
    sleep_us(10);
    gpio_put(PIN_TRIGGER, 0);
    sleep_us(2);
}

void echo_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { 
        stop_us = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8) { 
        start_us = to_us_since_boot(get_absolute_time());
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;
    return 0;
}

int main() {
    stdio_init_all();
    char comando;
    char comando_parar = 'p';
    alarm_id_t alarm;
    datetime_t agora = {
        .year  = 2025,
        .month = 03,
        .day   = 13,
        .dotw  = 3,
        .hour  = 12,
        .min   = 44,
        .sec   = 00
    };

    rtc_init();
    rtc_set_datetime(&agora);

    gpio_init(PIN_TRIGGER);
    gpio_set_dir(PIN_TRIGGER, GPIO_OUT);

    gpio_init(PIN_ECHO);
    gpio_set_dir(PIN_ECHO, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_ECHO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &echo_callback);

    while (true) {
        scanf("%c", &comando);
        
        while (comando == 's' && comando_parar != 'o'){            
            pulso_trigger(PIN_TRIGGER);
            alarm = add_alarm_in_ms(500, alarm_callback, NULL, false);

            while(stop_us == 0 && timer_fired == false){}

            rtc_get_datetime(&agora);

            if (!timer_fired){
                double dist = ((stop_us - start_us)*0.0343)/2;
                cancel_alarm(alarm);
                printf("%d:%d:%d - %lf cm\n", agora.hour, agora.min, agora.sec, dist);
            } else {
                printf("%d:%d:%d - FALHA \n", agora.hour, agora.min, agora.sec);
            }

            timer_fired = false;
            comando_parar = getchar_timeout_us(10000);
            stop_us = 0;
        }
    }
}
