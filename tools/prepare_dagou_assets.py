#!/usr/bin/env python3
"""Convert the publicly available default Dagou Tap assets for the LCD.

Input: downloaded audio-data.js and four PNGs in a supplied directory.
Only default, non-unlock-gated assets are included. Requires ffmpeg.
"""
import base64
import pathlib
import re
import subprocess
import sys
import tempfile

source = pathlib.Path(sys.argv[1])
dest = pathlib.Path("assets/dagou-tap")
dest.mkdir(exist_ok=True)
audio = (source / "dagou-audio.js").read_text()
for name in ("da", "gou", "jiao", "ha", "ji", "mi"):
    encoded = re.search(r"\b" + name + r":\s*'([^']+)'", audio)[1]
    with tempfile.NamedTemporaryFile(suffix=".wav") as wav:
        wav.write(base64.b64decode(encoded))
        wav.flush()
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", wav.name,
                        "-ar", "16000", "-ac", "1", "-f", "s16le",
                        str(dest / (name + ".pcm"))], check=True)
for name in ("dagou_close", "dagou_open", "cat_close", "cat_open"):
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", str(source / (name + ".png")),
                    "-vf", "scale=132:152:force_original_aspect_ratio=decrease,pad=132:152:(ow-iw)/2:(oh-ih)/2:color=black@0,format=bgra",
                    "-f", "rawvideo", str(dest / (name + ".bgra"))], check=True)
