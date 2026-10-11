# finish

STM32F103 角度闭环控制固件（CLion + CMake + Ninja + arm-none-eabi-gcc）。

## 功能

- MPU6050 姿态采集：I2C1，SCL=PB6，SDA=PB7，400 kHz
- SSD1306 OLED 显示：I2C2，SCL=PB10，SDA=PB11，400 kHz，地址 0x78
- 编码器测速与电机 PWM 输出
- 角度 PID 闭环控制，参数可通过上位机在线调整
- 双按键（速度 / 角度）与模拟旋钮设定
- VOFA+ 串口遥测与下行命令

## 目录结构

- `Core/` 应用代码（`Inc/` 头文件，`Src/` 实现）
- `Drivers/` STM32F1xx HAL 与 CMSIS
- `cmake/` 工具链与 CubeMX 子工程
- `docs/` 控制框架与代码走查文档
- `verify/key-host-test/` 按键逻辑的主机侧单元测试

## 构建

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake"
cmake --build build --parallel
```

## 文档

- `docs/control_framework.md`：控制框架、引脚分配、外设参数、VOFA+ 配置
- `docs/non_cubemx_code_walkthrough.md`：非 CubeMX 生成代码的逐行说明
