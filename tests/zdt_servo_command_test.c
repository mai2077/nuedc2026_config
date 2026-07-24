#ifdef ZDT_SERVO_COMMAND_HOST_TEST

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "app/ZDT_Servo_test.h"
#include "bsp/bsp_ZDT_Servo_emm_v5.h"
#include "bsp/key.h"
#include "ti_msp_dl_config.h"

typedef enum {
    TEST_CALL_NONE = 0,
    TEST_CALL_SET_QPOS,
    TEST_CALL_QPOS,
    TEST_CALL_VELOCITY,
    TEST_CALL_STOP
} TestCall;

static TestCall gCalls[4];
static size_t gCallCount;
static uint8_t gLastAddress;
static uint8_t gLastDirection;
static uint16_t gLastVelocity;
static uint8_t gLastAcceleration;
static uint8_t gLastQPosMode;
static int32_t gLastPosition;
static uint32_t gDelayCallCount;
static bool gStableEventAvailable;
static uint8_t gStableEventMask;

static void recordCall(TestCall call)
{
    assert(gCallCount < (sizeof(gCalls) / sizeof(gCalls[0])));
    gCalls[gCallCount++] = call;
}

static void resetCalls(void)
{
    gCallCount = 0U;
    gDelayCallCount = 0U;
}

static void updateWithoutKeyEvent(void)
{
    gStableEventAvailable = false;
    ZDT_Servo_Test_update10ms();
}

static void emitStableKeyEvent(uint8_t mask)
{
    gStableEventMask = mask;
    gStableEventAvailable = true;
    ZDT_Servo_Test_update10ms();
    assert(!gStableEventAvailable);
}

bool KEY_scan10ms(uint8_t *stableMask)
{
    if (!gStableEventAvailable) {
        return false;
    }
    assert(stableMask != NULL);
    *stableMask = gStableEventMask;
    gStableEventAvailable = false;
    return true;
}

BSP_ZDT_Servo_Status BSP_EmmV5_SetQPosParams(uint8_t addr,
    uint16_t vel, uint8_t acc, uint8_t raF, bool snF)
{
    (void)snF;
    recordCall(TEST_CALL_SET_QPOS);
    gLastAddress = addr;
    gLastVelocity = vel;
    gLastAcceleration = acc;
    gLastQPosMode = raF;
    return BSP_ZDT_SERVO_OK;
}

BSP_ZDT_Servo_Status BSP_EmmV5_QPosControl(uint8_t addr, int32_t clk)
{
    recordCall(TEST_CALL_QPOS);
    gLastAddress = addr;
    gLastPosition = clk;
    return BSP_ZDT_SERVO_OK;
}

BSP_ZDT_Servo_Status BSP_EmmV5_VelControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, bool snF)
{
    (void)snF;
    recordCall(TEST_CALL_VELOCITY);
    gLastAddress = addr;
    gLastDirection = dir;
    gLastVelocity = vel;
    gLastAcceleration = acc;
    return BSP_ZDT_SERVO_OK;
}

BSP_ZDT_Servo_Status BSP_EmmV5_StopNow(uint8_t addr, bool snF)
{
    (void)snF;
    recordCall(TEST_CALL_STOP);
    gLastAddress = addr;
    return BSP_ZDT_SERVO_OK;
}

void delay_cycles(uint32_t cycles)
{
    assert(cycles == (CPUCLK_FREQ / 1000U));
    ++gDelayCallCount;
}

static void pressAndReleaseKey1(void)
{
    emitStableKeyEvent(KEY_MASK_KEY1);
    emitStableKeyEvent(0U);
}

static void assertVelocity(uint8_t direction)
{
    assert(gCallCount == 1U);
    assert(gCalls[0] == TEST_CALL_VELOCITY);
    assert(gLastAddress == ZDT_SERVO_TEST_ADDRESS);
    assert(gLastDirection == direction);
    assert(gLastVelocity == ZDT_SERVO_TEST_SPEED_RPM);
    assert(gLastAcceleration == ZDT_SERVO_TEST_ACCEL);
}

static void assertStop(void)
{
    assert(gCallCount == 1U);
    assert(gCalls[0] == TEST_CALL_STOP);
    assert(gLastAddress == ZDT_SERVO_TEST_ADDRESS);
}

static void assertPosition(int32_t position)
{
    assert(gCallCount == 2U);
    assert(gCalls[0] == TEST_CALL_SET_QPOS);
    assert(gCalls[1] == TEST_CALL_QPOS);
    assert(gLastAddress == ZDT_SERVO_TEST_ADDRESS);
    assert(gLastVelocity == ZDT_SERVO_TEST_SPEED_RPM);
    assert(gLastAcceleration == ZDT_SERVO_TEST_ACCEL);
    assert(gLastQPosMode == BSP_EMM_V5_QPOS_REL_CURRENT);
    assert(gLastPosition == position);
    assert(gDelayCallCount == 1U);
}

static void testInitStopsMotor(void)
{
    resetCalls();
    ZDT_Servo_Test_init();
    assertStop();
}

static void testKey1RunsSequenceAndWraps(void)
{
    resetCalls();
    pressAndReleaseKey1();
    assertVelocity(BSP_EMM_V5_DIR_CW);

    resetCalls();
    pressAndReleaseKey1();
    assertStop();

    resetCalls();
    pressAndReleaseKey1();
    assertVelocity(BSP_EMM_V5_DIR_CCW);

    resetCalls();
    pressAndReleaseKey1();
    assertStop();

    resetCalls();
    pressAndReleaseKey1();
    assertPosition(3200);

    resetCalls();
    pressAndReleaseKey1();
    assertStop();

    resetCalls();
    pressAndReleaseKey1();
    assertPosition(-3200);

    resetCalls();
    pressAndReleaseKey1();
    assertStop();

    resetCalls();
    pressAndReleaseKey1();
    assertVelocity(BSP_EMM_V5_DIR_CW);
}

static void testHoldReleaseAndOtherKeysDoNotAdvance(void)
{
    resetCalls();
    ZDT_Servo_Test_init();
    resetCalls();

    updateWithoutKeyEvent();
    emitStableKeyEvent(KEY_MASK_KEY2 | KEY_MASK_KEY3 | KEY_MASK_KEY4);
    emitStableKeyEvent(0U);
    assert(gCallCount == 0U);

    emitStableKeyEvent(KEY_MASK_KEY1);
    assertVelocity(BSP_EMM_V5_DIR_CW);

    resetCalls();
    updateWithoutKeyEvent();
    updateWithoutKeyEvent();
    emitStableKeyEvent(0U);
    assert(gCallCount == 0U);

    pressAndReleaseKey1();
    assertStop();
}

static void assertAngleCommand(int16_t angleDegrees, int32_t expectedPulses)
{
    resetCalls();
    assert(ZDT_Servo_rotateDegrees(angleDegrees) == BSP_ZDT_SERVO_OK);
    assertPosition(expectedPulses);
}

static void testDegreeConversionAndRangeValidation(void)
{
    assertAngleCommand(1, 9);
    assertAngleCommand(-1, -9);
    assertAngleCommand(45, 400);
    assertAngleCommand(-45, -400);
    assertAngleCommand(90, 800);
    assertAngleCommand(-90, -800);
    assertAngleCommand(360, 3200);
    assertAngleCommand(-360, -3200);

    resetCalls();
    assert(ZDT_Servo_rotateDegrees(0) == BSP_ZDT_SERVO_OK);
    assert(gCallCount == 0U);
    assert(gDelayCallCount == 0U);

    resetCalls();
    assert(ZDT_Servo_rotateDegrees(361) ==
        BSP_ZDT_SERVO_INVALID_ARGUMENT);
    assert(ZDT_Servo_rotateDegrees(-361) ==
        BSP_ZDT_SERVO_INVALID_ARGUMENT);
    assert(gCallCount == 0U);
    assert(gDelayCallCount == 0U);
}

int main(void)
{
    testInitStopsMotor();
    testKey1RunsSequenceAndWraps();
    testHoldReleaseAndOtherKeysDoNotAdvance();
    testDegreeConversionAndRangeValidation();
    puts("zdt servo key sequence and angle tests passed");
    return 0;
}

#endif /* ZDT_SERVO_COMMAND_HOST_TEST */
