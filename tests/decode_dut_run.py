#!/usr/bin/env python3
"""解析连续两帧 SWD dump 并渲染成 PNG。

默认输入文件名见 tests/config.py：
  build/dut_sample.bin  ImuSample_t
  build/dut_frame1.bin  第一帧 oledInstance
  build/dut_frame2.bin  第二帧 oledInstance
"""

import struct

from config import (
    FRAMEBUFFER_OFFSET,
    FRAMEBUFFER_SIZE,
    RUN_DIFF_TXT,
    RUN_FRAME1_DUMP,
    RUN_FRAME1_PNG,
    RUN_FRAME2_DUMP,
    RUN_FRAME2_PNG,
    RUN_SAMPLE_DUMP,
)

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    Image = None


def fb_from_dump(path):
    raw = path.read_bytes()
    fb = raw[FRAMEBUFFER_OFFSET:FRAMEBUFFER_OFFSET + FRAMEBUFFER_SIZE]
    if len(fb) != FRAMEBUFFER_SIZE:
        raise SystemExit(f"{path}: short dump ({len(raw)} bytes)")
    return fb


def render(fb, path):
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


def main():
    sample = RUN_SAMPLE_DUMP.read_bytes()
    if len(sample) >= 28:
        ts, roll, pitch, yaw = struct.unpack("<Iiii", sample[0:16])
        gx, gy = struct.unpack("<ff", sample[16:24])
        print(f"timestamp={ts}ms  roll={roll / 100.0:+.2f}  pitch={pitch / 100.0:+.2f}  "
              f"yaw={yaw / 100.0:+.2f}")
        print(f"gravity=({gx:+.3f}, {gy:+.3f})  valid={sample[24]}")
    else:
        print("sample dump too short")

    f1 = fb_from_dump(RUN_FRAME1_DUMP)
    f2 = fb_from_dump(RUN_FRAME2_DUMP)
    lit1 = sum(bin(b).count("1") for b in f1)
    lit2 = sum(bin(b).count("1") for b in f2)
    diff = sum(1 for a, b in zip(f1, f2) if a != b)
    print(f"frame1 lit pixels={lit1}  frame2 lit pixels={lit2}  differing bytes={diff}")
    print("frames identical" if diff == 0 else "frames differ")

    render(f1, RUN_FRAME1_PNG)
    render(f2, RUN_FRAME2_PNG)
    RUN_DIFF_TXT.write_text(f"lit1={lit1} lit2={lit2} differing_bytes={diff}\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
