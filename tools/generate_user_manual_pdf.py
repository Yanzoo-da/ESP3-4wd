from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT_PATH = ROOT / "docs" / "ESP32-S3-4WD-Rover-User-Manual.pdf"
WIRING_DIAGRAM_PATH = ROOT / "docs" / "esp32-rover-wiring-diagram.png"
POWER_FLOW_IMAGE_PATH = ROOT / "docs" / "esp32-rover-power-flow-diagram.png"
SENSOR_PLACEMENT_IMAGE_PATH = ROOT / "docs" / "esp32-rover-sensor-placement.png"

PAGE_W = 1240
PAGE_H = 1754
MARGIN = 80

RED = (206, 32, 32)
DARK = (24, 24, 24)
MID = (92, 92, 92)
LIGHT = (248, 248, 248)
GREEN = (35, 152, 84)
BLUE = (43, 106, 201)
ORANGE = (225, 135, 36)
PALE = (255, 244, 244)
GRAY_BOX = (244, 244, 244)

HOTSPOT_URL = "http://192.168.4.1/"
STATUS_URL = "http://192.168.4.1/status"
REPO_URL = "https://github.com/Yanzoo-da/ESP3-4wd"
PAGES_SETTINGS_URL = "https://github.com/Yanzoo-da/ESP3-4wd/settings/pages"
PAGES_HOME_URL = "https://yanzoo-da.github.io/ESP3-4wd/"
REMOTE_APP_URL = "https://yanzoo-da.github.io/ESP3-4wd/remote-control/"
HIVEMQ_CONSOLE_URL = "https://console.hivemq.cloud"
HIVEMQ_HOST = "8038be31051d4e368ce62a5753aaf95d.s1.eu.hivemq.cloud"
HIVEMQ_TLS_PORT = "8883"
HIVEMQ_WS_PORT = "8884"
HIVEMQ_WS_PATH = "/mqtt"
HIVEMQ_USERNAME = "Yanzoo4wd"
MQTT_TOPIC_BASE = "rover/yanzoo-car-1"


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


TITLE_FONT = load_font(40, bold=True)
SUBTITLE_FONT = load_font(24, bold=True)
TEXT_FONT = load_font(22)
SMALL_FONT = load_font(17)

try:
    RESAMPLE_LANCZOS = Image.Resampling.LANCZOS
except AttributeError:
    RESAMPLE_LANCZOS = Image.LANCZOS


def wrap_text(draw: ImageDraw.ImageDraw, text: str, font, width: int):
    words = text.split()
    lines = []
    current = ""
    for word in words:
        trial = word if not current else f"{current} {word}"
        if draw.textbbox((0, 0), trial, font=font)[2] <= width:
            current = trial
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines


def draw_wrapped(draw, text, x, y, width, font, fill=DARK, line_gap=7):
    lines = wrap_text(draw, text, font, width)
    line_h = draw.textbbox((0, 0), "Ag", font=font)[3] + line_gap
    for index, line in enumerate(lines):
        draw.text((x, y + index * line_h), line, font=font, fill=fill)
    return y + len(lines) * line_h


def header(draw, title, subtitle=None):
    draw.rounded_rectangle(
        (MARGIN, MARGIN, PAGE_W - MARGIN, MARGIN + 120),
        radius=28,
        fill=(255, 241, 241),
        outline=RED,
        width=4,
    )
    draw.text((MARGIN + 30, MARGIN + 18), title, font=TITLE_FONT, fill=RED)
    if subtitle:
        draw.text((MARGIN + 34, MARGIN + 72), subtitle, font=SMALL_FONT, fill=MID)


def footer(draw, page_num):
    footer_text = f"ESP32-S3 4WD Rover User Manual  |  Page {page_num}"
    draw.text((MARGIN, PAGE_H - 46), footer_text, font=SMALL_FONT, fill=MID)


def section_box(draw, x, y, w, h, title):
    draw.rounded_rectangle((x, y, x + w, y + h), radius=24, fill=(252, 252, 252), outline=(210, 210, 210), width=2)
    draw.rounded_rectangle((x, y, x + w, y + 48), radius=24, fill=(255, 243, 243), outline=RED, width=0)
    draw.text((x + 20, y + 10), title, font=SUBTITLE_FONT, fill=RED)


def node(draw, box, label, fill, text_fill=(255, 255, 255)):
    draw.rounded_rectangle(box, radius=26, fill=fill)
    x1, y1, x2, y2 = box
    text_box = draw.textbbox((0, 0), label, font=TEXT_FONT)
    tw = text_box[2]
    th = text_box[3]
    draw.text(((x1 + x2 - tw) / 2, (y1 + y2 - th) / 2 - 2), label, font=TEXT_FONT, fill=text_fill)


def arrow(draw, start, end, fill=DARK, width=7):
    draw.line((start, end), fill=fill, width=width)
    ex, ey = end
    draw.polygon([(ex, ey), (ex - 20, ey - 12), (ex - 20, ey + 12)], fill=fill)


def bullet_list(draw, x, y, width, items):
    current_y = y
    for item in items:
        draw.ellipse((x, current_y + 7, x + 10, current_y + 17), fill=RED)
        current_y = draw_wrapped(draw, item, x + 24, current_y, width - 24, TEXT_FONT, fill=DARK)
        current_y += 6
    return current_y


def numbered_list(draw, x, y, width, items):
    current_y = y
    for index, item in enumerate(items, start=1):
        number = f"{index}."
        draw.text((x, current_y), number, font=SUBTITLE_FONT, fill=RED)
        current_y = draw_wrapped(draw, item, x + 42, current_y + 2, width - 42, TEXT_FONT, fill=DARK)
        current_y += 8
    return current_y


def info_bar(draw, x, y, w, title, value, fill=PALE, outline=RED):
    draw.rounded_rectangle((x, y, x + w, y + 80), radius=20, fill=fill, outline=outline, width=2)
    draw.text((x + 18, y + 10), title, font=SMALL_FONT, fill=MID)
    draw.text((x + 18, y + 38), value, font=TEXT_FONT, fill=DARK)


def paste_scaled_image(page, image_path: Path, box):
    image = Image.open(image_path).convert("RGB")
    x1, y1, x2, y2 = box
    max_w = x2 - x1
    max_h = y2 - y1
    scale = min(max_w / image.width, max_h / image.height)
    new_w = max(1, int(image.width * scale))
    new_h = max(1, int(image.height * scale))
    resized = image.resize((new_w, new_h), RESAMPLE_LANCZOS)
    paste_x = x1 + (max_w - new_w) // 2
    paste_y = y1 + (max_h - new_h) // 2
    page.paste(resized, (paste_x, paste_y))


def cover_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "ESP32-S3 4WD Rover Manual", "From first power-on to remote control from another internet connection")

    draw.rounded_rectangle((100, 220, PAGE_W - 100, 760), radius=40, fill=(255, 245, 245), outline=RED, width=4)
    draw.text((145, 280), "What this guide teaches", font=SUBTITLE_FONT, fill=RED)
    intro = (
        "This PDF explains the full workflow from zero: how to use the rover through the ESP hotspot, "
        "how to connect it to your home router, how to publish the GitHub Pages phone website, how to "
        "create an MQTT cloud broker, how the current firmware pin map is wired, how power should flow "
        "through the full rover, and how to drive the rover while your phone and rover are on "
        "different internet connections."
    )
    draw_wrapped(draw, intro, 145, 335, PAGE_W - 290, TEXT_FONT)

    node(draw, (165, 540, 395, 660), "Phone", BLUE)
    node(draw, (505, 540, 735, 660), "Cloud", ORANGE)
    node(draw, (845, 540, 1075, 660), "ESP32 Rover", GREEN)
    arrow(draw, (395, 600), (505, 600))
    arrow(draw, (735, 600), (845, 600))

    section_box(draw, 100, 840, PAGE_W - 200, 760, "The Three Real Ways You Will Use It")
    numbered_list(draw, 135, 920, PAGE_W - 270, [
        "Phone connected directly to the ESP32 hotspot. Use this for first setup and recovery. The address is always http://192.168.4.1/",
        "Phone and rover connected to the same home Wi-Fi router. Use the router IP shown by the rover page, for example http://192.168.1.23/",
        "Phone far away from home while the rover stays at home on Wi-Fi. Use the GitHub Pages remote app plus the MQTT cloud broker."
    ])

    footer(draw, 1)
    return image


def key_urls_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Important URLs", "These are the exact addresses you will use most often")

    info_bar(draw, 110, 220, 1020, "ESP hotspot control page", HOTSPOT_URL)
    info_bar(draw, 110, 320, 1020, "ESP hotspot status page", STATUS_URL)
    info_bar(draw, 110, 420, 1020, "GitHub repository", REPO_URL)
    info_bar(draw, 110, 520, 1020, "GitHub Pages settings", PAGES_SETTINGS_URL)
    info_bar(draw, 110, 620, 1020, "GitHub Pages docs home", PAGES_HOME_URL)
    info_bar(draw, 110, 720, 1020, "Remote phone control page", REMOTE_APP_URL)
    info_bar(draw, 110, 820, 1020, "HiveMQ console", HIVEMQ_CONSOLE_URL, fill=(247, 245, 236), outline=ORANGE)

    section_box(draw, 110, 940, 1020, 560, "How To Think About These URLs")
    bullet_list(draw, 145, 1015, 950, [
        "Use 192.168.4.1 only when your phone is connected to the ESP32 hotspot.",
        "Use the router IP only when your phone and rover are on the same home Wi-Fi.",
        "Use the GitHub Pages remote app when your phone is away from home and on a different internet connection.",
        "The remote app does not connect to the rover directly. It talks to the MQTT cloud broker, and the rover talks to that same broker.",
        "The router IP is not fixed unless you reserve it in your router."
    ])

    footer(draw, 2)
    return image


def project_values_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Current Project Values", "The exact GitHub and HiveMQ values currently used by this rover project")

    section_box(draw, 90, 220, PAGE_W - 180, 620, "GitHub Values")
    info_bar(draw, 120, 300, 1000, "Repository", REPO_URL)
    info_bar(draw, 120, 400, 1000, "Pages settings", PAGES_SETTINGS_URL)
    info_bar(draw, 120, 500, 1000, "Pages home", PAGES_HOME_URL)
    info_bar(draw, 120, 600, 1000, "Remote control page", REMOTE_APP_URL)

    section_box(draw, 90, 900, PAGE_W - 180, 620, "HiveMQ And Broker Values")
    info_bar(draw, 120, 980, 1000, "HiveMQ console", HIVEMQ_CONSOLE_URL, fill=(247, 245, 236), outline=ORANGE)
    info_bar(draw, 120, 1080, 1000, "Broker host", HIVEMQ_HOST, fill=(247, 245, 236), outline=ORANGE)
    info_bar(draw, 120, 1180, 485, "ESP TLS port", HIVEMQ_TLS_PORT, fill=(247, 245, 236), outline=ORANGE)
    info_bar(draw, 635, 1180, 485, "Browser WebSocket port", HIVEMQ_WS_PORT, fill=(247, 245, 236), outline=ORANGE)
    info_bar(draw, 120, 1280, 485, "WebSocket path", HIVEMQ_WS_PATH, fill=(247, 245, 236), outline=ORANGE)
    info_bar(draw, 635, 1280, 485, "MQTT username", HIVEMQ_USERNAME, fill=(247, 245, 236), outline=ORANGE)
    info_bar(draw, 120, 1380, 1000, "Topic base", MQTT_TOPIC_BASE, fill=(240, 247, 255), outline=BLUE)

    draw.rounded_rectangle((120, 1490, 1120, 1590), radius=18, fill=(255, 245, 225), outline=ORANGE, width=2)
    draw.text((145, 1512), "Broker password is intentionally not written into this PDF.", font=SUBTITLE_FONT, fill=ORANGE)
    draw.text((145, 1546), "Enter it manually on the ESP local page and on the GitHub Pages remote page.", font=SMALL_FONT, fill=DARK)

    footer(draw, 4)
    return image


def hotspot_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 1: Use The ESP32 Hotspot First", "This is the safest and simplest first test")

    section_box(draw, 90, 220, PAGE_W - 180, 520, "What The Connection Looks Like")
    node(draw, (180, 420, 420, 530), "Phone", BLUE)
    node(draw, (820, 420, 1060, 530), "ESP32 Rover", GREEN)
    arrow(draw, (420, 475), (820, 475))
    draw.text((470, 400), "Direct local connection", font=SUBTITLE_FONT, fill=RED)
    draw.text((470, 445), "SSID: ESP32-ROVER", font=TEXT_FONT, fill=DARK)
    draw.text((470, 485), "Password: 12345678", font=TEXT_FONT, fill=DARK)
    draw.text((470, 525), f"URL: {HOTSPOT_URL}", font=TEXT_FONT, fill=DARK)

    section_box(draw, 90, 790, PAGE_W - 180, 770, "Exact Steps")
    numbered_list(draw, 125, 870, PAGE_W - 250, [
        "Power on the rover and wait about 20 to 30 seconds.",
        "Open Wi-Fi settings on your phone and connect to ESP32-ROVER.",
        "Enter the password 12345678.",
        f"Open your phone browser and go to {HOTSPOT_URL}",
        "If you want raw status information, open http://192.168.4.1/status and look at mode, stationIp, apIp, wifiStatus, and message.",
        "Use Buttons or Joystick to test local movement with the wheels raised at first.",
        "If this page works, your ESP web server is working correctly."
    ])

    footer(draw, 3)
    return image


def wifi_setup_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 2: Connect The Rover To Your Home Wi-Fi", "Do this from the local rover page while the phone is on the ESP hotspot")

    section_box(draw, 90, 220, PAGE_W - 180, 1180, "Exact Steps")
    numbered_list(draw, 125, 300, PAGE_W - 250, [
        f"While still connected to the ESP hotspot, open {HOTSPOT_URL}",
        "Scroll to the section named Wi-Fi And Hotspot.",
        "Type your home router Wi-Fi name in the SSID field exactly as it appears on the router.",
        "Type your home router password in the password field exactly.",
        "Press Save And Connect.",
        "Wait up to about 30 seconds while the ESP tries to join the router.",
        "If it works, the status area should show something like Network: STA+AP ready and Router YourWiFiName @ 192.168.1.23",
        "Write down that router IP because that is the address you will use when your phone is also on the same router.",
        "After the first successful save, the local ESP page can auto-fill the stored Wi-Fi SSID and password on later visits because they are saved in the board preferences storage.",
        "If it fails, the page usually stays in AP fallback mode and the router IP remains 0.0.0.0.",
        "If wifiStatus shows WL_CONNECT_FAILED, the password is usually wrong. If it shows WL_NO_SSID_AVAIL, the SSID is wrong, hidden, or the 2.4 GHz router band is not available."
    ])

    info_bar(draw, 120, 1450, 1000, "Example same-router URL after success", "http://192.168.1.23/")

    footer(draw, 5)
    return image


def same_wifi_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 3: Use The Rover On The Same Home Wi-Fi", "This is normal local home use after the rover joins your router")

    section_box(draw, 90, 220, PAGE_W - 180, 560, "What The Connection Looks Like")
    node(draw, (150, 410, 360, 520), "Phone", BLUE)
    node(draw, (500, 380, 740, 550), "Home Router", ORANGE)
    node(draw, (880, 410, 1090, 520), "ESP32 Rover", GREEN)
    arrow(draw, (360, 465), (500, 465))
    arrow(draw, (740, 465), (880, 465))
    draw.text((175, 560), "Phone on home Wi-Fi", font=SMALL_FONT, fill=MID)
    draw.text((835, 560), "Rover on same Wi-Fi", font=SMALL_FONT, fill=MID)

    section_box(draw, 90, 830, PAGE_W - 180, 730, "Exact Steps")
    numbered_list(draw, 125, 910, PAGE_W - 250, [
        "Disconnect your phone from ESP32-ROVER.",
        "Connect your phone to your normal home Wi-Fi.",
        "Open the router IP that the rover showed earlier, for example http://192.168.1.23/",
        "If the page opens, you now control the rover through your home router instead of the hotspot.",
        "If it does not open, reconnect to ESP32-ROVER and check whether the router IP changed or whether the rover fell back to hotspot mode.",
        "This same-router mode is the best place to test driving, speed control, RGB effects, and Auto Avoid before internet control."
    ])

    footer(draw, 6)
    return image


def github_pages_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 4: Publish The Phone Website With GitHub Pages", "This makes the remote browser app available on the internet")

    section_box(draw, 90, 220, PAGE_W - 180, 1180, "Exact GitHub Pages Setup")
    numbered_list(draw, 125, 300, PAGE_W - 250, [
        f"Open your repository: {REPO_URL}",
        "Log in to GitHub.",
        f"Open the Pages settings directly: {PAGES_SETTINGS_URL}",
        "Under Build and deployment, find Source.",
        "Choose Deploy from a branch.",
        "For the branch, choose main.",
        "For the folder, choose /docs.",
        "Click Save.",
        "Wait a few minutes. GitHub says publication can take up to about 10 minutes.",
        f"Then open {PAGES_HOME_URL}",
        f"Your remote control page should be {REMOTE_APP_URL}",
        "If you see a 404 page, wait a little longer, then verify that the Pages source is main and /docs."
    ])

    info_bar(draw, 120, 1450, 1000, "Remote app URL after Pages is live", REMOTE_APP_URL)

    footer(draw, 7)
    return image


def hivemq_setup_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 5: Create The MQTT Cloud Broker", "Use HiveMQ Cloud so the rover and your phone can meet over the internet")

    section_box(draw, 90, 220, PAGE_W - 180, 1220, "Exact HiveMQ Setup")
    numbered_list(draw, 125, 300, PAGE_W - 250, [
        f"Open {HIVEMQ_CONSOLE_URL}",
        "Create an account using GitHub, Google, LinkedIn, or email.",
        "After sign-in, choose Start in HiveMQ Cloud.",
        "Click Create Cloud Cluster.",
        "Choose HiveMQ Cloud Serverless.",
        "Create the serverless cluster and wait until it becomes ready.",
        "Open Manage Cluster.",
        "On the cluster overview page, write down the cluster URL, MQTT TLS port, WebSocket port, and the secure WebSocket URL.",
        "For this project, the ESP usually uses the TLS MQTT port, often 8883, and the phone web app usually uses the secure WebSocket port, often 8884.",
        "If your cluster shows different values, use the values from your own cluster overview instead of assuming 8883 and 8884."
    ])

    section_box(draw, 120, 1490, 1000, 120, "What You Must Write Down")
    draw.text((145, 1540), "Broker host  |  TLS port  |  WebSocket port  |  WebSocket path", font=TEXT_FONT, fill=DARK)

    footer(draw, 8)
    return image


def hivemq_credentials_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 6: Create Broker Credentials", "The rover and your phone remote app must both use the same broker login")

    section_box(draw, 90, 220, PAGE_W - 180, 760, "Exact Credential Steps")
    numbered_list(draw, 125, 300, PAGE_W - 250, [
        "Inside the HiveMQ cluster, open Access Management.",
        "Open the credentials area and choose Add Credentials.",
        "Create a username, for example roveruser.",
        "Create a strong password.",
        "Give the credential permission to publish and subscribe.",
        "Save the credential."
    ])

    section_box(draw, 90, 1040, PAGE_W - 180, 520, "Pick One Topic Base And Keep It Identical")
    draw_wrapped(
        draw,
        "Now choose one topic base for your rover. Example: rover/yanzoo-car-1. The rover and the remote app must use the exact same topic base. If one character differs, the remote app may connect to the broker but the rover will not react.",
        125,
        1120,
        PAGE_W - 250,
        TEXT_FONT,
    )
    info_bar(draw, 125, 1280, 950, "Example topic base", "rover/yanzoo-car-1", fill=(240, 247, 255), outline=BLUE)
    info_bar(draw, 125, 1380, 950, "Example WebSocket path", "/mqtt", fill=(240, 247, 255), outline=BLUE)

    footer(draw, 9)
    return image


def save_mqtt_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 7: Save MQTT Settings Into The Rover", "Do this on the rover's local web page before trying remote control")

    section_box(draw, 90, 220, PAGE_W - 180, 1180, "Fields To Fill On The Rover Page")
    numbered_list(draw, 125, 300, PAGE_W - 250, [
        f"Open the rover local page, either {HOTSPOT_URL} or the rover's router IP.",
        "Open Cloud MQTT Remote Control.",
        "Broker host: paste the HiveMQ cluster URL.",
        "ESP TLS port: use the MQTT TLS port from HiveMQ, usually 8883.",
        "WebSocket port: use the secure WebSocket port from HiveMQ, usually 8884.",
        "WebSocket path: use /mqtt unless your cluster documentation says otherwise.",
        "MQTT username: enter the broker username you created.",
        "MQTT password: enter the broker password you created.",
        f"Topic base: use the project topic base {MQTT_TOPIC_BASE} unless you intentionally want a different one.",
        "Press Save Cloud MQTT and wait a few seconds.",
        "If it works, the rover page should show Cloud: connected.",
        "The GitHub Pages remote app now uses the same project topic base by default, so the hotspot page, router page, and cloud page can stay aligned."
    ])

    info_bar(draw, 120, 1450, 1000, "If Cloud stays disconnected", "Check host, ports, username, password, and topic base.")

    footer(draw, 10)
    return image


def remote_control_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Step 8: Control The Rover From Another Internet", "This is the away-from-home workflow")

    section_box(draw, 90, 220, PAGE_W - 180, 560, "What The Connection Looks Like")
    node(draw, (110, 430, 300, 540), "Phone", BLUE)
    node(draw, (380, 390, 650, 580), "GitHub Pages Remote App", RED)
    node(draw, (740, 390, 980, 580), "MQTT Cloud", ORANGE)
    node(draw, (1030, 250, 1170, 360), "Home Wi-Fi", (110, 110, 110))
    node(draw, (980, 430, 1170, 540), "ESP32 Rover", GREEN)
    arrow(draw, (300, 485), (380, 485))
    arrow(draw, (650, 485), (740, 485))
    arrow(draw, (980, 485), (980, 485))
    arrow(draw, (1070, 390), (1070, 360))

    section_box(draw, 90, 830, PAGE_W - 180, 730, "Exact Remote Steps")
    numbered_list(draw, 125, 910, PAGE_W - 250, [
        "Leave the rover at home powered on.",
        "Make sure the rover is connected to your home Wi-Fi.",
        "Make sure the rover local page already shows Cloud: connected.",
        "Leave home or switch your phone to mobile data or another Wi-Fi.",
        f"Open the remote app: {REMOTE_APP_URL}",
        "Enter the same broker host, WebSocket port, WebSocket path, username, password, and topic base that you used on the rover.",
        "Press Connect Broker.",
        "If the app shows Broker: connected, press Refresh Rover Status.",
        "If rover telemetry appears, remote control is ready. Test Stop first, then very slow movement."
    ])

    footer(draw, 11)
    return image


def remote_details_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "How Remote Control Works", "Why it still works even when phone and rover are on different internet connections")

    section_box(draw, 90, 220, PAGE_W - 180, 1260, "Important Rules")
    bullet_list(draw, 125, 300, PAGE_W - 250, [
        "When you are away from home, do not try to use the rover's private router IP like 192.168.1.23. That IP only works inside your home network.",
        f"When you are away, use {REMOTE_APP_URL}",
        "The remote app does not directly reach your home router. It reaches the MQTT broker on the internet.",
        "The rover also reaches that same MQTT broker on the internet.",
        "Because both sides open outgoing connections to the broker, they can communicate even when they are on different networks.",
        "If the remote app connects but the rover does not react, the most common cause is that the topic base on the phone and rover do not match exactly.",
        "If the rover loses home Wi-Fi and falls back to hotspot mode, remote internet control stops until the rover gets internet again.",
        "If the office Wi-Fi blocks secure WebSockets, the remote app may fail there but still work over phone mobile data.",
        "Without a camera feed, remote driving is risky. Start with low speed and safe test conditions."
    ])

    footer(draw, 12)
    return image


def parts_mapping_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Parts Used By Firmware vs Build Only", "What from your shopping list is actually represented in code")

    section_box(draw, 90, 220, 500, 620, "Used In Code")
    bullet_list(draw, 120, 300, 430, [
        "ESP32-S3-N16R8 development board",
        "Three HC-SR04 ultrasonic sensors",
        "Two L298 motor driver modules",
        "Four DC geared motors",
        "4WD rover platform logic",
        "RGB WS2812 user LED"
    ])

    section_box(draw, 650, 220, 500, 620, "Used In Wiring / Build")
    bullet_list(draw, 680, 300, 430, [
        "3 x 18650 cells and battery holder",
        "3S BMS, fuse, switch, DC connectors",
        "XL4015 buck converters and AMS1117 module",
        "Capacitors, resistors, jumper wires, breadboard",
        "Solder, heatsink, mechanical chassis parts"
    ])

    section_box(draw, 90, 900, 1060, 430, "Tools / Not Firmware Parts")
    bullet_list(draw, 120, 980, 990, [
        "Multimeter, screwdriver, HDMI cable, and spirit level are useful tools or accessories, but they are not firmware-controlled parts.",
        "The code is ready for the main controllable electronics, but the power and assembly parts belong to the wiring plan rather than to src/main.cpp."
    ])

    section_box(draw, 90, 1390, 1060, 180, "Important Note")
    draw_wrapped(
        draw,
        "Even for the parts already used in code, the final hardware still depends on your real wiring matching the GPIO pin assignments in src/main.cpp.",
        120,
        1460,
        1000,
        TEXT_FONT,
    )

    footer(draw, 13)
    return image


def pinout_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Project Pinout Diagram", "Current firmware GPIO map for the ESP32-S3 N16R8 rover reference build")

    section_box(draw, 80, 210, 1080, 760, "Main Pinout Diagram")
    node(draw, (455, 420, 785, 620), "ESP32-S3\nN16R8", BLUE)

    node(draw, (110, 320, 320, 410), "Front HC-SR04", GREEN)
    draw.text((122, 423), "TRIG GPIO4 | ECHO GPIO5", font=SMALL_FONT, fill=DARK)
    draw.text((122, 448), "ECHO via divider to 3.3V-safe input", font=SMALL_FONT, fill=MID)
    arrow(draw, (320, 365), (455, 470))

    node(draw, (110, 500, 320, 590), "Left HC-SR04", GREEN)
    draw.text((122, 603), "TRIG GPIO6 | ECHO GPIO7", font=SMALL_FONT, fill=DARK)
    draw.text((122, 628), "ECHO via divider to 3.3V-safe input", font=SMALL_FONT, fill=MID)
    arrow(draw, (320, 545), (455, 520))

    node(draw, (110, 680, 320, 770), "Right HC-SR04", GREEN)
    draw.text((122, 783), "TRIG GPIO8 | ECHO GPIO9", font=SMALL_FONT, fill=DARK)
    draw.text((122, 808), "ECHO via divider to 3.3V-safe input", font=SMALL_FONT, fill=MID)
    arrow(draw, (320, 725), (455, 570))

    node(draw, (870, 300, 1120, 430), "Left L298 Module", ORANGE)
    draw.text((888, 448), "ENA GPIO10 | IN1 GPIO11 | IN2 GPIO12", font=SMALL_FONT, fill=DARK)
    draw.text((888, 473), "ENB GPIO13 | IN3 GPIO14 | IN4 GPIO15", font=SMALL_FONT, fill=DARK)
    draw.text((888, 498), "Motors: front-left + rear-left", font=SMALL_FONT, fill=MID)
    arrow(draw, (785, 470), (870, 370))

    node(draw, (870, 570, 1120, 700), "Right L298 Module", ORANGE)
    draw.text((888, 718), "ENA GPIO16 | IN1 GPIO17 | IN2 GPIO18", font=SMALL_FONT, fill=DARK)
    draw.text((888, 743), "ENB GPIO21 | IN3 GPIO38 | IN4 GPIO39", font=SMALL_FONT, fill=DARK)
    draw.text((888, 768), "Motors: front-right + rear-right", font=SMALL_FONT, fill=MID)
    arrow(draw, (785, 570), (870, 635))

    info_bar(draw, 120, 1010, 1000, "Onboard RGB LED", "WS2812 / NeoPixel on GPIO48")
    info_bar(draw, 120, 1110, 1000, "Reserved ESP32-S3 pins", "Do not use GPIO35, GPIO36, or GPIO37 on this N16R8 board.")

    section_box(draw, 80, 1230, 1080, 330, "Pinout Notes")
    bullet_list(draw, 115, 1310, 1010, [
        "The motor GPIO map above matches the current src/main.cpp firmware and the project PDF should be kept in sync with that file.",
        "Each HC-SR04 ECHO signal must be reduced to a 3.3V-safe level before it reaches the ESP32-S3 input pin.",
        "The ESP32 onboard WS2812 LED is already handled by firmware and does not need separate control wiring."
    ])

    return image


def wiring_reference_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Exact Wiring Reference", "What connects to what in the current rover firmware layout")

    section_box(draw, 80, 210, 500, 620, "Left Side Drive")
    bullet_list(draw, 115, 290, 430, [
        "L298 module A is the left-side driver reference.",
        "Front-left motor connects to output channel A on the left L298.",
        "Rear-left motor connects to output channel B on the left L298.",
        "Left L298 ENA -> GPIO10",
        "Left L298 IN1 -> GPIO11",
        "Left L298 IN2 -> GPIO12",
        "Left L298 ENB -> GPIO13",
        "Left L298 IN3 -> GPIO14",
        "Left L298 IN4 -> GPIO15"
    ])

    section_box(draw, 660, 210, 500, 620, "Right Side Drive")
    bullet_list(draw, 695, 290, 430, [
        "L298 module B is the right-side driver reference.",
        "Front-right motor connects to output channel A on the right L298.",
        "Rear-right motor connects to output channel B on the right L298.",
        "Right L298 ENA -> GPIO16",
        "Right L298 IN1 -> GPIO17",
        "Right L298 IN2 -> GPIO18",
        "Right L298 ENB -> GPIO21",
        "Right L298 IN3 -> GPIO38",
        "Right L298 IN4 -> GPIO39"
    ])

    section_box(draw, 80, 900, 1080, 620, "Sensors And Support Wiring")
    bullet_list(draw, 115, 980, 1010, [
        "Front HC-SR04: TRIG -> GPIO4, ECHO -> GPIO5 through a voltage divider, VCC -> 5V rail, GND -> common ground.",
        "Left HC-SR04: TRIG -> GPIO6, ECHO -> GPIO7 through a voltage divider, VCC -> 5V rail, GND -> common ground.",
        "Right HC-SR04: TRIG -> GPIO8, ECHO -> GPIO9 through a voltage divider, VCC -> 5V rail, GND -> common ground.",
        "Recommended ECHO divider examples: 10k from ECHO to GPIO and two 10k resistors in series from GPIO to GND, or any equivalent 5V-to-3.3V divider.",
        "Place a 1000uF capacitor near the motor supply rail and 100nF decoupling close to logic rails where practical.",
        "All grounds must join: battery, BMS output, buck converters, ESP32, both L298 modules, and all sensors."
    ])

    return image


def power_flow_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Full Project Power Flow Diagram", "Reference power architecture using the parts already purchased")

    section_box(draw, 70, 210, 1100, 740, "Recommended Power Flow")
    node(draw, (90, 480, 250, 610), "3 x 18650\n3S Pack", GREEN)
    node(draw, (300, 480, 480, 610), "3S BMS", ORANGE)
    node(draw, (530, 480, 670, 610), "10A Fuse", RED)
    node(draw, (720, 480, 880, 610), "Main Switch", BLUE)
    node(draw, (930, 330, 1110, 450), "Motor Rail", ORANGE)
    node(draw, (930, 500, 1110, 620), "5V Logic Buck\nXL4015", GREEN)
    node(draw, (930, 670, 1110, 790), "Optional 2nd Buck\nAccessories / future loads", MID, text_fill=LIGHT)

    arrow(draw, (250, 545), (300, 545))
    arrow(draw, (480, 545), (530, 545))
    arrow(draw, (670, 545), (720, 545))
    draw.line((880, 545, 920, 545), fill=DARK, width=7)
    draw.line((920, 545, 920, 390), fill=DARK, width=7)
    draw.line((920, 545, 920, 560), fill=DARK, width=7)
    draw.line((920, 545, 920, 730), fill=DARK, width=7)
    arrow(draw, (920, 390), (930, 390))
    arrow(draw, (920, 560), (930, 560))
    arrow(draw, (920, 730), (930, 730))

    draw.text((930, 460), "Direct battery path for motor supply", font=SMALL_FONT, fill=MID)
    draw.text((930, 630), "Main regulated 5V rail", font=SMALL_FONT, fill=MID)

    section_box(draw, 70, 1010, 530, 520, "What Connects To The Motor Rail")
    bullet_list(draw, 105, 1090, 460, [
        "Motor supply input of the left L298 module",
        "Motor supply input of the right L298 module",
        "Large bulk capacitor near the driver supply rail",
        "This path carries the noisiest current and should use short, heavier wiring"
    ])

    section_box(draw, 640, 1010, 530, 520, "What Connects To The 5V Logic Rail")
    bullet_list(draw, 675, 1090, 460, [
        "ESP32-S3 board 5V / VBUS input or short USB power lead",
        "All three HC-SR04 VCC pins",
        "Optional external logic 5V input for L298 modules if your board and jumpers support it",
        "Future light accessories or low-current logic loads"
    ])

    draw.rounded_rectangle((90, 1570, 1080, 1660), radius=18, fill=(255, 245, 225), outline=ORANGE, width=2)
    draw.text((115, 1594), "Important: do not use AMS1117 directly from the 3S battery as the main ESP supply. Use a buck converter for the 5V rail.", font=SMALL_FONT, fill=DARK)

    return image


def assembly_checklist_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Assembly Order Checklist", "A compact build order so you can wire the real rover without losing your place")

    section_box(draw, 80, 210, 500, 1260, "Build In This Order")
    numbered_list(draw, 115, 290, 430, [
        "ESP32 only over USB-C. Confirm hotspot, local page, Wi-Fi save, and MQTT cloud first.",
        "Set the main XL4015 buck to exactly 5.0V with the multimeter before it touches the ESP or sensors.",
        "Build the battery path: 3-cell holder to 3S BMS to 10A fuse to main switch.",
        "Create the shared ground network between BMS output, ESP32, both L298 modules, and all sensors.",
        "Connect the 5V logic rail from the XL4015 to the ESP32 and sensor VCC lines.",
        "Wire the left L298 first, then one left motor, and verify direction control with the wheels lifted.",
        "Wire the right L298, then the right motors, and retest manual controls.",
        "Add the front ultrasonic sensor with its ECHO divider first, then left and right sensors.",
        "Only after all of that should you test Auto Avoid on the floor."
    ])

    section_box(draw, 650, 210, 500, 600, "Parts To Use In Each Stage")
    bullet_list(draw, 685, 290, 430, [
        "Stage 1: ESP32-S3 board, USB-C cable, phone or laptop browser.",
        "Stage 2: XL4015 buck, multimeter, jumper wires.",
        "Stage 3: battery holder, 3 x 18650, 3S BMS, fuse holder, glass fuse, toggle switch.",
        "Stage 4: common ground wiring, breadboard only if still prototyping.",
        "Stage 5: left and right L298 modules plus 4 motors.",
        "Stage 6: 3 x HC-SR04 plus 10k resistor dividers on each ECHO line.",
        "Stage 7: capacitors, final cable cleanup, and chassis assembly."
    ])

    section_box(draw, 650, 860, 500, 610, "Quick Safety Rules")
    bullet_list(draw, 685, 940, 430, [
        "Do not connect raw 3S battery voltage directly to the ESP32 logic supply.",
        "Do not use GPIO35, GPIO36, or GPIO37 on this board.",
        "Keep the wheels lifted while you first test motor directions.",
        "Do not trust Auto Avoid until all three sensors return believable values.",
        "If the rover behaves strangely, return to hotspot mode and manual stop first."
    ])

    return image


def wiring_diagram_image_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Wiring Diagram Image", "Visual map of the exact firmware pin layout for sensors, drivers, and the onboard LED")

    section_box(draw, 70, 210, 1100, 980, "Wiring Diagram")
    paste_scaled_image(image, WIRING_DIAGRAM_PATH, (100, 290, 1140, 1140))

    section_box(draw, 70, 1240, 1100, 320, "What To Notice")
    bullet_list(draw, 105, 1315, 1030, [
        "The left and right L298 modules each get their own dedicated GPIO group.",
        "Each HC-SR04 ECHO line must pass through a voltage divider before it reaches the ESP32-S3.",
        "GPIO48 is reserved for the onboard RGB LED in the current firmware.",
        "GPIO35, GPIO36, and GPIO37 stay unused because this N16R8 module needs them internally."
    ])

    return image


def power_flow_image_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Power Flow Image", "Visual path from the 3S pack to the motor rail, 5V logic rail, ESP32, and sensors")

    section_box(draw, 70, 210, 1100, 980, "Power Flow Diagram")
    paste_scaled_image(image, POWER_FLOW_IMAGE_PATH, (100, 290, 1140, 1140))

    section_box(draw, 70, 1240, 1100, 320, "What To Notice")
    bullet_list(draw, 105, 1315, 1030, [
        "The battery path splits after the switch into a raw motor rail and a regulated 5V logic rail.",
        "Both L298 boards stay on the raw motor side, while the ESP32 and HC-SR04 modules use the 5V buck rail.",
        "All grounds must join even though the positive rails split.",
        "The second XL4015 is optional and can stay reserved for future accessories."
    ])

    return image


def sensor_placement_image_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Sensor Placement Image", "Recommended top-view positions for the three HC-SR04 sensors on the 4WD chassis")

    section_box(draw, 70, 210, 1100, 980, "Top-View Sensor Placement")
    paste_scaled_image(image, SENSOR_PLACEMENT_IMAGE_PATH, (100, 290, 1140, 1140))

    section_box(draw, 70, 1240, 1100, 320, "Placement Rules")
    bullet_list(draw, 105, 1315, 1030, [
        "Front sensor centered at the front edge and kept straight forward.",
        "Left and right sensors placed slightly behind the front bumper line and angled outward about 20 to 35 degrees.",
        "Keep the sensors level rather than tilted toward the floor or sky.",
        "Try to mount them high enough to avoid floor reflections but low enough to see chair legs and walls clearly."
    ])

    return image


def controls_and_troubleshooting_page():
    image = Image.new("RGB", (PAGE_W, PAGE_H), LIGHT)
    draw = ImageDraw.Draw(image)
    header(draw, "Controls And Troubleshooting", "What each control means and what to check when something fails")

    section_box(draw, 90, 220, 500, 1260, "Main Controls")
    bullet_list(draw, 120, 300, 430, [
        "Buttons: hold a direction button to keep moving.",
        "Joystick: drag to steer and release to stop.",
        "Speed slider: sets manual speed.",
        "Start Auto Avoid: enables autonomous obstacle avoidance.",
        "Manual Mode: returns control to the phone.",
        "Stop Auto: exits auto and stops the rover.",
        "RGB panel: choose color or police effect.",
        "Wi-Fi section: save or clear router credentials.",
        "Cloud MQTT: save or clear remote broker settings."
    ])

    section_box(draw, 650, 220, 500, 1260, "Quick Fixes")
    bullet_list(draw, 680, 300, 430, [
        "Hotspot not visible: wait 30 seconds and scan again.",
        f"Hotspot page not opening: type {HOTSPOT_URL} exactly.",
        "Router page not opening: the rover may not be on Wi-Fi anymore.",
        "Pages URL gives 404: Pages may not be enabled yet or still publishing.",
        "Remote app connects but rover does nothing: check topic base, WebSocket port, and broker login on both sides.",
        "Wheels do not move: confirm L298 wiring and GPIO mapping.",
        "Sensors read badly: recheck ultrasonic wiring and ECHO voltage compatibility.",
        "When confused, go back to the simplest path: hotspot mode, local page, manual stop, low speed."
    ])

    footer(draw, 14)
    return image


def main():
    pages = [
        cover_page(),
        key_urls_page(),
        project_values_page(),
        hotspot_page(),
        wifi_setup_page(),
        same_wifi_page(),
        github_pages_page(),
        hivemq_setup_page(),
        hivemq_credentials_page(),
        save_mqtt_page(),
        remote_control_page(),
        remote_details_page(),
        parts_mapping_page(),
        pinout_page(),
        wiring_reference_page(),
        power_flow_page(),
        assembly_checklist_page(),
        wiring_diagram_image_page(),
        power_flow_image_page(),
        sensor_placement_image_page(),
        controls_and_troubleshooting_page(),
    ]
    for index, page in enumerate(pages, start=1):
        draw = ImageDraw.Draw(page)
        draw.rectangle((0, PAGE_H - 70, PAGE_W, PAGE_H), fill=LIGHT)
        footer(draw, index)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    pages[0].save(OUT_PATH, "PDF", save_all=True, append_images=pages[1:], resolution=150.0)
    print(f"Created {OUT_PATH}")


if __name__ == "__main__":
    main()
