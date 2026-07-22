# 大模型辅助轮速 PI 调参设计

## 结论

方案可行，但每轮实验只运行一个车轮，并由 MCU 限制参数范围、PWM 和状态转换。大模型不直接发送 PWM，也不能在实验运行中修改 PI。

## 实验流程

默认选择右轮。KEY1 或 `RUN` 启动，KEY2 或 `STOP` 立即停止。`WHEEL,R` 和 `WHEEL,L` 只能在停止状态选择车轮。

速度序列为：0 mm/s 保持 0.5 s，然后 300、500、700、500、300 mm/s 各保持 2 s，最后自动停止。控制周期保持 10 ms，CSV 每 20 ms 输出一次。

## 串口格式

RESV1_UART 保持 UART0、PA10 TX、PA11 RX、115200 8N1。运行开始发送元数据和表头：

```text
#BEGIN,wheel=R,kp_x100=2000,ki_x100=200,sample_ms=10,telemetry_ms=20
timestamp_ms,setpoint,input,pwm,error,p,i,d
```

CSV 单位：时间为 ms，setpoint/input/error 为 mm/s，pwm/p/i/d 为 TB6612 软件 PWM 单位；当前只使用 PI，因此 d 恒为 0。

## 命令

```text
WHEEL,R
WHEEL,L
PI,R,2000,200
PI,L,2000,200
RUN
STOP
STATUS
```

PI 参数放大 100 倍传输。Kp 范围 0.00 到 100.00，Ki 范围 0.00 到 20.00。运行中修改 PI 或切换轮子返回 `ERR,BUSY`，非法格式返回 `ERR,CMD` 或 `ERR,RANGE`。

## 安全边界

- PWM 继续限制在 700。
- 实验仅允许正向速度序列，不自动换向。
- KEY2 和 STOP 优先立即清目标并停止两轮。
- UART 行缓冲区溢出时丢弃整行，不执行部分命令。
- 每次接受新参数后清除对应积分状态。
