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


def draw_double_arrow(draw: ImageDraw.ImageDraw, box, color):
    left, top, right, bottom = box
    mid_y = (top + bottom) // 2
    shaft_height = 34
    head_width = 58
    head_height = 78
    draw.rounded_rectangle(
        (left + head_width, mid_y - shaft_height // 2, right - head_width, mid_y + shaft_height // 2),
        radius=shaft_height // 2,
        fill=color,
    )
    draw.polygon(
        [
            (left, mid_y),
            (left + head_width, mid_y - head_height // 2),
            (left + head_width, mid_y + head_height // 2),
        ],
        fill=color,
    )
    draw.polygon(
        [
            (right, mid_y),
            (right - head_width, mid_y - head_height // 2),
            (right - head_width, mid_y + head_height // 2),
        ],
        fill=color,
    )


def draw_keycap(draw: ImageDraw.ImageDraw, box, label: str, fill, outline, label_color):
    left, top, right, bottom = box
    rounded_rect(draw, box, 62, fill, outline, 8)
    rounded_rect(
        draw,
        (left + 28, top + 26, right - 28, bottom - 30),
        42,
        tuple(min(channel + 18, 255) for channel in fill[:3]) + (255,),
        tuple(max(channel - 22, 0) for channel in outline[:3]) + (255,),
        4,
    )
    centered_text(draw, (left + 18, top + 18, right - 18, bottom - 8), label, label_color, font(112, bold=True))


def make_icon() -> Image.Image:
    size = 1024
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    rounded_rect(sd, (96, 130, 928, 858), 150, (40, 55, 68, 95))
    shadow = shadow.filter(ImageFilter.GaussianBlur(36))
    img.alpha_composite(shadow, (0, 20))

    draw = ImageDraw.Draw(img)
    rounded_rect(draw, (82, 92, 942, 840), 160, (249, 246, 235, 255), (215, 209, 190, 255), 10)
    rounded_rect(draw, (130, 144, 894, 792), 122, (255, 253, 244, 255), (235, 229, 211, 255), 5)

    # The icon should read as "WIN <-> ALT" even at small sizes.
    draw_keycap(
        draw,
        (154, 276, 440, 566),
        "WIN",
        (47, 169, 173, 255),
        (26, 124, 132, 255),
        (255, 255, 250, 255),
    )
    draw_keycap(
        draw,
        (584, 276, 870, 566),
        "ALT",
        (238, 104, 87, 255),
        (194, 73, 61, 255),
        (255, 255, 250, 255),
    )

    draw_double_arrow(draw, (418, 382, 606, 468), (54, 75, 88, 255))

    # Small cute face, kept secondary so the key-swap meaning stays dominant.
    draw.ellipse((384, 640, 424, 680), fill=(46, 58, 70, 255))
    draw.ellipse((600, 640, 640, 680), fill=(46, 58, 70, 255))
    draw.arc((438, 648, 586, 738), 18, 162, fill=(46, 58, 70, 255), width=12)
    draw.ellipse((306, 690, 364, 738), fill=(255, 144, 140, 195))
    draw.ellipse((660, 690, 718, 738), fill=(255, 144, 140, 195))

    # Small sparkle accents
    for cx, cy, r, color in [
        (204, 218, 18, (250, 204, 87, 255)),
        (818, 224, 16, (41, 171, 171, 255)),
        (794, 660, 12, (239, 111, 92, 255)),
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
