
//
// THIS IS A DEMO SOURCE CODE FOR GMP LIBRARY.
//
// User should define your own controller objects,
// and initilize them.
//
// User should implement a ctl loop function, this
// function would be called every main loop.
//
// User should implement a state machine if you are using
// Controller Nanon framework.
//

#include <gmp_core.h>

#include <ctrl_settings.h>

#include "ctl_main.h"

#include <xplt.peripheral.h>

#include <core/pm/function_scheduler.h>

#include <core/dev/pil_core.h>


ctl_lead_t lead_comp;
ctrl_gt comp_out;
ctrl_gt pwm_duty;
pwm_channel_t comp_pwm;

//=================================================================================================
// global controller variables



//=================================================================================================
// CTL initialize routine

void ctl_init()
{
    ctl_init_lead_form3(&lead_comp, 3.14159265f / 4.0f, 100.0f, CONTROLLER_FREQUENCY);
    ctl_init_pwm_channel(&comp_pwm, 0, CTRL_PWM_CMP_MAX);
}

//=================================================================================================
// CTL endless loop routine

void ctl_mainloop(void)
{
    return;
}


void gmp_pil_sim_step(const gmp_sim_rx_buf_t* rx, gmp_sim_tx_buf_t* tx)
{

}

//=================================================================================================
// Controller Tasks

//=================================================================================================
// CiA402 default callback routine

