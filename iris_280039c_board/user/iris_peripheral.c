// GMP basic core header
#include <gmp_core.h>

#include "ctl_main.h"
#include "user_main.h"

#include <stdio.h>
#include <stdlib.h>

#include <core/dev/display/ht16k33.h>
#include <core/dev/gpio/pca9555.h>
#include <core/dev/sensor/hdc1080.h>

#include <oled_driver.h>

//=================================================================================================
// GPIO helpers

gpio_halt gpio_beep;
gpio_halt gpio_alarm_led;
gpio_halt gpio_output_led;

void beep_on(void)
{
    gmp_hal_gpio_write(gpio_beep, PSU_BEEP_ON_LEVEL);
}

void beep_off(void)
{
    gmp_hal_gpio_write(gpio_beep, PSU_BEEP_OFF_LEVEL);
}

void alarm_led_on(void)
{
    gmp_hal_gpio_write(gpio_alarm_led, PSU_ALARM_LED_ON_LEVEL);
}

void alarm_led_off(void)
{
    gmp_hal_gpio_write(gpio_alarm_led, PSU_ALARM_LED_OFF_LEVEL);
}

void output_led_on(void)
{
    gmp_hal_gpio_write(gpio_output_led, PSU_OUTPUT_LED_ON_LEVEL);
}

void output_led_off(void)
{
    gmp_hal_gpio_write(gpio_output_led, PSU_OUTPUT_LED_OFF_LEVEL);
}

//=================================================================================================
// HT16K33 display and keypad devices

iic_halt iic_bus;
ht16k33_dev_t ht16k33;
hdc1080_dev_t hdc1080;

// Common-cathode seven-segment lookup table.
const unsigned char led_lut[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71, // F
    0x76, // H
    0x38, // L
    0x73, // P
    0x3E, // U
    0x40, // -
    0x80, // decimal point
    0x00  // blank
};

#define LED_LUT_A_INDEX       (10U)
#define LED_LUT_C_INDEX       (12U)
#define LED_LUT_E_INDEX       (14U)
#define LED_LUT_U_INDEX       (19U)
#define LED_LUT_DASH_INDEX    (20U)
#define LED_LUT_DOT_INDEX     (21U)
#define LED_LUT_BLANK_INDEX   (22U)

void update_led_content_8byte(
    ht16k33_dev_t* dev,
    uint16_t ch1,
    uint16_t ch2,
    uint16_t ch3,
    uint16_t ch4,
    uint16_t ch5,
    uint16_t ch6,
    uint16_t ch7,
    uint16_t ch8)
{
    dev->display_ram[0] = ch1;
    dev->display_ram[2] = ch2;
    dev->display_ram[4] = ch3;
    dev->display_ram[6] = ch4;
    dev->display_ram[8] = ch5;
    dev->display_ram[10] = ch6;
    dev->display_ram[12] = ch7;
    dev->display_ram[14] = ch8;
    dev->is_dirty = 1;
}

gmp_task_status_t tsk_LED_flush(gmp_task_t* tsk)
{
    ht16k33_dev_t* dev = (ht16k33_dev_t*)tsk->user_data;

    if (flag_init_cmpt != 0U)
    {
        ec_gt ret = ht16k33_update_display(dev);

        if (ret != GMP_EC_OK)
        {
            tsk->is_enabled = 0;
        }
    }

    return GMP_TASK_DONE;
}

//=================================================================================================
// Numeric keypad state machine

typedef enum _tag_psu_input_error_t
{
    PSU_INPUT_ERROR_NONE = 0,
    PSU_INPUT_ERROR_FORMAT,
    PSU_INPUT_ERROR_RANGE
} psu_input_error_t;

typedef struct _tag_psu_keypad_input_t
{
    fast_gt active;
    psu_edit_target_t target;
    char buffer[PSU_KEYPAD_BUFFER_SIZE];
    uint16_t length;
    uint16_t idle_ticks;
    psu_input_error_t error;
} psu_keypad_input_t;

static psu_keypad_input_t psu_keypad_input = {
    0,
    PSU_EDIT_VOLTAGE,
    {0},
    0U,
    0U,
    PSU_INPUT_ERROR_NONE};

typedef enum _tag_psu_step_mode_t
{
    PSU_STEP_FINE = 0,
    PSU_STEP_COARSE = 1
} psu_step_mode_t;

static psu_step_mode_t psu_step_mode = PSU_STEP_FINE;

static const char* psu_ui_operating_mode_name(psu_operating_mode_t mode)
{
    switch (mode)
    {
    case PSU_OPERATING_MODE_CV:
        return "CV";
    case PSU_OPERATING_MODE_CC:
        return "CC";
    default:
        return "AUTO";
    }
}

static const char* psu_ui_fault_name(psu_fault_code_t fault_code)
{
    switch (fault_code)
    {
    case PSU_FAULT_CV_OVERCURRENT:
        return "CV OVERCURRENT";
    case PSU_FAULT_CC_OVERVOLTAGE:
        return "CC OVERVOLT";
    case PSU_FAULT_HARD_OVERCURRENT:
        return "HARD OVERCURR";
    default:
        return "UNKNOWN";
    }
}

static void psu_keypad_reset(void)
{
    uint16_t index;

    psu_keypad_input.active = 0;
    psu_keypad_input.length = 0U;
    psu_keypad_input.idle_ticks = 0U;
    psu_keypad_input.error = PSU_INPUT_ERROR_NONE;

    for (index = 0U; index < PSU_KEYPAD_BUFFER_SIZE; index++)
    {
        psu_keypad_input.buffer[index] = '\0';
    }
}

static void psu_keypad_start(psu_edit_target_t target)
{
    psu_keypad_reset();
    psu_keypad_input.active = 1;
    psu_keypad_input.target = target;
    ctl_set_psu_edit_target(&psu_ctrl, target);
}

static void psu_ui_toggle_step(void)
{
    psu_step_mode =
        (psu_step_mode == PSU_STEP_FINE) ?
            PSU_STEP_COARSE : PSU_STEP_FINE;

    // A user-interface mode change cancels an unfinished numeric entry.
    psu_keypad_reset();

    gmp_base_print(
        "Encoder step: %s\r\n",
        (psu_step_mode == PSU_STEP_FINE) ? "fine" : "coarse");
}

static void psu_keypad_append_digit(uint16_t digit)
{
    if (digit > 9U)
    {
        return;
    }

    // Numeric keys directly begin an entry for the currently selected item.
    if (psu_keypad_input.active == 0)
    {
        psu_keypad_start(ctl_get_psu_edit_target(&psu_ctrl));
    }

    if (psu_keypad_input.length >= (PSU_KEYPAD_BUFFER_SIZE - 1U))
    {
        return;
    }

    psu_keypad_input.buffer[psu_keypad_input.length] =
        (char)('0' + digit);
    psu_keypad_input.length++;
    psu_keypad_input.buffer[psu_keypad_input.length] = '\0';
    psu_keypad_input.idle_ticks = 0U;
    psu_keypad_input.error = PSU_INPUT_ERROR_NONE;
}

static void psu_keypad_append_decimal(void)
{
    uint16_t index;

    if (psu_keypad_input.active == 0)
    {
        psu_keypad_start(ctl_get_psu_edit_target(&psu_ctrl));
    }

    if (psu_keypad_input.target != PSU_EDIT_VOLTAGE)
    {
        psu_keypad_input.error = PSU_INPUT_ERROR_FORMAT;
        return;
    }

    for (index = 0U; index < psu_keypad_input.length; index++)
    {
        if (psu_keypad_input.buffer[index] == '.')
        {
            psu_keypad_input.error = PSU_INPUT_ERROR_FORMAT;
            return;
        }
    }

    if (psu_keypad_input.length == 0U)
    {
        psu_keypad_append_digit(0U);
    }

    if (psu_keypad_input.length < (PSU_KEYPAD_BUFFER_SIZE - 1U))
    {
        psu_keypad_input.buffer[psu_keypad_input.length] = '.';
        psu_keypad_input.length++;
        psu_keypad_input.buffer[psu_keypad_input.length] = '\0';
        psu_keypad_input.idle_ticks = 0U;
        psu_keypad_input.error = PSU_INPUT_ERROR_NONE;
    }
}

static void psu_keypad_backspace(void)
{
    if ((psu_keypad_input.active == 0) ||
        (psu_keypad_input.length == 0U))
    {
        return;
    }

    psu_keypad_input.length--;
    psu_keypad_input.buffer[psu_keypad_input.length] = '\0';
    psu_keypad_input.idle_ticks = 0U;
    psu_keypad_input.error = PSU_INPUT_ERROR_NONE;
}

static fast_gt psu_keypad_parse_voltage(uint16_t* voltage_decivolt)
{
    uint16_t index;
    uint16_t whole = 0U;
    uint16_t fraction = 0U;
    uint16_t digit_count = 0U;
    uint16_t fraction_count = 0U;
    fast_gt decimal_seen = 0;

    if (psu_keypad_input.length == 0U)
    {
        return 0;
    }

    for (index = 0U; index < psu_keypad_input.length; index++)
    {
        char ch = psu_keypad_input.buffer[index];

        if (ch == '.')
        {
            if (decimal_seen != 0)
            {
                return 0;
            }

            decimal_seen = 1;
            continue;
        }

        if ((ch < '0') || (ch > '9'))
        {
            return 0;
        }

        digit_count++;

        if (decimal_seen == 0)
        {
            whole = (uint16_t)(whole * 10U + (uint16_t)(ch - '0'));

            if (whole > 10U)
            {
                return 0;
            }
        }
        else
        {
            fraction_count++;

            if (fraction_count > 1U)
            {
                return 0;
            }

            fraction = (uint16_t)(ch - '0');
        }
    }

    if (digit_count == 0U)
    {
        return 0;
    }

    if ((whole > 10U) || ((whole == 10U) && (fraction != 0U)))
    {
        return 0;
    }

    *voltage_decivolt = (uint16_t)(whole * 10U + fraction);
    return 1;
}

static fast_gt psu_keypad_parse_current(uint16_t* current_milliamp)
{
    uint16_t index;
    uint16_t value = 0U;

    if (psu_keypad_input.length == 0U)
    {
        return 0;
    }

    for (index = 0U; index < psu_keypad_input.length; index++)
    {
        char ch = psu_keypad_input.buffer[index];

        if ((ch < '0') || (ch > '9'))
        {
            return 0;
        }

        value = (uint16_t)(value * 10U + (uint16_t)(ch - '0'));

        if (value > PSU_CURRENT_SET_MAX_MA)
        {
            return 0;
        }
    }

    *current_milliamp = value;
    return 1;
}

static void psu_keypad_commit(void)
{
    uint16_t setting;
    fast_gt valid;

    if (psu_keypad_input.active == 0)
    {
        return;
    }

    if (psu_keypad_input.target == PSU_EDIT_VOLTAGE)
    {
        valid = psu_keypad_parse_voltage(&setting);

        if (valid != 0)
        {
            ctl_set_psu_voltage_setting_decivolt(&psu_ctrl, setting);
            gmp_base_print(
                "Voltage setting accepted: %u.%u V\r\n",
                setting / 10U,
                setting % 10U);
            psu_keypad_reset();
            return;
        }
    }
    else
    {
        valid = psu_keypad_parse_current(&setting);

        if (valid != 0)
        {
            ctl_set_psu_current_setting_milliamp(&psu_ctrl, setting);
            gmp_base_print(
                "Current setting accepted: %u mA\r\n",
                setting);
            psu_keypad_reset();
            return;
        }
    }

    psu_keypad_input.error = PSU_INPUT_ERROR_RANGE;
    psu_keypad_input.idle_ticks = 0U;
}

static fast_gt psu_keypad_get_digit(
    fast_gt key_id,
    uint16_t* digit)
{
    switch ((uint16_t)key_id)
    {
    case PSU_KEY_DIGIT_0_ID:
        *digit = 0U;
        return 1;
    case PSU_KEY_DIGIT_1_ID:
        *digit = 1U;
        return 1;
    case PSU_KEY_DIGIT_2_ID:
        *digit = 2U;
        return 1;
    case PSU_KEY_DIGIT_3_ID:
        *digit = 3U;
        return 1;
    case PSU_KEY_DIGIT_4_ID:
        *digit = 4U;
        return 1;
    case PSU_KEY_DIGIT_5_ID:
        *digit = 5U;
        return 1;
    case PSU_KEY_DIGIT_6_ID:
        *digit = 6U;
        return 1;
    case PSU_KEY_DIGIT_7_ID:
        *digit = 7U;
        return 1;
    case PSU_KEY_DIGIT_8_ID:
        *digit = 8U;
        return 1;
    case PSU_KEY_DIGIT_9_ID:
        *digit = 9U;
        return 1;
    default:
        *digit = 0U;
        return 0;
    }
}

static void psu_ui_handle_output_key(void)
{
    if (ctl_get_psu_fault_state(&psu_ctrl) != 0)
    {
        ctl_set_psu_output(&psu_ctrl, 0);

        if (ctl_clear_psu_fault(&psu_ctrl) != 0)
        {
            gmp_base_print("PSU fault cleared; output remains off.\r\n");
        }
        return;
    }

    if ((ctl_get_psu_output_request(&psu_ctrl) != 0) ||
        (ctl_get_psu_output_state(&psu_ctrl) != 0))
    {
        ctl_set_psu_output(&psu_ctrl, 0);
        gmp_base_print("PSU output-off requested.\r\n");
    }
    else
    {
        ctl_set_psu_output(&psu_ctrl, 1);
        gmp_base_print("PSU output-on requested.\r\n");
    }
}

static void psu_ui_handle_mode_key(void)
{
    if (ctl_cycle_psu_operating_mode(&psu_ctrl) == 0)
    {
        gmp_base_print("Turn output off before changing CV/CC/AUTO mode.\r\n");
        return;
    }

    gmp_base_print(
        "Operating mode: %s\r\n",
        psu_ui_operating_mode_name(
            ctl_get_psu_operating_mode(&psu_ctrl)));
}

gmp_task_status_t tsk_key_flush(gmp_task_t* tsk)
{
    ht16k33_dev_t* dev = (ht16k33_dev_t*)tsk->user_data;
    static fast_gt key_armed = 1;
    static uint16_t key_quiet_ticks = 0U;
    fast_gt key_id = 0;
    uint16_t digit = 0U;
    ec_gt ret;

    if (flag_init_cmpt == 0U)
    {
        return GMP_TASK_DONE;
    }

    ret = ht16k33_read_keys(dev, &key_id);

    if (ret != GMP_EC_OK)
    {
        tsk->is_enabled = 0;
        return GMP_TASK_DONE;
    }

    if (key_id == 0)
    {
        if (key_armed == 0)
        {
            if (key_quiet_ticks < PSU_KEY_RELEASE_TICKS)
            {
                key_quiet_ticks++;
            }

            if (key_quiet_ticks >= PSU_KEY_RELEASE_TICKS)
            {
                key_armed = 1;
                key_quiet_ticks = 0U;
            }
        }

        if (psu_keypad_input.active != 0)
        {
            if (psu_keypad_input.idle_ticks < PSU_KEY_INPUT_TIMEOUT_TICKS)
            {
                psu_keypad_input.idle_ticks++;
            }

            if (psu_keypad_input.idle_ticks >= PSU_KEY_INPUT_TIMEOUT_TICKS)
            {
                gmp_base_print("Numeric input timed out.\r\n");
                psu_keypad_reset();
            }
        }

        return GMP_TASK_DONE;
    }

    psu_keypad_input.idle_ticks = 0U;

    if (key_armed == 0)
    {
        key_quiet_ticks = 0U;
        return GMP_TASK_DONE;
    }

    key_armed = 0;
    key_quiet_ticks = 0U;

    gmp_base_print("Key ID: %d\r\n", key_id);

    if (psu_keypad_get_digit(key_id, &digit) != 0)
    {
        psu_keypad_append_digit(digit);
        return GMP_TASK_DONE;
    }

    switch ((uint16_t)key_id)
    {
    case PSU_KEY_DECIMAL_ID:
        psu_keypad_append_decimal();
        break;

    case PSU_KEY_CONFIRM_ID:
        psu_keypad_commit();
        break;

    case PSU_KEY_EDIT_TOGGLE_ID:
        psu_keypad_reset();
        ctl_toggle_psu_edit_target(&psu_ctrl);
        gmp_base_print(
            "Edit target: %s\r\n",
            (ctl_get_psu_edit_target(&psu_ctrl) == PSU_EDIT_VOLTAGE) ?
                "voltage" : "current");
        break;

    case PSU_KEY_MODE_TOGGLE_ID:
        psu_keypad_reset();
        psu_ui_handle_mode_key();
        break;

    case PSU_KEY_OUTPUT_ID:
        // Output control keeps its original protection/fault-clear behavior.
        psu_keypad_reset();
        psu_ui_handle_output_key();
        break;

    case PSU_KEY_STEP_TOGGLE_ID:
        psu_ui_toggle_step();
        break;

    case PSU_KEY_CLEAR_CANCEL_ID:
        if (psu_keypad_input.active != 0)
        {
            psu_keypad_reset();
            gmp_base_print("Numeric entry cancelled and cleared.\r\n");
        }
        break;

    case PSU_KEY_BACKSPACE_ID:
        psu_keypad_backspace();
        break;

    default:
        break;
    }

    return GMP_TASK_DONE;
}

//=================================================================================================
// Rotary encoder

static void psu_ui_adjust_selected_setting(int32_t step_count)
{
    int32_t setting;
    int32_t unit_step;

    if (step_count == 0L)
    {
        return;
    }

    // Turning the knob cancels any unconfirmed keypad entry.
    psu_keypad_reset();

    if (ctl_get_psu_edit_target(&psu_ctrl) == PSU_EDIT_VOLTAGE)
    {
        unit_step =
            (psu_step_mode == PSU_STEP_FINE) ?
                PSU_ENCODER_V_FINE_STEP_DV :
                PSU_ENCODER_V_COARSE_STEP_DV;

        setting = (int32_t)ctl_get_psu_voltage_setting_decivolt(&psu_ctrl);
        setting += step_count * unit_step;

        if (setting < (int32_t)PSU_VOLTAGE_SET_MIN_DV)
        {
            setting = (int32_t)PSU_VOLTAGE_SET_MIN_DV;
        }
        if (setting > (int32_t)PSU_VOLTAGE_SET_MAX_DV)
        {
            setting = (int32_t)PSU_VOLTAGE_SET_MAX_DV;
        }

        ctl_set_psu_voltage_setting_decivolt(&psu_ctrl, (uint16_t)setting);
    }
    else
    {
        unit_step =
            (psu_step_mode == PSU_STEP_FINE) ?
                PSU_ENCODER_I_FINE_STEP_MA :
                PSU_ENCODER_I_COARSE_STEP_MA;

        setting = (int32_t)ctl_get_psu_current_setting_milliamp(&psu_ctrl);
        setting += step_count * unit_step;

        if (setting < (int32_t)PSU_CURRENT_SET_MIN_MA)
        {
            setting = (int32_t)PSU_CURRENT_SET_MIN_MA;
        }
        if (setting > (int32_t)PSU_CURRENT_SET_MAX_MA)
        {
            setting = (int32_t)PSU_CURRENT_SET_MAX_MA;
        }

        ctl_set_psu_current_setting_milliamp(&psu_ctrl, (uint16_t)setting);
    }
}

gmp_task_status_t tsk_psu_encoder(gmp_task_t* tsk)
{
    static fast_gt initialized = 0;
    static uint32_t previous_position = 0U;
    static int32_t count_accumulator = 0L;
    uint32_t current_position;
    int32_t position_delta;
    int32_t step_count = 0L;

    GMP_UNUSED_VAR(tsk);

    if (flag_init_cmpt == 0U)
    {
        return GMP_TASK_DONE;
    }

    if (initialized == 0)
    {
        EQEP_setPositionCounterConfig(
            PSU_ENCODER_BASE,
            EQEP_POSITION_RESET_MAX_POS,
            (uint32_t)PSU_ENCODER_MAX_POSITION);

        EQEP_setPosition(PSU_ENCODER_BASE, 0U);

        previous_position = 0U;
        count_accumulator = 0L;
        initialized = 1;

        return GMP_TASK_DONE;
    }

    current_position = EQEP_getPosition(PSU_ENCODER_BASE);
    position_delta =
        (int32_t)current_position - (int32_t)previous_position;
    previous_position = current_position;

    // Correct the signed delta when the QEP position counter wraps around.
    if (position_delta > (int32_t)(PSU_ENCODER_MAX_POSITION / 2UL))
    {
        position_delta -= (int32_t)(PSU_ENCODER_MAX_POSITION + 1UL);
    }
    else if (position_delta < -((int32_t)(PSU_ENCODER_MAX_POSITION / 2UL)))
    {
        position_delta += (int32_t)(PSU_ENCODER_MAX_POSITION + 1UL);
    }

    count_accumulator += position_delta;

    // Preserve all detents when the knob is rotated quickly between task calls.
    while (count_accumulator >= PSU_ENCODER_COUNTS_PER_STEP)
    {
        step_count++;
        count_accumulator -= PSU_ENCODER_COUNTS_PER_STEP;
    }

    while (count_accumulator <= -PSU_ENCODER_COUNTS_PER_STEP)
    {
        step_count--;
        count_accumulator += PSU_ENCODER_COUNTS_PER_STEP;
    }

#if PSU_ENCODER_REVERSE
    step_count = -step_count;
#endif

    if (step_count != 0L)
    {
        psu_ui_adjust_selected_setting(step_count);
    }

    return GMP_TASK_DONE;
}

//=================================================================================================
// Display helpers

static void psu_ui_pad_oled_line(char* line)
{
    uint16_t index = 0U;

    while ((index < PSU_OLED_LINE_CHAR_COUNT) && (line[index] != '\0'))
    {
        index++;
    }

    while (index < PSU_OLED_LINE_CHAR_COUNT)
    {
        line[index] = ' ';
        index++;
    }

    line[PSU_OLED_LINE_CHAR_COUNT] = '\0';
}

static uint16_t psu_ui_scale_to_uint16(
    parameter_gt value,
    parameter_gt scale,
    uint16_t maximum)
{
    parameter_gt scaled_value;

    if (value <= 0.0f)
    {
        return 0U;
    }

    scaled_value = value * scale + 0.5f;

    if (scaled_value >= (parameter_gt)maximum)
    {
        return maximum;
    }

    return (uint16_t)scaled_value;
}

static void psu_ui_write_led_segments(
    ht16k33_dev_t* dev,
    uint16_t segments[8])
{
    update_led_content_8byte(
        dev,
        segments[0],
        segments[1],
        segments[2],
        segments[3],
        segments[4],
        segments[5],
        segments[6],
        segments[7]);
}

static void psu_ui_update_keypad_led(ht16k33_dev_t* dev)
{
    uint16_t segments[8];
    uint16_t digits[6];
    uint16_t digit_count = 0U;
    uint16_t index;
    uint16_t start;

    for (index = 0U; index < 8U; index++)
    {
        segments[index] = led_lut[LED_LUT_BLANK_INDEX];
    }

    segments[0] =
        (psu_keypad_input.target == PSU_EDIT_VOLTAGE) ?
            led_lut[LED_LUT_U_INDEX] : led_lut[LED_LUT_A_INDEX];

    if (psu_keypad_input.error != PSU_INPUT_ERROR_NONE)
    {
        segments[2] = led_lut[LED_LUT_E_INDEX];
        segments[3] = led_lut[LED_LUT_DASH_INDEX];
        psu_ui_write_led_segments(dev, segments);
        return;
    }

    for (index = 0U;
         (index < psu_keypad_input.length) && (digit_count < 6U);
         index++)
    {
        char ch = psu_keypad_input.buffer[index];

        if ((ch >= '0') && (ch <= '9'))
        {
            digits[digit_count] = led_lut[(uint16_t)(ch - '0')];
            digit_count++;
        }
        else if ((ch == '.') && (digit_count > 0U))
        {
            digits[digit_count - 1U] |= led_lut[LED_LUT_DOT_INDEX];
        }
    }

    start = (uint16_t)(8U - digit_count);

    if (start < 2U)
    {
        start = 2U;
    }

    for (index = 0U; (index < digit_count) && ((start + index) < 8U); index++)
    {
        segments[start + index] = digits[index];
    }

    psu_ui_write_led_segments(dev, segments);
}

static void psu_ui_update_normal_led(
    ht16k33_dev_t* dev,
    parameter_gt display_voltage_v,
    parameter_gt display_current_a)
{
    uint16_t segments[8];
    uint16_t voltage_set_dv;
    uint16_t current_set_ma;
    uint16_t voltage_meas_dv;
    uint16_t current_meas_ma;
    uint16_t fault_current_ma;
    uint16_t fault_voltage_dv;
    uint16_t index;

    for (index = 0U; index < 8U; index++)
    {
        segments[index] = led_lut[LED_LUT_BLANK_INDEX];
    }

    if (ctl_get_psu_fault_state(&psu_ctrl) != 0)
    {
        segments[0] = led_lut[LED_LUT_E_INDEX];
        segments[1] = led_lut[LED_LUT_DASH_INDEX];

        if (ctl_get_psu_fault_code(&psu_ctrl) == PSU_FAULT_CC_OVERVOLTAGE)
        {
            fault_voltage_dv = psu_ui_scale_to_uint16(
                ctrl2float(ctl_get_psu_fault_voltage(&psu_ctrl)),
                10.0f,
                999U);

            segments[2] = led_lut[LED_LUT_U_INDEX];
            segments[5] = led_lut[(fault_voltage_dv / 100U) % 10U];
            segments[6] = led_lut[(fault_voltage_dv / 10U) % 10U];
            segments[7] = led_lut[fault_voltage_dv % 10U];
        }
        else
        {
            fault_current_ma = psu_ui_scale_to_uint16(
                ctrl2float(ctl_get_psu_fault_current(&psu_ctrl)),
                1000.0f,
                999U);

            segments[2] = led_lut[0];
            segments[3] = led_lut[LED_LUT_C_INDEX];
            segments[5] = led_lut[(fault_current_ma / 100U) % 10U];
            segments[6] = led_lut[(fault_current_ma / 10U) % 10U];
            segments[7] = led_lut[fault_current_ma % 10U];
        }

        psu_ui_write_led_segments(dev, segments);
        return;
    }

    if (ctl_get_psu_edit_target(&psu_ctrl) == PSU_EDIT_VOLTAGE)
    {
        voltage_set_dv = ctl_get_psu_voltage_setting_decivolt(&psu_ctrl);
        voltage_meas_dv = psu_ui_scale_to_uint16(display_voltage_v, 10.0f, 100U);

        segments[0] = led_lut[LED_LUT_U_INDEX];
        segments[1] = led_lut[(voltage_set_dv / 100U) % 10U];
        segments[2] = led_lut[(voltage_set_dv / 10U) % 10U] |
                      led_lut[LED_LUT_DOT_INDEX];
        segments[3] = led_lut[voltage_set_dv % 10U];
        segments[5] = led_lut[(voltage_meas_dv / 100U) % 10U];
        segments[6] = led_lut[(voltage_meas_dv / 10U) % 10U] |
                      led_lut[LED_LUT_DOT_INDEX];
        segments[7] = led_lut[voltage_meas_dv % 10U];
    }
    else
    {
        current_set_ma = ctl_get_psu_current_setting_milliamp(&psu_ctrl);
        current_meas_ma = psu_ui_scale_to_uint16(display_current_a, 1000.0f, 999U);

        segments[0] = led_lut[LED_LUT_A_INDEX];
        segments[1] = led_lut[(current_set_ma / 100U) % 10U];
        segments[2] = led_lut[(current_set_ma / 10U) % 10U];
        segments[3] = led_lut[current_set_ma % 10U];
        segments[5] = led_lut[(current_meas_ma / 100U) % 10U];
        segments[6] = led_lut[(current_meas_ma / 10U) % 10U];
        segments[7] = led_lut[current_meas_ma % 10U];
    }

    psu_ui_write_led_segments(dev, segments);
}

gmp_task_status_t tsk_psu_display(gmp_task_t* tsk)
{
    ht16k33_dev_t* dev = (ht16k33_dev_t*)tsk->user_data;
    static uint16_t oled_line_index = 0U;
    static fast_gt filter_initialized = 0;
    static parameter_gt display_voltage_v = 0.0f;
    static parameter_gt display_current_a = 0.0f;
    parameter_gt voltage_sample_v;
    parameter_gt current_sample_a;
    uint16_t voltage_set_dv;
    uint16_t current_set_ma;
    uint16_t voltage_meas_cv;
    uint16_t current_meas_ma;
    char output_line[24];

    if (flag_init_cmpt == 0U)
    {
        return GMP_TASK_DONE;
    }

    voltage_sample_v = ctrl2float(ctl_get_psu_voltage_measurement(&psu_ctrl));
    current_sample_a = ctrl2float(ctl_get_psu_current_measurement(&psu_ctrl));

    if (voltage_sample_v < 0.0f)
    {
        voltage_sample_v = 0.0f;
    }
    if (current_sample_a < 0.0f)
    {
        current_sample_a = 0.0f;
    }

    if ((filter_initialized == 0) ||
        (ctl_get_psu_output_state(&psu_ctrl) == 0))
    {
        display_voltage_v = voltage_sample_v;
        display_current_a = current_sample_a;
        filter_initialized = 1;
    }
    else
    {
        display_voltage_v +=
            PSU_DISPLAY_FILTER_ALPHA * (voltage_sample_v - display_voltage_v);
        display_current_a +=
            PSU_DISPLAY_FILTER_ALPHA * (current_sample_a - display_current_a);
    }

    if ((ctl_get_psu_fault_state(&psu_ctrl) == 0) &&
        (psu_keypad_input.active != 0))
    {
        psu_ui_update_keypad_led(dev);
    }
    else
    {
        psu_ui_update_normal_led(dev, display_voltage_v, display_current_a);
    }

    voltage_set_dv = ctl_get_psu_voltage_setting_decivolt(&psu_ctrl);
    current_set_ma = ctl_get_psu_current_setting_milliamp(&psu_ctrl);
    voltage_meas_cv = psu_ui_scale_to_uint16(display_voltage_v, 100.0f, 9999U);
    current_meas_ma = psu_ui_scale_to_uint16(display_current_a, 1000.0f, 999U);

    if (ctl_get_psu_fault_state(&psu_ctrl) != 0)
    {
        switch (oled_line_index)
        {
        case 0U:
            sprintf(
                output_line,
                "FAULT:%s",
                psu_ui_fault_name(ctl_get_psu_fault_code(&psu_ctrl)));
            break;
        case 1U:
            sprintf(
                output_line,
                "TRIP I:%03umA",
                psu_ui_scale_to_uint16(
                    ctrl2float(ctl_get_psu_fault_current(&psu_ctrl)),
                    1000.0f,
                    999U));
            break;
        case 2U:
            sprintf(
                output_line,
                "TRIP V:%02u.%02u",
                psu_ui_scale_to_uint16(
                    ctrl2float(ctl_get_psu_fault_voltage(&psu_ctrl)),
                    100.0f,
                    9999U) / 100U,
                psu_ui_scale_to_uint16(
                    ctrl2float(ctl_get_psu_fault_voltage(&psu_ctrl)),
                    100.0f,
                    9999U) % 100U);
            break;
        default:
            sprintf(output_line, "SW14=CLEAR");
            break;
        }
    }
    else if (psu_keypad_input.active != 0)
    {
        switch (oled_line_index)
        {
        case 0U:
            sprintf(
                output_line,
                "INPUT:%s",
                (psu_keypad_input.target == PSU_EDIT_VOLTAGE) ? "VOLTAGE" : "CURRENT");
            break;
        case 1U:
            sprintf(
                output_line,
                "VALUE:%s%s",
                psu_keypad_input.buffer,
                (psu_keypad_input.target == PSU_EDIT_VOLTAGE) ? "V" : "mA");
            break;
        case 2U:
            sprintf(output_line, "ENTER OK BS DEL");
            break;
        default:
            if (psu_keypad_input.error == PSU_INPUT_ERROR_NONE)
            {
                sprintf(
                    output_line,
                    "%s",
                    (psu_keypad_input.target == PSU_EDIT_VOLTAGE) ?
                        "RANGE 0-10.0V" : "RANGE 0-100mA");
            }
            else
            {
                sprintf(output_line, "ERROR:CHECK VAL");
            }
            break;
        }
    }
    else
    {
        switch (oled_line_index)
        {
        case 0U:
            sprintf(
                output_line,
                "%s/%s %s E:%c",
                psu_ui_operating_mode_name(
                    ctl_get_psu_operating_mode(&psu_ctrl)),
                (ctl_get_psu_output_state(&psu_ctrl) != 0) ?
                    ((ctl_get_psu_mode(&psu_ctrl) == PSU_MODE_CV) ? "CV" : "CC") : "--",
                (ctl_get_psu_output_state(&psu_ctrl) != 0) ? "ON" : "OFF",
                (ctl_get_psu_edit_target(&psu_ctrl) == PSU_EDIT_VOLTAGE) ? 'V' : 'I');
            break;
        case 1U:
            sprintf(
                output_line,
                "SET %02u.%uV %03umA",
                voltage_set_dv / 10U,
                voltage_set_dv % 10U,
                current_set_ma);
            break;
        case 2U:
            sprintf(
                output_line,
                "ACT %02u.%02uV %03umA",
                voltage_meas_cv / 100U,
                voltage_meas_cv % 100U,
                current_meas_ma);
            break;
        default:
            if (ctl_get_psu_output_state(&psu_ctrl) != 0)
            {
                sprintf(output_line, "STATUS:RUN");
            }
            else if (ctl_get_psu_output_request(&psu_ctrl) != 0)
            {
                sprintf(output_line, "STATUS:START");
            }
            else
            {
                sprintf(output_line, "SW18:MODE READY");
            }
            break;
        }
    }

    psu_ui_pad_oled_line(output_line);
    oled_show_str(0U, (uint8_t)(oled_line_index * 2U), output_line);

    oled_line_index++;
    if (oled_line_index >= 4U)
    {
        oled_line_index = 0U;
    }

    return GMP_TASK_DONE;
}

//=================================================================================================
// Audible/visual alarm and output indicator

gmp_task_status_t tsk_psu_alarm(gmp_task_t* tsk)
{
    static uint16_t alarm_tick = 0U;

    GMP_UNUSED_VAR(tsk);

    if (flag_init_cmpt == 0U)
    {
        return GMP_TASK_DONE;
    }

    if (ctl_get_psu_output_state(&psu_ctrl) != 0)
    {
        output_led_on();
    }
    else
    {
        output_led_off();
    }

    if (ctl_get_psu_fault_state(&psu_ctrl) == 0)
    {
        alarm_tick = 0U;
        beep_off();
        alarm_led_off();
        return GMP_TASK_DONE;
    }

    if (alarm_tick < PSU_ALARM_ON_TICKS)
    {
        beep_on();
        alarm_led_on();
    }
    else
    {
        beep_off();
        alarm_led_off();
    }

    alarm_tick++;

    if (alarm_tick >= (PSU_ALARM_ON_TICKS + PSU_ALARM_OFF_TICKS))
    {
        alarm_tick = 0U;
    }

    return GMP_TASK_DONE;
}
