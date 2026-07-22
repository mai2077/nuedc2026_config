/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gTB_PWMBackup;
DL_TimerG_backupConfig gENCODER2Backup;
DL_TimerA_backupConfig gENCODER1Backup;
DL_TimerG_backupConfig gTIMER_0Backup;
DL_UART_Main_backupConfig gRESV2_UARTBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_TB_PWM_init();
    SYSCFG_DL_ENCODER2_init();
    SYSCFG_DL_ENCODER1_init();
    SYSCFG_DL_TIMER_0_init();
    SYSCFG_DL_ICM_I2C_init();
    SYSCFG_DL_TRACK_I2C_init();
    SYSCFG_DL_RESV1_UART_init();
    SYSCFG_DL_STEP1_UART_init();
    SYSCFG_DL_BT_UART_init();
    SYSCFG_DL_RESV2_UART_init();
    /* Ensure backup structures have no valid state */
	gTB_PWMBackup.backupRdy 	= false;
	gENCODER2Backup.backupRdy 	= false;
	gENCODER1Backup.backupRdy 	= false;
	gTIMER_0Backup.backupRdy 	= false;
	gRESV2_UARTBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(TB_PWM_INST, &gTB_PWMBackup);
	retStatus &= DL_TimerG_saveConfiguration(ENCODER2_INST, &gENCODER2Backup);
	retStatus &= DL_TimerA_saveConfiguration(ENCODER1_INST, &gENCODER1Backup);
	retStatus &= DL_TimerG_saveConfiguration(TIMER_0_INST, &gTIMER_0Backup);
	retStatus &= DL_UART_Main_saveConfiguration(RESV2_UART_INST, &gRESV2_UARTBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(TB_PWM_INST, &gTB_PWMBackup, false);
	retStatus &= DL_TimerG_restoreConfiguration(ENCODER2_INST, &gENCODER2Backup, false);
	retStatus &= DL_TimerA_restoreConfiguration(ENCODER1_INST, &gENCODER1Backup, false);
	retStatus &= DL_TimerG_restoreConfiguration(TIMER_0_INST, &gTIMER_0Backup, false);
	retStatus &= DL_UART_Main_restoreConfiguration(RESV2_UART_INST, &gRESV2_UARTBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(TB_PWM_INST);
    DL_TimerG_reset(ENCODER2_INST);
    DL_TimerA_reset(ENCODER1_INST);
    DL_TimerG_reset(TIMER_0_INST);
    DL_I2C_reset(ICM_I2C_INST);
    DL_I2C_reset(TRACK_I2C_INST);
    DL_UART_Main_reset(RESV1_UART_INST);
    DL_UART_Main_reset(STEP1_UART_INST);
    DL_UART_Main_reset(BT_UART_INST);
    DL_UART_Main_reset(RESV2_UART_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(TB_PWM_INST);
    DL_TimerG_enablePower(ENCODER2_INST);
    DL_TimerA_enablePower(ENCODER1_INST);
    DL_TimerG_enablePower(TIMER_0_INST);
    DL_I2C_enablePower(ICM_I2C_INST);
    DL_I2C_enablePower(TRACK_I2C_INST);
    DL_UART_Main_enablePower(RESV1_UART_INST);
    DL_UART_Main_enablePower(STEP1_UART_INST);
    DL_UART_Main_enablePower(BT_UART_INST);
    DL_UART_Main_enablePower(RESV2_UART_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(GPIO_TB_PWM_C0_IOMUX,GPIO_TB_PWM_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_TB_PWM_C0_PORT, GPIO_TB_PWM_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_TB_PWM_C1_IOMUX,GPIO_TB_PWM_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_TB_PWM_C1_PORT, GPIO_TB_PWM_C1_PIN);

    DL_GPIO_initPeripheralInputFunction(GPIO_ENCODER2_C0_IOMUX,GPIO_ENCODER2_C0_IOMUX_FUNC);
    DL_GPIO_initPeripheralInputFunction(GPIO_ENCODER1_C0_IOMUX,GPIO_ENCODER1_C0_IOMUX_FUNC);

    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_ICM_I2C_IOMUX_SDA,
        GPIO_ICM_I2C_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_ICM_I2C_IOMUX_SCL,
        GPIO_ICM_I2C_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_ICM_I2C_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_ICM_I2C_IOMUX_SCL);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_TRACK_I2C_IOMUX_SDA,
        GPIO_TRACK_I2C_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_TRACK_I2C_IOMUX_SCL,
        GPIO_TRACK_I2C_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_TRACK_I2C_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_TRACK_I2C_IOMUX_SCL);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_RESV1_UART_IOMUX_TX, GPIO_RESV1_UART_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_RESV1_UART_IOMUX_RX, GPIO_RESV1_UART_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_STEP1_UART_IOMUX_TX, GPIO_STEP1_UART_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_STEP1_UART_IOMUX_RX, GPIO_STEP1_UART_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_BT_UART_IOMUX_TX, GPIO_BT_UART_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_BT_UART_IOMUX_RX, GPIO_BT_UART_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_RESV2_UART_IOMUX_TX, GPIO_RESV2_UART_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_RESV2_UART_IOMUX_RX, GPIO_RESV2_UART_IOMUX_RX_FUNC);

    DL_GPIO_initDigitalOutput(BT_EN_PIN_1_IOMUX);

    DL_GPIO_initDigitalOutput(BUZZER_PIN_IOMUX);

    DL_GPIO_initDigitalOutput(TB_DIR_AIN1_IOMUX);

    DL_GPIO_initDigitalOutput(TB_DIR_AIN2_IOMUX);

    DL_GPIO_initDigitalOutput(TB_DIR_BIN1_IOMUX);

    DL_GPIO_initDigitalOutput(TB_DIR_BIN2_IOMUX);

    DL_GPIO_initDigitalOutput(I2C0_SCL_IOMUX);

    DL_GPIO_initDigitalOutput(I2C0_SDA_IOMUX);

    DL_GPIO_initDigitalOutput(LED_LED1_IOMUX);

    DL_GPIO_initDigitalOutput(LED_LED2_IOMUX);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENCODER_DIR_DIR1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(ENCODER_DIR_DIR2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(TRACK_AD_AD0_IOMUX);

    DL_GPIO_initDigitalOutput(TRACK_AD_AD1_IOMUX);

    DL_GPIO_initDigitalOutput(TRACK_AD_AD2_IOMUX);

    DL_GPIO_initDigitalOutput(OLED_I2C_OLED_SCL_IOMUX);

    DL_GPIO_initDigitalOutput(OLED_I2C_OLED_SDA_IOMUX);

    DL_GPIO_initDigitalInputFeatures(TRACK3_OUT1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK3_OUT2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK3_OUT3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK3_OUT4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(TRACK3_OUT5_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearPins(GPIOA, TB_DIR_AIN1_PIN |
		I2C0_SCL_PIN |
		LED_LED1_PIN |
		LED_LED2_PIN |
		TRACK_AD_AD0_PIN);
    DL_GPIO_setPins(GPIOA, OLED_I2C_OLED_SDA_PIN);
    DL_GPIO_enableOutput(GPIOA, TB_DIR_AIN1_PIN |
		I2C0_SCL_PIN |
		LED_LED1_PIN |
		LED_LED2_PIN |
		TRACK_AD_AD0_PIN |
		OLED_I2C_OLED_SDA_PIN);
    DL_GPIO_clearPins(GPIOB, BT_EN_PIN_1_PIN |
		BUZZER_PIN_PIN |
		TB_DIR_AIN2_PIN |
		TB_DIR_BIN1_PIN |
		TB_DIR_BIN2_PIN |
		I2C0_SDA_PIN |
		TRACK_AD_AD1_PIN |
		TRACK_AD_AD2_PIN);
    DL_GPIO_setPins(GPIOB, OLED_I2C_OLED_SCL_PIN);
    DL_GPIO_enableOutput(GPIOB, BT_EN_PIN_1_PIN |
		BUZZER_PIN_PIN |
		TB_DIR_AIN2_PIN |
		TB_DIR_BIN1_PIN |
		TB_DIR_BIN2_PIN |
		I2C0_SDA_PIN |
		TRACK_AD_AD1_PIN |
		TRACK_AD_AD2_PIN |
		OLED_I2C_OLED_SCL_PIN);
    DL_GPIO_setLowerPinsPolarity(GPIOB, DL_GPIO_PIN_13_EDGE_RISE);
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_22_EDGE_RISE);

}


SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    /* Set default configuration */
    DL_SYSCTL_disableHFXT();
    DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_1);
    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);
    DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);
    DL_SYSCTL_enableMFPCLK();

}


/*
 * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gTB_PWMClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerA_PWMConfig gTB_PWMConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = 1000,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TB_PWM_init(void) {

    DL_TimerA_setClockConfig(
        TB_PWM_INST, (DL_TimerA_ClockConfig *) &gTB_PWMClockConfig);

    DL_TimerA_initPWMMode(
        TB_PWM_INST, (DL_TimerA_PWMConfig *) &gTB_PWMConfig);

    // Set Counter control to the smallest CC index being used
    DL_TimerA_setCounterControl(TB_PWM_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerA_setCaptureCompareOutCtl(TB_PWM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_0_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(TB_PWM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptureCompareValue(TB_PWM_INST, 0, DL_TIMER_CC_0_INDEX);

    DL_TimerA_setCaptureCompareOutCtl(TB_PWM_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_1_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(TB_PWM_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptureCompareValue(TB_PWM_INST, 0, DL_TIMER_CC_1_INDEX);

    DL_TimerA_enableClock(TB_PWM_INST);


    
    DL_TimerA_setCCPDirection(TB_PWM_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   4000000 Hz = 4000000 Hz / (8 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gENCODER2ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale = 0U
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * ENCODER2_INST_LOAD_VALUE = (10ms * 4000000 Hz) - 1
 */
static const DL_TimerG_CaptureConfig gENCODER2CaptureConfig = {
    .captureMode    = DL_TIMER_CAPTURE_MODE_EDGE_TIME,
    .period         = ENCODER2_INST_LOAD_VALUE,
    .startTimer     = DL_TIMER_START,
    .edgeCaptMode   = DL_TIMER_CAPTURE_EDGE_DETECTION_MODE_RISING,
    .inputChan      = DL_TIMER_INPUT_CHAN_0,
    .inputInvMode   = DL_TIMER_CC_INPUT_INV_NOINVERT,
};

SYSCONFIG_WEAK void SYSCFG_DL_ENCODER2_init(void) {

    DL_TimerG_setClockConfig(ENCODER2_INST,
        (DL_TimerG_ClockConfig *) &gENCODER2ClockConfig);

    DL_TimerG_initCaptureMode(ENCODER2_INST,
        (DL_TimerG_CaptureConfig *) &gENCODER2CaptureConfig);
    DL_TimerG_enableInterrupt(ENCODER2_INST , DL_TIMERG_INTERRUPT_CC0_DN_EVENT);

    NVIC_SetPriority(ENCODER2_INST_INT_IRQN, 0);
    DL_TimerG_enableClock(ENCODER2_INST);

}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   4000000 Hz = 4000000 Hz / (8 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gENCODER1ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale = 0U
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * ENCODER1_INST_LOAD_VALUE = (10ms * 4000000 Hz) - 1
 */
static const DL_TimerA_CaptureConfig gENCODER1CaptureConfig = {
    .captureMode    = DL_TIMER_CAPTURE_MODE_EDGE_TIME,
    .period         = ENCODER1_INST_LOAD_VALUE,
    .startTimer     = DL_TIMER_START,
    .edgeCaptMode   = DL_TIMER_CAPTURE_EDGE_DETECTION_MODE_RISING,
    .inputChan      = DL_TIMER_INPUT_CHAN_0,
    .inputInvMode   = DL_TIMER_CC_INPUT_INV_NOINVERT,
};

SYSCONFIG_WEAK void SYSCFG_DL_ENCODER1_init(void) {

    DL_TimerA_setClockConfig(ENCODER1_INST,
        (DL_TimerA_ClockConfig *) &gENCODER1ClockConfig);

    DL_TimerA_initCaptureMode(ENCODER1_INST,
        (DL_TimerA_CaptureConfig *) &gENCODER1CaptureConfig);
    DL_TimerA_enableInterrupt(ENCODER1_INST , DL_TIMERA_INTERRUPT_CC0_DN_EVENT);

    NVIC_SetPriority(ENCODER1_INST_INT_IRQN, 0);
    DL_TimerA_enableClock(ENCODER1_INST);

}


/*
 * Timer clock configuration to be sourced by BUSCLK /  (4000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   4000000 Hz = 4000000 Hz / (8 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_0ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_0_INST_LOAD_VALUE = (10ms * 4000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_0TimerConfig = {
    .period     = TIMER_0_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_0_init(void) {

    DL_TimerG_setClockConfig(TIMER_0_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_0ClockConfig);

    DL_TimerG_initTimerMode(TIMER_0_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_0TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_0_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
	NVIC_SetPriority(TIMER_0_INST_INT_IRQN, 1);
    DL_TimerG_enableClock(TIMER_0_INST);





}


static const DL_I2C_ClockConfig gICM_I2CClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_ICM_I2C_init(void) {

    DL_I2C_setClockConfig(ICM_I2C_INST,
        (DL_I2C_ClockConfig *) &gICM_I2CClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(ICM_I2C_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(ICM_I2C_INST);

    /* Configure Controller Mode */
    DL_I2C_resetControllerTransfer(ICM_I2C_INST);
    /* Set frequency to 100000 Hz*/
    DL_I2C_setTimerPeriod(ICM_I2C_INST, 31);
    DL_I2C_setControllerTXFIFOThreshold(ICM_I2C_INST, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(ICM_I2C_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(ICM_I2C_INST);


    /* Enable module */
    DL_I2C_enableController(ICM_I2C_INST);


}
static const DL_I2C_ClockConfig gTRACK_I2CClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_TRACK_I2C_init(void) {

    DL_I2C_setClockConfig(TRACK_I2C_INST,
        (DL_I2C_ClockConfig *) &gTRACK_I2CClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(TRACK_I2C_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(TRACK_I2C_INST);




}

static const DL_UART_Main_ClockConfig gRESV1_UARTClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gRESV1_UARTConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_RESV1_UART_init(void)
{
    DL_UART_Main_setClockConfig(RESV1_UART_INST, (DL_UART_Main_ClockConfig *) &gRESV1_UARTClockConfig);

    DL_UART_Main_init(RESV1_UART_INST, (DL_UART_Main_Config *) &gRESV1_UARTConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(RESV1_UART_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(RESV1_UART_INST, RESV1_UART_IBRD_32_MHZ_115200_BAUD, RESV1_UART_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(RESV1_UART_INST);
}
static const DL_UART_Main_ClockConfig gSTEP1_UARTClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gSTEP1_UARTConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_STEP1_UART_init(void)
{
    DL_UART_Main_setClockConfig(STEP1_UART_INST, (DL_UART_Main_ClockConfig *) &gSTEP1_UARTClockConfig);

    DL_UART_Main_init(STEP1_UART_INST, (DL_UART_Main_Config *) &gSTEP1_UARTConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(STEP1_UART_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(STEP1_UART_INST, STEP1_UART_IBRD_32_MHZ_115200_BAUD, STEP1_UART_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(STEP1_UART_INST);
}
static const DL_UART_Main_ClockConfig gBT_UARTClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gBT_UARTConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_BT_UART_init(void)
{
    DL_UART_Main_setClockConfig(BT_UART_INST, (DL_UART_Main_ClockConfig *) &gBT_UARTClockConfig);

    DL_UART_Main_init(BT_UART_INST, (DL_UART_Main_Config *) &gBT_UARTConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(BT_UART_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(BT_UART_INST, BT_UART_IBRD_32_MHZ_115200_BAUD, BT_UART_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(BT_UART_INST);
}
static const DL_UART_Main_ClockConfig gRESV2_UARTClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gRESV2_UARTConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_RESV2_UART_init(void)
{
    DL_UART_Main_setClockConfig(RESV2_UART_INST, (DL_UART_Main_ClockConfig *) &gRESV2_UARTClockConfig);

    DL_UART_Main_init(RESV2_UART_INST, (DL_UART_Main_Config *) &gRESV2_UARTConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(RESV2_UART_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(RESV2_UART_INST, RESV2_UART_IBRD_32_MHZ_115200_BAUD, RESV2_UART_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(RESV2_UART_INST);
}

