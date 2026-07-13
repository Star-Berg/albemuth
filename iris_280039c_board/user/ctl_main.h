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
    PSU_MODE_CV = 0,     //!< Voltage loop is currently controlling the output.
    PSU_MODE_CC = 1      //!< Current loop is currently controlling the output.
} psu_mode_t;

typedef enum _tag_psu_operating_mode_t
{
    PSU_OPERATING_MODE_CV = 0,    //!< Fixed CV mode; entering CC is a fault.
    PSU_OPERATING_MODE_CC = 1,    //!< Fixed CC mode; entering CV is a fault.
    PSU_OPERATING_MODE_AUTO = 2   //!< Automatic CV/CC transition; no mode alarm.
} psu_operating_mode_t;

typedef enum _tag_psu_edit_target_t
{
    PSU_EDIT_VOLTAGE = 0,
    PSU_EDIT_CURRENT = 1
} psu_edit_target_t;

typedef enum _tag_psu_fault_code_t
{
    PSU_FAULT_NONE = 0,
    PSU_FAULT_HARD_OVERCURRENT = 1,
    PSU_FAULT_CV_OVERCURRENT = 2,
    PSU_FAULT_CC_OVERVOLTAGE = 3
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
    ctrl_gt voltage_dac_slew_pu;
    ctrl_gt current_dac_slew_pu;

    // Measurements.
    ctrl_gt voltage_meas_v;
    ctrl_gt current_meas_a;

    // User-selected operating mode and automatically detected active analog loop.
    volatile psu_operating_mode_t operating_mode;
    volatile psu_mode_t mode;
    ctrl_gt mode_cc_enter_current_margin_a;
    ctrl_gt mode_cc_exit_current_margin_a;
    ctrl_gt mode_cc_enter_voltage_drop_v;
    ctrl_gt mode_cc_exit_voltage_drop_v;
    uint16_t mode_detect_cycles;
    uint16_t mode_enter_cc_counter;
    uint16_t mode_exit_cc_counter;

    // Fixed-mode limit protection. AUTO mode does not use this counter.
    uint16_t mode_limit_trip_cycles;
    uint16_t mode_violation_counter;
    uint16_t cc_acquire_timeout_cycles;
    uint16_t cc_acquire_counter;
    fast_gt cc_mode_armed;

    // Independent hard over-current protection.
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

GMP_STATIC_INLINE psu_operating_mode_t ctl_get_psu_operating_mode(
    const ctl_psu_t* hpsu)
{
    return hpsu->operating_mode;
}

GMP_STATIC_INLINE fast_gt ctl_set_psu_operating_mode(
    ctl_psu_t* hpsu,
    psu_operating_mode_t operating_mode)
{
    // Changing regulation policy while the output is live is intentionally blocked.
    if ((hpsu->output_request != 0) ||
        (hpsu->output_enable != 0))
    {
        return 0;
    }

    if (operating_mode > PSU_OPERATING_MODE_AUTO)
    {
        operating_mode = PSU_OPERATING_MODE_AUTO;
    }

    hpsu->operating_mode = operating_mode;
    hpsu->mode = PSU_MODE_CV;
    hpsu->mode_enter_cc_counter = 0U;
    hpsu->mode_exit_cc_counter = 0U;
    hpsu->mode_violation_counter = 0U;
    hpsu->cc_acquire_counter = 0U;
    hpsu->cc_mode_armed = 0;

    return 1;
}

GMP_STATIC_INLINE fast_gt ctl_cycle_psu_operating_mode(ctl_psu_t* hpsu)
{
    psu_operating_mode_t next_mode;

    if (hpsu->operating_mode == PSU_OPERATING_MODE_CV)
    {
        next_mode = PSU_OPERATING_MODE_CC;
    }
    else if (hpsu->operating_mode == PSU_OPERATING_MODE_CC)
    {
        next_mode = PSU_OPERATING_MODE_AUTO;
    }
    else
    {
        next_mode = PSU_OPERATING_MODE_CV;
    }

    return ctl_set_psu_operating_mode(hpsu, next_mode);
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
    hpsu->mode_violation_counter = 0U;
    hpsu->cc_acquire_counter = 0U;
    hpsu->cc_mode_armed = 0;

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

GMP_STATIC_INLINE ctrl_gt ctl_slew_psu_command(
    ctrl_gt current,
    ctrl_gt target,
    ctrl_gt increment)
{
    if (increment <= float2ctrl(0.0f))
    {
        return target;
    }

    if (current < target)
    {
        return ((target - current) <= increment) ?
            target : (current + increment);
    }

    return ((current - target) <= increment) ?
        target : (current - increment);
}

GMP_STATIC_INLINE void ctl_trip_psu_fault(
    ctl_psu_t* hpsu,
    psu_fault_code_t fault_code)
{
    hpsu->fault_current_a = hpsu->current_meas_a;
    hpsu->fault_voltage_v = hpsu->voltage_meas_v;
    hpsu->fault_code = fault_code;
    hpsu->fault_latched = 1;

    hpsu->output_request = 0;
    hpsu->output_enable = 0;
    hpsu->output_delay_counter = 0U;
    hpsu->voltage_dac_pu = float2ctrl(0.0f);
    hpsu->current_dac_pu = float2ctrl(0.0f);
    hpsu->mode_violation_counter = 0U;
    hpsu->cc_acquire_counter = 0U;
    hpsu->cc_mode_armed = 0;
}

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
        ctl_trip_psu_fault(hpsu, PSU_FAULT_HARD_OVERCURRENT);
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

GMP_STATIC_INLINE void ctl_step_psu_mode_limit_protection(ctl_psu_t* hpsu)
{
    fast_gt mode_violation = 0;
    psu_fault_code_t fault_code = PSU_FAULT_NONE;

    if ((hpsu->fault_latched != 0) ||
        (hpsu->output_request == 0) ||
        (hpsu->output_enable == 0) ||
        (hpsu->voltage_set_decivolt == 0U) ||
        (hpsu->current_set_milliamp == 0U) ||
        (hpsu->operating_mode == PSU_OPERATING_MODE_AUTO))
    {
        hpsu->mode_violation_counter = 0U;
        hpsu->cc_acquire_counter = 0U;
        hpsu->cc_mode_armed = 0;
        return;
    }

    // The active-loop detector starts in CV.  In fixed CC mode, observe the
    // complete startup interval before arming mode-limit protection; otherwise
    // an early CC indication followed by a startup spike can cause a false trip.
    if ((hpsu->operating_mode == PSU_OPERATING_MODE_CC) &&
        (hpsu->cc_mode_armed == 0))
    {
        if (hpsu->cc_acquire_counter < hpsu->cc_acquire_timeout_cycles)
        {
            hpsu->cc_acquire_counter++;
            hpsu->mode_violation_counter = 0U;
            return;
        }

        if (hpsu->mode == PSU_MODE_CC)
        {
            hpsu->cc_mode_armed = 1;
            hpsu->cc_acquire_counter = 0U;
            hpsu->mode_violation_counter = 0U;
            return;
        }
    }

    // Fixed CV and an already-established fixed CC do not use acquisition state.
    if ((hpsu->operating_mode == PSU_OPERATING_MODE_CV) ||
        (hpsu->cc_mode_armed != 0))
    {
        hpsu->cc_acquire_counter = 0U;
    }

    // The analog minimum-selector remains active in every operating mode.
    // In fixed CV/CC modes, a loop handover is treated as a protection event.
    if ((hpsu->operating_mode == PSU_OPERATING_MODE_CV) &&
        (hpsu->mode == PSU_MODE_CC))
    {
        mode_violation = 1;
        fault_code = PSU_FAULT_CV_OVERCURRENT;
    }
    else if ((hpsu->operating_mode == PSU_OPERATING_MODE_CC) &&
             (hpsu->mode == PSU_MODE_CV))
    {
        mode_violation = 1;
        fault_code = PSU_FAULT_CC_OVERVOLTAGE;
    }

    if (mode_violation != 0)
    {
        if (hpsu->mode_violation_counter < hpsu->mode_limit_trip_cycles)
        {
            hpsu->mode_violation_counter++;
        }
    }
    else
    {
        hpsu->mode_violation_counter = 0U;
    }

    if (hpsu->mode_violation_counter >= hpsu->mode_limit_trip_cycles)
    {
        ctl_trip_psu_fault(hpsu, fault_code);
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
        hpsu->voltage_dac_pu = float2ctrl(0.0f);
        hpsu->current_dac_pu = float2ctrl(0.0f);
        hpsu->cc_acquire_counter = 0U;
        hpsu->cc_mode_armed = 0;
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
    ctrl_gt voltage_command_v;
    ctrl_gt current_command_a;
    ctrl_gt voltage_target_pu;
    ctrl_gt current_target_pu;

    // Convert the atomic integer UI settings into real-time physical values.
    hpsu->voltage_set_v =
        float2ctrl((parameter_gt)hpsu->voltage_set_decivolt * 0.1f);
    hpsu->current_set_a =
        float2ctrl((parameter_gt)hpsu->current_set_milliamp * 0.001f);

    // Correct only the physical DAC commands. ADC calibration remains an
    // independent measurement concern. Negative low-end commands clamp to 0.
    if (hpsu->voltage_set_decivolt == 0U)
    {
        voltage_command_v = float2ctrl(0.0f);
    }
    else
    {
        voltage_command_v =
            ctl_mul(
                hpsu->voltage_set_v,
                float2ctrl(PSU_VOLTAGE_OUTPUT_CAL_SLOPE)) +
            float2ctrl(PSU_VOLTAGE_OUTPUT_CAL_BIAS_V);
    }

    if (hpsu->current_set_milliamp == 0U)
    {
        current_command_a = float2ctrl(0.0f);
    }
    else
    {
        current_command_a =
            ctl_mul(
                hpsu->current_set_a,
                float2ctrl(PSU_CURRENT_OUTPUT_CAL_SLOPE)) +
            float2ctrl(PSU_CURRENT_OUTPUT_CAL_BIAS_A);
    }

    voltage_target_pu =
        ctl_sat(
            ctl_mul(voltage_command_v, hpsu->voltage_to_dac_pu),
            float2ctrl(1.0f),
            float2ctrl(0.0f));

    // The current reference remains independent of the voltage setting.
    current_target_pu =
        ctl_sat(
            ctl_mul(current_command_a, hpsu->current_to_dac_pu),
            float2ctrl(1.0f),
            float2ctrl(0.0f));

    ctl_step_psu_overcurrent_protection(hpsu);
    ctl_step_psu_mode_detection(hpsu);
    ctl_step_psu_mode_limit_protection(hpsu);
    ctl_step_psu_output(hpsu);

    // Keep both DACs at zero while OFF and during the relay delay.  Once Vsw
    // is enabled, approach the calibrated targets gradually.  This preserves
    // fast fault shutdown while avoiding a full-reference startup step.
    if ((hpsu->output_enable != 0) &&
        (hpsu->fault_latched == 0))
    {
        hpsu->voltage_dac_pu =
            ctl_slew_psu_command(
                hpsu->voltage_dac_pu,
                voltage_target_pu,
                hpsu->voltage_dac_slew_pu);

        hpsu->current_dac_pu =
            ctl_slew_psu_command(
                hpsu->current_dac_pu,
                current_target_pu,
                hpsu->current_dac_slew_pu);
    }
    else
    {
        hpsu->voltage_dac_pu = float2ctrl(0.0f);
        hpsu->current_dac_pu = float2ctrl(0.0f);
    }
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
