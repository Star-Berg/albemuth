/**
 * @file ctl_main.c
 * @brief Initialization for the programmable DC power-supply controller.
 */

#include <gmp_core.h>
#include <ctrl_settings.h>

#include "ctl_main.h"

#include <core/dev/pil_core.h>

//=================================================================================================
// Controller objects

ctl_psu_t psu_ctrl;
adc_channel_t psu_voltage_fb_adc;
adc_channel_t psu_current_fb_adc;

//=================================================================================================
// Module initialization

void ctl_init_psu(
    ctl_psu_t* hpsu,
    uint16_t voltage_set_decivolt,
    uint16_t current_set_milliamp)
{
    hpsu->voltage_to_dac_pu =
        float2ctrl(PSU_VOLTAGE_FB_RATIO / PSU_DAC_REF_V);

    hpsu->current_to_dac_pu =
        float2ctrl(
            PSU_CURRENT_SHUNT_OHM *
            PSU_CURRENT_AMP_GAIN /
            PSU_DAC_REF_V);

    hpsu->voltage_set_v = float2ctrl(0.0f);
    hpsu->current_set_a = float2ctrl(0.0f);
    hpsu->voltage_dac_pu = float2ctrl(0.0f);
    hpsu->current_dac_pu = float2ctrl(0.0f);
    hpsu->voltage_meas_v = float2ctrl(0.0f);
    hpsu->current_meas_a = float2ctrl(0.0f);

    hpsu->mode = PSU_MODE_CV;
    hpsu->mode_enter_cc_counter = 0U;
    hpsu->mode_exit_cc_counter = 0U;

    hpsu->overcurrent_counter = 0U;
    hpsu->fault_latched = 0;
    hpsu->fault_code = PSU_FAULT_NONE;
    hpsu->fault_current_a = float2ctrl(0.0f);
    hpsu->fault_voltage_v = float2ctrl(0.0f);

    hpsu->output_request = 0;
    hpsu->output_enable = 0;
    hpsu->output_delay_counter = 0U;

    hpsu->edit_target = PSU_EDIT_VOLTAGE;
    ctl_set_psu_voltage_setting_decivolt(hpsu, voltage_set_decivolt);
    ctl_set_psu_current_setting_milliamp(hpsu, current_set_milliamp);

    // Generate valid references while keeping the output relay open.
    ctl_step_psu(hpsu);
}

void ctl_init_psu_mode_detection(
    ctl_psu_t* hpsu,
    parameter_gt enter_current_margin_a,
    parameter_gt exit_current_margin_a,
    parameter_gt enter_voltage_drop_v,
    parameter_gt exit_voltage_drop_v,
    uint16_t detect_cycles)
{
    hpsu->mode_cc_enter_current_margin_a =
        float2ctrl(enter_current_margin_a);
    hpsu->mode_cc_exit_current_margin_a =
        float2ctrl(exit_current_margin_a);
    hpsu->mode_cc_enter_voltage_drop_v =
        float2ctrl(enter_voltage_drop_v);
    hpsu->mode_cc_exit_voltage_drop_v =
        float2ctrl(exit_voltage_drop_v);

    hpsu->mode_detect_cycles = (detect_cycles == 0U) ? 1U : detect_cycles;
    hpsu->mode_enter_cc_counter = 0U;
    hpsu->mode_exit_cc_counter = 0U;
    hpsu->mode = PSU_MODE_CV;
}

void ctl_init_psu_protection(
    ctl_psu_t* hpsu,
    parameter_gt overcurrent_trip_a,
    uint16_t trip_cycles)
{
    hpsu->overcurrent_trip_a = float2ctrl(overcurrent_trip_a);
    hpsu->overcurrent_trip_cycles = (trip_cycles == 0U) ? 1U : trip_cycles;
    hpsu->overcurrent_counter = 0U;
    hpsu->fault_latched = 0;
    hpsu->fault_code = PSU_FAULT_NONE;
    hpsu->fault_current_a = float2ctrl(0.0f);
    hpsu->fault_voltage_v = float2ctrl(0.0f);
}

//=================================================================================================
// GMP controller entry points

void ctl_init(void)
{
    ctl_init_adc_channel(
        &psu_voltage_fb_adc,
        float2ctrl(PSU_ADC_REF_V / PSU_VOLTAGE_FB_RATIO),
        float2ctrl(0.0f),
        PSU_ADC_RESOLUTION_BITS,
        PSU_ADC_IQN);

    ctl_init_adc_channel(
        &psu_current_fb_adc,
        float2ctrl(
            PSU_ADC_REF_V /
            (PSU_CURRENT_SHUNT_OHM * PSU_CURRENT_AMP_GAIN)),
        float2ctrl(0.0f),
        PSU_ADC_RESOLUTION_BITS,
        PSU_ADC_IQN);

    ctl_init_psu(
        &psu_ctrl,
        PSU_DEFAULT_VOLTAGE_SET_DV,
        PSU_DEFAULT_CURRENT_SET_MA);

    ctl_init_psu_mode_detection(
        &psu_ctrl,
        PSU_MODE_CC_ENTER_CURRENT_MARGIN_A,
        PSU_MODE_CC_EXIT_CURRENT_MARGIN_A,
        PSU_MODE_CC_ENTER_VOLTAGE_DROP_V,
        PSU_MODE_CC_EXIT_VOLTAGE_DROP_V,
        PSU_MODE_DETECT_CYCLES);

    ctl_init_psu_protection(
        &psu_ctrl,
        PSU_OVERCURRENT_TRIP_A,
        PSU_OVERCURRENT_TRIP_CYCLES);
}

void clear_all_controllers(void)
{
    ctl_set_psu_output(&psu_ctrl, 0);
    psu_ctrl.output_enable = 0;
    psu_ctrl.output_delay_counter = 0U;
    psu_ctrl.overcurrent_counter = 0U;
    psu_ctrl.mode_enter_cc_counter = 0U;
    psu_ctrl.mode_exit_cc_counter = 0U;
}

void ctl_mainloop(void)
{
    return;
}

void gmp_pil_sim_step(const gmp_sim_rx_buf_t* rx, gmp_sim_tx_buf_t* tx)
{
    GMP_UNUSED_VAR(rx);
    GMP_UNUSED_VAR(tx);
}
