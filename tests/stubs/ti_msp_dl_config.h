#ifndef TEST_TI_MSP_DL_CONFIG_H_
#define TEST_TI_MSP_DL_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t unused;
} GPIO_Regs;

typedef struct {
    uint32_t unused;
} UART_Regs;

typedef struct {
    uint32_t unused;
} I2C_Regs;

typedef struct {
    uint32_t unused;
} GPTIMER_Regs;

typedef uint32_t DL_TIMER_CC_INDEX;

typedef enum {
    DL_I2C_CONTROLLER_DIRECTION_TX = 0,
    DL_I2C_CONTROLLER_DIRECTION_RX = 1
} DL_I2C_CONTROLLER_DIRECTION;

typedef enum {
    DL_I2C_CONTROLLER_START_DISABLE = 0,
    DL_I2C_CONTROLLER_START_ENABLE = 1
} DL_I2C_CONTROLLER_START;

typedef enum {
    DL_I2C_CONTROLLER_STOP_DISABLE = 0,
    DL_I2C_CONTROLLER_STOP_ENABLE = 1
} DL_I2C_CONTROLLER_STOP;

typedef enum {
    DL_I2C_CONTROLLER_ACK_DISABLE = 0,
    DL_I2C_CONTROLLER_ACK_ENABLE = 1
} DL_I2C_CONTROLLER_ACK;

extern GPIO_Regs gTestGPIOB;
extern GPIO_Regs gTestGPIOA;
extern UART_Regs gTestUART0;
extern UART_Regs gTestUART1;
extern UART_Regs gTestUART2;
extern I2C_Regs gTestI2C0;
extern GPTIMER_Regs gTestTIMA0;
extern GPTIMER_Regs gTestTIMA1;
extern GPTIMER_Regs gTestTIMG6;

#define TRACK3_PORT       (&gTestGPIOB)
#define TRACK3_OUT1_PIN   (1U << 18)
#define TRACK3_OUT2_PIN   (1U << 1)
#define TRACK3_OUT3_PIN   (1U << 10)
#define TRACK3_OUT4_PIN   (1U << 11)
#define TRACK3_OUT5_PIN   (1U << 14)

#define KEY_PORT          (&gTestGPIOB)
#define KEY_KEY1_PIN      (1U << 15)
#define KEY_KEY2_PIN      (1U << 16)
#define KEY_KEY3_PIN      (1U << 3)
#define KEY_KEY4_PIN      (1U << 8)

#define BUZZER_PORT       (&gTestGPIOB)
#define BUZZER_PIN_PIN    (1U << 6)
#define LED_PORT          (&gTestGPIOA)
#define LED_LED1_PIN      (1U << 8)
#define LED_LED2_PIN      (1U << 9)

#define RESV1_UART_INST   (&gTestUART0)
#define STEP1_UART_INST   (&gTestUART1)
#define BT_UART_INST      (&gTestUART2)
#define ICM_I2C_INST      (&gTestI2C0)
#define TB_PWM_INST       (&gTestTIMA1)
#define TB_PWM_INST_CLK_FREQ (32000000U)

#define ENCODER1_INST                 (&gTestTIMA0)
#define ENCODER1_INST_IRQHandler      TestEncoder1_IRQHandler
#define ENCODER1_INST_INT_IRQN        (10)
#define ENCODER2_INST                 (&gTestTIMG6)
#define ENCODER2_INST_IRQHandler      TestEncoder2_IRQHandler
#define ENCODER2_INST_INT_IRQN        (11)
#define ENCODER_DIR_PORT              (&gTestGPIOB)
#define ENCODER_DIR_DIR1_PIN          (1U << 22)
#define ENCODER_DIR_DIR2_PIN          (1U << 13)
#define DL_TIMER_IIDX_CC0_DN          (1U)

#define DL_TIMER_CC_0_INDEX (0U)
#define DL_TIMER_CC_1_INDEX (1U)

#define TB_DIR_AIN1_PORT (&gTestGPIOA)
#define TB_DIR_AIN1_PIN  (1U << 2)
#define TB_DIR_AIN2_PORT (&gTestGPIOB)
#define TB_DIR_AIN2_PIN  (1U << 21)
#define TB_DIR_BIN1_PORT (&gTestGPIOB)
#define TB_DIR_BIN1_PIN  (1U << 19)
#define TB_DIR_BIN2_PORT (&gTestGPIOB)
#define TB_DIR_BIN2_PIN  (1U << 17)

#define OLED_I2C_OLED_SCL_PORT (&gTestGPIOB)
#define OLED_I2C_OLED_SCL_PIN  (1U << 24)
#define OLED_I2C_OLED_SDA_PORT (&gTestGPIOA)
#define OLED_I2C_OLED_SDA_PIN  (1U << 24)

#define CPUCLK_FREQ                         (1000U)
#define DL_I2C_CONTROLLER_STATUS_BUSY       (1U << 0)
#define DL_I2C_CONTROLLER_STATUS_ERROR      (1U << 1)
#define DL_I2C_CONTROLLER_STATUS_IDLE       (1U << 2)
#define DL_I2C_CONTROLLER_STATUS_BUSY_BUS   (1U << 3)
#define DL_I2C_INTERRUPT_CONTROLLER_NACK    (1U << 4)
#define DL_I2C_INTERRUPT_CONTROLLER_STOP    (1U << 5)
#define DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST (1U << 6)
#define DL_I2C_INTERRUPT_TIMEOUT_A          (1U << 7)
#define DL_I2C_INTERRUPT_TIMEOUT_B          (1U << 8)

uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_enableOutput(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_disableOutput(GPIO_Regs *gpio, uint32_t pins);
void DL_UART_Main_transmitDataBlocking(UART_Regs *uart, uint8_t data);
bool DL_UART_Main_isRXFIFOEmpty(const UART_Regs *uart);
uint8_t DL_UART_Main_receiveData(const UART_Regs *uart);
uint32_t DL_TimerA_getLoadValue(const GPTIMER_Regs *timer);
uint32_t DL_TimerA_getPendingInterrupt(GPTIMER_Regs *timer);
void DL_TimerA_setCaptureCompareValue(
    GPTIMER_Regs *timer, uint32_t value, DL_TIMER_CC_INDEX index);
uint32_t DL_I2C_getControllerStatus(const I2C_Regs *i2c);
void DL_I2C_flushControllerTXFIFO(I2C_Regs *i2c);
void DL_I2C_flushControllerRXFIFO(I2C_Regs *i2c);
void DL_I2C_resetControllerTransfer(I2C_Regs *i2c);
void DL_I2C_enableStopCondition(I2C_Regs *i2c);
void DL_I2C_clearInterruptStatus(I2C_Regs *i2c, uint32_t mask);
uint16_t DL_I2C_fillControllerTXFIFO(
    I2C_Regs *i2c, const uint8_t *buffer, uint16_t count);
void DL_I2C_startControllerTransferAdvanced(I2C_Regs *i2c,
    uint32_t targetAddr, DL_I2C_CONTROLLER_DIRECTION direction,
    uint16_t length, DL_I2C_CONTROLLER_START start,
    DL_I2C_CONTROLLER_STOP stop, DL_I2C_CONTROLLER_ACK ack);
int DL_I2C_isControllerRXFIFOEmpty(const I2C_Regs *i2c);
uint8_t DL_I2C_receiveControllerData(const I2C_Regs *i2c);
void delay_cycles(uint32_t cycles);
uint32_t DL_TimerG_getPendingInterrupt(GPTIMER_Regs *timer);
void NVIC_ClearPendingIRQ(int irqn);
void NVIC_EnableIRQ(int irqn);

#endif /* TEST_TI_MSP_DL_CONFIG_H_ */
