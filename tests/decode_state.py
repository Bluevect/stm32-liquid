#!/usr/bin/env python3
"""解析 SWD 抓回的运行状态并渲染显存。

默认输入文件名见 tests/config.py：
  build/state_sample.bin  ImuSample_t
  build/state_gx.bin      滤波后的板体系重力
  build/state_h.bin       液体高度数组，32 个 float
  build/state_fb.bin      oledInstance，前 8 字节是 I2C 字段
"""

import struct

from config import (
    FRAMEBUFFER_OFFSET,
    FRAMEBUFFER_PNG,
    FRAMEBUFFER_SIZE,
    GRAVITY_DUMP,
    HEIGHT_DUMP,
    SAMPLE_DUMP,
    FRAMEBUFFER_DUMP,
)

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    Image = None


def render_framebuffer(raw, path):
    fb = raw[FRAMEBUFFER_OFFSET:FRAMEBUFFER_OFFSET + FRAMEBUFFER_SIZE]
    if len(fb) != FRAMEBUFFER_SIZE:
        raise SystemExit(f"{FRAMEBUFFER_DUMP}: short framebuffer dump")

    lit = sum(bin(b).count("1") for b in fb)
    print(f"framebuffer lit pixels = {lit}")

    if Image is None:
        print("Pillow missing, skip PNG")
        return

    img = Image.new("L", (128, 64), 0)
    px = img.load()
    for y in range(64):
        for x in range(128):
            if (fb[(y // 8) * 128 + x] >> (y % 8)) & 1:
                px[x, y] = 255
    img.resize((512, 256), Image.NEAREST).save(path)
    print(f"saved {path}")


def main():
    sample = SAMPLE_DUMP.read_bytes()
    ts, roll, pitch, yaw = struct.unpack("<Iiii", sample[0:16])
    gx, gy = struct.unpack("<ff", sample[16:24])
    print(f"sample ts={ts} roll={roll / 100:+.2f} pitch={pitch / 100:+.2f} yaw={yaw / 100:+.2f}")
    print(f"mapped gravity=({gx:+.3f},{gy:+.3f}) valid={sample[24]}")

    fgx, fgy = struct.unpack("<ff", GRAVITY_DUMP.read_bytes()[0:8])
    print(f"filtered body gravity = ({fgx:+.3f},{fgy:+.3f})")

    heights = struct.unpack("<32f", HEIGHT_DUMP.read_bytes())
    print("liquid heights:", " ".join(f"{h:.2f}" for h in heights))
    print(f"total liquid = {sum(heights):.2f}  min = {min(heights):.3f}  max = {max(heights):.3f}")

    render_framebuffer(FRAMEBUFFER_DUMP.read_bytes(), FRAMEBUFFER_PNG)


if __name__ == "__main__":
    main()
