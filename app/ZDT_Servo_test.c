#include "ZDT_Servo_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "bsp/bsp_ZDT_Servo_emm_v5.h"
#include "bsp/key.h"
#include "ti_msp_dl_config.h"

#define ZDT_SERVO_TEST_ACTION_COUNT (8U)
#define ZDT_SERVO_DEGREES_PER_REV   (360L)

static uint8_t gNextAction;

BSP_ZDT_Servo_Status ZDT_Servo_rotateDegrees(int16_t angleDegrees)
{
    int32_t scaledPulses;
    int32_t positionPulses;
    BSP_ZDT_Servo_Status status;

    if ((angleDegrees < -ZDT_SERVO_ANGLE_LIMIT_DEG) ||
        (angleDegrees > ZDT_SERVO_ANGLE_LIMIT_DEG)) {
        return BSP_ZDT_SERVO_INVALID_ARGUMENT;
    }
    if (angleDegrees == 0) {
        return BSP_ZDT_SERVO_OK;
    }

    scaledPulses = (int32_t)angleDegrees * ZDT_SERVO_PULSES_PER_REV;
    scaledPulses += (scaledPulses > 0) ?
        (ZDT_SERVO_DEGREES_PER_REV / 2L) :
        -(ZDT_SERVO_DEGREES_PER_REV / 2L);
    positionPulses = scaledPulses / ZDT_SERVO_DEGREES_PER_REV;

    status = BSP_EmmV5_SetQPosParams(ZDT_SERVO_TEST_ADDRESS,
        ZDT_SERVO_TEST_SPEED_RPM, ZDT_SERVO_TEST_ACCEL,
        BSP_EMM_V5_QPOS_REL_CURRENT, false);
    if (status != BSP_ZDT_SERVO_OK) {
        return status;
    }

    delay_cycles(CPUCLK_FREQ / 1000U);
    return BSP_EmmV5_QPosControl(
        ZDT_SERVO_TEST_ADDRESS, positionPulses);
}

static void runAction(uint8_t action)
{
    switch (action) {
        case 0U:
            (void)BSP_EmmV5_VelControl(ZDT_SERVO_TEST_ADDRESS,
                BSP_EMM_V5_DIR_CW, ZDT_SERVO_TEST_SPEED_RPM,
                ZDT_SERVO_TEST_ACCEL, false);
            break;
        case 1U:
        case 3U:
        case 5U:
        case 7U:
            (void)BSP_EmmV5_StopNow(ZDT_SERVO_TEST_ADDRESS, false);
            break;
        case 2U:
            (void)BSP_EmmV5_VelControl(ZDT_SERVO_TEST_ADDRESS,
                BSP_EMM_V5_DIR_CCW, ZDT_SERVO_TEST_SPEED_RPM,
                ZDT_SERVO_TEST_ACCEL, false);
            break;
        case 4U:
            (void)ZDT_Servo_rotateDegrees(360);
            break;
        case 6U:
            (void)ZDT_Servo_rotateDegrees(-360);
            break;
        default:
            break;
    }
}

void ZDT_Servo_Test_init(void)
{
    gNextAction = 0U;
    (void)BSP_EmmV5_StopNow(ZDT_SERVO_TEST_ADDRESS, false);
}

void ZDT_Servo_Test_update10ms(void)
{
    uint8_t stableMask;

    if (!KEY_scan10ms(&stableMask) ||
        ((stableMask & KEY_MASK_KEY1) == 0U)) {
        return;
    }

    runAction(gNextAction);
    ++gNextAction;
    if (gNextAction >= ZDT_SERVO_TEST_ACTION_COUNT) {
        gNextAction = 0U;
    }
}
