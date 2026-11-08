#!/usr/bin/env python3
"""快速查看一次 SWD 抓取的重力映射关系。

默认输入文件名见 tests/config.py。
"""

import struct

from config import GRAVITY_DUMP, SAMPLE_DUMP


def main():
    sample = SAMPLE_DUMP.read_bytes()
    _, roll, pitch, yaw = struct.unpack("<Iiii", sample[0:16])
    gx, gy = struct.unpack("<ff", sample[16:24])
    fgx, fgy = struct.unpack("<ff", GRAVITY_DUMP.read_bytes()[0:8])

    print(f"Pitch={pitch / 100:+.2f}  Roll={roll / 100:+.2f}  Yaw={yaw / 100:+.2f}")
    print(f"body gravity(filtered) = ({fgx:+.3f}, {fgy:+.3f})")
    print(f"screen gravity         = ({gx:+.3f}, {gy:+.3f})   x 正向右，y 正向下")


if __name__ == "__main__":
    main()
