# Local Build Isolation Report

## 1. 修改目标

本次修改目标是将训练任务实现从外部工程拷贝路径中独立出来，使当前工程目录 `D:/Github/albemuth/iris_280039c_board` 自己的 `user`、`xplt`、`F280039_Iris_node.syscfg` 和 `Debug/syscfg` 成为构建时使用的任务代码与配置来源。

## 2. 修改文件列表

```text
.cproject
Debug/subdir_rules.mk
Debug/user/subdir_rules.mk
Debug/xplt/subdir_rules.mk
Debug/oled_driver/subdir_rules.mk
Debug/gmp_src_mgr/gmp_src/subdir_rules.mk
Debug/device_support/subdir_rules.mk
Debug/driverlib/subdir_rules.mk
Debug/**/*.d
Debug/**/*.d_raw
docs/change_reports/2026-07-06-local-build-isolation.md
```

说明：`D:/Github/gmp_pro` 仍作为 GMP 库 include 根目录保留；移除的是外部工程拷贝 `D:/Github/gmp_pro/csp/c28x_syscfg/iris_280039c_board` 作为任务代码来源的依赖。

## 3. 逐文件修改说明

文件：`.cproject`

- 增加 `${PROJECT_LOC}/Debug/syscfg` include 路径。
- 保留 `${PROJECT_LOC}/user`、`${PROJECT_LOC}/xplt`、`${PROJECT_LOC}/oled_driver` 等本工程 include。

文件：`Debug/subdir_rules.mk`

- 将 SysConfig 脚本输入从外部绝对路径改为 `../F280039_Iris_node.syscfg`。
- 将工程头文件 include 路径改为 `..`、`../user`、`../xplt`、`../oled_driver`、`./syscfg`。

文件：`Debug/*/subdir_rules.mk`

- 将各子目录编译规则中的外部工程 include 路径替换为当前工程相对路径。
- 保留 GMP 库路径 `D:/Github/gmp_pro` 和 C2000Ware 路径。

文件：`Debug/**/*.d` 与 `Debug/**/*.d_raw`

- 将旧依赖记录中的外部工程头文件路径机械替换为当前工程相对路径，避免后续 make 读取旧依赖时重新依赖外部工程拷贝。

## 4. 新增或修改的变量

本次未新增控制变量，也未改变第 2 至第 5 题中的全局变量定义。

## 5. 新增或修改的宏

本次未新增或修改控制宏。

## 6. 新增或修改的函数调用

本次未新增控制函数调用；仅调整构建路径。

## 7. 分层规范检查

| 内容 | 所在位置 | 是否符合规范 | 说明 |
|---|---|---|---|
| 控制算法 | `user` | 是 | 本次未改变控制算法位置 |
| 外设操作 | `xplt` | 是 | 本次未改变外设操作位置 |
| SysConfig 输入 | 当前工程 `F280039_Iris_node.syscfg` | 是 | 不再读取外部工程拷贝 |
| 构建 include | 当前工程相对路径 | 是 | `user/xplt/Debug/syscfg` 均来自当前目录 |

## 8. 功能实现情况

| 题号 | 功能 | 状态 | 说明 |
|---|---|---|---|
| 第 2 题 | ADC 采集并标幺化 | 已保持 | 当前目录代码继续生效 |
| 第 3 题 | 超前补偿器 | 已保持 | 当前目录代码继续生效 |
| 第 4 题 | DAC B 输出 | 已保持 | 当前目录代码继续生效 |
| 第 5 题 | PWM 输出 | 已保持 | 当前目录代码继续生效 |

## 9. 编译和验证结果

执行命令：

```text
gmake -C .\Debug user/ctl_main.obj xplt/xplt.peripheral.obj gmp_src_mgr/gmp_src/PCA9555.obj oled_driver/oled_driver.obj --jobs=1 -B
gmake -C .\Debug all --jobs=1
```

结果：通过。

验证结果：

- SysConfig 调用已使用 `--script "../F280039_Iris_node.syscfg"`。
- 代表性依赖文件已指向 `../user`、`../xplt`、`../oled_driver`。
- `Debug`、`.cproject`、`.project`、`.ccsproject`、`.settings` 中已无外部工程拷贝路径 `D:/Github/gmp_pro/csp/c28x_syscfg/iris_280039c_board`。
- 输出文件：`Debug/Iris_Node_Env.out`。

## 10. 风险点和未确认问题

- `Debug/subdir_rules.mk` 是 CCS 自动生成文件；如果以后由 CCS 重新生成，需确认 `.cproject` 中的本地 include 配置仍能生成等价规则。
- `D:/Github/gmp_pro` 仍是 GMP 库依赖路径，这是库依赖，不是训练任务工程拷贝依赖。
- 构建仍有项目既有警告，例如 `.sysmem` 默认大小提示，与本次本地化无关。

## 11. 建议 commit message

```text
Use local project paths for GMP training build
```

## 12. 本次修改摘要

本次修改将构建脚本从外部工程拷贝路径中独立出来，SysConfig 现在读取当前目录的 `F280039_Iris_node.syscfg`。编译 include 现在使用当前目录的 `user`、`xplt`、`oled_driver` 和 `Debug/syscfg`。代表性依赖文件也已更新为本地相对路径，避免旧依赖记录继续引用外部工程拷贝。最终通过 `gmake -C .\Debug all --jobs=1` 构建验证。
