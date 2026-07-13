/**
 * @file xplt.ctl_interface.h
 * @brief Platform callbacks connecting the PSU controller to ADC, DAC and GPIO peripherals.
 */

#ifndef _FILE_CTL_INTERFACE_H_
#define _FILE_CTL_INTERFACE_H_

#include <xplt.peripheral.h>
#include "ctl_main.h"

#ifdef __cplusplus
extern "C"
{
#endif

//=================================================================================================
// Controller interface

GMP_STATIC_INLINE uint16_t psu_dac_pu_to_code(ctrl_gt command_pu)
{
    parameter_gt command = ctrl2float(command_pu);

    if (command <= 0.0f)
    {
        return 0U;
    }

    if (command >= 1.0f)
    {
        return PSU_DAC_MAX_CODE;
    }

    return (uint16_t)(command * (parameter_gt)PSU_DAC_MAX_CODE + 0.5f);
}

// Apply a positive-domain linear calibration without creating an offset at ADC zero.
GMP_STATIC_INLINE ctrl_gt psu_calibrate_measurement(
    ctrl_gt sample,
    parameter_gt slope,
    parameter_gt bias)
{
    ctrl_gt calibrated;

    if (sample <= float2ctrl(0.0f))
    {
        return float2ctrl(0.0f);
    }

    calibrated =
        ctl_mul(sample, float2ctrl(slope)) +
        float2ctrl(bias);

    return (calibrated > float2ctrl(0.0f)) ?
        calibrated : float2ctrl(0.0f);
}

// Read ADC samples at the beginning of every control interrupt.
GMP_STATIC_INLINE void ctl_input_callback(void)
{
    adc_gt voltage_raw;
    adc_gt current_raw;
    ctrl_gt voltage_sample;
    ctrl_gt current_sample;

    voltage_raw = ADC_readResult(
        PSU_VFU_RESULT_BASE,
        PSU_VFU_SOC);

    current_raw = ADC_readResult(
        PSU_VFI_RESULT_BASE,
        PSU_VFI_SOC);

    voltage_sample =
        ctl_step_adc_channel(
            &psu_voltage_fb_adc,
            voltage_raw);

    current_sample =
        ctl_step_adc_channel(
            &psu_current_fb_adc,
            current_raw);

    psu_ctrl.voltage_meas_v =
        psu_calibrate_measurement(
            voltage_sample,
            PSU_VOLTAGE_MEAS_CAL_SLOPE,
            PSU_VOLTAGE_MEAS_CAL_BIAS_V);

    psu_ctrl.current_meas_a =
        psu_calibrate_measurement(
            current_sample,
            PSU_CURRENT_MEAS_CAL_SLOPE,
            PSU_CURRENT_MEAS_CAL_BIAS_A);
}

// Write the two analog references and the output switch command.
//
// The DAC references stay at zero during the 20 ms output delay.  After Vsw
// turns on, the controller slews DACA/DACB toward their calibrated targets.
// Manual shutdown or a fault still clears Vsw and both DACs immediately.
GMP_STATIC_INLINE void ctl_output_callback(void)
{
    fast_gt reference_enable;

    reference_enable =
        ((psu_ctrl.output_request != 0) &&
         (psu_ctrl.fault_latched == 0));

    if (reference_enable != 0)
    {
        // During startup these applied commands rise through the controller's
        // soft slew instead of jumping directly to the final references.
        DAC_setShadowValue(
            IRIS_DACA_BASE,
            psu_dac_pu_to_code(psu_ctrl.voltage_dac_pu));

        DAC_setShadowValue(
            IRIS_DACB_BASE,
            psu_dac_pu_to_code(psu_ctrl.current_dac_pu));

        // output_enable becomes 1 only after the existing 20 ms delay.
        psu_write_output_switch(
            psu_ctrl.output_enable);
    }
    else
    {
        // Disconnect the output first, then remove both references.
        psu_write_output_switch(0);

        DAC_setShadowValue(
            IRIS_DACA_BASE,
            0U);

        DAC_setShadowValue(
            IRIS_DACB_BASE,
            0U);
    }
}

// Compatibility interfaces retained for the GMP controller framework.
GMP_STATIC_INLINE void ctl_fast_enable_output(void)
{
    ctl_set_psu_output(&psu_ctrl, 1);
}

GMP_STATIC_INLINE void ctl_fast_disable_output(void)
{
    ctl_set_psu_output(&psu_ctrl, 0);
}

#ifdef __cplusplus
}
#endif

#endif // _FILE_CTL_INTERFACE_H_
