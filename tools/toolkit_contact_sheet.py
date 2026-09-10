#!/usr/bin/env python3
"""Arrange host-rendered PPM frames; this does not modify reference artwork."""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
pages = [(0, 0, "Home"), (1, 0, "Identity"), (2, 0, "Offline chat"),
         (3, 0, "Productivity"), (5, 1, "Maze"), (7, 1, "Word card"),
         (8, 1, "Quiz"), (9, 0, "Pixel pet"), (11, 0, "To-do"),
         (15, 1, "Meeting"), (16, 1, "Countdown"), (10, 0, "Settings")]
sheet = Image.new("RGB", (4 * 264 + 24, 3 * 362 + 24), "#202c36")
draw = ImageDraw.Draw(sheet)
for index, (page, variant, title) in enumerate(pages):
    x, y = 24 + index % 4 * 264, 24 + index // 4 * 362
    with Image.open(args.directory / f"page-{page:02d}-{variant}.ppm") as frame:
        sheet.paste(frame, (x, y))
    draw.text((x, y + 330), title, fill="#dce8df")
sheet.save(args.output)
