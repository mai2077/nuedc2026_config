# ZDT EMM_V5 单电机串口测试设计

## 目标

把 STM32 参考工程中的 ZDT EMM_V5 串口协议层移植到 MSPM0G3507 工程，并建立一个只测试串口电机的独立固件入口。测试期间不初始化循迹、TB6612、编码器、OLED、IMU、轮速 PI 或角度控制。

## 已确认硬件

- 电机驱动器可直接使用 3.3 V UART-TTL 通信，不需要 RS485 收发器。
- 电机地址为 `0x01`。
- 使用现有 SysConfig 实例 `STEP1_UART`：UART1，PB4 TX、PB5 RX、115200 baud。
- MSPM0 TX PB4 接驱动器 RX，MSPM0 RX PB5 接驱动器 TX，双方共地。
- 首次测试速度为 60 RPM，加速度参数为 10。
- 快速位置控制使用相对当前位置模式；`+3200/-3200` 分别表示正向/反向一圈。
- 驱动器由自身配置保持使能，MSPM0 不新增使能 GPIO。

## 范围

本次只迁移 `bsp_emm_v5.c` 中的协议构帧和发送能力。不迁移 `bsp_gimbal.c`、`bsp_system.c`、`bsp_actuator.c`，因为这些文件属于双轴视觉云台、K230、激光和位置反馈业务，并依赖未提供的头文件及模块。

## 文件结构

### `bsp/bsp_ZDT_Servo_emm_v5.h/.c`

负责 EMM_V5 协议帧和 STEP1_UART 发送。构帧接口保留原函数名、参数和作用：

```c
size_t BSP_EmmV5_Build_SetQPosParams(...);
size_t BSP_EmmV5_Build_QPosControl(...);
size_t BSP_EmmV5_Build_VelControl(...);
size_t BSP_EmmV5_Build_PosControl(...);
size_t BSP_EmmV5_Build_StopNow(...);
size_t BSP_EmmV5_Build_SynchronousMotion(...);
size_t BSP_EmmV5_Build_ReadSysParam(...);
```

发送接口也保留原函数名和作用，但去掉 STM32 的 `UART_HandleTypeDef *` 参数，固定发送到 `STEP1_UART_INST`，返回 MSPM0 专用状态枚举：

```c
BSP_ZDT_Servo_Status BSP_EmmV5_SetQPosParams(...);
BSP_ZDT_Servo_Status BSP_EmmV5_QPosControl(...);
BSP_ZDT_Servo_Status BSP_EmmV5_VelControl(...);
BSP_ZDT_Servo_Status BSP_EmmV5_PosControl(...);
BSP_ZDT_Servo_Status BSP_EmmV5_StopNow(...);
BSP_ZDT_Servo_Status BSP_EmmV5_SynchronousMotion(...);
BSP_ZDT_Servo_Status BSP_EmmV5_ReadSysParam(...);
```

驱动保存最近一次发送帧，提供只读复制接口供测试模块打印。所有写串口操作在主循环中阻塞发送短帧，不在 ISR 中执行。

### `app/ZDT_Servo_test.h/.c`

负责 RESV1 命令行接收、参数校验、调用驱动和诊断输出。公共接口为：

```c
void ZDT_Servo_Test_init(void);
void ZDT_Servo_Test_poll(void);
```

支持命令：

```text
ZDT FWD
ZDT REV
ZDT STOP
ZDT POS 3200
ZDT POS -3200
```

- `FWD`：地址 1、正方向、60 RPM、加速度 10、立即启动。
- `REV`：地址 1、反方向、60 RPM、加速度 10、立即启动。
- `STOP`：地址 1 立即停止。
- `POS n`：先设置快速位置参数为 60 RPM、加速度 10、相对当前位置，再发送带符号增量 `n`。
- 每次发送后从 RESV1 输出 `OK/ERROR` 和最后发送的十六进制帧。
- 空行、超长行、未知命令、缺失参数、非十进制参数和超出 `int32_t` 的参数均拒绝，不发送电机帧。

初始化时主动发送一次 STOP，之后不自动运行电机。

### `app/main.c`

测试阶段改成最小入口：

```text
SYSCFG_DL_init
 -> DEBUG_UART_initRxInterrupt
 -> ZDT_Servo_Test_init（先 STOP）
 -> while (1) ZDT_Servo_Test_poll
```

不调用 `ENCODER_init()`、`PI_TUNER_init()`、`OLED_Init()`，不启动 `TIMER_0`，因此 TB6612 和循迹链不会产生运行命令。原模块源码保留在工程中，测试结束后可恢复原 `main.c` 调用链。

## 安全和错误处理

- 上电只发送 STOP，不自动发送速度或位置命令。
- 所有动作由完整的 LF/CRLF 文本行触发。
- 速度和加速度固定为保守值，避免首次测试误输入高速度。
- `POS` 使用严格 `int32_t` 解析，拒绝溢出。
- 构帧函数在空指针或容量不足时返回 0。
- 发送函数检测构帧失败并返回错误状态。
- 参考源码未包含响应帧解析规范，本次不把“UART 已发送”当成“驱动器已执行”；实际动作和回包需要硬件验证。

## 验证

1. 主机测试构帧长度、字节序、方向位、带符号位置和帧尾 `0x6B`。
2. 主机测试命令解析，确认非法命令不发送。
3. 运行 SysConfig 静态检查，确认 STEP1_UART 映射和 115200 baud，无需修改 `empty.syscfg`。
4. 使用 TI Arm Clang/CCS 完整编译，确认没有 STM32 HAL 类型或符号残留。
5. 机械负载断开或架空后，依次执行 `STOP`、`FWD`、`STOP`、`REV`、`STOP`、`POS 3200`、`STOP`、`POS -3200`、`STOP`。

