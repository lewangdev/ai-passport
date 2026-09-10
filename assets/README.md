<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- `fonts/niuma_font.c`: Source Han Sans SC Medium, 14 px, 4 bpp, ASCII plus
  the non-ASCII characters in `main/niuma_*` sources. Compiled by the NiuMa
  application. SIL Open Font License: `fonts/niuma-font-LICENSE.txt`.
  Regenerate with `node tools/prepare_niuma_font.cjs FONT.otf LV_FONT_CONV.js`
  using Source Han Sans SC Medium and `lv_font_conv` 1.5.3.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

- `images/niuma/community-cover-v1.png`: user-supplied cover for the startup
  screen, preserved unchanged. `niuma_cover.c` is its 240 x 320 RGB565 version.
  Regenerate with `python3 tools/prepare_niuma_cover.py` (Pillow). The image uses
  153,600 Flash bytes and the existing LVGL partial buffer, not a full-screen RAM
  allocation. `startup-preview.png` is a production-renderer preview.
  The any-key hint is rendered separately; no game character artwork is replaced.

- `images/niuma/ponytail-user-reference.png`: local-only user-supplied character
  reference, excluded from source commits and firmware. Redistribution rights
  for this design reference are not asserted by this repository.
- `images/niuma/ponytail-standing-preview.png` and `ponytail-home-preview.png`
  in the same folder: 240 x 320 previews rendered from the application sprite
  code. Regenerate with the `tests/niuma_render` target and `NM_VISUAL_ONLY=1`;
  the executable writes PPM previews in the system temporary directory.

Store reusable source images and generated display assets in `images/`.

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
