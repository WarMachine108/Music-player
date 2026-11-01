"""Render the player's ANSI snapshot for documentation. Requires Pillow.

Usage: python scripts/render_snapshot.py build/ui.ansi docs/screenshot.png
This renders real FTXUI output, not a separately drawn UI mockup.
"""
import re
import sys
import unicodedata
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

source, destination = map(Path, sys.argv[1:3])
stream = source.read_text(encoding="utf-8")
regular = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 17)
bold_font = ImageFont.truetype("C:/Windows/Fonts/consolab.ttf", 17)
symbol = ImageFont.truetype("C:/Windows/Fonts/seguisym.ttf", 16)
fg, bg, bold = (226, 232, 243), (12, 15, 24), False
x, y = 0, 0
cells = []
tokens = re.split(r"(\x1b\[[0-9;]*[a-zA-Z])", stream)
for token in tokens:
    if token.startswith("\x1b["):
        if not token.endswith("m"):
            continue
        codes = [int(v or 0) for v in token[2:-1].split(";")]
        i = 0
        while i < len(codes):
            code = codes[i]
            if code == 0:
                fg, bg, bold = (226, 232, 243), (12, 15, 24), False
            elif code == 1:
                bold = True
            elif code == 22:
                bold = False
            elif code in (38, 48) and codes[i + 1:i + 2] == [2]:
                value = tuple(codes[i + 2:i + 5])
                if code == 38:
                    fg = value
                else:
                    bg = value
                i += 4
            elif code == 39:
                fg = (226, 232, 243)
            elif code == 49:
                bg = (12, 15, 24)
            i += 1
        continue
    for char in token:
        if char == "\n":
            y += 1
            x = 0
        elif char == "\r":
            x = 0
        elif char.isprintable() or "\u2800" <= char <= "\u28ff":
            cells.append((x, y, char, fg, bg, bold))
            x += 2 if unicodedata.east_asian_width(char) in "WF" else 1

width = max(c[0] for c in cells) + 1
height = max(c[1] for c in cells) + 1
image = Image.new("RGB", (width * 11 + 48, height * 22 + 48), (8, 10, 17))
draw = ImageDraw.Draw(image)
for x, y, char, fg, bg, bold in cells:
    left, top = x * 11 + 24, y * 22 + 24
    draw.rectangle((left, top, left + 10, top + 21), fill=bg)
    font = symbol if "\u2800" <= char <= "\u28ff" or char in "◉♫▎●○Ⅱ" else bold_font if bold else regular
    draw.text((left, top), char, fill=fg, font=font)
destination.parent.mkdir(parents=True, exist_ok=True)
image.save(destination)
print(destination.resolve())
