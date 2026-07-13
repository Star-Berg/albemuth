/**
 * @file user_main.h
 * @brief User-interface tasks for the programmable DC power supply.
 */

#ifndef _FILE_USER_MAIN_H_
#define _FILE_USER_MAIN_H_

#include <core/dev/at_device.h>
#include <core/dev/datalink.h>
#include <core/pm/function_scheduler.h>

#include <core/dev/display/ht16k33.h>
#include <core/dev/gpio/pca9555.h>
#include <core/dev/sensor/hdc1080.h>

#ifdef __cplusplus
extern "C"
{
#endif

//=================================================================================================
// Global peripherals

extern const unsigned char led_lut[];

extern iic_halt iic_bus;
extern ht16k33_dev_t ht16k33;
extern hdc1080_dev_t hdc1080;

extern gpio_halt gpio_beep;
extern gpio_halt gpio_alarm_led;
extern gpio_halt gpio_output_led;

extern volatile uint16_t flag_init_cmpt;
extern gmp_scheduler_t sched;

//=================================================================================================
// GMP user entry points

void init(void);
void mainloop(void);
void setup_peripheral(void);

void ctl_init(void);
void ctl_mainloop(void);

//=================================================================================================
// Non-blocking tasks

gmp_task_status_t tsk_startup(gmp_task_t* tsk);
gmp_task_status_t tsk_key_flush(gmp_task_t* tsk);
gmp_task_status_t tsk_LED_flush(gmp_task_t* tsk);
gmp_task_status_t tsk_psu_display(gmp_task_t* tsk);
gmp_task_status_t tsk_psu_alarm(gmp_task_t* tsk);
gmp_task_status_t tsk_psu_encoder(gmp_task_t* tsk);
gmp_task_status_t tsk_psu_persistence(gmp_task_t* tsk);

//=================================================================================================
// Peripheral helpers

void beep_on(void);
void beep_off(void);
void alarm_led_on(void);
void alarm_led_off(void);
void output_led_on(void);
void output_led_off(void);

void update_led_content_8byte(
    ht16k33_dev_t* dev,
    uint16_t ch1,
    uint16_t ch2,
    uint16_t ch3,
    uint16_t ch4,
    uint16_t ch5,
    uint16_t ch6,
    uint16_t ch7,
    uint16_t ch8);

#ifdef __cplusplus
}
#endif

#endif // _FILE_USER_MAIN_H_
