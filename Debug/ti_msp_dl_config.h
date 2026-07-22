/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for TB_PWM */
#define TB_PWM_INST                                                        TIMA1
#define TB_PWM_INST_IRQHandler                                  TIMA1_IRQHandler
#define TB_PWM_INST_INT_IRQN                                    (TIMA1_INT_IRQn)
#define TB_PWM_INST_CLK_FREQ                                            32000000
/* GPIO defines for channel 0 */
#define GPIO_TB_PWM_C0_PORT                                                GPIOB
#define GPIO_TB_PWM_C0_PIN                                         DL_GPIO_PIN_0
#define GPIO_TB_PWM_C0_IOMUX                                     (IOMUX_PINCM12)
#define GPIO_TB_PWM_C0_IOMUX_FUNC                    IOMUX_PINCM12_PF_TIMA1_CCP0
#define GPIO_TB_PWM_C0_IDX                                   DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_TB_PWM_C1_PORT                                                GPIOA
#define GPIO_TB_PWM_C1_PIN                                        DL_GPIO_PIN_16
#define GPIO_TB_PWM_C1_IOMUX                                     (IOMUX_PINCM38)
#define GPIO_TB_PWM_C1_IOMUX_FUNC                    IOMUX_PINCM38_PF_TIMA1_CCP1
#define GPIO_TB_PWM_C1_IDX                                   DL_TIMER_CC_1_INDEX



/* Defines for ENCODER2 */
#define ENCODER2_INST                                                    (TIMG6)
#define ENCODER2_INST_IRQHandler                                TIMG6_IRQHandler
#define ENCODER2_INST_INT_IRQN                                  (TIMG6_INT_IRQn)
#define ENCODER2_INST_LOAD_VALUE                                        (39999U)
/* GPIO defines for channel 0 */
#define GPIO_ENCODER2_C0_PORT                                              GPIOB
#define GPIO_ENCODER2_C0_PIN                                       DL_GPIO_PIN_2
#define GPIO_ENCODER2_C0_IOMUX                                   (IOMUX_PINCM15)
#define GPIO_ENCODER2_C0_IOMUX_FUNC                  IOMUX_PINCM15_PF_TIMG6_CCP0

/* Defines for ENCODER1 */
#define ENCODER1_INST                                                    (TIMA0)
#define ENCODER1_INST_IRQHandler                                TIMA0_IRQHandler
#define ENCODER1_INST_INT_IRQN                                  (TIMA0_INT_IRQn)
#define ENCODER1_INST_LOAD_VALUE                                        (39999U)
/* GPIO defines for channel 0 */
#define GPIO_ENCODER1_C0_PORT                                              GPIOA
#define GPIO_ENCODER1_C0_PIN                                       DL_GPIO_PIN_0
#define GPIO_ENCODER1_C0_IOMUX                                    (IOMUX_PINCM1)
#define GPIO_ENCODER1_C0_IOMUX_FUNC                   IOMUX_PINCM1_PF_TIMA0_CCP0





/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMG7)
#define TIMER_0_INST_IRQHandler                                 TIMG7_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMG7_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                         (39999U)




/* Defines for ICM_I2C */
#define ICM_I2C_INST                                                        I2C0
#define ICM_I2C_INST_IRQHandler                                  I2C0_IRQHandler
#define ICM_I2C_INST_INT_IRQN                                      I2C0_INT_IRQn
#define ICM_I2C_BUS_SPEED_HZ                                              100000
#define GPIO_ICM_I2C_SDA_PORT                                              GPIOA
#define GPIO_ICM_I2C_SDA_PIN                                      DL_GPIO_PIN_28
#define GPIO_ICM_I2C_IOMUX_SDA                                    (IOMUX_PINCM3)
#define GPIO_ICM_I2C_IOMUX_SDA_FUNC                     IOMUX_PINCM3_PF_I2C0_SDA
#define GPIO_ICM_I2C_SCL_PORT                                              GPIOA
#define GPIO_ICM_I2C_SCL_PIN                                      DL_GPIO_PIN_31
#define GPIO_ICM_I2C_IOMUX_SCL                                    (IOMUX_PINCM6)
#define GPIO_ICM_I2C_IOMUX_SCL_FUNC                     IOMUX_PINCM6_PF_I2C0_SCL

/* Defines for TRACK_I2C */
#define TRACK_I2C_INST                                                      I2C1
#define TRACK_I2C_INST_IRQHandler                                I2C1_IRQHandler
#define TRACK_I2C_INST_INT_IRQN                                    I2C1_INT_IRQn
#define GPIO_TRACK_I2C_SDA_PORT                                            GPIOA
#define GPIO_TRACK_I2C_SDA_PIN                                    DL_GPIO_PIN_30
#define GPIO_TRACK_I2C_IOMUX_SDA                                  (IOMUX_PINCM5)
#define GPIO_TRACK_I2C_IOMUX_SDA_FUNC                   IOMUX_PINCM5_PF_I2C1_SDA
#define GPIO_TRACK_I2C_SCL_PORT                                            GPIOA
#define GPIO_TRACK_I2C_SCL_PIN                                    DL_GPIO_PIN_29
#define GPIO_TRACK_I2C_IOMUX_SCL                                  (IOMUX_PINCM4)
#define GPIO_TRACK_I2C_IOMUX_SCL_FUNC                   IOMUX_PINCM4_PF_I2C1_SCL


/* Defines for RESV1_UART */
#define RESV1_UART_INST                                                    UART0
#define RESV1_UART_INST_FREQUENCY                                       32000000
#define RESV1_UART_INST_IRQHandler                              UART0_IRQHandler
#define RESV1_UART_INST_INT_IRQN                                  UART0_INT_IRQn
#define GPIO_RESV1_UART_RX_PORT                                            GPIOA
#define GPIO_RESV1_UART_TX_PORT                                            GPIOA
#define GPIO_RESV1_UART_RX_PIN                                    DL_GPIO_PIN_11
#define GPIO_RESV1_UART_TX_PIN                                    DL_GPIO_PIN_10
#define GPIO_RESV1_UART_IOMUX_RX                                 (IOMUX_PINCM22)
#define GPIO_RESV1_UART_IOMUX_TX                                 (IOMUX_PINCM21)
#define GPIO_RESV1_UART_IOMUX_RX_FUNC                  IOMUX_PINCM22_PF_UART0_RX
#define GPIO_RESV1_UART_IOMUX_TX_FUNC                  IOMUX_PINCM21_PF_UART0_TX
#define RESV1_UART_BAUD_RATE                                            (115200)
#define RESV1_UART_IBRD_32_MHZ_115200_BAUD                                  (17)
#define RESV1_UART_FBRD_32_MHZ_115200_BAUD                                  (23)
/* Defines for STEP1_UART */
#define STEP1_UART_INST                                                    UART1
#define STEP1_UART_INST_FREQUENCY                                       32000000
#define STEP1_UART_INST_IRQHandler                              UART1_IRQHandler
#define STEP1_UART_INST_INT_IRQN                                  UART1_INT_IRQn
#define GPIO_STEP1_UART_RX_PORT                                            GPIOB
#define GPIO_STEP1_UART_TX_PORT                                            GPIOB
#define GPIO_STEP1_UART_RX_PIN                                     DL_GPIO_PIN_5
#define GPIO_STEP1_UART_TX_PIN                                     DL_GPIO_PIN_4
#define GPIO_STEP1_UART_IOMUX_RX                                 (IOMUX_PINCM18)
#define GPIO_STEP1_UART_IOMUX_TX                                 (IOMUX_PINCM17)
#define GPIO_STEP1_UART_IOMUX_RX_FUNC                  IOMUX_PINCM18_PF_UART1_RX
#define GPIO_STEP1_UART_IOMUX_TX_FUNC                  IOMUX_PINCM17_PF_UART1_TX
#define STEP1_UART_BAUD_RATE                                            (115200)
#define STEP1_UART_IBRD_32_MHZ_115200_BAUD                                  (17)
#define STEP1_UART_FBRD_32_MHZ_115200_BAUD                                  (23)
/* Defines for BT_UART */
#define BT_UART_INST                                                       UART2
#define BT_UART_INST_FREQUENCY                                          32000000
#define BT_UART_INST_IRQHandler                                 UART2_IRQHandler
#define BT_UART_INST_INT_IRQN                                     UART2_INT_IRQn
#define GPIO_BT_UART_RX_PORT                                               GPIOA
#define GPIO_BT_UART_TX_PORT                                               GPIOA
#define GPIO_BT_UART_RX_PIN                                       DL_GPIO_PIN_22
#define GPIO_BT_UART_TX_PIN                                       DL_GPIO_PIN_21
#define GPIO_BT_UART_IOMUX_RX                                    (IOMUX_PINCM47)
#define GPIO_BT_UART_IOMUX_TX                                    (IOMUX_PINCM46)
#define GPIO_BT_UART_IOMUX_RX_FUNC                     IOMUX_PINCM47_PF_UART2_RX
#define GPIO_BT_UART_IOMUX_TX_FUNC                     IOMUX_PINCM46_PF_UART2_TX
#define BT_UART_BAUD_RATE                                               (115200)
#define BT_UART_IBRD_32_MHZ_115200_BAUD                                     (17)
#define BT_UART_FBRD_32_MHZ_115200_BAUD                                     (23)
/* Defines for RESV2_UART */
#define RESV2_UART_INST                                                    UART3
#define RESV2_UART_INST_FREQUENCY                                       32000000
#define RESV2_UART_INST_IRQHandler                              UART3_IRQHandler
#define RESV2_UART_INST_INT_IRQN                                  UART3_INT_IRQn
#define GPIO_RESV2_UART_RX_PORT                                            GPIOA
#define GPIO_RESV2_UART_TX_PORT                                            GPIOA
#define GPIO_RESV2_UART_RX_PIN                                    DL_GPIO_PIN_25
#define GPIO_RESV2_UART_TX_PIN                                    DL_GPIO_PIN_26
#define GPIO_RESV2_UART_IOMUX_RX                                 (IOMUX_PINCM55)
#define GPIO_RESV2_UART_IOMUX_TX                                 (IOMUX_PINCM59)
#define GPIO_RESV2_UART_IOMUX_RX_FUNC                  IOMUX_PINCM55_PF_UART3_RX
#define GPIO_RESV2_UART_IOMUX_TX_FUNC                  IOMUX_PINCM59_PF_UART3_TX
#define RESV2_UART_BAUD_RATE                                            (115200)
#define RESV2_UART_IBRD_32_MHZ_115200_BAUD                                  (17)
#define RESV2_UART_FBRD_32_MHZ_115200_BAUD                                  (23)





/* Port definition for Pin Group BT_EN */
#define BT_EN_PORT                                                       (GPIOB)

/* Defines for PIN_1: GPIOB.12 with pinCMx 29 on package pin 64 */
#define BT_EN_PIN_1_PIN                                         (DL_GPIO_PIN_12)
#define BT_EN_PIN_1_IOMUX                                        (IOMUX_PINCM29)
/* Port definition for Pin Group BUZZER */
#define BUZZER_PORT                                                      (GPIOB)

/* Defines for PIN: GPIOB.6 with pinCMx 23 on package pin 58 */
#define BUZZER_PIN_PIN                                           (DL_GPIO_PIN_6)
#define BUZZER_PIN_IOMUX                                         (IOMUX_PINCM23)
/* Defines for AIN1: GPIOA.2 with pinCMx 7 on package pin 42 */
#define TB_DIR_AIN1_PORT                                                 (GPIOA)
#define TB_DIR_AIN1_PIN                                          (DL_GPIO_PIN_2)
#define TB_DIR_AIN1_IOMUX                                         (IOMUX_PINCM7)
/* Defines for AIN2: GPIOB.21 with pinCMx 49 on package pin 20 */
#define TB_DIR_AIN2_PORT                                                 (GPIOB)
#define TB_DIR_AIN2_PIN                                         (DL_GPIO_PIN_21)
#define TB_DIR_AIN2_IOMUX                                        (IOMUX_PINCM49)
/* Defines for BIN1: GPIOB.19 with pinCMx 45 on package pin 16 */
#define TB_DIR_BIN1_PORT                                                 (GPIOB)
#define TB_DIR_BIN1_PIN                                         (DL_GPIO_PIN_19)
#define TB_DIR_BIN1_IOMUX                                        (IOMUX_PINCM45)
/* Defines for BIN2: GPIOB.17 with pinCMx 43 on package pin 14 */
#define TB_DIR_BIN2_PORT                                                 (GPIOB)
#define TB_DIR_BIN2_PIN                                         (DL_GPIO_PIN_17)
#define TB_DIR_BIN2_IOMUX                                        (IOMUX_PINCM43)
/* Defines for SCL: GPIOA.13 with pinCMx 35 on package pin 6 */
#define I2C0_SCL_PORT                                                    (GPIOA)
#define I2C0_SCL_PIN                                            (DL_GPIO_PIN_13)
#define I2C0_SCL_IOMUX                                           (IOMUX_PINCM35)
/* Defines for SDA: GPIOB.26 with pinCMx 57 on package pin 28 */
#define I2C0_SDA_PORT                                                    (GPIOB)
#define I2C0_SDA_PIN                                            (DL_GPIO_PIN_26)
#define I2C0_SDA_IOMUX                                           (IOMUX_PINCM57)
/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOA)

/* Defines for LED1: GPIOA.8 with pinCMx 19 on package pin 54 */
#define LED_LED1_PIN                                             (DL_GPIO_PIN_8)
#define LED_LED1_IOMUX                                           (IOMUX_PINCM19)
/* Defines for LED2: GPIOA.9 with pinCMx 20 on package pin 55 */
#define LED_LED2_PIN                                             (DL_GPIO_PIN_9)
#define LED_LED2_IOMUX                                           (IOMUX_PINCM20)
/* Port definition for Pin Group KEY */
#define KEY_PORT                                                         (GPIOB)

/* Defines for KEY1: GPIOB.15 with pinCMx 32 on package pin 3 */
#define KEY_KEY1_PIN                                            (DL_GPIO_PIN_15)
#define KEY_KEY1_IOMUX                                           (IOMUX_PINCM32)
/* Defines for KEY2: GPIOB.16 with pinCMx 33 on package pin 4 */
#define KEY_KEY2_PIN                                            (DL_GPIO_PIN_16)
#define KEY_KEY2_IOMUX                                           (IOMUX_PINCM33)
/* Defines for KEY3: GPIOB.3 with pinCMx 16 on package pin 51 */
#define KEY_KEY3_PIN                                             (DL_GPIO_PIN_3)
#define KEY_KEY3_IOMUX                                           (IOMUX_PINCM16)
/* Defines for KEY4: GPIOB.8 with pinCMx 25 on package pin 60 */
#define KEY_KEY4_PIN                                             (DL_GPIO_PIN_8)
#define KEY_KEY4_IOMUX                                           (IOMUX_PINCM25)
/* Port definition for Pin Group ENCODER_DIR */
#define ENCODER_DIR_PORT                                                 (GPIOB)

/* Defines for DIR1: GPIOB.22 with pinCMx 50 on package pin 21 */
#define ENCODER_DIR_DIR1_PIN                                    (DL_GPIO_PIN_22)
#define ENCODER_DIR_DIR1_IOMUX                                   (IOMUX_PINCM50)
/* Defines for DIR2: GPIOB.13 with pinCMx 30 on package pin 1 */
#define ENCODER_DIR_DIR2_PIN                                    (DL_GPIO_PIN_13)
#define ENCODER_DIR_DIR2_IOMUX                                   (IOMUX_PINCM30)
/* Defines for AD0: GPIOA.14 with pinCMx 36 on package pin 7 */
#define TRACK_AD_AD0_PORT                                                (GPIOA)
#define TRACK_AD_AD0_PIN                                        (DL_GPIO_PIN_14)
#define TRACK_AD_AD0_IOMUX                                       (IOMUX_PINCM36)
/* Defines for AD1: GPIOB.20 with pinCMx 48 on package pin 19 */
#define TRACK_AD_AD1_PORT                                                (GPIOB)
#define TRACK_AD_AD1_PIN                                        (DL_GPIO_PIN_20)
#define TRACK_AD_AD1_IOMUX                                       (IOMUX_PINCM48)
/* Defines for AD2: GPIOB.25 with pinCMx 56 on package pin 27 */
#define TRACK_AD_AD2_PORT                                                (GPIOB)
#define TRACK_AD_AD2_PIN                                        (DL_GPIO_PIN_25)
#define TRACK_AD_AD2_IOMUX                                       (IOMUX_PINCM56)
/* Defines for OLED_SCL: GPIOB.24 with pinCMx 52 on package pin 23 */
#define OLED_I2C_OLED_SCL_PORT                                           (GPIOB)
#define OLED_I2C_OLED_SCL_PIN                                   (DL_GPIO_PIN_24)
#define OLED_I2C_OLED_SCL_IOMUX                                  (IOMUX_PINCM52)
/* Defines for OLED_SDA: GPIOA.24 with pinCMx 54 on package pin 25 */
#define OLED_I2C_OLED_SDA_PORT                                           (GPIOA)
#define OLED_I2C_OLED_SDA_PIN                                   (DL_GPIO_PIN_24)
#define OLED_I2C_OLED_SDA_IOMUX                                  (IOMUX_PINCM54)
/* Port definition for Pin Group TRACK3 */
#define TRACK3_PORT                                                      (GPIOB)

/* Defines for OUT1: GPIOB.18 with pinCMx 44 on package pin 15 */
#define TRACK3_OUT1_PIN                                         (DL_GPIO_PIN_18)
#define TRACK3_OUT1_IOMUX                                        (IOMUX_PINCM44)
/* Defines for OUT2: GPIOB.1 with pinCMx 13 on package pin 48 */
#define TRACK3_OUT2_PIN                                          (DL_GPIO_PIN_1)
#define TRACK3_OUT2_IOMUX                                        (IOMUX_PINCM13)
/* Defines for OUT3: GPIOB.10 with pinCMx 27 on package pin 62 */
#define TRACK3_OUT3_PIN                                         (DL_GPIO_PIN_10)
#define TRACK3_OUT3_IOMUX                                        (IOMUX_PINCM27)
/* Defines for OUT4: GPIOB.11 with pinCMx 28 on package pin 63 */
#define TRACK3_OUT4_PIN                                         (DL_GPIO_PIN_11)
#define TRACK3_OUT4_IOMUX                                        (IOMUX_PINCM28)
/* Defines for OUT5: GPIOB.14 with pinCMx 31 on package pin 2 */
#define TRACK3_OUT5_PIN                                         (DL_GPIO_PIN_14)
#define TRACK3_OUT5_IOMUX                                        (IOMUX_PINCM31)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TB_PWM_init(void);
void SYSCFG_DL_ENCODER2_init(void);
void SYSCFG_DL_ENCODER1_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_ICM_I2C_init(void);
void SYSCFG_DL_TRACK_I2C_init(void);
void SYSCFG_DL_RESV1_UART_init(void);
void SYSCFG_DL_STEP1_UART_init(void);
void SYSCFG_DL_BT_UART_init(void);
void SYSCFG_DL_RESV2_UART_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
