/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

const int PIN_TRIGGER = 14;
const int PIN_ECHO = 13;

volatile echo_flag = 0;
volatile bool timer_fired = false;

void pulso_trigger(int TRIGGER){
    gpio_put(PIN_TRIGGER, 1);
    sleep_ms(10);
    gpio_put(PIN_TRIGGER, 0);
    sleep_ms(2);
}

void echo_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        echo_flag = 1;
    } else if (events == 0x8) { // rise edge
        echo_flag = 0;
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;
    return 0;
}

int main() {
    stdio_init_all();

    char[100] comando;
    char[4] comando_parar = "abcd";
    alarm_id_t alarm;
    datetime_t agora = {
        .year  = 2025,
        .month = 03,
        .day   = 12,
        .dotw  = 3,
        .hour  = 08,
        .min   = 05
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
        scanf("%s", comando);

        while (comando == "start" && comando_parar != "stop"){
            pulso_trigger(PIN_TRIGGER);
            int dist = 0;
            alarm = add_alarm_in_ms(5000/343, alarm_callback, NULL, false);

            if (echo_flag){
                uint32_t start_ms = to_ms_since_boot(get_absolute_time());

                while(echo_flag){
                    sleep_ms(10);
                }

                uint32_t stop_ms = to_ms_since_boot(get_absolute_time());
                dist = (100*(((stop_ms/1000) - (start_ms/1000))*343))/2;
                cancel_alarm(alarm);
                timer_fired = false;
            }

            rtc_get_datetime(&agora);

            if(timer_fired && (dist == 0)){
                printf("%d:%d:%d - FALHA", agora.hour, agora.min, agora.sec);
            } else {
                printf("%d:%d:%d - %d", agora.hour, agora.min, agora.sec, dist);
            }

            for(int i = 0; i < 4; i++){
                comando_parar[i] = getchar_timeout_us(10000);
            }
        }
    }
}
