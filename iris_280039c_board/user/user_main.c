// GMP basic core header
#include <gmp_core.h>

#include "ctl_main.h"
#include "psu_persistence.h"
#include "user_main.h"

#include <core/dev/mem_presp.h>
#include <core/dev/pil_core.h>
#include <core/dev/tunable.h>

#include <oled_driver.h>

//=================================================================================================
// Datalink protocol online-debug module retained from the board template

ctrl_gt kp, ki, kd;
ctrl_gt ram_range[512];

gmp_datalink_t dl;
gmp_pil_sim_t pil;

const gmp_param_item_t dict_m1[] = {
    {&kp, GMP_PARAM_TYPE_F32, GMP_PARAM_PERM_RO},
    {&ki, GMP_PARAM_TYPE_F32, GMP_PARAM_PERM_RW},
    {&kd, GMP_PARAM_TYPE_F32, GMP_PARAM_PERM_RW},
};
const uint16_t var_tunable_count = sizeof(dict_m1) / sizeof(dict_m1[0]);
gmp_param_tunable_t tunable;

const gmp_mem_region_t mem_regions[] = {
    {.base_addr = &ram_range,
     .byte_length = sizeof(ram_range) * GMP_PORT_DATA_SIZE_PER_BYTES,
     .perm = GMP_MEM_PERM_RW},
};
const uint16_t mem_regions_count = sizeof(mem_regions) / sizeof(mem_regions[0]);
gmp_mem_persp_t mem_persp_server;

gmp_task_status_t tsk_dl_debug_device(gmp_task_t* tsk)
{
    GMP_UNUSED_VAR(tsk);

    flush_dl_rx_buffer();

#ifndef SPECIFY_PC_ENVIRONMENT
    {
        gmp_dl_event_t e = gmp_dev_dl_loop_cb(&dl);

        switch (e)
        {
        case GMP_DL_EVENT_TX_RDY:
            flush_dl_tx_buffer();
            gmp_dev_dl_tx_state_done(&dl);
            break;

        case GMP_DL_EVENT_RX_OK:
            if (gmp_pil_sim_rx_cb(&pil))
            {
                break;
            }

            if (gmp_param_tunable_rx_cb(&tunable))
            {
                break;
            }

            if (gmp_mem_persp_rx_cb(&mem_persp_server))
            {
                break;
            }

            if (dl.rx_head.cmd == 0x99)
            {
                gmp_dev_dl_tx_request(
                    &dl,
                    dl.rx_head.seq_id,
                    GMP_DL_CMD_ECHO,
                    dl.expected_payload_len,
                    dl.payload_buf);

                gmp_dev_dl_msg_handled(&dl);
                break;
            }

            gmp_dev_dl_default_rx_handler(&dl);
            break;

        default:
            break;
        }
    }
#endif

    return GMP_TASK_DONE;
}

//=================================================================================================
// Task manager

volatile uint16_t flag_init_cmpt = 0U;
gmp_scheduler_t sched;

typedef enum _tag_psu_task_index_t
{
    PSU_TASK_DL = 0,
    PSU_TASK_KEY,
    PSU_TASK_DISPLAY,
    PSU_TASK_LED_FLUSH,
    PSU_TASK_STARTUP,
    PSU_TASK_ALARM,
    PSU_TASK_ENCODER,
    PSU_TASK_PERSISTENCE,
    PSU_TASK_COUNT
} psu_task_index_t;

// All tasks must be non-blocking.
gmp_task_t tasks[] = {
    {"dl_online",   tsk_dl_debug_device, 2,                            0,  1, NULL},
    {"flush_key",   tsk_key_flush,      PSU_KEY_TASK_PERIOD_MS,       10, 0, (void*)&ht16k33},
    {"psu_display", tsk_psu_display,    PSU_DISPLAY_TASK_PERIOD_MS,   30, 0, (void*)&ht16k33},
    {"flush_led",   tsk_LED_flush,      PSU_LED_FLUSH_TASK_PERIOD_MS, 20, 0, (void*)&ht16k33},
    {"startup",     tsk_startup,        250,                          0,  1, NULL},
    {"psu_alarm",   tsk_psu_alarm,      PSU_ALARM_TASK_PERIOD_MS,     50, 1, NULL},
    {"psu_encoder", tsk_psu_encoder,    PSU_ENCODER_TASK_PERIOD_MS,   40, 1, NULL},
    {"psu_save",    tsk_psu_persistence, 100,                        60, 1, NULL},
};

//=================================================================================================
// Initialization

GMP_NO_OPT_PREFIX
void init(void) GMP_NO_OPT_SUFFIX
{
    int i;

    gmp_scheduler_init(&sched);

    for (i = 0; i < (int)(sizeof(tasks) / sizeof(gmp_task_t)); ++i)
    {
        gmp_scheduler_add_task(&sched, &tasks[i]);
    }

    gmp_dev_dl_init(&dl);
    gmp_pil_sim_init(&pil, &dl, 0x10);
    gmp_param_tunable_init(&tunable, &dl, 0x30, dict_m1, var_tunable_count);
    gmp_mem_persp_init(&mem_persp_server, &dl, 0x50, mem_regions, mem_regions_count);
}

// Initialization task after board peripherals and controller objects are ready.
gmp_task_status_t tsk_startup(gmp_task_t* tsk)
{
    static uint16_t beep_counter = 0U;

    if (beep_counter == 0U)
    {
        beep_on();
    }
    else if (beep_counter == 1U)
    {
        beep_off();
    }
    else if (beep_counter == 2U)
    {
        beep_on();
    }
    else if (beep_counter == 3U)
    {
        beep_off();
    }

    beep_counter++;

    if (beep_counter >= 4U)
    {
        ht16k33_init_t ht16k33_init_struct = {
            .brightness = 15,
            .blink_rate = 0,
            .int_enable = 0,
            .int_act_high = 0};

        ec_gt ec = ht16k33_init(
            &ht16k33,
            iic_bus,
            HT16K33_DEFAULT_DEV_ADDR,
            &ht16k33_init_struct);

        oled_init();
        oled_clear();

        psu_persistence_init(&psu_ctrl);

        flag_init_cmpt = 1U;

        // OLED display can run even if the HT16K33 board is absent.
        sched.task_list[PSU_TASK_DISPLAY]->is_enabled = 1;

        if (ec == GMP_EC_OK)
        {
            sched.task_list[PSU_TASK_KEY]->is_enabled = 1;
            sched.task_list[PSU_TASK_LED_FLUSH]->is_enabled = 1;
        }
        else
        {
            gmp_base_print("HT16K33 init failed; keypad/7-seg disabled.\r\n");
        }

        tsk->is_enabled = 0;
    }

    return GMP_TASK_DONE;
}

//=================================================================================================
// Endless loop

GMP_NO_OPT_PREFIX
void mainloop(void) GMP_NO_OPT_SUFFIX
{
    gmp_scheduler_dispatch(&sched);
}
