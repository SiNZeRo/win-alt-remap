#!/usr/bin/env python3

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
PNG_PATH = ASSETS / "app-icon.png"
ICO_PATH = ASSETS / "app-icon.ico"


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    ]
    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            pass
    return ImageFont.load_default()


def rounded_rect(draw: ImageDraw.ImageDraw, box, radius: int, fill, outline=None, width: int = 1):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def centered_text(draw: ImageDraw.ImageDraw, box, text: str, fill, text_font):
    left, top, right, bottom = box
    text_box = draw.textbbox((0, 0), text, font=text_font)
    width = text_box[2] - text_box[0]
    height = text_box[3] - text_box[1]
    x = left + (right - left - width) / 2
    y = top + (bottom - top - height) / 2 - 2
    draw.text((x, y), text, fill=fill, font=text_font)


def draw_arrow(draw: ImageDraw.ImageDraw, start, end, color, width: int):
    draw.line([start, end], fill=color, width=width)
    sx, sy = start
    ex, ey = end
    direction = 1 if ex >= sx else -1
    head = 22
    draw.polygon(
        [(ex, ey), (ex - direction * head, ey - 13), (ex - direction * head, ey + 13)],
        fill=color,
    )


def make_icon() -> Image.Image:
    size = 1024
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    rounded_rect(sd, (160, 190, 864, 840), 170, (40, 55, 68, 95))
    shadow = shadow.filter(ImageFilter.GaussianBlur(34))
    img.alpha_composite(shadow, (0, 18))

    draw = ImageDraw.Draw(img)
    rounded_rect(draw, (136, 132, 888, 832), 176, (248, 244, 230, 255), (216, 210, 190, 255), 8)
    rounded_rect(draw, (190, 186, 834, 778), 134, (255, 252, 241, 255), (236, 229, 207, 255), 5)

    # Face
    draw.ellipse((360, 400, 415, 455), fill=(46, 58, 70, 255))
    draw.ellipse((609, 400, 664, 455), fill=(46, 58, 70, 255))
    draw.arc((405, 438, 619, 590), 18, 162, fill=(46, 58, 70, 255), width=16)
    draw.ellipse((278, 478, 352, 538), fill=(255, 144, 140, 210))
    draw.ellipse((672, 478, 746, 538), fill=(255, 144, 140, 210))

    # Modifier tiles
    rounded_rect(draw, (198, 664, 407, 785), 38, (41, 171, 171, 255), (29, 132, 134, 255), 5)
    rounded_rect(draw, (617, 664, 826, 785), 38, (239, 111, 92, 255), (198, 75, 62, 255), 5)
    centered_text(draw, (198, 664, 407, 785), "ALT", (255, 255, 250, 255), font(58, bold=True))
    centered_text(draw, (617, 664, 826, 785), "WIN", (255, 255, 250, 255), font(58, bold=True))

    # Swap arrows
    draw_arrow(draw, (418, 706), (583, 706), (64, 91, 107, 255), 15)
    draw_arrow(draw, (606, 744), (441, 744), (64, 91, 107, 255), 15)

    # Small sparkle accents
    for cx, cy, r, color in [
        (235, 230, 20, (250, 204, 87, 255)),
        (795, 254, 15, (41, 171, 171, 255)),
        (774, 598, 11, (239, 111, 92, 255)),
    ]:
        draw.line((cx - r, cy, cx + r, cy), fill=color, width=6)
        draw.line((cx, cy - r, cx, cy + r), fill=color, width=6)

    return img


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    icon = make_icon()
    icon.save(PNG_PATH)

    sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    icon.save(ICO_PATH, sizes=sizes)
    print(PNG_PATH)
    print(ICO_PATH)


if __name__ == "__main__":
    main()
