# Training Tasks 2-5 Change Report

## 1. 修改目标

本次修改用于完成 GMP 训练任务第 2 至第 5 题：

- 第 2 题：保持 `adc_channel_t input_wave_adc` 对输入波形 ADC 做标幺化。
- 第 3 题：使用 `ctl_lead_t lead_comp` 对 ADC 标幺化结果做 100 Hz、45 度的超前补偿。
- 第 4 题：将补偿后的 `comp_out` 通过 DAC B 输出，且不再对 `comp_out` 调用 `ctl_sin()`。
- 第 5 题：将 `comp_out` 映射到 `[0, 1]` 的 PWM 占空比，并输出到 `IRIS_EPWM1`，EPWM1 死区配置为 500 ns、低电平极性。

## 2. 修改文件列表

人工修改的工作区文件：

```text
user/ctl_main.c
user/ctl_main.h
xplt/xplt.ctl_interface.h
xplt/xplt.peripheral.h
xplt/ctrl_settings.h
F280039_Iris_node.syscfg
docs/change_reports/2026-07-06-training-tasks-2-5.md
```

为匹配当前 `Debug` 构建脚本实际引用路径，已同步同名文件到：

```text
D:/Github/gmp_pro/csp/c28x_syscfg/iris_280039c_board/
```

构建自动更新了 `Debug/` 下的 SysConfig 生成文件和目标文件，主要包括：

```text
Debug/syscfg/board.c
Debug/syscfg/board.h
Debug/syscfg/epwm.dot
Debug/Iris_Node_Env.out
Debug/Iris_Node_Env.map
Debug/Iris_Node_Env_linkInfo.xml
Debug/**/*.obj
Debug/**/*.d
```

## 3. 逐文件修改说明

文件：`user/ctl_main.c`

- 复用并保留 `ctl_lead_t lead_comp` 和 `ctrl_gt comp_out`。
- 新增 `ctrl_gt pwm_duty` 与 `pwm_channel_t comp_pwm` 的唯一定义。
- 在 `ctl_init()` 中使用 `ctl_init_lead_form3(&lead_comp, pi/4, 100 Hz, CONTROLLER_FREQUENCY)` 初始化超前补偿器。
- 在 `ctl_init()` 中使用 `ctl_init_pwm_channel(&comp_pwm, 0, CTRL_PWM_CMP_MAX)` 初始化 PWM 通道。

文件：`user/ctl_main.h`

- 为 `lead_comp`、`comp_out`、`pwm_duty`、`comp_pwm` 提供 `extern` 声明。
- 在 `ctl_dispatch()` 中执行控制层实时 step：`ctl_step_lead()`、`pwm_duty = 0.5 * comp_out + 0.5`、`ctl_step_pwm_channel()`。
- 未在 `user` 层加入 ADC/DAC/EPWM 寄存器操作。

文件：`xplt/xplt.ctl_interface.h`

- 在 `ctl_input_callback()` 中保留 `ctl_step_adc_channel()`，ADC 原始读取仍位于平台层。
- 修正 DAC B 输出：使用限幅后的 `comp_out * 1024.0f + 2048.0f`，不再对 `comp_out` 调用 `ctl_sin()`。
- 新增 EPWM 输出：使用 `EPWM_setCounterCompareValue(IRIS_EPWM1_BASE, EPWM_COUNTER_COMPARE_A, comp_pwm.value)`。
- 移除错误的 `comp_put`、`IRIS_EPWM1`、`fsbb_mod`、`ctl_get_fsbb_boost_cmp()` 用法。

文件：`xplt/xplt.peripheral.h`

- 增加 `adc_channel_t input_wave_adc` 的 `extern` 声明。
- 引入 `adc_channel.h`，保证 `adc_channel_t` 类型声明完整。

文件：`xplt/ctrl_settings.h`

- 设置 `CTRL_PWM_CMP_MAX` 为 `3000`，对应 120 MHz、20 kHz、上下计数模式下的 PWM 周期。
- 设置 `CTRL_PWM_DEADBAND_CMP` 为 `60`，对应 120 MHz TBCLK 下约 500 ns 死区。

文件：`F280039_Iris_node.syscfg`

- 将 `IRIS_EPWM1` 的 RED/FED 死区计数设置为 `60`。
- 保持 `IRIS_EPWM1` 为 up-down 计数，周期为 `3000`，FED 极性为 active low。

## 4. 新增或修改的变量

| 变量名 | 类型 | 定义位置 | 声明位置 | 作用 |
|---|---|---|---|---|
| `input_wave_adc` | `adc_channel_t` | `xplt/xplt.peripheral.c:85` | `xplt/xplt.peripheral.h:53` | ADC 标幺化对象 |
| `lead_comp` | `ctl_lead_t` | `user/ctl_main.c:28` | `user/ctl_main.h:51` | 超前补偿器对象 |
| `comp_out` | `ctrl_gt` | `user/ctl_main.c:29` | `user/ctl_main.h:52`, `xplt/xplt.ctl_interface.h:28` | 超前补偿后的输出 |
| `pwm_duty` | `ctrl_gt` | `user/ctl_main.c:30` | `user/ctl_main.h:53` | `[0, 1]` PWM 占空比 |
| `comp_pwm` | `pwm_channel_t` | `user/ctl_main.c:31` | `user/ctl_main.h:54`, `xplt/xplt.ctl_interface.h:29` | PWM 比较值通道对象 |

没有在 `.h` 文件中定义全局变量；所有跨文件使用均通过 `extern`。

## 5. 新增或修改的宏

| 宏名 | 定义位置 | 含义 |
|---|---|---|
| `CTRL_PWM_CMP_MAX` | `xplt/ctrl_settings.h:34` | PWM 计数周期/比较满量程，设置为 `3000` |
| `CTRL_PWM_DEADBAND_CMP` | `xplt/ctrl_settings.h:37` | 500 ns 死区对应的计数值，设置为 `60` |

宏通过头文件包含使用，没有使用 `extern`。

## 6. 新增或修改的函数调用

| 函数调用 | 位置 | 作用 |
|---|---|---|
| `ctl_init_adc_channel(&input_wave_adc, 4.0f, 0.5f, 12, 24)` | `xplt/xplt.peripheral.c:124` | ADC 标幺化配置，对应理论范围 `[-1, 1]` |
| `ctl_step_adc_channel()` | `xplt/xplt.ctl_interface.h:34` | 读取 ADC 原始值并更新标幺化结果 |
| `ctl_init_lead_form3()` | `user/ctl_main.c:43` | 初始化 100 Hz、45 度超前补偿器 |
| `ctl_step_lead()` | `user/ctl_main.h:59` | 执行超前补偿 step |
| `ctl_sat()` | `user/ctl_main.h:60`, `xplt/xplt.ctl_interface.h:49` | PWM 占空比和 DAC B 输出限幅 |
| `ctl_init_pwm_channel()` | `user/ctl_main.c:44` | 初始化 PWM 通道对象 |
| `ctl_step_pwm_channel()` | `user/ctl_main.h:61` | 将 `[0, 1]` 占空比转换为 EPWM 比较值 |
| `DAC_setShadowValue()` | `xplt/xplt.ctl_interface.h:47`, `xplt/xplt.ctl_interface.h:50` | 输出 DAC A 测试正弦波和 DAC B 补偿波形 |
| `EPWM_setCounterCompareValue()` | `xplt/xplt.ctl_interface.h:52` | 写入 `IRIS_EPWM1` CMPA |

## 7. 分层规范检查

| 内容 | 所在位置 | 是否符合规范 | 说明 |
|---|---|---|---|
| ADC 原始读取 | `xplt/xplt.ctl_interface.h` | 是 | ADC 外设读取位于 `input_callback` |
| ADC 标幺化对象定义 | `xplt/xplt.peripheral.c` | 是 | 平台输入对象由 `xplt` 管理，`user` 只读取结果 |
| 超前补偿计算 | `user/ctl_main.h` | 是 | 属于平台无关控制算法 |
| PWM 占空比计算 | `user/ctl_main.h` | 是 | 属于控制层信号映射 |
| DAC B 写寄存器 | `xplt/xplt.ctl_interface.h` | 是 | DAC 输出位于 `output_callback` |
| EPWM 写比较寄存器 | `xplt/xplt.ctl_interface.h` | 是 | EPWM 外设操作位于 `output_callback` |
| EPWM1 死区配置 | `F280039_Iris_node.syscfg` | 是 | 属于平台外设初始化配置 |
| 全局变量定义 | `.c` 文件 | 是 | 每个全局变量只有一个定义 |
| 全局变量声明 | `.h` 文件 `extern` | 是 | 没有在头文件中重复定义 |

残留检查结果：

```text
未发现 ctl_sin(comp_out)
未发现 comp_put
未发现 fsbb_mod
未发现 EPWM_setCounterCompareValue(IRIS_EPWM1, ...)
```

## 8. 功能实现情况

| 题号 | 功能 | 状态 | 说明 |
|---|---|---|---|
| 第 2 题 | ADC 采集并标幺化 | 已完成 | `input_wave_adc` 使用 `gain=4.0f`、`bias=0.5f`、12 bit、IQ24 配置 |
| 第 3 题 | 超前补偿器 | 已完成 | 使用 `ctl_init_lead_form3()`，角度为 `pi / 4`，频率为 100 Hz，采样频率为 20 kHz |
| 第 4 题 | DAC B 输出补偿波形 | 已完成 | DAC B 直接输出限幅后的 `comp_out`，没有再套 `ctl_sin()` |
| 第 5 题 | PWM 输出补偿波形 | 已完成 | `comp_out` 映射为 `pwm_duty`，再通过 `comp_pwm.value` 写入 `IRIS_EPWM1_BASE` CMPA |

## 9. 编译和验证结果

执行命令：

```text
gmake -C .\Debug all --jobs=1
```

结果：通过。

生成目标：

```text
Debug/Iris_Node_Env.out
```

验证补充：

- 第一次构建失败，因为 `Debug` 构建脚本实际引用 `D:/Github/gmp_pro/csp/c28x_syscfg/iris_280039c_board` 中的旧工程拷贝。已同步本次修改到该路径后继续验证。
- 第二次构建在 SysConfig 写入 `Debug/syscfg/epwm.dot` 时失败，原因是该生成文件为只读。已去掉只读属性后重新构建。
- 最终构建成功，输出 `.out` 文件。
- 构建仍有既有警告，包括 `ctl_fault_recover_routine`、`flush_dl_rx_buffer`、`flush_dl_tx_buffer` 隐式声明，`rx_raw` 未引用，以及 `.sysmem` 默认大小提示；这些警告不属于本次任务新增逻辑导致的错误。

## 10. 风险点和未确认问题

- 当前 `Debug` 构建脚本使用的 include/source 路径指向 `D:/Github/gmp_pro/csp/c28x_syscfg/iris_280039c_board`，不是单纯使用当前工作区路径。因此本次已同步两处同名文件；后续若继续修改，建议统一工程路径或继续保持两处同步。
- 第 5 题已完成软件侧 PWM 输出和 EPWM1 死区配置，但尚未做真实硬件示波器验证。
- `comp_out` 输出到 DAC B 前已限幅到 `[-1, 1]`，PWM duty 已限幅到 `[0, 1]`，可避免越界写入。

## 11. 建议 commit message

```text
Implement GMP training tasks 2-5 waveform outputs
```

## 12. 本次修改摘要

本次修改完成了第 2 至第 5 题的本地实现：ADC 标幺化结果进入超前补偿器，补偿输出保存到 `comp_out`。DAC B 输出改为直接使用限幅后的 `comp_out`，修复了错误的 `ctl_sin(comp_out)` 用法。PWM 输出新增 `pwm_duty` 与 `comp_pwm`，在控制层完成占空比映射，在平台层写入 `IRIS_EPWM1_BASE` CMPA。EPWM1 的周期保持为 `3000`，死区配置改为 `60` 个 TBCLK，对应约 500 ns，并已通过 `gmake -C .\Debug all --jobs=1` 构建验证。
