#!/usr/bin/env python3
"""Build a complete 14px CJK subset for the production UI (Source Han Sans)."""
import pathlib
import re
import subprocess
import sys

source = pathlib.Path("main/dagou_app.c").read_text()
symbols = "".join(sorted(set(re.findall(r"[^\x00-\x7f]", source))))
destination = pathlib.Path("assets/fonts/dagou_ui.c")
destination.parent.mkdir(exist_ok=True)
subprocess.run(["node", sys.argv[1], "--font", sys.argv[2], "--size", "14",
                "--bpp", "4", "--format", "lvgl", "--no-compress",
                "--symbols", symbols, "--lv-font-name", "dagou_ui_font",
                "--output", str(destination)], check=True)
print(f"Generated {len(symbols)} non-ASCII glyphs")
