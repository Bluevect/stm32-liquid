"""测试与调试脚本的默认路径和文件名。

所有路径都从项目根目录解析，脚本在任意工作目录启动都能找到文件。
需要换文件名时只改这里。
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# 目录
BUILD_DIR = ROOT / "build"
PREVIEW_DIR = BUILD_DIR / "preview"
ANIM_DIR = PREVIEW_DIR / "anim"
DOCS_DIR = ROOT / "docs"

# 预览图输出
PREVIEW_CONTACT = PREVIEW_DIR / "liquid_preview.png"
PREVIEW_GIF = PREVIEW_DIR / "liquid_anim.gif"
README_GIF = DOCS_DIR / "liquid_anim.gif"

# SWD 抓取的运行状态，默认文件名
SAMPLE_DUMP = BUILD_DIR / "state_sample.bin"
GRAVITY_DUMP = BUILD_DIR / "state_gx.bin"
HEIGHT_DUMP = BUILD_DIR / "state_h.bin"
FRAMEBUFFER_DUMP = BUILD_DIR / "state_fb.bin"
FRAMEBUFFER_PNG = BUILD_DIR / "state_fb.png"

# 连续两帧抓取，默认文件名
RUN_SAMPLE_DUMP = BUILD_DIR / "dut_sample.bin"
RUN_FRAME1_DUMP = BUILD_DIR / "dut_frame1.bin"
RUN_FRAME2_DUMP = BUILD_DIR / "dut_frame2.bin"
RUN_FRAME1_PNG = BUILD_DIR / "dut_frame1.png"
RUN_FRAME2_PNG = BUILD_DIR / "dut_frame2.png"
RUN_DIFF_TXT = BUILD_DIR / "dut_frames_diff.txt"

# 显存布局：BSP_OLED_t 前 8 字节是 I2C 字段，显存从偏移 8 开始
FRAMEBUFFER_OFFSET = 8
FRAMEBUFFER_SIZE = 1024
OLED_WIDTH = 128
OLED_HEIGHT = 64
