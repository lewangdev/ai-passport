# FoloToy Pocket Toolkit

English | [简体中文](README.zh_CN.md)

A six-in-one AI Passport example inspired by the supplied black-and-green
concept image. Built on branch `codex/multi-toolkit`; no hardware pins or
partition layout changes. Original BSP examples remain in the source tree.

## Plan and implementation

| Order | Module | Play and interaction |
| --- | --- | --- |
| 1 | Identity | Four preset nicknames, three avatar colors, local random ID, shared level and coins. Not an identity credential or cloud account. |
| 2 | Conversation | Four selectable topics: greeting, current task count, pet hunger, suggestion. Explicitly offline, template-driven demonstration, not AI inference. |
| 3 | Productivity | Meeting timer (15/25/45 minutes) with three equal agenda phases and follow-up tasks; eight-item to-do list from twelve presets; pausable countdown (5/10/25/45 minutes); relative reminder (5/15/30/60 minutes). |
| 4 | Maze | Random connected 5×5, 7×7 or 9×9 mazes. Collect the gold star before reaching the green-heart exit. Leaving the page keeps the current maze until restart. |
| 5 | Vocabulary | 24 words with English examples, flip cards, three-answer quizzes, persistent mastery and wrong-word review. |
| 6 | Pixel pet | Feed, play, rest and clean. Four needs, shared level, short care cooldown and idle animation. |

Every 100 XP advances the shared level. A completed maze gives 15/20/25 XP and
8 coins; a completed meeting gives 10 XP and 5 coins; first completion of a task
entry gives 5 XP and 2 coins; first mastery of a word gives 5 XP and 2 coins.
Repeated toggles or correct answers do not repeat these rewards. Deleting and
re-adding a task creates a new rewardable entry; this is not a tamper-proof economy.
XP is capped at 999,999 and coins at 9,999.

Pet care restores 30 points and gives 2 XP with a 10-second cooldown. Feeding
costs 5 coins; play costs 10 energy. Needs decline by one every three powered-on
minutes, never below zero. There is no offline punishment.

## Controls and timing

- Up/down: select; OK: activate. Hold OK: back one level.
- Hold OK on home: settings. OK cycles volume (including mute) or brightness.
- Maze: up turns left, down turns right, OK moves forward one cell.
- Word card: OK flips, then choose remembered/not yet.
- Quiz: choose an answer; OK reveals the result; OK again advances.

The top bar shows **elapsed powered-on hours/minutes**, sound level and actual
battery SOC (`?` when unavailable), not a wall clock. Timers continue on other
pages; simultaneous alerts are listed together and remain until acknowledged.
Timers and maze progress reset on restart. Reminders do not wake a powered-off
device. The meeting assistant handles agenda/timing, not recording/transcription.

## Storage and limitations

Identity, XP/coins, tasks, learned/wrong words, pet needs and settings are saved
in NVS namespace `toolkit`, using a versioned 64-byte checksum-protected record.
Writes are batched on a two-second schedule: wait for saved status in settings
before switching off. Failed storage is visibly reported. Unreadable or future
records remain read-only for that boot. NVS is never erased; `cardid` is not read.

Online AI, arbitrary text entry, calendar synchronization, pronunciation audio,
cloud identity and power-off wake-up are **not implemented**. No backend or API
key has been provided. Responses must not be presented as real AI output.
The UI is Chinese with English vocabulary examples.

The 240×320 interface uses rounded black corners, dark cards, green focus
outlines and original code-native pixel icons; it does not crop artwork from
the supplied marketing image. See [assets and font license](assets/README.md).

## Build and verification

Use ESP-IDF 5.5.3 and the [build guide](docs/development/engineering/build-and-test.md).

```sh
./tools/validate.sh --static
./tools/validate.sh

# Optional host screenshots; requires the managed LVGL component and CMake:
cmake -S tests/toolkit_render -B /tmp/toolkit-render -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/toolkit-render -j8
mkdir -p /tmp/toolkit-screens
/tmp/toolkit-render/toolkit_render /tmp/toolkit-screens
python3 tools/toolkit_contact_sheet.py /tmp/toolkit-screens /tmp/toolkit-pages.png
```

`toolkit_model.*` contains pure logic; `toolkit_view.*` uses one lifetime-scoped
LVGL screen. `toolkit_main.c` queues button input and runs audio, NVS and battery
work separately. Only the LVGL task mutates the model/UI. Audio uses original
short synthesized chirps. No networking or continuous recording tasks are started.

Host tests cover 1,500 mazes, connectivity/reciprocal walls, reward idempotence,
paused and simultaneous timers, corrupt saves and 100,000 navigation events.
The renderer checks text fit and corners with a 24 KB LVGL pool and 20-line
display buffer. Compilation and rendering are not physical-device validation.

On-device acceptance: test all keys/long press, six modules, volume/mute,
brightness, battery validity, readable corners, maze turns, all three timers
expiring together, and saved data after restart. Speaker quality, input latency
and power consumption require real hardware. Flashing, commits and publishing
are separate user-authorized actions, not automatic development steps.
