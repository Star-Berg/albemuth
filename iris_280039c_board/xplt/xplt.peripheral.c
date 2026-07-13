//
// THIS IS A DEMO SOURCE CODE FOR GMP LIBRARY.
//
// User should add all definitions of peripheral objects in this file.
//
// User should implement the peripheral objects initialization in setup_peripheral function.
//
// This file is platform-related.
//

// GMP basic core header
#include <gmp_core.h>

#include "user_main.h"
#include <xplt.peripheral.h>


//=================================================================================================
// definitions of peripheral

// inverter side voltage feedback
tri_ptr_adc_channel_t uuvw;
adc_gt uuvw_src[3];

// inverter side current feedback
tri_ptr_adc_channel_t iuvw;
adc_gt iuvw_src[3];

// grid side voltage feedback
tri_ptr_adc_channel_t vabc;
adc_gt vabc_src[3];

// grid side current feedback
tri_ptr_adc_channel_t iabc;
adc_gt iabc_src[3];

// DC bus current & voltage feedback
ptr_adc_channel_t udc;
adc_gt udc_src;
ptr_adc_channel_t idc;
adc_gt idc_src;

//=================================================================================================
// peripheral setup function

extern iic_halt iic_bus;
extern gpio_halt user_led;
extern gpio_halt gpio_beep;
extern gpio_halt gpio_alarm_led;
extern gpio_halt gpio_output_led;

/**
 * @brief Write the physical PSU output-switch command.
 *
 * EPWM7A is routed to GPIO28 / J3-17. Continuous software force makes
 * the ePWM pin behave as a static digital output without changing the
 * existing output-request, delay, fault, or CV/CC control logic.
 */
void psu_write_output_switch(fast_gt enable)
{
    EPWM_setActionQualifierContSWForceAction(
        PSU_OUTPUT_SW_EPWM_BASE,
        PSU_OUTPUT_SW_EPWM_OUTPUT,
        (enable != 0) ?
            EPWM_AQ_SW_OUTPUT_HIGH :
            EPWM_AQ_SW_OUTPUT_LOW);
}

//
// Function to configure I2C A in FIFO mode.
//
void initI2C()
{
    //
    // Must put I2C into reset before configuring it
    //
    I2C_disableModule(I2CA_BASE);

    //
    // I2C configuration. Use a 400kHz I2CCLK with a 33% duty cycle.
    //
    I2C_initController(I2CA_BASE, DEVICE_SYSCLK_FREQ, 400000, I2C_DUTYCYCLE_33);
    I2C_setBitCount(I2CA_BASE, I2C_BITCOUNT_8);
    I2C_setTargetAddress(I2CA_BASE, 0x70);
    I2C_setEmulationMode(I2CA_BASE, I2C_EMULATION_FREE_RUN);

    //
    // Enable stop condition and register-access-ready interrupts
    I2C_enableInterrupt(I2CA_BASE, I2C_INT_STOP_CONDITION |
                                   I2C_INT_REG_ACCESS_RDY);

    //
    // FIFO configuration
    //
    I2C_enableFIFO(I2CA_BASE);
    I2C_clearInterruptStatus(I2CA_BASE, I2C_INT_RXFF | I2C_INT_TXFF);

    //
    // Configuration complete. Enable the module.
    //
    I2C_enableModule(I2CA_BASE);
}



// User should setup all the peripheral in this function.
void setup_peripheral(void)
{
    // Safe startup state: disconnect Vsw and clear both analog references.
    psu_write_output_switch(0);
    DAC_setShadowValue(IRIS_DACA_BASE, 0U);
    DAC_setShadowValue(IRIS_DACB_BASE, 0U);

    // Setup Debug Uart
    debug_uart = IRIS_UART_USB_BASE;

    // Test print function
    gmp_base_print(TEXT_STRING("Hello World!\r\n"));
    asm(" RPT #255 || NOP");

    //
    // Initialize GPIOs for use as SDA A and SCL A respectively
    //
    // GPIO_setPinConfig(DEVICE_GPIO_CFG_SDAA);
    GPIO_setPadConfig(IRIS_IIC_I2CSDA_GPIO, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(IRIS_IIC_I2CSDA_GPIO, GPIO_QUAL_ASYNC);

    // GPIO_setPinConfig(DEVICE_GPIO_CFG_SCLA);
    GPIO_setPadConfig(IRIS_IIC_I2CSCL_GPIO, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(IRIS_IIC_I2CSCL_GPIO, GPIO_QUAL_ASYNC);

    GPIO_setPadConfig(IRIS_IIC_I2CSDA_GPIO, GPIO_PIN_TYPE_PULLUP);

    //
    // Set I2C use, initializing it for FIFO mode
    //
    initI2C();

    iic_bus = I2CA_BASE;

    user_led = SYSTEM_LED;

    gpio_beep = PSU_BEEP_PORT;
    gpio_alarm_led = PSU_ALARM_LED_PORT;
    gpio_output_led = PSU_OUTPUT_LED_PORT;

    // Keep all indicators in their safe startup states.
    gmp_hal_gpio_write(gpio_beep, PSU_BEEP_OFF_LEVEL);
    gmp_hal_gpio_write(gpio_alarm_led, PSU_ALARM_LED_OFF_LEVEL);
    gmp_hal_gpio_write(gpio_output_led, PSU_OUTPUT_LED_OFF_LEVEL);

}



//=================================================================================================
// ADC Interrupt ISR and controller related function

// ADC interrupt
interrupt void MainISR(void)
{
    //
    // call GMP ISR  Controller operation callback function
    //
    gmp_base_ctl_step();

    //
    // Call GMP Timer
    //
    gmp_step_system_tick();

    //
    // Clear the interrupt flag
    //
    ADC_clearInterruptStatus(IRIS_ADCA_BASE, ADC_INT_NUMBER1);

    //
    // Check if overflow has occurred
    //
    if (true == ADC_getInterruptOverflowStatus(IRIS_ADCA_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(IRIS_ADCA_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(IRIS_ADCA_BASE, ADC_INT_NUMBER1);
    }

    //
    // Acknowledge the interrupt
    //
    Interrupt_clearACKGroup(INT_IRIS_ADCA_1_INTERRUPT_ACK_GROUP);
}

void reset_controller(void)
{
    int i = 0;

    GPIO_writePin(PWM_RESET_PORT, 0U);

    for(i=0;i<10000;++i);

    GPIO_writePin(PWM_RESET_PORT, 1U);

}

//=================================================================================================
// communication functions and interrupt functions here

// 10000 -> 1.0
#define CAN_SCALE_FACTOR 10000

// 32 bit union
typedef union {
    int32_t i32;
    uint16_t u16[2]; // C2000uint16_tռ1word32λռ2word
} can_data_t;

// CAN interrupt
interrupt void INT_IRIS_CAN_0_ISR(void)
{
    uint32_t status = CAN_getInterruptCause(IRIS_CAN_BASE);

    uint16_t rx_data[4];
    can_data_t recv_content[2];

    if (status == 1)
    {
        CAN_readMessage(IRIS_CAN_BASE, 1, rx_data);
        CAN_clearInterruptStatus(CANA_BASE, 1);

        // Control Flag, Enable System
//        if (rx_data[0] == 1)
//        {
//            cia402_send_cmd(&cia402_sm, CIA402_CMD_ENABLE_OPERATION);
//        }
//        if (rx_data[0] == 0)
//        {
//            cia402_send_cmd(&cia402_sm, CIA402_CMD_DISABLE_VOLTAGE);
//        }
    }
    else if (status == 2)
    {
        CAN_readMessage(IRIS_CAN_BASE, 2, (uint16_t*)recv_content);
        CAN_clearInterruptStatus(CANA_BASE, 2);

        // set target value
#if BUILD_LEVEL == 1
        // For level 1 Set target voltage
        ctl_set_gfl_inv_voltage_openloop(&inv_ctrl, float2ctrl((float)recv_content[0].i32 / CAN_SCALE_FACTOR),
                                         float2ctrl((float)recv_content[1].i32 / CAN_SCALE_FACTOR));

#endif // BUILD_LEVEL
    }

    //
    // Clear the interrupt flag
    //
    CAN_clearGlobalInterruptStatus(IRIS_CAN_BASE, CAN_GLOBAL_INT_CANINT0);

    //
    // Acknowledge the interrupt
    //
    Interrupt_clearACKGroup(INT_IRIS_CAN_0_INTERRUPT_ACK_GROUP);
}

interrupt void INT_IRIS_CAN_1_ISR(void)
{
    // Nothing here

    //
    // Clear the interrupt flag
    //
    CAN_clearGlobalInterruptStatus(IRIS_CAN_BASE, CAN_GLOBAL_INT_CANINT1);

    //
    // Acknowledge the interrupt
    //
    Interrupt_clearACKGroup(INT_IRIS_CAN_1_INTERRUPT_ACK_GROUP);
}

#if BOARD_SELECTION == GMP_IRIS

interrupt void INT_IRIS_UART_RS232_RX_ISR(void)
{
    // Nothing here

    //
    // Acknowledge the interrupt
    //
    Interrupt_clearACKGroup(INT_IRIS_UART_RS232_RX_INTERRUPT_ACK_GROUP);
}

#endif // BOARD_SELECTION == GMP_IRIS

//=================================================================================================
// Debug interface

// a local small cache size, capable of covering the depth of the hardware FIFO (typically 16 bytes)
#define ISR_LOCAL_BUF_SIZE 16

extern gmp_datalink_t dl;

void flush_dl_tx_buffer(void)
{
    // Send head
    gmp_hal_uart_write(IRIS_UART_USB_BASE, gmp_dev_dl_get_tx_hw_hdr_ptr(&dl), gmp_dev_dl_get_tx_hw_hdr_size(&dl), 10);

    // Send data body, if necessary
    if (gmp_dev_dl_get_tx_hw_pld_size(&dl) > 0)
    {
        gmp_hal_uart_write(IRIS_UART_USB_BASE, gmp_dev_dl_get_tx_hw_pld_ptr(&dl), gmp_dev_dl_get_tx_hw_pld_size(&dl),
                           10);
    }
}

void flush_dl_rx_buffer(void)
{
    uint16_t fifoLevel;
    data_gt rxBuf[ISR_LOCAL_BUF_SIZE];

    // read all FIFO messages
    fifoLevel = SCI_getRxFIFOStatus(IRIS_UART_USB_BASE);

    if (fifoLevel > 0)
    {
        SCI_readCharArray(IRIS_UART_USB_BASE, (uint16_t*)rxBuf, fifoLevel);

        // Lock-free ring queue pushed into the protocol stack (very fast, O(1))
        gmp_dev_dl_push_str(&dl, rxBuf, fifoLevel);
    }
}

interrupt void INT_IRIS_UART_USB_RX_ISR(void)
{
    flush_dl_rx_buffer();

    //
    // deal with overrun
    //
    if (SCI_getRxStatus(IRIS_UART_USB_BASE) & SCI_RXSTATUS_OVERRUN)
    {
        SCI_clearOverflowStatus(IRIS_UART_USB_BASE);
    }

    //
    // Clear interrupt flags
    //
    SCI_clearInterruptStatus(IRIS_UART_USB_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INT_IRIS_UART_USB_RX_INTERRUPT_ACK_GROUP);
}

////


//=========================================================
// 1. SPI дײ㺯װ
//=========================================================

//  FPGA дĴ
// Э: ֡1=[15λ=1(д), 14:8=ַ, 7:0=] -> ֡2=[16λ]
void SPI_writeReg(uint16_t addr, uint16_t data)
{
    // дλΪ 0
    uint16_t cmd = 0x0000 | ((addr & 0x7F) << 8); // λȻ 0

    //  16-bit word ѹ TX FIFO 
    SPI_writeDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE, cmd);
    SPI_writeDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE, data);

    // ȴ FPGA ղ 16-bit word
    // Ȼд SPI ȫ˫յԷصķ
    while(SPI_getRxFIFOStatus(IRIS_SPI_FPGA_BRIDGE_BASE) < SPI_FIFO_RX2);

    // ѽյݶ RX FIFOֹӰͨ
    SPI_readDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE);
    SPI_readDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE);
}

//  FPGA ȡĴ
// Э: ֡1=[15λ=0(), 14:8=ַ, 7:0=] -> ֡2=[16λռλ(0x0000)]
uint16_t SPI_readReg(uint16_t addr)
{
    // λΪ 1
    uint16_t cmd = 0x8000 | ((addr & 0x7F) << 8); // ǿưλ
    uint16_t dummy_data = 0x0000; // ڲʱӵ

    // ѹ֡֡
    SPI_writeDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE, cmd);
    SPI_writeDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE, dummy_data);

    // ȴ 2 
    while(SPI_getRxFIFOStatus(IRIS_SPI_FPGA_BRIDGE_BASE) < SPI_FIFO_RX2);

    // ĵһǷ֡ʱ FPGA صģͨ״̬λȫ0ֱӶ
    SPI_readDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE);

    // ĵڶֲҪʵ֡
    uint16_t read_data = SPI_readDataBlockingFIFO(IRIS_SPI_FPGA_BRIDGE_BASE);

    return read_data;
}
