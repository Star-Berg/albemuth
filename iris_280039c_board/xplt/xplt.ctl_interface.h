//
// THIS IS A DEMO SOURCE CODE FOR GMP LIBRARY.
//
// User should add all declarations of controller objects in this file.
//
// User should implement the Main ISR of the controller tasks.
//
// User should ensure that all the controller codes here is platform-independent.
//
// WARNING: This file must be kept in the include search path during compilation.
//

#include <xplt.peripheral.h>
#include <ctl/component/interface/adc_channel.h>
#include <ctl/component/interface/pwm_channel.h>

#ifndef _FILE_CTL_INTERFACE_H_
#define _FILE_CTL_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//=================================================================================================
// Controller interface

extern ctrl_gt comp_out;
extern pwm_channel_t comp_pwm;

// Input Callback
GMP_STATIC_INLINE void ctl_input_callback(void)
{
    ctl_step_adc_channel(&input_wave_adc, ADC_readResult(INPUT_WAVE_RESULT_REG, INPUT_WAVE));
}

// Output Callback
GMP_STATIC_INLINE void ctl_output_callback(void)
{
    static uint32_t tick = 0;
    ctrl_gt dac_b_out;

    tick += 1;

    ctrl_gt output_signal = (tick % 200) / 200.0f;

    DAC_setShadowValue(IRIS_DACA_BASE, (uint16_t)(ctl_sin(output_signal) * 1024.0f + 2048.0f));

    dac_b_out = ctl_sat(comp_out, float2ctrl(1.0f), float2ctrl(-1.0f));
    DAC_setShadowValue(IRIS_DACB_BASE, (uint16_t)(dac_b_out * 1024.0f + 2048.0f));

    EPWM_setCounterCompareValue(IRIS_EPWM1_BASE, EPWM_COUNTER_COMPARE_A, comp_pwm.value);


}

// function prototype
void GPIO_WritePin(uint16_t gpioNumber, uint16_t outVal);

// Enable Motor Controller
// Enable Output
GMP_STATIC_INLINE void ctl_fast_enable_output()
{
    // Clear any Trip Zone flag
    EPWM_clearTripZoneFlag(PHASE_U_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_clearTripZoneFlag(PHASE_V_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_clearTripZoneFlag(PHASE_W_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

// Disable Output
GMP_STATIC_INLINE void ctl_fast_disable_output()
{
    // Disables the PWM device
    EPWM_forceTripZoneEvent(PHASE_U_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(PHASE_V_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(PHASE_W_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _FILE_CTL_INTERFACE_H_
