#include <gmp_core.h>
#include <driverlib.h>
#include <device.h>
#include <F021_F28003x_C28x.h>

#include "psu_persistence.h"

#define PSU_FLASH_MAGIC                 (0x5053U)
#define PSU_FLASH_VERSION               (1U)
#define PSU_FLASH_SECTOR_A_START        (0x000AE000UL)
#define PSU_FLASH_SECTOR_B_START        (0x000AF000UL)
#define PSU_FLASH_SECTOR_USABLE_WORDS   (0x0FF0UL)
#define PSU_FLASH_RECORD_WORDS          (8U)
#define PSU_FLASH_SAVE_DELAY_TICKS      (20U)

typedef struct _tag_psu_saved_settings_t
{
    uint16_t voltage_set_decivolt;
    uint16_t current_set_milliamp;
    uint16_t operating_mode;
    uint16_t edit_target;
} psu_saved_settings_t;

typedef struct _tag_psu_flash_scan_t
{
    const uint16_t* latest_record;
    uint32_t next_address;
} psu_flash_scan_t;

static ctl_psu_t* persistence_psu = NULL;
static psu_saved_settings_t saved_settings;
static psu_saved_settings_t pending_settings;
static uint32_t active_sector_start = PSU_FLASH_SECTOR_A_START;
static uint32_t next_record_address = PSU_FLASH_SECTOR_A_START;
static uint16_t next_sequence = 1U;
static uint16_t pending_ticks = 0U;
static fast_gt persistence_ready = 0;

#pragma DATA_ALIGN(flash_write_buffer, 8)
static uint16_t flash_write_buffer[PSU_FLASH_RECORD_WORDS];

static uint16_t psu_flash_crc16(const uint16_t* words, uint16_t count)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint16_t bit;

    for (i = 0U; i < count; i++)
    {
        crc ^= words[i];
        for (bit = 0U; bit < 16U; bit++)
        {
            crc = (crc & 1U) ?
                (uint16_t)((crc >> 1U) ^ 0xA001U) :
                (uint16_t)(crc >> 1U);
        }
    }

    return crc;
}

static void psu_capture_settings(
    const ctl_psu_t* hpsu,
    psu_saved_settings_t* settings)
{
    settings->voltage_set_decivolt = hpsu->voltage_set_decivolt;
    settings->current_set_milliamp = hpsu->current_set_milliamp;
    settings->operating_mode = (uint16_t)hpsu->operating_mode;
    settings->edit_target = (uint16_t)hpsu->edit_target;
}

static fast_gt psu_settings_equal(
    const psu_saved_settings_t* lhs,
    const psu_saved_settings_t* rhs)
{
    return
        (lhs->voltage_set_decivolt == rhs->voltage_set_decivolt) &&
        (lhs->current_set_milliamp == rhs->current_set_milliamp) &&
        (lhs->operating_mode == rhs->operating_mode) &&
        (lhs->edit_target == rhs->edit_target);
}

static fast_gt psu_flash_record_valid(const uint16_t* record)
{
    if ((record[0] != PSU_FLASH_MAGIC) ||
        (record[1] != PSU_FLASH_VERSION) ||
        (record[3] > PSU_VOLTAGE_SET_MAX_DV) ||
        (record[4] > PSU_CURRENT_SET_MAX_MA) ||
        (record[5] > (uint16_t)PSU_OPERATING_MODE_AUTO) ||
        (record[6] > (uint16_t)PSU_EDIT_CURRENT))
    {
        return 0;
    }

    return record[7] == psu_flash_crc16(record, 7U);
}

static fast_gt psu_flash_sequence_is_newer(uint16_t lhs, uint16_t rhs)
{
    return ((int16_t)(lhs - rhs)) > 0;
}

static uint32_t psu_flash_other_sector(uint32_t sector_start)
{
    return (sector_start == PSU_FLASH_SECTOR_A_START) ?
        PSU_FLASH_SECTOR_B_START : PSU_FLASH_SECTOR_A_START;
}

static void psu_flash_scan_sector(
    uint32_t sector_start,
    psu_flash_scan_t* scan)
{
    uint32_t address;
    const uint16_t* record;

    scan->latest_record = NULL;
    scan->next_address = sector_start + PSU_FLASH_SECTOR_USABLE_WORDS;

    for (address = sector_start;
         address < (sector_start + PSU_FLASH_SECTOR_USABLE_WORDS);
         address += PSU_FLASH_RECORD_WORDS)
    {
        record = (const uint16_t*)address;
        if (record[0] == 0xFFFFU)
        {
            scan->next_address = address;
            break;
        }
        if (psu_flash_record_valid(record) != 0)
        {
            scan->latest_record = record;
        }
    }
}

static fast_gt psu_flash_prepare(void)
{
    Fapi_StatusType status;

    status = Fapi_initializeAPI(
        F021_CPU0_BASE_ADDRESS,
        DEVICE_SYSCLK_FREQ / 1000000UL);
    if (status != Fapi_Status_Success)
    {
        return 0;
    }

    status = Fapi_setActiveFlashBank(Fapi_FlashBank2);
    return status == Fapi_Status_Success;
}

#pragma CODE_SECTION(psu_flash_erase_sector, ".TI.ramfunc")
static fast_gt psu_flash_erase_sector(uint32_t sector_start)
{
    Fapi_StatusType status;

    status = Fapi_issueAsyncCommand(Fapi_ClearMore);
    while (Fapi_checkFsmForReady() == Fapi_Status_FsmBusy)
    {
    }
    if (status != Fapi_Status_Success)
    {
        return 0;
    }

    status = Fapi_issueAsyncCommandWithAddress(
        Fapi_EraseSector,
        (uint32_t*)sector_start);
    while (Fapi_checkFsmForReady() == Fapi_Status_FsmBusy)
    {
    }

    return (status == Fapi_Status_Success) && (Fapi_getFsmStatus() == 0U);
}

#pragma CODE_SECTION(psu_flash_program_record, ".TI.ramfunc")
static fast_gt psu_flash_program_record(uint32_t address)
{
    Fapi_StatusType status;
    Fapi_FlashStatusWordType flash_status;

    status = Fapi_issueProgrammingCommand(
        (uint32_t*)address,
        flash_write_buffer,
        PSU_FLASH_RECORD_WORDS,
        0,
        0U,
        Fapi_AutoEccGeneration);
    while (Fapi_checkFsmForReady() == Fapi_Status_FsmBusy)
    {
    }
    if ((status != Fapi_Status_Success) || (Fapi_getFsmStatus() != 0U))
    {
        return 0;
    }

    status = Fapi_doVerifyBy16bits(
        (uint16_t*)address,
        PSU_FLASH_RECORD_WORDS,
        flash_write_buffer,
        &flash_status);

    return (status == Fapi_Status_Success) &&
           (psu_flash_record_valid((const uint16_t*)address) != 0);
}

static fast_gt psu_flash_save(const psu_saved_settings_t* settings)
{
    uint32_t write_address = next_record_address;
    uint32_t target_sector = active_sector_start;
    fast_gt rollover = 0;
    fast_gt ok;

    if (write_address >=
        (active_sector_start + PSU_FLASH_SECTOR_USABLE_WORDS))
    {
        target_sector = psu_flash_other_sector(active_sector_start);
        write_address = target_sector;
        rollover = 1;
    }

    flash_write_buffer[0] = PSU_FLASH_MAGIC;
    flash_write_buffer[1] = PSU_FLASH_VERSION;
    flash_write_buffer[2] = next_sequence;
    flash_write_buffer[3] = settings->voltage_set_decivolt;
    flash_write_buffer[4] = settings->current_set_milliamp;
    flash_write_buffer[5] = settings->operating_mode;
    flash_write_buffer[6] = settings->edit_target;
    flash_write_buffer[7] = psu_flash_crc16(flash_write_buffer, 7U);

    DINT;
    EALLOW;

    ok = psu_flash_prepare();
    if ((ok != 0) && (rollover != 0))
    {
        // Keep the current sector intact until the replacement record has
        // been successfully programmed and verified in the other sector.
        ok = psu_flash_erase_sector(target_sector);
    }
    if (ok != 0)
    {
        ok = psu_flash_program_record(write_address);
    }

    EDIS;
    EINT;

    if (ok != 0)
    {
        active_sector_start = target_sector;
        next_record_address = write_address + PSU_FLASH_RECORD_WORDS;
        next_sequence++;
    }
    else if (rollover == 0)
    {
        // Never retry a potentially partially programmed 128-bit row.
        next_record_address += PSU_FLASH_RECORD_WORDS;
    }

    return ok;
}

void psu_persistence_init(ctl_psu_t* hpsu)
{
    psu_flash_scan_t scan_a;
    psu_flash_scan_t scan_b;
    const uint16_t* latest_record = NULL;

    persistence_psu = hpsu;
    psu_flash_scan_sector(PSU_FLASH_SECTOR_A_START, &scan_a);
    psu_flash_scan_sector(PSU_FLASH_SECTOR_B_START, &scan_b);

    if (scan_a.latest_record != NULL)
    {
        latest_record = scan_a.latest_record;
        active_sector_start = PSU_FLASH_SECTOR_A_START;
        next_record_address = scan_a.next_address;
    }

    if ((scan_b.latest_record != NULL) &&
        ((latest_record == NULL) ||
         (psu_flash_sequence_is_newer(
              scan_b.latest_record[2], latest_record[2]) != 0)))
    {
        latest_record = scan_b.latest_record;
        active_sector_start = PSU_FLASH_SECTOR_B_START;
        next_record_address = scan_b.next_address;
    }

    if (latest_record != NULL)
    {
        next_sequence = (uint16_t)(latest_record[2] + 1U);
        ctl_set_psu_voltage_setting_decivolt(hpsu, latest_record[3]);
        ctl_set_psu_current_setting_milliamp(hpsu, latest_record[4]);
        ctl_set_psu_operating_mode(
            hpsu,
            (psu_operating_mode_t)latest_record[5]);
        ctl_set_psu_edit_target(hpsu, (psu_edit_target_t)latest_record[6]);
        gmp_base_print("PSU settings restored from Flash.\r\n");
    }
    else
    {
        active_sector_start = PSU_FLASH_SECTOR_A_START;
        next_record_address = PSU_FLASH_SECTOR_A_START;
        next_sequence = 1U;
        gmp_base_print("No valid PSU Flash settings; defaults loaded.\r\n");
    }

    // Restoring settings must never energize the physical output.
    ctl_set_psu_output(hpsu, 0);
    hpsu->output_enable = 0;
    hpsu->output_delay_counter = 0U;

    psu_capture_settings(hpsu, &saved_settings);
    pending_settings = saved_settings;
    pending_ticks = 0U;
    persistence_ready = 1;
}

gmp_task_status_t tsk_psu_persistence(gmp_task_t* tsk)
{
    psu_saved_settings_t current;

    GMP_UNUSED_VAR(tsk);

    if ((persistence_ready == 0) || (persistence_psu == NULL))
    {
        return GMP_TASK_DONE;
    }

    psu_capture_settings(persistence_psu, &current);

    if (psu_settings_equal(&current, &saved_settings) != 0)
    {
        pending_ticks = 0U;
        return GMP_TASK_DONE;
    }

    if (psu_settings_equal(&current, &pending_settings) == 0)
    {
        pending_settings = current;
        pending_ticks = 0U;
        return GMP_TASK_DONE;
    }

    if (pending_ticks < PSU_FLASH_SAVE_DELAY_TICKS)
    {
        pending_ticks++;
    }

    if (pending_ticks >= PSU_FLASH_SAVE_DELAY_TICKS)
    {
        // Erasing a sector takes much longer than appending one record. Wait
        // for a safe output-off state only when a sector rollover is needed.
        if ((next_record_address >=
             (active_sector_start + PSU_FLASH_SECTOR_USABLE_WORDS)) &&
            ((persistence_psu->output_request != 0) ||
             (persistence_psu->output_enable != 0)))
        {
            return GMP_TASK_DONE;
        }

        if (psu_flash_save(&pending_settings) != 0)
        {
            saved_settings = pending_settings;
            gmp_base_print("PSU settings saved to Flash.\r\n");
        }
        else
        {
            gmp_base_print("PSU Flash save/verify failed.\r\n");
        }
        pending_ticks = 0U;
    }

    return GMP_TASK_DONE;
}
