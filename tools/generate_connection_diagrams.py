from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
WIRING_OUT = DOCS / "esp32-rover-wiring-diagram.png"
POWER_OUT = DOCS / "esp32-rover-power-flow-diagram.png"

W = 1800
H = 1200
BG = (246, 246, 248)
DARK = (30, 32, 36)
MID = (110, 110, 120)
RED = (204, 42, 42)
BLUE = (40, 96, 196)
GREEN = (42, 150, 84)
ORANGE = (215, 132, 37)
PURPLE = (108, 72, 180)
GRAY = (86, 92, 102)
WHITE = (255, 255, 255)


def load_font(size: int, bold: bool = False):
    candidates = [
        "C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeuib.ttf" if bold else "C:/Windows/Fonts/segoeui.ttf",
    ]
    for candidate in candidates:
        path = Path(candidate)
        if path.exists():
            return ImageFont.truetype(str(path), size=size)
    return ImageFont.load_default()


TITLE = load_font(42, bold=True)
SUB = load_font(28, bold=True)
TEXT = load_font(23)
SMALL = load_font(19)


def rounded(draw, box, fill, outline=None, width=3, radius=26):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline or fill, width=width)


def text_center(draw, box, text, font, fill):
    x1, y1, x2, y2 = box
    bbox = draw.multiline_textbbox((0, 0), text, font=font, spacing=4, align="center")
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    draw.multiline_text(
        ((x1 + x2 - tw) / 2, (y1 + y2 - th) / 2),
        text,
        font=font,
        fill=fill,
        spacing=4,
        align="center",
    )


def node(draw, box, title, lines, fill, accent=None):
    rounded(draw, box, fill=fill, outline=accent or DARK, width=4, radius=28)
    x1, y1, x2, y2 = box
    draw.rounded_rectangle((x1, y1, x2, y1 + 48), radius=28, fill=accent or fill, outline=accent or fill)
    draw.text((x1 + 18, y1 + 10), title, font=SUB, fill=WHITE if accent else DARK)
    current_y = y1 + 68
    for line in lines:
        draw.text((x1 + 18, current_y), line, font=TEXT, fill=DARK)
        current_y += 32


def title_block(draw, title, subtitle):
    rounded(draw, (40, 30, W - 40, 140), fill=(255, 241, 241), outline=RED, width=4, radius=32)
    draw.text((70, 54), title, font=TITLE, fill=RED)
    draw.text((74, 103), subtitle, font=SMALL, fill=MID)


def arrow(draw, start, end, fill=DARK, width=8):
    draw.line((start, end), fill=fill, width=width)
    ex, ey = end
    if abs(end[0] - start[0]) >= abs(end[1] - start[1]):
        direction = 1 if end[0] >= start[0] else -1
        tip = [(ex, ey), (ex - 24 * direction, ey - 14), (ex - 24 * direction, ey + 14)]
    else:
        direction = 1 if end[1] >= start[1] else -1
        tip = [(ex, ey), (ex - 14, ey - 24 * direction), (ex + 14, ey - 24 * direction)]
    draw.polygon(tip, fill=fill)


def wiring_diagram():
    image = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(image)
    title_block(draw, "ESP32 Rover Wiring Diagram", "Current firmware pin map for sensors, motor drivers, onboard LED, and shared grounds")

    node(draw, (700, 260, 1100, 540), "ESP32-S3 N16R8", [
        "GPIO4  -> Front TRIG",
        "GPIO5  <- Front ECHO",
        "GPIO6  -> Left TRIG",
        "GPIO7  <- Left ECHO",
        "GPIO8  -> Right TRIG",
        "GPIO9  <- Right ECHO",
        "GPIO48 -> Onboard RGB LED",
    ], fill=(232, 240, 255), accent=BLUE)

    node(draw, (700, 570, 1100, 930), "ESP32 Motor Pins", [
        "GPIO10 -> Left L298 ENA",
        "GPIO11 -> Left L298 IN1",
        "GPIO12 -> Left L298 IN2",
        "GPIO13 -> Left L298 ENB",
        "GPIO14 -> Left L298 IN3",
        "GPIO15 -> Left L298 IN4",
        "GPIO16 -> Right L298 ENA",
        "GPIO17 -> Right L298 IN1",
        "GPIO18 -> Right L298 IN2",
        "GPIO21 -> Right L298 ENB",
        "GPIO38 -> Right L298 IN3",
        "GPIO39 -> Right L298 IN4",
    ], fill=(236, 246, 240), accent=GREEN)

    node(draw, (110, 240, 450, 390), "Front HC-SR04", [
        "VCC -> 5V logic rail",
        "GND -> common ground",
        "TRIG -> GPIO4",
        "ECHO -> divider -> GPIO5",
    ], fill=(247, 245, 232), accent=ORANGE)
    node(draw, (110, 435, 450, 585), "Left HC-SR04", [
        "VCC -> 5V logic rail",
        "GND -> common ground",
        "TRIG -> GPIO6",
        "ECHO -> divider -> GPIO7",
    ], fill=(247, 245, 232), accent=ORANGE)
    node(draw, (110, 630, 450, 780), "Right HC-SR04", [
        "VCC -> 5V logic rail",
        "GND -> common ground",
        "TRIG -> GPIO8",
        "ECHO -> divider -> GPIO9",
    ], fill=(247, 245, 232), accent=ORANGE)

    node(draw, (1260, 290, 1650, 480), "Left L298 Module", [
        "ENA <- GPIO10",
        "IN1 <- GPIO11",
        "IN2 <- GPIO12",
        "ENB <- GPIO13",
        "IN3 <- GPIO14",
        "IN4 <- GPIO15",
        "OUT A -> front-left motor",
        "OUT B -> rear-left motor",
    ], fill=(255, 238, 233), accent=RED)
    node(draw, (1260, 560, 1650, 780), "Right L298 Module", [
        "ENA <- GPIO16",
        "IN1 <- GPIO17",
        "IN2 <- GPIO18",
        "ENB <- GPIO21",
        "IN3 <- GPIO38",
        "IN4 <- GPIO39",
        "OUT A -> front-right motor",
        "OUT B -> rear-right motor",
    ], fill=(255, 238, 233), accent=RED)

    node(draw, (160, 930, 520, 1090), "ECHO Divider Rule", [
        "HC-SR04 ECHO is 5V logic.",
        "Reduce it to 3.3V before ESP32 input.",
        "Example: 10k top + 20k bottom.",
    ], fill=(239, 238, 252), accent=PURPLE)
    node(draw, (690, 960, 1110, 1110), "Reserved Pins", [
        "Do not use GPIO35, GPIO36, GPIO37",
        "on this N16R8 board.",
        "They are reserved by memory hardware.",
    ], fill=(242, 242, 242), accent=GRAY)
    node(draw, (1230, 930, 1660, 1100), "Ground Rule", [
        "ESP32 GND, both L298 GNDs,",
        "all sensor GNDs, BMS output GND,",
        "and buck converter GNDs must join.",
    ], fill=(236, 246, 240), accent=GREEN)

    arrow(draw, (450, 315), (700, 320))
    arrow(draw, (450, 510), (700, 355))
    arrow(draw, (450, 705), (700, 390))
    arrow(draw, (1100, 660), (1260, 385))
    arrow(draw, (1100, 770), (1260, 665))

    draw.text((58, H - 48), "Reference diagram for docs/wiring-reference.md and src/main.cpp", font=SMALL, fill=MID)
    image.save(WIRING_OUT)


def power_diagram():
    image = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(image)
    title_block(draw, "ESP32 Rover Power Flow", "Battery, protection, switch, motor rail, buck rail, and what each rail powers")

    node(draw, (80, 500, 260, 660), "3S Battery Pack", [
        "3 x Samsung 18650",
        "series holder",
        "~12.6V full",
    ], fill=(236, 246, 240), accent=GREEN)
    node(draw, (320, 500, 500, 660), "3S BMS", [
        "battery protection",
        "pack balance / cutoff",
    ], fill=(247, 245, 232), accent=ORANGE)
    node(draw, (560, 500, 710, 660), "10A Fuse", [
        "inline protection",
    ], fill=(255, 238, 233), accent=RED)
    node(draw, (770, 500, 950, 660), "Main Switch", [
        "system on / off",
    ], fill=(232, 240, 255), accent=BLUE)

    node(draw, (1100, 260, 1410, 430), "Motor Rail", [
        "battery-level motor supply",
        "to both L298 motor inputs",
        "add bulk capacitor nearby",
    ], fill=(255, 238, 233), accent=RED)
    node(draw, (1100, 500, 1410, 700), "XL4015 5V Buck", [
        "set output to 5.0V",
        "main logic rail",
        "powers ESP32 and sensors",
    ], fill=(236, 246, 240), accent=GREEN)
    node(draw, (1100, 770, 1410, 980), "Optional 2nd XL4015", [
        "future accessories",
        "camera / extra lights / spare rail",
    ], fill=(242, 242, 242), accent=GRAY)

    node(draw, (1510, 220, 1730, 390), "Left L298", [
        "motor supply from rail",
        "logic ground shared",
    ], fill=(255, 238, 233), accent=RED)
    node(draw, (1510, 420, 1730, 590), "Right L298", [
        "motor supply from rail",
        "logic ground shared",
    ], fill=(255, 238, 233), accent=RED)
    node(draw, (1510, 640, 1730, 820), "ESP32-S3", [
        "5V / VBUS input",
        "from buck rail",
    ], fill=(232, 240, 255), accent=BLUE)
    node(draw, (1510, 860, 1730, 1080), "3 x HC-SR04", [
        "VCC from 5V rail",
        "signal grounds shared",
        "echo through dividers",
    ], fill=(247, 245, 232), accent=ORANGE)

    arrow(draw, (260, 580), (320, 580))
    arrow(draw, (500, 580), (560, 580))
    arrow(draw, (710, 580), (770, 580))
    draw.line((950, 580, 1030, 580), fill=DARK, width=8)
    draw.line((1030, 580, 1030, 345), fill=DARK, width=8)
    draw.line((1030, 580, 1030, 600), fill=DARK, width=8)
    draw.line((1030, 580, 1030, 875), fill=DARK, width=8)
    arrow(draw, (1030, 345), (1100, 345))
    arrow(draw, (1030, 600), (1100, 600))
    arrow(draw, (1030, 875), (1100, 875))

    arrow(draw, (1410, 345), (1510, 305))
    arrow(draw, (1410, 345), (1510, 505))
    arrow(draw, (1410, 600), (1510, 730))
    arrow(draw, (1410, 600), (1510, 940))

    draw.text((1080, 455), "Raw battery side", font=SMALL, fill=MID)
    draw.text((1080, 720), "Regulated 5V logic side", font=SMALL, fill=MID)

    rounded(draw, (70, 910, 570, 1110), fill=(255, 245, 225), outline=ORANGE, width=3, radius=26)
    draw.text((98, 945), "Important power notes", font=SUB, fill=ORANGE)
    for idx, line in enumerate([
        "Do not use AMS1117 directly from the 3S pack as the main ESP supply.",
        "All grounds must be common across battery, ESP32, drivers, and sensors.",
        "The 5V buck should be set and verified with the multimeter before connection.",
        "Test motor rail and logic rail separately before full assembly.",
    ]):
        draw.text((98, 995 + idx * 28), line, font=SMALL, fill=DARK)

    draw.text((58, H - 48), "Reference diagram for docs/wiring-reference.md and the user manual PDF", font=SMALL, fill=MID)
    image.save(POWER_OUT)


def main():
    DOCS.mkdir(parents=True, exist_ok=True)
    wiring_diagram()
    power_diagram()
    print(f"Created {WIRING_OUT}")
    print(f"Created {POWER_OUT}")


if __name__ == "__main__":
    main()
