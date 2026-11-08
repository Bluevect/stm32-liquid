# 测试

测试与调试工具都在 `tests/`。主机端测试不需要开发板。

## 快速运行

```powershell
powershell -ExecutionPolicy Bypass -File tests/run_host_tests.ps1
```

依赖 gcc、Python 3 和 Pillow。脚本依次完成：

1. 编译并运行液体行为测试
2. 生成液体预览图与动画

所有产物写入 `build/`。

## 主机端测试

### 液体行为测试

`tests/liquid_test.c`，32 项检查，覆盖：

- `BSP_OLED_DrawPixel` 的坐标、页地址、位序和擦除语义
- 四个方向的重力，水平面，板子倒置
- 无面内重力时液体保持不动
- 质量守恒、确定性、随机重力下的稳定性

测试直接复用固件的 `App/Src/liquid.c` 和 `Drivers/BSP/Src/bsp_oled.c`，主机端只替换
HAL 和 I2C 接口，见 `tests/host_hal_stub.c` 和 `tests/host_hal/`。

### 预览渲染

`tests/preview.c` 渲染 6 种姿态和 200 帧动画，输出 PGM 文件。`tests/make_preview.py`
提供三种转换：

| 命令 | 输出 |
|---|---|
| `python tests/make_preview.py contact` | `build/preview/liquid_preview.png` 姿态拼图 |
| `python tests/make_preview.py anim` | `build/preview/liquid_anim.gif` 完整动画 |
| `python tests/make_preview.py readme` | `docs/liquid_anim.gif` README 引用的小图 |

### 平衡探针

```powershell
build/liquid_probe.exe 0.5 0.87 2500
```

参数为 gx、gy 和步数，打印渲染后的亮像素数与质心。

## 运行状态检查

调试时可以读回运行中的 RAM，确认 IMU 和液体状态。先用 `arm-none-eabi-nm` 查符号
地址，例如：

```powershell
arm-none-eabi-nm -S build/Debug/stm32-liquid.elf | findstr "s_sample s_h oledInstance"
```

然后按地址 `dump_image`。`tests/decode_dut_run.py` 解析两帧显存和 IMU 样本，
`tests/decode_state.py` 解析液体高度数组、重力滤波值和显存，`tests/decode_once.py`
只打印一次重力映射。这些脚本默认的输入文件名都集中在 `tests/config.py`，换文件名只改
那一处。

## 文件清单

| 文件 | 用途 |
|---|---|
| `run_host_tests.ps1` | 一键编译并运行全部主机端测试 |
| `liquid_test.c` | 液体行为回归测试 |
| `preview.c` | 主机端渲染预览帧 |
| `make_preview.py` | PGM 转 GIF 和拼图 |
| `liquid_probe.c` | 平衡态覆盖率与质心探针 |
| `config.py` | 测试脚本的默认路径和文件名 |
| `decode_dut_run.py` | 解析两帧显存和 IMU 样本 |
| `decode_state.py` | 解析液体高度、重力和显存 |
| `decode_once.py` | 打印一次重力映射关系 |
| `host_hal_stub.c` | 主机端 HAL 和 I2C 替身 |
| `host_hal/stm32f1xx_hal.h` | 主机端 HAL 类型定义 |
