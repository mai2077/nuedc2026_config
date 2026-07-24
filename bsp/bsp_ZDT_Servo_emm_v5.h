#ifndef BSP_ZDT_SERVO_EMM_V5_H_
#define BSP_ZDT_SERVO_EMM_V5_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_EMM_V5_FRAME_END             (0x6BU)
#define BSP_EMM_V5_QPOS_REL_LAST         (0U)
#define BSP_EMM_V5_QPOS_ABSOLUTE         (1U)
#define BSP_EMM_V5_QPOS_REL_CURRENT      (2U)
#define BSP_EMM_V5_DIR_CW                (0U)
#define BSP_EMM_V5_DIR_CCW               (1U)
#define BSP_EMM_V5_POS_RELATIVE          (0U)
#define BSP_EMM_V5_POS_ABSOLUTE          (1U)
#define BSP_ZDT_SERVO_MAX_FRAME_LENGTH   (13U)

/* Compatibility aliases retained for code written against the first port. */
#define BSP_EMM_V5_DIRECTION_CW          BSP_EMM_V5_DIR_CW
#define BSP_EMM_V5_DIRECTION_CCW         BSP_EMM_V5_DIR_CCW
#define BSP_EMM_V5_POSITION_RELATIVE     BSP_EMM_V5_POS_RELATIVE
#define BSP_EMM_V5_POSITION_ABSOLUTE     BSP_EMM_V5_POS_ABSOLUTE

typedef enum {
    BSP_ZDT_SERVO_OK = 0,
    BSP_ZDT_SERVO_INVALID_ARGUMENT,
    BSP_ZDT_SERVO_TX_ERROR
} BSP_ZDT_Servo_Status;

typedef enum {
    BSP_EMM_V5_SYS_VBUS = 0,
    BSP_EMM_V5_SYS_CBUS,
    BSP_EMM_V5_SYS_VEL,
    BSP_EMM_V5_SYS_CPOS
} BSPEmmV5SysParam;

size_t BSP_EmmV5_Build_SetQPosParams(uint8_t addr, uint16_t vel,
    uint8_t acc, uint8_t raF, bool snF, uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_QPosControl(uint8_t addr, int32_t clk,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_VelControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, bool snF, uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_PosControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, uint32_t clk, uint8_t raF, bool snF,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_StopNow(uint8_t addr, bool snF,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_SynchronousMotion(uint8_t addr,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_ReadSysParam(uint8_t addr,
    BSPEmmV5SysParam param, uint8_t *buf, size_t bufSize);

BSP_ZDT_Servo_Status BSP_EmmV5_SetQPosParams(uint8_t addr,
    uint16_t vel, uint8_t acc, uint8_t raF, bool snF);
BSP_ZDT_Servo_Status BSP_EmmV5_QPosControl(uint8_t addr, int32_t clk);
BSP_ZDT_Servo_Status BSP_EmmV5_VelControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, bool snF);
BSP_ZDT_Servo_Status BSP_EmmV5_PosControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, uint32_t clk, uint8_t raF, bool snF);
BSP_ZDT_Servo_Status BSP_EmmV5_StopNow(uint8_t addr, bool snF);
BSP_ZDT_Servo_Status BSP_EmmV5_SynchronousMotion(uint8_t addr);
BSP_ZDT_Servo_Status BSP_EmmV5_ReadSysParam(
    uint8_t addr, BSPEmmV5SysParam param);

size_t BSP_ZDT_Servo_getLastTxFrame(uint8_t *buffer, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ZDT_SERVO_EMM_V5_H_ */
