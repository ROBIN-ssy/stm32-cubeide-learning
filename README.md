# STM32 学习轨迹（CubeIDE 工程集）

17 个独立工程，按**一个概念一个工程**的方式推进：从点灯到 BLE + FFT。
每个工程只保留 `.ioc`（CubeMX 配置）和 `Core/`（我写的那部分代码），
`Drivers/`、`Middlewares/`、`Debug/` 全部不入库——需要时用 CubeMX 重新生成即可。

| # | 工程 | 目标芯片 | 练什么 |
|---|---|---|---|
| 1 | `blink` | F103C8 | 时钟树、GPIO、第一个延时 |
| 2 | `LED` | F103C8 | 多引脚输出、位操作 |
| 3 | `key` | F103C8 | 输入、消抖 |
| 4 | `interrupt` | F103C8 | EXTI 外部中断、中断服务函数 |
| 5 | `serial` | F103C8 | USART 收发、printf 重定向 |
| 6 | `PWM_Light` | F103C8 | 定时器 PWM、占空比调光 |
| 7 | `oled` | F103C8 | I²C、SSD1306 驱动、字模 |
| 8 | `TFT_TEST` | F407VE | SPI、ILI9341、刷屏 |
| 9 | `TIM` | F103C8 | 基本定时器、更新中断 |
| 10 | `TIM_ExternalInput` | F103C8 | 外部时钟模式、计数外部脉冲 |
| 11 | `TIM_InputCapture` | F103C8 | 输入捕获、测频率/脉宽 |
| 12 | `TIM_SlaveMode` | F103C8 | 从模式、门控/触发、两个定时器联动 |
| 13 | `ADC_Test` | F429I-E | ADC 多通道、DMA 搬运、电压换算 |
| 14 | `ble` | F103C8 | UART 接 BLE 模块、AT 指令 |
| 15 | `BLETTFF4` | F429I-E | BLE + TFT 合流 |
| 16 | `f429_fft_ble` | F429I-E | CMSIS-DSP 做 FFT，结果经 BLE 发出去 |
| 17 | `FPGATEST` | F429I-E | STM32 与 FPGA 互联 |

---

## 这条线是怎么走的

```
点灯 ──→ 输入 ──→ 中断 ──→ 串口 ──→ PWM
                                    │
              ┌─────────────────────┘
              ▼
          显示(OLED→TFT)
              │
              ▼
        定时器四连(TIM → ExternalInput → InputCapture → SlaveMode)
              │
              ▼
            ADC + DMA
              │
              ▼
        BLE ──→ FFT ──→ 与 FPGA 对接
```

**为什么定时器要拆成 4 个工程**：`TIM` 只证明"能进中断"，`TIM_ExternalInput` 证明
"能数外部来的脉冲"，`TIM_InputCapture` 证明"能测一个边沿到另一个边沿有多久"，
`TIM_SlaveMode` 证明"一个定时器可以被另一个触发/门控"。
这四个能力组合起来才是编码器、测频、测距、PWM 输入这些东西的地基。
**分开做的好处是：哪个环节没通，一眼能看出来。**

---

## 目录

```
stm32-cubeide-learning/
├── ADC_Test/
│   ├── ADC_Test.ioc        ← CubeMX 配置（时钟、引脚、外设都在这）
│   └── Core/
│       ├── Inc/            头文件
│       └── Src/            main.c 及各外设初始化 + 中断回调
├── ble/  BLETTFF4/  blink/  ...
└── ...
```

`Core/Src/main.c` 里的 `/* USER CODE BEGIN */ ... /* USER CODE END */` 之间是我写的内容，
其余是 CubeMX 生成的骨架。

---

## 怎么复现

```bash
# 1. 装 STM32CubeIDE（或 CubeMX + 你的 IDE）
# 2. 新建工程，选对应芯片
# 3. 把该目录下的 .ioc 导入（File → Import → 或用 CubeMX 直接打开 .ioc）
# 4. 把 Core/Src、Core/Inc 下我写的文件覆盖进去
```

用 CubeMX 打开 `.ioc` 就能看到完整的时钟树、引脚分配和外设参数——
这比看代码更能说明"这个工程当时在练什么"。

---

## 说明

- 芯片用 `STM32F103C8`（蓝板）和 `STM32F429I-E` / `STM32F407VE`（开发板）
- 全程使用 **HAL 库 + CubeMX 图形化配置**，与裸寄存器写法区分
- 这些工程在板子上跑过，但**没有逐一提写测试记录**，这一点如实说明

---

## 许可

MIT。
