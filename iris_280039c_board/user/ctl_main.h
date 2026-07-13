/**
 * @file ctl_main.h
 * @brief Controller and protection logic for the programmable DC power supply.
 */

#ifndef _FILE_CTL_MAIN_H_
#define _FILE_CTL_MAIN_H_

#include <xplt.peripheral.h>
#include <ctl/component/interface/adc_channel.h>

#ifdef __cplusplus
extern "C"
{
#endif

//=================================================================================================
// Power-supply states

typedef enum _tag_psu_mode_t
{
    PSU_MODE_CV = 0,     //!< Voltage loop is controlling the output.
    PSU_MODE_CC = 1      //!< Current loop is controlling the output.
} psu_mode_t;

typedef enum _tag_psu_edit_target_t
{
    PSU_EDIT_VOLTAGE = 0,
    PSU_EDIT_CURRENT = 1
} psu_edit_target_t;

typedef enum _tag_psu_fault_code_t
{
    PSU_FAULT_NONE = 0,
    PSU_FAULT_OVERCURRENT = 1
} psu_fault_code_t;

//=================================================================================================
// Main controller object

typedef struct _tag_ctl_psu_t
{
    // Canonical user settings. These 16-bit values are shared with background UI tasks.
    volatile uint16_t voltage_set_decivolt;   //!< 0 to 100 means 0.0 to 10.0 V.
    volatile uint16_t current_set_milliamp;   //!< 0 to 100 means 0 to 100 mA.
    volatile psu_edit_target_t edit_target;

    // Physical settings, updated only by the real-time controller.
    ctrl_gt voltage_set_v;
    ctrl_gt current_set_a;

    // DAC commands and conversion coefficients.
    ctrl_gt voltage_dac_pu;
    ctrl_gt current_dac_pu;
    ctrl_gt voltage_to_dac_pu;
    ctrl_gt current_to_dac_pu;

    // Measurements.
    ctrl_gt voltage_meas_v;
    ctrl_gt current_meas_a;

    // Automatically detected active analog loop.
    volatile psu_mode_t mode;
    ctrl_gt mode_cc_enter_current_margin_a;
    ctrl_gt mode_cc_exit_current_margin_a;
    ctrl_gt mode_cc_enter_voltage_drop_v;
    ctrl_gt mode_cc_exit_voltage_drop_v;
    uint16_t mode_detect_cycles;
    uint16_t mode_enter_cc_counter;
    uint16_t mode_exit_cc_counter;

    // Independent software over-current protection.
    ctrl_gt overcurrent_trip_a;
    uint16_t overcurrent_trip_cycles;
    uint16_t overcurrent_counter;

    // Latched fault information.
    volatile fast_gt fault_latched;
    volatile psu_fault_code_t fault_code;
    ctrl_gt fault_current_a;
    ctrl_gt fault_voltage_v;

    // Output relay control.
    volatile fast_gt output_request;
    volatile fast_gt output_enable;
    uint32_t output_delay_counter;
} ctl_psu_t;

//=================================================================================================
// Controller objects

extern ctl_psu_t psu_ctrl;
extern adc_channel_t psu_voltage_fb_adc;
extern adc_channel_t psu_current_fb_adc;

//=================================================================================================
// Initialization and background entry points

void ctl_init(void);
void ctl_mainloop(void);
void clear_all_controllers(void);

void ctl_init_psu(
    ctl_psu_t* hpsu,
    uint16_t voltage_set_decivolt,
    uint16_t current_set_milliamp);

void ctl_init_psu_mode_detection(
    ctl_psu_t* hpsu,
    parameter_gt enter_current_margin_a,
    parameter_gt exit_current_margin_a,
    parameter_gt enter_voltage_drop_v,
    parameter_gt exit_voltage_drop_v,
    uint16_t detect_cycles);

void ctl_init_psu_protection(
    ctl_psu_t* hpsu,
    parameter_gt overcurrent_trip_a,
    uint16_t trip_cycles);

//=================================================================================================
// Setting interfaces

GMP_STATIC_INLINE void ctl_set_psu_voltage_setting_decivolt(
    ctl_psu_t* hpsu,
    uint16_t voltage_decivolt)
{
    if (voltage_decivolt > PSU_VOLTAGE_SET_MAX_DV)
    {
        voltage_decivolt = PSU_VOLTAGE_SET_MAX_DV;
    }

    hpsu->voltage_set_decivolt = voltage_decivolt;
}

GMP_STATIC_INLINE uint16_t ctl_get_psu_voltage_setting_decivolt(
    const ctl_psu_t* hpsu)
{
    return hpsu->voltage_set_decivolt;
}

GMP_STATIC_INLINE void ctl_set_psu_current_setting_milliamp(
    ctl_psu_t* hpsu,
    uint16_t current_milliamp)
{
    if (current_milliamp > PSU_CURRENT_SET_MAX_MA)
    {
        current_milliamp = PSU_CURRENT_SET_MAX_MA;
    }

    hpsu->current_set_milliamp = current_milliamp;
}

GMP_STATIC_INLINE uint16_t ctl_get_psu_current_setting_milliamp(
    const ctl_psu_t* hpsu)
{
    return hpsu->current_set_milliamp;
}

GMP_STATIC_INLINE void ctl_set_psu_edit_target(
    ctl_psu_t* hpsu,
    psu_edit_target_t target)
{
    hpsu->edit_target =
        (target == PSU_EDIT_CURRENT) ? PSU_EDIT_CURRENT : PSU_EDIT_VOLTAGE;
}

GMP_STATIC_INLINE psu_edit_target_t ctl_get_psu_edit_target(
    const ctl_psu_t* hpsu)
{
    return hpsu->edit_target;
}

GMP_STATIC_INLINE void ctl_toggle_psu_edit_target(ctl_psu_t* hpsu)
{
    ctl_set_psu_edit_target(
        hpsu,
        (hpsu->edit_target == PSU_EDIT_VOLTAGE) ?
            PSU_EDIT_CURRENT : PSU_EDIT_VOLTAGE);
}

//=================================================================================================
// Output and fault interfaces

GMP_STATIC_INLINE void ctl_set_psu_output(
    ctl_psu_t* hpsu,
    fast_gt enable)
{
    if (enable == 0)
    {
        hpsu->output_request = 0;
        return;
    }

    if (hpsu->fault_latched == 0)
    {
        hpsu->output_request = 1;
    }
}

GMP_STATIC_INLINE fast_gt ctl_get_psu_output_request(
    const ctl_psu_t* hpsu)
{
    return hpsu->output_request;
}

GMP_STATIC_INLINE fast_gt ctl_get_psu_output_state(
    const ctl_psu_t* hpsu)
{
    return hpsu->output_enable;
}

GMP_STATIC_INLINE fast_gt ctl_get_psu_fault_state(
    const ctl_psu_t* hpsu)
{
    return hpsu->fault_latched;
}

GMP_STATIC_INLINE psu_fault_code_t ctl_get_psu_fault_code(
    const ctl_psu_t* hpsu)
{
    return hpsu->fault_code;
}

GMP_STATIC_INLINE ctrl_gt ctl_get_psu_fault_current(
    const ctl_psu_t* hpsu)
{
    return hpsu->fault_current_a;
}

GMP_STATIC_INLINE ctrl_gt ctl_get_psu_fault_voltage(
    const ctl_psu_t* hpsu)
{
    return hpsu->fault_voltage_v;
}

GMP_STATIC_INLINE fast_gt ctl_clear_psu_fault(ctl_psu_t* hpsu)
{
    if ((hpsu->output_request != 0) ||
        (hpsu->output_enable != 0))
    {
        return 0;
    }

    hpsu->fault_latched = 0;
    hpsu->fault_code = PSU_FAULT_NONE;
    hpsu->fault_current_a = float2ctrl(0.0f);
    hpsu->fault_voltage_v = float2ctrl(0.0f);
    hpsu->overcurrent_counter = 0U;
    hpsu->mode = PSU_MODE_CV;
    hpsu->mode_enter_cc_counter = 0U;
    hpsu->mode_exit_cc_counter = 0U;

    return 1;
}

//=================================================================================================
// Measurement and status interfaces

GMP_STATIC_INLINE psu_mode_t ctl_get_psu_mode(const ctl_psu_t* hpsu)
{
    return hpsu->mode;
}

GMP_STATIC_INLINE ctrl_gt ctl_get_psu_voltage_measurement(
    const ctl_psu_t* hpsu)
{
    return hpsu->voltage_meas_v;
}

GMP_STATIC_INLINE ctrl_gt ctl_get_psu_current_measurement(
    const ctl_psu_t* hpsu)
{
    return hpsu->current_meas_a;
}

//=================================================================================================
// Real-time control and protection steps

GMP_STATIC_INLINE void ctl_step_psu_overcurrent_protection(
    ctl_psu_t* hpsu)
{
    if (hpsu->fault_latched != 0)
    {
        hpsu->output_request = 0;
        hpsu->output_enable = 0;
        hpsu->output_delay_counter = 0U;
        hpsu->voltage_dac_pu = float2ctrl(0.0f);
        hpsu->current_dac_pu = float2ctrl(0.0f);
        return;
    }

    if (hpsu->output_enable == 0)
    {
        hpsu->overcurrent_counter = 0U;
        return;
    }

    if (hpsu->current_meas_a >= hpsu->overcurrent_trip_a)
    {
        if (hpsu->overcurrent_counter < hpsu->overcurrent_trip_cycles)
        {
            hpsu->overcurrent_counter++;
        }
    }
    else
    {
        hpsu->overcurrent_counter = 0U;
    }

    if (hpsu->overcurrent_counter >= hpsu->overcurrent_trip_cycles)
    {
        hpsu->fault_current_a = hpsu->current_meas_a;
        hpsu->fault_voltage_v = hpsu->voltage_meas_v;
        hpsu->fault_code = PSU_FAULT_OVERCURRENT;
        hpsu->fault_latched = 1;

        hpsu->output_request = 0;
        hpsu->output_enable = 0;
        hpsu->output_delay_counter = 0U;
        hpsu->voltage_dac_pu = float2ctrl(0.0f);
        hpsu->current_dac_pu = float2ctrl(0.0f);
    }
}

GMP_STATIC_INLINE void ctl_step_psu_mode_detection(ctl_psu_t* hpsu)
{
    ctrl_gt enter_current_threshold;
    ctrl_gt exit_current_threshold;
    ctrl_gt enter_voltage_threshold;
    ctrl_gt exit_voltage_threshold;

    if ((hpsu->output_enable == 0) ||
        (hpsu->fault_latched != 0) ||
        (hpsu->voltage_set_decivolt == 0U) ||
        (hpsu->current_set_milliamp == 0U))
    {
        hpsu->mode = PSU_MODE_CV;
        hpsu->mode_enter_cc_counter = 0U;
        hpsu->mode_exit_cc_counter = 0U;
        return;
    }

    enter_current_threshold =
        hpsu->current_set_a - hpsu->mode_cc_enter_current_margin_a;
    exit_current_threshold =
        hpsu->current_set_a - hpsu->mode_cc_exit_current_margin_a;
    enter_voltage_threshold =
        hpsu->voltage_set_v - hpsu->mode_cc_enter_voltage_drop_v;
    exit_voltage_threshold =
        hpsu->voltage_set_v - hpsu->mode_cc_exit_voltage_drop_v;

    if (enter_current_threshold < float2ctrl(0.0f))
    {
        enter_current_threshold = float2ctrl(0.0f);
    }
    if (exit_current_threshold < float2ctrl(0.0f))
    {
        exit_current_threshold = float2ctrl(0.0f);
    }
    if (enter_voltage_threshold < float2ctrl(0.0f))
    {
        enter_voltage_threshold = float2ctrl(0.0f);
    }
    if (exit_voltage_threshold < float2ctrl(0.0f))
    {
        exit_voltage_threshold = float2ctrl(0.0f);
    }

    if (hpsu->mode == PSU_MODE_CV)
    {
        if ((hpsu->current_meas_a >= enter_current_threshold) &&
            (hpsu->voltage_meas_v <= enter_voltage_threshold))
        {
            if (hpsu->mode_enter_cc_counter < hpsu->mode_detect_cycles)
            {
                hpsu->mode_enter_cc_counter++;
            }
        }
        else
        {
            hpsu->mode_enter_cc_counter = 0U;
        }

        if (hpsu->mode_enter_cc_counter >= hpsu->mode_detect_cycles)
        {
            hpsu->mode = PSU_MODE_CC;
            hpsu->mode_enter_cc_counter = 0U;
            hpsu->mode_exit_cc_counter = 0U;
        }
    }
    else
    {
        if ((hpsu->voltage_meas_v >= exit_voltage_threshold) ||
            (hpsu->current_meas_a <= exit_current_threshold))
        {
            if (hpsu->mode_exit_cc_counter < hpsu->mode_detect_cycles)
            {
                hpsu->mode_exit_cc_counter++;
            }
        }
        else
        {
            hpsu->mode_exit_cc_counter = 0U;
        }

        if (hpsu->mode_exit_cc_counter >= hpsu->mode_detect_cycles)
        {
            hpsu->mode = PSU_MODE_CV;
            hpsu->mode_enter_cc_counter = 0U;
            hpsu->mode_exit_cc_counter = 0U;
        }
    }
}

GMP_STATIC_INLINE void ctl_step_psu_output(ctl_psu_t* hpsu)
{
    if (hpsu->fault_latched != 0)
    {
        hpsu->output_request = 0;
        hpsu->output_enable = 0;
        hpsu->output_delay_counter = 0U;
        return;
    }

    if (hpsu->output_request == 0)
    {
        hpsu->output_enable = 0;
        hpsu->output_delay_counter = 0U;
        return;
    }

    if (hpsu->output_enable != 0)
    {
        return;
    }

    if (hpsu->output_delay_counter < PSU_OUTPUT_ON_DELAY_CYCLES)
    {
        hpsu->output_delay_counter++;
    }

    if (hpsu->output_delay_counter >= PSU_OUTPUT_ON_DELAY_CYCLES)
    {
        hpsu->output_delay_counter = PSU_OUTPUT_ON_DELAY_CYCLES;
        hpsu->output_enable = 1;
    }
}

GMP_STATIC_INLINE void ctl_step_psu(ctl_psu_t* hpsu)
{
    ctrl_gt calibrated_voltage_set_v;

    // Convert the atomic integer UI settings into real-time physical values.
    hpsu->voltage_set_v =
        float2ctrl((parameter_gt)hpsu->voltage_set_decivolt * 0.1f);
    hpsu->current_set_a =
        float2ctrl((parameter_gt)hpsu->current_set_milliamp * 0.001f);

    // Compensate the measured set-to-output voltage gain and offset before driving DACA.
    calibrated_voltage_set_v =
        ctl_mul(
            hpsu->voltage_set_v,
            float2ctrl(PSU_VOLTAGE_OUTPUT_CAL_SLOPE)) +
        float2ctrl(PSU_VOLTAGE_OUTPUT_CAL_BIAS_V);

    // Both references remain active. The analog minimum-selector performs CV/CC transition.
    hpsu->voltage_dac_pu =
        ctl_sat(
            ctl_mul(calibrated_voltage_set_v, hpsu->voltage_to_dac_pu),
            float2ctrl(1.0f),
            float2ctrl(0.0f));

    hpsu->current_dac_pu =
        ctl_sat(
            ctl_mul(hpsu->current_set_a, hpsu->current_to_dac_pu),
            float2ctrl(1.0f),
            float2ctrl(0.0f));

    ctl_step_psu_overcurrent_protection(hpsu);
    ctl_step_psu_mode_detection(hpsu);
    ctl_step_psu_output(hpsu);
}

// Periodic controller callback called by the GMP control framework.
GMP_STATIC_INLINE void ctl_dispatch(void)
{
    ctl_step_psu(&psu_ctrl);
}

#ifdef __cplusplus
}
#endif

#endif // _FILE_CTL_MAIN_H_
