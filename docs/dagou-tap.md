English | [简体中文](dagou-tap.zh_CN.md)

# Pocket Dagou Tap

A standalone, offline adaptation of [MarkCup's Dagou Tap](https://www.bilibili.com/toy/Dagou-Tap/index.html) for AI Passport. The UI uses Simplified Chinese with Latin note names. It starts with a greeting for the recipient; press any key to begin.

## Controls

| Context | UP | DOWN | OK |
| --- | --- | --- | --- |
| Normal performance | DA / HA | GOU / JI | JIAO / MI |
| Hold in normal mode | Next pitch tier | Open settings | Sustain vowel, swell and shake |
| Piano | Next white key | Previous white key | Play selected syllable and note |
| Hold in piano | Next octave if enabled | Open settings | Sustain the third syllable |
| Settings | Previous item | Next item | Enter/leave editing |
| Editing | Decrease/change | Increase/change | Finish editing |

Long DOWN also closes settings. In piano mode choose the syllable in settings to reach all 24 note/syllable combinations; the three resistor-ladder keys do not support multitouch or chords. The first sound still occurs before a normal-mode navigation hold is recognized. A 15-second sustain limit prevents stuck notes after a lost input event.

## Web comparison

| Original | Hardware adaptation |
| --- | --- |
| Three syllables across four pitch tiers | Same two default sample packs, same target MIDI pitches; hold UP or use settings to select a tier |
| Eight white notes, C3–C6 starting octave | Same major scale and octave range, selected with physical keys |
| Long vowel with WSOLA texture | Short crossfaded sample loop with release fade; lower-memory approximation |
| 128 BPM, C–G–Am–F backing | Locally synthesized kick, snare, hi-hat, bass and arpeggio; similar pattern, not an exact Web Audio mix |
| Eighth-note rhythm snapping | Audio sample-clock scheduling, optional; at most about 234 ms of intentional quantization delay |
| Open mouth, bounce, growing/red/shaking character | Default open/closed images, beat bounce, held scale/tint/shake |
| Twelve geometric effects | Twelve lightweight effect families with bounded reusable objects; simplified shapes and motion |
| Music/SFX mute, piano, octave, snap, grid | Local settings; also volume, brightness, pitch tier, syllable and reset |
| Bilibili cloud configuration | Versioned, validated local NVS settings; delayed save, explicit save status |
| Coin-unlocked chicken and Emperor skin | Informational locked entries; gated media and entitlement checks are not bypassed or bundled |
| Creator/video links | Creator attribution in About; original website link in this guide |

The screen shows volume and battery (`--` if unavailable). The background keeps playing in settings. Audio failure is reported on screen; visual interaction remains available. This is a personal local adaptation, not an official release from the original creator.

## Resources and implementation

`assets/dagou-tap/` contains four 132 × 152 BGRA images and six 16 kHz signed 16-bit mono sample buffers, about 375 KiB total. They are embedded in Flash and streamed without loading complete audio/image copies into RAM. The original public default media is not covered by this repository's MIT license; no redistribution license was found. Obtain the creator's permission before publicly publishing the media or firmware containing it.

`tools/prepare_dagou_assets.py /path/to/downloads` reproducibly converts the public `audio-data.js` and the four PNGs (`dagou_close.png`, `dagou_open.png`, `cat_close.png`, `cat_open.png`) with ffmpeg. Only the default DA/GOU/JIAO and HA/JI/MI samples are extracted. The source page analyzed was version `12420391163904-v7471`, main script `20260817-coin-errors`.

The UI timer owns every LVGL operation. Button callbacks enqueue events. A dedicated task owns the audio mix, and another task handles NVS and battery polling so those operations do not interrupt sample generation. All tasks and UI objects live for the application lifetime; there is no deletion/reentry race. The factory partition contract is unchanged: 3 MB app, protected `cardid` at `0x356000`, and Recovery at `0x700000`.

## Validation

The 144-glyph Chinese UI subset is generated from Source Han Sans SC Regular
under SIL OFL; see `assets/fonts/dagou-font-LICENSE.txt`. Rebuild it with
`python3 tools/prepare_dagou_font.py /path/to/lv_font_conv.js /path/to/SourceHanSansSC-Regular.otf`.
The LVGL pool is 48 KiB. The desktop rendering smoke test uses the production
screen code, exercises all settings and repeated notes, and measured about
26.6 KB peak UI allocation (host allocation overhead differs from ESP32).

To repeat the visual check after dependencies have been downloaded:

```bash
cmake -S tests/dagou_render -B /tmp/dagou-render-build -G Ninja
cmake --build /tmp/dagou-render-build
/tmp/dagou-render-build/dagou_render
```

Run the renderer from the repository root. It emits five 240 × 320 PPM captures
under `/tmp/dagou-*.ppm` and checks settings layout and LVGL heap integrity.
Board and audio adapters are mocked; this does not test the physical display
or actual I2S timing.

Run `./tools/validate.sh` in ESP-IDF 5.5.3. Host tests cover pitch tables, octave boundaries, invalid settings, wrap-around and quantization. The merged build gate validates image contents and protected regions.

On a device, verify sustained sound without clicks/underruns, all keys and long holds, note/octave range, settings after power cycling, legible text, stable animation, volume/backlight changes and plausible battery readings. A compiler result is not proof of these physical checks.
