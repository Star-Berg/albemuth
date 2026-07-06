/**
 * @file ctl_main.cpp
 * @author Javnson (javnson@zju.edu.cn)
 * @brief
 * @version 0.1
 * @date 2024-09-30
 *
 * @copyright Copyright GMP(c) 2024
 *
 */

#include <xplt.peripheral.h>

//=================================================================================================
// include Necessary control modules

#include <ctl/component/interface/adc_channel.h>
#include <ctl/component/interface/pwm_channel.h>
#include <ctl/component/interface/spwm_modulator.h>

#include <ctl/framework/cia402_state_machine.h>

#include <ctl/component/intrinsic/discrete/lead_lag.h>

#ifndef _FILE_CTL_MAIN_H_
#define _FILE_CTL_MAIN_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//=================================================================================================
// controller modules with extern




//=================================================================================================
// function prototype

void ctl_init(void);
void ctl_mainloop(void);

void clear_all_controllers();

//=================================================================================================
// controller process


extern ctl_lead_t lead_comp;
extern ctrl_gt comp_out;
extern ctrl_gt pwm_duty;
extern pwm_channel_t comp_pwm;

// periodic callback function things.
GMP_STATIC_INLINE void ctl_dispatch(void)
{
    comp_out = ctl_step_lead(&lead_comp, input_wave_adc.control_port.value);
    pwm_duty = ctl_sat(float2ctrl(0.5f) * comp_out + float2ctrl(0.5f), float2ctrl(1.0f), float2ctrl(0.0f));
    ctl_step_pwm_channel(&comp_pwm, pwm_duty);
}

#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _FILE_CTL_MAIN_H_
