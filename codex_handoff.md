# MSPM0G3507 电赛小车 Codex 交接文档

更新时间：2026-07-22

工程目录：`D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig`

## 1. 交接目标

这是一个基于 TI MSPM0G3507 的差速小车工程，当前已经完成 TB6612 电机驱动、双轮霍尔编码器测速、右轮速度 PI 闭环、OLED 显示、RESV1_UART 调参协议和 BT_UART 测试接口。

当前最重要的未解决问题是：

> MCU 通过 BT_UART 主动发送的 `BT_UART_TEST` 可以收到，但从 RESV1_UART 接收到的数据没有出现在蓝牙模块端。

当前判断更偏向 RESV1_UART 的物理 RX 路径、接线或上位机串口方向问题，而不是 BT_UART TX 代码问题。

## 2. 硬件与引脚

### MCU 与电机

- MCU：MSPM0G3507，CCS 工程，TI MSPM0 SDK 2.10.00.04。
- 电机驱动：TB6612FNG。
- 电机电源：9 V 稳压。
- 电机工作范围：7~13 V，堵转电流小于 2 A。
- TB6612 `STBY`：硬件接 `+3.3 V`，没有由软件控制。
- PWM：`TB_PWM`，TIMA1，32 kHz，软件 PWM 限幅 `-700~+700`。
- 电机 1/右轮：控制器内部使用 `TB6612_setMotor1(-rightPwm)`，因为右轮机械安装方向相反。
- 电机 2/左轮：控制器内部使用 `TB6612_setMotor2(leftPwm)`。

### 编码器

- 右轮电机 1 编码器 A 相：`TIMA0_CCP0 / PA0`，实例名 `ENCODER1`。
- 右轮方向锁存 Q：`PB22`，实例名 `DIR1`。
- 左轮电机 2 编码器 A 相：`TIMG6_CCP0 / PB2`，实例名 `ENCODER2`。
- 左轮方向锁存 Q：`PB13`，实例名 `DIR2`。
- `ENCODER1_DIR_HIGH_IS_POSITIVE = 0`：右轮方向低电平记正。
- `ENCODER2_DIR_HIGH_IS_POSITIVE = 1`：左轮方向高电平记正。
- 编码器计数：实测约 265 count/输出轴一圈。
- 车轮直径：48 mm。
- 编码器分辨率造成速度量化：约 56.9 mm/s 每 count/10 ms。
- 霍尔编码器供电为 3.3 V。

### UART

`empty.syscfg` 是配置源，不能手工编辑 `Debug/ti_msp_dl_config.*`。

| 实例名 | 外设 | TX | RX | 波特率 | 当前用途 |
|---|---|---|---|---:|---|
| `RESV1_UART` | UART0 | PA10 | PA11 | 115200 | 上位机调参输入；遥测默认关闭 |
| `BT_UART` | UART2 | PA21 | PA22 | 115200 | 蓝牙模块输出和 RESV1 字节转发 |

串口均按 8N1 使用。外接 USB-TTL 时必须满足：

```text
USB-TTL TX -> MCU PA11 (RESV1_UART RX)
USB-TTL RX -> MCU PA10 (RESV1_UART TX)
GND        -> MCU GND
```

蓝牙测试时：

```text
MCU PA21 (BT_UART TX) -> 蓝牙模块 RX
MCU PA22 (BT_UART RX) -> 蓝牙模块 TX（本次只测发送时可暂不接）
GND                  -> 蓝牙模块 GND
```

## 3. SysConfig 定时器

- `TIMER_0`：TIMG7，周期 10 ms，时钟 8 分频，中断优先级 1。
- `ENCODER1`：TIMA0 输入捕获，周期配置 10 ms，中断优先级 0。
- `ENCODER2`：TIMG6 输入捕获，周期配置 10 ms，中断优先级 0。
- 编码器 ISR 只读取方向锁存并更新有符号位置。
- `TIMER_0_INST_IRQHandler()` 中调用 `ENCODER_updateSpeed10ms()`，再递增 `gTick10ms`。
- PI 计算在主循环执行，不在 TIMER ISR 中执行。

## 4. 当前主程序行为

入口文件：[app/main.c](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/app/main.c)

初始化顺序：

1. `SYSCFG_DL_init()`。
2. `ENCODER_init()`。
3. `PI_TUNER_init()`，右轮目标初始化为 300 mm/s，左轮目标为 0。
4. 直接通过 BT_UART 发送一次 `BT_UART_TEST\r\n`。
5. 初始化 OLED 并显示速度环参数。
6. 开启 `TIMER_0` 中断和计数器。

运行中每秒通过 BT_UART 发送一次：

```text
BT_UART_TEST
```

这条主动发送用于单独验证 MCU 到蓝牙模块的 TX 路径。

## 5. PI 调参状态机

主要文件：

- [app/pi_tuner.c](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/app/pi_tuner.c)
- [app/pi_tuner.h](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/app/pi_tuner.h)
- [control/wheel_speed.c](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/control/wheel_speed.c)
- [control/wheel_speed.h](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/control/wheel_speed.h)

右轮目标速度循环为：

```text
300 mm/s，2 s
500 mm/s，2 s
700 mm/s，2 s
500 mm/s，2 s
300 mm/s，2 s
```

循环周期 10 s，程序不会自动停止。左轮每次控制更新都写入目标 0、PWM 0。

PI 默认参数：

```c
WHEEL_SPEED_RIGHT_KP_X100 = 2000U; // 20.00
WHEEL_SPEED_RIGHT_KI_X100 = 0U;    // 0.00
WHEEL_SPEED_LEFT_KP_X100  = 2000U; // 20.00
WHEEL_SPEED_LEFT_KI_X100  = 200U;  // 2.00
```

运行时命令格式：

```text
SET P:20 I:2 D:0
SET P:20.50 I:1.75 D:0
```

命令以 LF 或 CRLF 结束。P/I 最多两位小数，转换为 x100 定点值。D 当前必须为 0。合法参数运行中立即生效并清除 PI 输出和积分状态。命令不会通过 RESV1_UART 回显。

## 6. 当前 UART 转发与临时开关

`PI_TUNER_pollUart()` 的逻辑顺序是：

1. 从 `RESV1_UART` 读取一个字节。
2. 立即调用 `BT_UART_writeByte(data)` 原样转发。
3. 再把该字节送入 `SET P/I/D` 行解析器。

因此 CR、LF、空格和非法字符理论上都会被原样转发。

RESV1 周期遥测当前默认关闭：

```c
// app/pi_tuner.h
#define PI_TUNER_ENABLE_RESV1_TELEMETRY (0U)
```

需要恢复 CSV 遥测时改为：

```c
#define PI_TUNER_ENABLE_RESV1_TELEMETRY (1U)
```

遥测开启时格式为：

```text
timestamp_ms,setpoint,input,pwm,error,p,i,d
```

实际数据示例：

```text
10,300.00,0.00,105.00,300.00,20.000,0.000,0.000
```

最后三列是当前 P/I/D 增益，不是 P/I 对 PWM 的贡献。

## 7. OLED

OLED 使用软件 I2C，现有接口在：

- [bsp/oled.h](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/bsp/oled.h)
- [bsp/oled.c](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/bsp/oled.c)
- [bsp/oled_soft_i2c.c](D:/MCU_Development/TI/MSPM0/nuedc2026_Pinconfig/bsp/oled_soft_i2c.c)

显示内容为目标速度、实测速度、PWM、P、I、D。OLED 字模数据事务已经批量化；运行期每个非遥测节拍最多刷新一个动态字符，避免一次整页刷新阻塞 10 ms 控制环。

## 8. 其他已存在模块

工程中还保留以下 BSP/应用代码，但不一定由当前 `main.c` 调用：

- `bsp/icm20948.c/.h`：ICM-20948 驱动和寄存器访问。
- `bsp/bsp_iic.c/.h`：MSPM0 I2C 适配。
- `bsp/bsp_imu.c/.h`：从 STM32 示例迁移的 IMU 兼容接口。
- `bsp/ano_protocol.c/.h`：匿名通信协议测试代码。
- `bsp/track.c/.h`：循迹输入读取。
- `bsp/key.c/.h`：按键消抖和按键读取，当前 PI 自动调参主流程不再使用按键。
- `bsp/tb6612.c/.h`：TB6612 方向、PWM、停止和方向切换保护。

不要因为这些文件存在就假设 ICM、循迹或按键功能已经接入当前主循环。

## 9. 当前未解决问题：RESV1 RX

已确认：

- BT_UART 主动发送 `BT_UART_TEST` 能被蓝牙端收到。
- `BT_UART_writeByte()` 和 BT_UART 配置有效。
- SysConfig 中 RESV1_UART 为 UART0，RX 为 PA11，TX 为 PA10，115200 8N1。
- TI Arm Clang 和 CCS 构建均通过。

现象：

- 上位机发送到 RESV1_UART 的数据没有在蓝牙端出现。

下一步按以下顺序排查：

1. 示波器测 PA11。上位机发送时 PA11 应出现 0~3.3 V UART 波形，空闲电平为高。
2. 确认 USB-TTL TX 接的是 PA11，不是 PA10。
3. 确认 USB-TTL 与 MCU 共地，且为 3.3 V TTL，不是 RS-232 电平。
4. 确认上位机为 115200 8N1。
5. 如果 PA11 有波形但仍无转发，再在 `PI_TUNER_pollUart()` 中加入接收计数器，通过 OLED 或 BT_UART 周期输出计数，区分“UART 未收到”和“转发未发送”。
6. 如果 PA11 没有波形，先处理串口适配器、TX/RX 方向、板载串口冲突或接线问题，不要先改 DriverLib 轮询代码。

## 10. 验证命令

主机 PI 测试，遥测关闭：

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DPI_TUNER_HOST_TEST -Ibsp -Icontrol -Iapp `
  tests/pi_tuner_host_test.c app/pi_tuner.c control/wheel_speed.c `
  -o tests/pi_tuner_host_test_off.exe
& .\tests\pi_tuner_host_test_off.exe
```

主机 PI 测试，遥测开启：

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DPI_TUNER_HOST_TEST -DPI_TUNER_ENABLE_RESV1_TELEMETRY=1 `
  -Ibsp -Icontrol -Iapp `
  tests/pi_tuner_host_test.c app/pi_tuner.c control/wheel_speed.c `
  -o tests/pi_tuner_host_test_on.exe
& .\tests\pi_tuner_host_test_on.exe
```

蓝牙/调试 UART 测试：

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DTRACK_DIAG_HOST_TEST -Itests/stubs -Ibsp `
  tests/track_uart_diag_test.c bsp/track.c bsp/debug_uart.c bsp/bt_uart.c `
  -o tests/track_uart_diag_test.exe
& .\tests\track_uart_diag_test.exe
```

TI Arm Clang 语法检查：

```powershell
& 'C:\ti\ccs2100\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe' `
  -fsyntax-only -D__MSPM0G3507__ -D__USE_SYSCONFIG__ `
  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft `
  -mlittle-endian -mthumb -I. -IDebug -Ibsp -Iapp -Icontrol `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source\third_party\CMSIS\Core\Include' `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source' `
  app/main.c app/pi_tuner.c control/wheel_speed.c `
  bsp/debug_uart.c bsp/bt_uart.c bsp/encoder.c bsp/tb6612.c `
  bsp/oled.c bsp/oled_soft_i2c.c
```

SysConfig 检查：

```powershell
python 'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' .
```

CCS 干净构建：

```powershell
& 'C:\ti\ccs2100\ccs\utils\bin\gmake.exe' -C Debug clean all
```

生成文件：

```text
Debug/nuedc2026_Pinconfig.out
```

## 11. 接手时的安全规则

- 不要手工编辑 `Debug/ti_msp_dl_config.c`、`Debug/ti_msp_dl_config.h`、`device.opt` 或 `.out`。
- 修改外设、引脚、时钟或 UART 配置时，只改 `empty.syscfg`，然后重新生成和构建。
- 当前按键急停未接入 PI 自动调参主流程；首次烧录必须架空车轮，并准备直接断开电机电源的方法。
- 不要在没有检查 TX/RX 波形和共地的情况下反复修改 UART 轮询代码。
- 不要复制整个用户目录下的 `.codex` 状态来迁移任务；只迁移工程和本交接文档。

## 12. 给新 Codex 的启动提示词

```text
请先阅读工程根目录的 codex_handoff.md，再检查其中引用的当前源文件和 empty.syscfg。
当前优先任务是定位 RESV1_UART RX（PA11）没有被 MCU 识别的问题。
不要重做已经完成的 PI、TB6612、编码器、OLED 和 BT_UART 主动发送功能。
先通过示波器/配置/轮询数据流分层判断问题，再决定是否改代码。
不要手工编辑 Debug 下的 SysConfig 生成文件。
```
