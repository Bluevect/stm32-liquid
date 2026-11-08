"""Turn the PGM frames from tests/preview.c into a preview GIF / contact sheet.

Usage (from the project root):
    python tests/make_preview.py anim      # animated GIF in build/preview
    python tests/make_preview.py contact   # 2x3 contact sheet PNG
    python tests/make_preview.py readme    # small GIF referenced by README
"""
from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageFont

from config import ANIM_DIR, PREVIEW_CONTACT, PREVIEW_DIR, PREVIEW_GIF, README_GIF

FRAME_DIR = PREVIEW_DIR
SCALE = 4


def load_font(size: int):
    for name in ("arial.ttf", "segoeui.ttf", "consola.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def load_frame(path: Path) -> Image.Image:
    img = Image.open(path).convert("L").point(lambda v: 255 if v > 0 else 0)
    return img.resize((img.width * SCALE, img.height * SCALE), Image.NEAREST)


def make_gif() -> None:
    frames = sorted(ANIM_DIR.glob("anim*.pgm"))
    if not frames:
        raise SystemExit("no animation frames; run `build/preview.exe anim` first")
    images = []
    for path in frames:
        panel = Image.new("RGB", (128 * SCALE, 64 * SCALE), (0, 0, 0))
        panel.paste((225, 240, 255), mask=load_frame(path))
        images.append(panel)
    out = PREVIEW_GIF
    images[0].save(out, save_all=True, append_images=images[1:], duration=40,
                   loop=0, optimize=True)
    print(f"wrote {out} ({len(images)} frames, {images[0].size[0]}x{images[0].size[1]})")


def make_contact() -> None:
    frames = sorted(FRAME_DIR.glob("frame*.pgm"))
    labels_path = FRAME_DIR / "frames.txt"
    labels = labels_path.read_text(encoding="utf-8").splitlines() if labels_path.exists() else []
    if not frames:
        raise SystemExit("no still frames; run `build/preview.exe contact` first")
    images = [load_frame(p) for p in frames]
    pw, ph = images[0].size
    cols, pad, label_h, title_h = 3, 16, 28, 44
    rows = (len(images) + cols - 1) // cols
    cell_w, cell_h = pw + pad, ph + label_h + pad
    width = cols * cell_w + pad
    height = title_h + rows * cell_h + pad
    sheet = Image.new("RGB", (width, height), (18, 20, 26))
    draw = ImageDraw.Draw(sheet)
    draw.text((pad, 12), "STM32F103 - liquid gravity on SSD1306 128x64 (host render)",
              fill=(235, 238, 245), font=load_font(20))
    for index, img in enumerate(images):
        col, row = index % cols, index // cols
        x, y = pad + col * cell_w, title_h + row * cell_h
        panel = Image.new("RGB", (pw, ph), (0, 0, 0))
        panel.paste((225, 240, 255), mask=img)
        sheet.paste(panel, (x, y))
        draw.rectangle([x - 1, y - 1, x + pw, y + ph], outline=(70, 80, 100))
        label = labels[index].replace("_", " ") if index < len(labels) else frames[index].stem
        draw.text((x, y + ph + 4), label, fill=(150, 200, 255), font=load_font(15))
    out = PREVIEW_CONTACT
    sheet.save(out)
    print(f"wrote {out} ({width}x{height})")


def make_readme_gif() -> None:
    """生成 README 里引用的小尺寸 GIF，二值调色板，文件尽量小。"""
    frames = sorted(ANIM_DIR.glob("anim*.pgm"))
    if not frames:
        raise SystemExit("no animation frames; run `build/preview.exe anim` first")

    images = []
    for path in frames[::2]:
        img = Image.open(path).convert("L").point(lambda v: 255 if v > 0 else 0)
        img = img.resize((256, 128), Image.NEAREST)
        images.append(img.convert("P", palette=Image.ADAPTIVE, colors=2))

    README_GIF.parent.mkdir(parents=True, exist_ok=True)
    images[0].save(README_GIF, save_all=True, append_images=images[1:],
                   duration=80, loop=0, optimize=True, disposal=2)
    print(f"wrote {README_GIF} ({len(images)} frames, {README_GIF.stat().st_size} bytes)")


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "contact"
    if mode == "anim":
        make_gif()
    elif mode == "readme":
        make_readme_gif()
    else:
        make_contact()
