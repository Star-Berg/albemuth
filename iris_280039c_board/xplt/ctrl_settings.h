
#ifndef _FILE_CTRL_SETTINGS_H_
#define _FILE_CTRL_SETTINGS_H_

//=================================================================================================
// Incremental Debug Options

// BUILD_LEVEL 1: hardware validate, VF, voltage open loop
// BUILD_LEVEL 2: IF, current close loop
// BUILD_LEVEL 3: current loop with actual angle
// BUILD_LEVEL 4: speed loop
// BUILD_LEVEL 5: position loop
// BUILD_LEVEL 6: communication mode
#define BUILD_LEVEL (2)

//=================================================================================================
// Select Board Pin definition
#define LAUNCHPAD 0
#define GMP_IRIS  1

#define BOARD_SELECTION GMP_IRIS


//=================================================================================================
// Controller basic parameters

// Startup Delay, ms
#define CTRL_STARTUP_DELAY (100)

// Controller Frequency
#define CONTROLLER_FREQUENCY (20e3)

// PWM depth
#define CTRL_PWM_CMP_MAX (2500 - 1)

// PWM dead band
#define CTRL_PWM_DEADBAND_CMP (100)

// System tick
#define DSP_C2000_DSP_TIME_DIV (100000 / CTRL_PWM_CMP_MAX / 2)

// ADC Voltage Reference
#define CTRL_ADC_VOLTAGE_REF (3.3f)

//=================================================================================================
// Hardware parameters

#define BOOSTXL_3PHGANINV_IS_DEFAULT_PARAM

// invoke motor parameters
#include <ctl/component/hardware_preset/pmsm_motor/TYI_5008_KV335.h>

// invoke motor controller parameters
#include <ctl/component/hardware_preset/inverter_3ph/TI_BOOSTXL_3PhGaNInv.h>

///////////////////////////////////////////////////////////
// Encoder Propeties

// Encoder Full scale
#define CTRL_POS_ENC_FS (8000)

// Speed & Position control division
#define CTRL_MECH_DIV (5)


///////////////////////////////////////////////////////////
// Controller Base value

// DC bus voltage
#define CTRL_DCBUS_VOLTAGE (80.0f)

// phase voltage base, SVPWM modulation
#define CTRL_VOLTAGE_BASE (CTRL_DCBUS_VOLTAGE / 1.73205081f)

// voltage base, SPWM modulation
//#define CTRL_VOLTAGE_BASE (CTRL_DCBUS_VOLTAGE/2.0f)

// Current base, 10 A
#define CTRL_CURRENT_BASE (10.0f)

///////////////////////////////////////////////////////////
// inverter side sensor

// Current sensor sensitivity, TMCS1133A2B, V/A
#define CTRL_INVERTER_CURRENT_SENSITIVITY (50e-3f)

// Current sensor bias, V
#define CTRL_INVERTER_CURRENT_BIAS (1.65f)

// Voltage sensor sensitivity, V/V
#define CTRL_INVERTER_VOLTAGE_SENSITIVITY (0.02738589f)

// Voltage sensor bias, V
#define CTRL_INVERTER_VOLTAGE_BIAS (0.0f)

///////////////////////////////////////////////////////////
// DC Bus side sensor

// Current sensor sensitivity, V/A
#define CTRL_DC_CURRENT_SENSITIVITY (24.75e-3f)

// Current sensor bias, V
#define CTRL_DC_CURRENT_BIAS (1.65f)

// Voltage sensor sensitivity, maximum 120V, V/V
#define CTRL_DC_VOLTAGE_SENSITIVITY (0.02738589f)

// Voltage sensor bias, V
#define CTRL_DC_VOLTAGE_BIAS (0.0f)

//=================================================================================================
// Controller Settings

// Use discrete PID controller
// Discrete controller may bring more smooth response.
//#define PMSM_CTRL_USING_DISCRETE_CTRL

// Enable Discrete PID controller anti-saturation algorithm
#define _USE_DEBUG_DISCRETE_PID

// Enable ADC Calibrate
#define SPECIFY_ENABLE_ADC_CALIBRATE

// SPLL Close loop criteria
#define CTRL_SPLL_EPSILON ((float2ctrl(0.005)))

// Using negative modulator logic
#define PWM_MODULATOR_USING_NEGATIVE_LOGIC (1)

// Using three level modulator or two level modulator
//#define USING_NPC_MODULATOR

//=================================================================================================
// Board peripheral mapping

// Launchpad Board Pin Mapping
#if BOARD_SELECTION == LAUNCHPAD
#ifndef BOARD_PIN_MAPPING
#define BOARD_PIN_MAPPING

// PWM Channels
#define PHASE_U_BASE EPWM_J4_PHASE_U_BASE
#define PHASE_V_BASE EPWM_J4_PHASE_V_BASE
#define PHASE_W_BASE EPWM_J4_PHASE_W_BASE

// PWM Enable
#define PWM_ENABLE_PORT ENABLE_GATE
#define PWM_RESET_PORT  RESET_GATE

// DC Bus Voltage & Current
#define INV_VBUS_RESULT_BASE J3_VDC_RESULT_BASE
#define INV_IBUS_RESULT_BASE J7_VDC_RESULT_BASE

#define INV_VBUS J3_VDC
#define INV_IBUS J7_VDC

// Grid side Voltage & Current
#define INV_UA_RESULT_BASE J7_VU_RESULT_BASE
#define INV_UB_RESULT_BASE J7_VV_RESULT_BASE
#define INV_UC_RESULT_BASE J7_VW_RESULT_BASE

#define INV_UA J7_VU
#define INV_UB J7_VV
#define INV_UC J7_VW

#define INV_IA_RESULT_BASE J7_IU_RESULT_BASE
#define INV_IB_RESULT_BASE J7_IV_RESULT_BASE
#define INV_IC_RESULT_BASE J7_IW_RESULT_BASE

#define INV_IA J7_IU
#define INV_IB J7_IV
#define INV_IC J7_IW

// Converter side Voltage & Current
#define INV_UU_RESULT_BASE J3_VU_RESULT_BASE
#define INV_UV_RESULT_BASE J3_VV_RESULT_BASE
#define INV_UW_RESULT_BASE J3_VW_RESULT_BASE

#define INV_UU J3_VU
#define INV_UV J3_VV
#define INV_UW J3_VW

#define INV_IU_RESULT_BASE J3_IU_RESULT_BASE
#define INV_IV_RESULT_BASE J3_IV_RESULT_BASE
#define INV_IW_RESULT_BASE J3_IW_RESULT_BASE

#define INV_IU J3_IU
#define INV_IV J3_IV
#define INV_IW J3_IW

// System LED
#define SYSTEM_LED     LED_R
#define CONTROLLER_LED LED_G

#endif //BOARD_PIN_MAPPING

#else // BOARD_SELECTION == GMP_IRIS

#ifndef BOARD_PIN_MAPPING
#define BOARD_PIN_MAPPING

// PWM Channels
#define PHASE_U_BASE    IRIS_EPWM1_BASE
#define PHASE_V_BASE    IRIS_EPWM2_BASE
#define PHASE_W_BASE    IRIS_EPWM3_BASE

// PWM Enable
#define PWM_ENABLE_PORT IRIS_GPIO1
#define PWM_RESET_PORT  IRIS_GPIO3

// Vbus Voltage Channels
//#define MOTOR_VBUS_RESULT_BASE IRIS_ADCA_RESULT_BASE
//#define MOTOR_VBUS

// ADC Voltage Channels
//#define MOTOR_VU_RESULT_BASE IRIS_ADCA_RESULT_BASE
//#define MOTOR_VV_RESULT_BASE IRIS_ADCB_RESULT_BASE
//#define MOTOR_VW_RESULT_BASE IRIS_ADCC_RESULT_BASE

//#define MOTOR_VU
//#define MOTOR_VV
//#define MOTOR_VW

// ADC Current Channels
//#define MOTOR_IU_RESULT_BASE IRIS_ADCA_RESULT_BASE
//#define MOTOR_IV_RESULT_BASE IRIS_ADCB_RESULT_BASE
//#define MOTOR_IW_RESULT_BASE IRIS_ADCC_RESULT_BASE

//#define MOTOR_IU
//#define MOTOR_IV
//#define MOTOR_IW

// System LED
#define SYSTEM_LED      IRIS_LED1
#define CONTROLLER_LED  IRIS_LED2

#endif //BOARD_PIN_MAPPING

#endif // BOARD_PIN_MAPPING



//=================================================================================================
// Programmable DC power-supply hardware mapping

#define PSU_DAC_REF_V                         (3.3f)
#define PSU_DAC_MAX_CODE                      (4095U)

// Output-command inverse fits, excluding the 0 V / 0 A measurements.
// The voltage coefficients include the follow-up measured residual model:
// Vactual = 1.0034483 * Vset + 0.1400000 V. The two affine mappings are
// composed here so the real-time path still applies one calibration only.
#define PSU_VOLTAGE_OUTPUT_CAL_SLOPE          (0.9832997f)
#define PSU_VOLTAGE_OUTPUT_CAL_BIAS_V         (-0.1139542f)

// Iactual = 0.9970769 * Icommand + 0.0037433 A.
#define PSU_CURRENT_OUTPUT_CAL_SLOPE          (1.0029316f)
#define PSU_CURRENT_OUTPUT_CAL_BIAS_A         (-0.0037542f)

// Output voltage feedback: Vfu = Vo * 100k / (300k + 100k) = Vo / 4.
#define PSU_VOLTAGE_FB_RATIO                  (0.25f)

// ADC measurement correction: physical value = raw value * slope + bias.
// A zero raw sample remains zero; the bias is applied only to positive samples.
#define PSU_VOLTAGE_MEAS_CAL_SLOPE            (1.0621702f)
#define PSU_VOLTAGE_MEAS_CAL_BIAS_V           (0.0414826f)
#define PSU_CURRENT_MEAS_CAL_SLOPE            (0.9970769f)
#define PSU_CURRENT_MEAS_CAL_BIAS_A            (0.0037433f)

// Current feedback: Vfi = Io * Rshunt * amplifier gain.
#define PSU_CURRENT_SHUNT_OHM                 (1.0f)
#define PSU_CURRENT_AMP_GAIN                  (20.0f)

// ADC channels: Vfu -> J7-1 / ADC_CH1, Vfi -> J7-3 / ADC_CH3.
#define PSU_ADC_REF_V                         (CTRL_ADC_VOLTAGE_REF)
#define PSU_ADC_RESOLUTION_BITS               (12U)
#define PSU_ADC_IQN                           (24U)
#define PSU_VFU_RESULT_BASE                   (ADC_CH1_RESULT_BASE)
#define PSU_VFU_SOC                           (ADC_CH1)
#define PSU_VFI_RESULT_BASE                   (ADC_CH3_RESULT_BASE)
#define PSU_VFI_SOC                           (ADC_CH3)

// Output relay/MOSF switch:
// IRIS_EPWM6_BASE maps to EPWM7A / GPIO28 / J3-17.
// The ePWM output is forced to a static high or low level; no PWM waveform is used.
#define PSU_OUTPUT_SW_EPWM_BASE               (IRIS_EPWM6_BASE)
#define PSU_OUTPUT_SW_EPWM_OUTPUT             (EPWM_AQ_OUTPUT_A)
#define PSU_OUTPUT_ON_DELAY_CYCLES            (400U)  // 20 ms at 20 kHz

// Buzzer and status LEDs.
#define PSU_BEEP_PORT                         (IRIS_GPIO1)
#define PSU_BEEP_ON_LEVEL                     (1U)
#define PSU_BEEP_OFF_LEVEL                    (0U)
#define PSU_OUTPUT_LED_PORT                   (SYSTEM_LED)
#define PSU_OUTPUT_LED_ON_LEVEL               (0U)
#define PSU_OUTPUT_LED_OFF_LEVEL              (1U)
#define PSU_ALARM_LED_PORT                    (CONTROLLER_LED)
#define PSU_ALARM_LED_ON_LEVEL                (0U)
#define PSU_ALARM_LED_OFF_LEVEL               (1U)

//=================================================================================================
// User settings and three-mode CV/CC operation

// Voltage setting: 0.0 to 10.0 V, stored in 0.1 V units.
#define PSU_VOLTAGE_SET_MIN_DV                (0U)
#define PSU_VOLTAGE_SET_MAX_DV                (100U)
#define PSU_DEFAULT_VOLTAGE_SET_DV            (85U)

// Current setting: 0 to 100 mA, stored in 1 mA units.
#define PSU_CURRENT_SET_MIN_MA                (0U)
#define PSU_CURRENT_SET_MAX_MA                (100U)
#define PSU_DEFAULT_CURRENT_SET_MA            (50U)

// Selected operating mode: 0 = fixed CV, 1 = fixed CC, 2 = AUTO.
// AUTO is the default so the original CV/CC transition behavior is preserved.
#define PSU_DEFAULT_OPERATING_MODE            (2U)

// The analog minimum-selector performs the physical CV/CC handover in all modes.
// Software detects the active loop and decides whether the handover is normal or a fault.
#define PSU_MODE_CC_ENTER_CURRENT_MARGIN_A    (0.002f)
#define PSU_MODE_CC_EXIT_CURRENT_MARGIN_A     (0.005f)
#define PSU_MODE_CC_ENTER_VOLTAGE_DROP_V      (0.10f)
#define PSU_MODE_CC_EXIT_VOLTAGE_DROP_V       (0.03f)
#define PSU_MODE_DETECT_CYCLES                (20U)

// Fixed-mode protection filters the analog loop's normal startup overshoot.
// After Vsw turns on, ignore mode mismatch for 50 ms; afterwards a mismatch
// must remain continuous for 10 ms before it becomes a latched fault.
#define PSU_MODE_LIMIT_BLANK_CYCLES           (1000U) // 50 ms at 20 kHz
#define PSU_MODE_LIMIT_TRIP_CYCLES            (200U)  // 10 ms at 20 kHz

// Independent hard over-current trip applies in CV, CC and AUTO modes.
#define PSU_OVERCURRENT_TRIP_A                (0.103f)
#define PSU_OVERCURRENT_TRIP_CYCLES           (20U)

//=================================================================================================
// Keypad mapping

// HT16K33 key IDs are determined by the key-matrix wiring, not by the printed
// SW number. These values follow the verified reference-board mapping.
// Numeric and decimal keys.
#define PSU_KEY_DIGIT_1_ID                    (15U)
#define PSU_KEY_DIGIT_2_ID                    (14U)
#define PSU_KEY_DIGIT_3_ID                    (22U)
#define PSU_KEY_DIGIT_4_ID                    (6U)
#define PSU_KEY_DIGIT_5_ID                    (5U)
#define PSU_KEY_DIGIT_6_ID                    (4U)
#define PSU_KEY_DIGIT_7_ID                    (19U)
#define PSU_KEY_DIGIT_8_ID                    (18U)
#define PSU_KEY_DIGIT_9_ID                    (17U)
#define PSU_KEY_DIGIT_0_ID                    (2U)
#define PSU_KEY_DECIMAL_ID                    (1U)

// Functional keys according to the verified physical keyboard labels.
#define PSU_KEY_CONFIRM_ID                    (9U)  // SW2: confirm numeric input
#define PSU_KEY_MODE_TOGGLE_ID                (7U)  // SW9: cycle CV/CC/AUTO
#define PSU_KEY_OUTPUT_ID                     (16U) // SW14: output on/off or clear fault
#define PSU_KEY_CLEAR_CANCEL_ID               (21U) // SW10: cancel and clear current entry
#define PSU_KEY_BACKSPACE_ID                  (8U)  // SW1: delete last input character
#define PSU_KEY_EDIT_TOGGLE_ID                (3U)  // SW5: select voltage/current

#define PSU_KEY_TASK_PERIOD_MS                (50U)
#define PSU_KEY_RELEASE_TICKS                 (4U)
#define PSU_KEY_INPUT_TIMEOUT_TICKS           (200U) // 10 seconds
#define PSU_KEYPAD_BUFFER_SIZE                (8U)

//=================================================================================================
// Rotary encoder and display tasks

#define PSU_ENCODER_BASE                      (IRIS_EQEP1_BASE)
#define PSU_ENCODER_TASK_PERIOD_MS            (20U)
#define PSU_ENCODER_MAX_POSITION              (10000UL)
#define PSU_ENCODER_COUNTS_PER_STEP           (4L)
#define PSU_ENCODER_REVERSE                   (0U)

// The encoder push switch is wired to the eQEP1 INDEX input (GPIO13).
#define PSU_ENCODER_BUTTON_PORT               (IRIS_EQEP1_EQEPINDEX_GPIO)
#define PSU_ENCODER_BUTTON_ACTIVE_LEVEL       (0U)
#define PSU_ENCODER_BUTTON_RELEASE_TICKS      (2U)

// Fine/coarse adjustment increments. Voltage units are 0.1 V; current units are mA.
#define PSU_ENCODER_V_FINE_STEP_DV             (1L)
#define PSU_ENCODER_V_COARSE_STEP_DV           (5L)
#define PSU_ENCODER_I_FINE_STEP_MA             (1L)
#define PSU_ENCODER_I_COARSE_STEP_MA           (5L)

#define PSU_DISPLAY_TASK_PERIOD_MS            (50U)
#define PSU_LED_FLUSH_TASK_PERIOD_MS          (50U)
#define PSU_DISPLAY_FILTER_ALPHA              (0.25f)
#define PSU_OLED_LINE_CHAR_COUNT              (16U)

// HT16K33 indicator outputs. Adjust these three RAM locations/masks if the
// peripheral board routes its CV/CC/AUTO LEDs differently.
#define PSU_ALARM_TASK_PERIOD_MS              (100U)
#define PSU_ALARM_ON_TICKS                    (2U)
#define PSU_ALARM_OFF_TICKS                   (2U)

#endif // _FILE_CTRL_SETTINGS_H_
