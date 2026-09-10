English | [简体中文](niuma-development.zh_CN.md)

# NiuMa implementation and acceptance ledger

The goal is the complete office-pet game, not a clickable page collection.
The current branch is `feature/niuma-gezi`. Do not flash or publish without a
separate user request. Preserve the baseline protected Flash layout.

## Approved experience

- 240 x 320 portrait, four-color retro pixel UI, physical UP/DOWN/OK controls.
- Male reference: broad square face, flat chin, large head, short suited body
  and legs. Female: distinct short rounded oval face, high ponytail, blazer
  and trousers. Both have the same gameplay capabilities and body scale.
- Five character poses: new hire, working, burnout, leaving, rehired; additional
  eating, resting, exercise, learning, social and cleaning feedback.
- Six action slots per simulated day, five weekdays plus two weekend days.
  Offline time causes no deterioration or punishment. Restore mid-day saves.
- Satiety, energy, mood, stress and health; separate career XP and daily
  performance. Income, expenses and net day result must reconcile.
- Work, overtime, feeding, coffee, water, free basic meal, cooking, rest,
  slacking, three learning tracks, exercise, cleaning, toilet, treatment,
  free recovery, chatting and gifts. Preview effects before confirmation.
- Six genuinely interactive minigames: typing timing/lane selection, meeting
  responses, commute seating, tea-break detection, falling bento catching,
  and elevator/stairs escape. Skipping keeps base action rewards; extra rewards
  are granted once only. Audio must not be necessary to perceive game cues.
- Daily choice events, relationship conditions, quarterly review, initially
  unsuccessful salary negotiation followed by achievable raises, recoverable
  burnout, resignation, three employers, rehiring, and continuing milestones.
- Six reversible play styles computed from recent actions; three relationships;
  office/home/clothing shop categories, inventory, decoration preview,
  achievements and dated memories. No random paid loot or irreversible death.
- Start/create/name/badge/help, office/home/weekend, action menus and results,
  events, all six minigames, pay slip, stats/career/style, resignation/company,
  shop/item/bag/decorate, album, and all settings screens.
- Volume, button sound, optional original quiet office background music,
  brightness, inactivity blanking, Chinese/English, battery unknown/low states,
  explicit save errors/retry, no-money fallback, no-action guidance, and
  double-confirmed reset. Do not reuse unrelated earlier applications' music.

## Runtime architecture

Keep rules, game timing, and save encoding independent of LVGL/ESP-IDF. One UI
owner processes queued button events; callbacks must never write audio/NVS.
Storage and audio workers receive snapshots/messages. No access to deleted UI
objects, no LVGL calls without ownership/lock. Sleep pauses game time; waking
consumes the wake button instead of triggering an accidental action.

## Implementation evidence and device acceptance

| Area | Current evidence | Remaining |
| --- | --- | --- |
| Rules | `niuma_model.*`, 30 x 365 mixed-action days; `test_niuma_journey.c` reaches all routes/items/achievements for both appearances by day 62 through public actions, verifies natural burnout/recovery and switches through all six styles by day 127; save roundtrips throughout; 24 transactional event choices | Model and queued-button career reachability proven; visual review completed below |
| Minigames | `niuma_game.*`, six modes, skip/single-settlement tests; bilingual pre-play guides and base-only alternative; tea warning precedes danger by 300 ms; six meeting topics with distinct growth effects; three-floor lift/stairs escape with bypassable meetings and early-exit bonus | Six-game gestures, bilingual meeting/route/result pages, lift-window boundaries, safe stairs, partial finish and skip tested; balancing and on-device feel remain |
| Save format | `niuma_save.*`, 244-byte NMA4 with CRC; `tests/niuma_storage/test_worker.c` runs the production worker with injected write, commit, torn-write, readback, initialization and open failures; tests restart, damaged slots, revision wrap, latest queued snapshot and initialization retry | Read-only startup retry and reset/autosave controller tests pass; real-device power interruption and task concurrency remain; older development formats are not migrated |
| UI / art | `niuma_app.*`, `niuma_view.*`, `niuma_art.*`, reference-based sprites; source-derived font; all action result mappings and bilingual growth feedback tested; five home needs and weekend/career navigation; animated care scenes; natural controller burnout/recovery notices; `test_niuma_art.c` checks scenes across characters, fatigue, equipment and frames for bounds, palette and purity | Page layout/glyph checks and scene/lifecycle visual review complete; physical display acceptance remains |
| Sound / hardware | Six original synthesized effects and quiet melody, mute/clipping/chunk tests; settings, backlight, idle wake, audio queue and battery polling wired; volume/brightness cancel and preview-safe autosave tested | Device page and right-aligned low/unknown battery states tested; real-device audio/display coexistence remains |
| Delivery | Rules and production LVGL UI suites registered in the full `tools/validate.sh` gate; NiuMa is the firmware entry point | Bilingual guide and 74-page requirement mapping complete; verified artifact recorded below |

### Integration audit

Long menus show a page counter. Bilingual controller tests cover crossing
the five-item boundary, wrapping, selecting the last item and returning;
the full page sweep checks counter and reduced title-width layout.

The six-entry style gallery now opens at the current classification, explains
the action patterns and allows free cycling. Resignation renders the leaving
pose with rest/employer choices; rehiring clears obsolete confirmation pages.
Production-controller tests cover both appearances and languages, all six
styles, non-mutating browsing, cancellation, saved resignation, retained assets
and all three employers. Rendered style/departure pages were visually checked.

Music rest and the learning notebook are implemented. Bilingual controller tests
check cancellation, exact rest effects and saves, temporary playback, mute,
unavailable audio, idle wake, global music preferences and insufficient actions.
Notebook browsing is non-mutating; overtime now defaults to Cancel.
Insufficient-money/no-actions pages now provide direct free-action previews,
settlement/sleep and stats navigation. Bilingual controller tests verify no
failed-action spending, confirmation/cancel, exact saved recovery, evening sleep
and no duplicate wages. Reset ignores press/double-click and short click cancels.
The button-component long-press threshold is explicitly 2,000 ms in defaults;
the firmware entry point asserts this effective configuration at compile time.
Physical hold timing still needs device acceptance.

Morning and memory pages now have their intended content. Bilingual controller
tests cover an eight-day morning cycle, weekend scenes, stats/back/continue
without another day or payment, all eight locked/unlocked memories, saved dates,
collection markers and continuation after the life milestone.

### Prototype page mapping

These IDs refer to the original NiuMa UI prototype. Shared templates preserve
the interaction rather than requiring a separate C page enum for every mockup.
This is a source audit, not a claim that all device behavior has been accepted.

| Prototype IDs | Firmware mapping and evidence |
| --- | --- |
| `boot`, `create`, `name`, `welcome`, `help` | Title/create/name/badge/help pages; startup protection and initial controller journey |
| `office`, `house`, `weekend`, `more` | Contextual home, evening and weekend scenes; shared More access; home-context and settlement tests |
| `care`, `clinic`, `recovery` | Personal-care menu, treatment/recovery confirmations and receipts; action preview/result tests |
| `work`, `overtime` | Work menu and cancel-default overtime confirmation; rules and cancellation tests |
| `food`, `fooditem`, `eating` | Food menu, cost/effect confirmation, stock purchase and eating receipt; inventory and action tests |
| `rest`, `resting` | Nap, slack, recovery and temporary music rest; music/idle tests |
| `study`, `learned` | Three learning tracks, notebook and exact growth receipts; action and notebook tests |
| `exercise`, `clean`, `toilet` | Walk, cleaning and quiet-moment action scenes; scene mapping and pixel tests |
| `social`, `friend`, `socialresult` | Three bonds, chat/gift previews and actual gains; friendship/event tests |
| `event`, `eventdone` | Eight events, 24 conditional choices and results; transactional/controller tests |
| `typing`, `meeting`, `commute`, `slack`, `catch`, `escape`, `gameresult` | Six game models, guides, pause, skip and single-settlement results; game/gesture suites |
| `pay`, `nextday` | Payslip, evening, sleep and morning; settlement and morning-cycle tests |
| `stats`, `career`, `style` | Five needs, badge/routes and six-style gallery; stats and gallery tests |
| `review`, `salary`, `pie`, `raise` | Shared review/negotiation and distinct failure/success receipts; natural 56-day controller journey |
| `burnout`, `resign`, `company`, `newjob` | Recovery notice, departure and employer/rehire transitions; natural burnout and career tests |
| `ending`, `album`, `achievement` | Eight dated memories and life-milestone continuation; model reachability and memory-page tests |
| `shop`, `shopitem`, `bag`, `decorate` | Twelve items, stock, price previews, separate purchase/equip; inventory/model/controller tests |
| `settings`, `sound`, `brightness`, `sleep`, `language`, `english` | Bilingual settings, value previews and inactivity; settings/idle tests |
| `save`, `saved`, `saveerror`, `reset` | Live save status/retry and separate reset; storage fault injection and startup/reset tests |
| `lowbattery`, `poor`, `noactions` | Non-modal battery indication/device status and direct resource fallbacks; device-state/fallback tests |
| `pose-new`, `pose-work`, `pose-tired`, `pose-leave`, `pose-return` | Shared standing/seated/tired/walking sprite system; exhaustive bounds/palette tests; lifecycle visual comparison complete |

Visual review covers both appearances across lifecycle poses, the 21 scene
families and the six game screens. Rice, bento, noodles, water, coffee and milk
tea have distinct silhouettes. The female sprite now follows the short rounded
face/small-eye direction while keeping the ponytail and body proportions; the
generated raster remains a concept reference, not a runtime image dependency.
The tea-game status now describes sipping/busy mode, not irrelevant lane data.
`tests/niuma_render/career_journey.inc` now verifies all five optional career
directions through queued menu clicks. Both appearances in both languages reach
day 62, all twelve separately purchased/equipped items and all eight memories,
with action/transition snapshots checked against the save service. Only the
initial new-character fixture is assigned; no progress values are injected.
The lifecycle contact sheet exposed overly high office seating; it is corrected,
with feet below the desk and burnout dots above the head. Pixel assertions cover
both appearances and normal/tired seating. Meals no longer overlap the face.

Do not mistake first successful integration for completion. In particular:

- A natural 56-day production-controller journey covers study, review waiting,
  the first unsuccessful negotiation, the second raise and saved results in
  both languages. Boundary checks cover capped pay, no employer and flexible
  company income after maximum rank.

- Employee badge, company/rank/base pay, item prices and decoration previews are
  wired. Game guides show controls, costs, up to eight rounds and bonus rules before
  charging an action. Action previews run the actual rules on a copy, including
  stat caps, repeated coffee and career effects. Locked cooking/work explains
  the requirement. All 24 event choices have bilingual production-controller
  result/layout tests, saved-state checks and completed-event protection.
  Team-bond and weekend-overtime rejection show specific requirements;
  event relationship gains unlock the dated friendship achievement.
- Evening flow is implemented: closing work pays once without advancing time,
  home actions consume remaining slots, and sleep advances to the next day.
  Model and production controller tests cover settlement, home and sleep.
- Settings cancel restores the previous value; autosave excludes unconfirmed
  volume/brightness changes. The live save page distinguishes pending, saved,
  unsaved, empty, recovered and failed states, with retry. Controller tests
  inject save rejection and asynchronous status changes.
- Pause holds do not spend attempts; clicks use the press-time game state.
  Six-game controller tests cover resume, duplicate click, lost completion and
  skip without a second base charge. Physical button feel remains unverified.
- Startup errors offer read-only retry or separately confirmed reset. Before
  creation/restore, autosave never writes the default character; loading key
  gestures are discarded. Controller tests cover idle, retry, restore, cancel
  and confirmed reset without returning to the destructive dialog.
- Every page option in both languages has been checked, not just the first selection;
  tests check glyph coverage and body/button text boundaries.
- Five optional career directions now have entry conditions, distinct rule
  effects, dated milestones and persistent selection. Natural-action model
  journeys now prove their entry conditions without assigning progress fields;
  this does not replace controller or physical gameplay acceptance.
- Storage-worker fault injection now preserves the previous valid slot on
  simulated write failures. Initialization retries re-scan generations before
  writing, fixing a reproduced save-success/old-progress-on-reboot bug.
  Deterministic worker interleaving tests reproduce and cover the
  announce-before-publish pending race, including failed earlier writes,
  repeated revisions, ticket wrap and queue replacement during commit.
  Real-device scheduling and physical power interruption remain unverified.
- Scene/lifecycle atlases and both-language game screens have been rendered and reviewed.
  No flashing has been requested.

Do not count the unchanged baseline firmware as a NiuMa build. Do not mark the
goal complete until this ledger is satisfied with implementation and matching
tests. Hardware behavior not exercised must remain explicitly unverified.

## Software completion audit

The software implementation audit is complete. The complete validation gate
passed after the icon, expression, font and spacing refinements. This does not constitute
device acceptance, a release, a commit or a flash operation.

- Application image: 895,312 bytes; merged image: 960,848 bytes.
- Artifact: `build/FoloToy-AI-Passport-full.bin` (rebuild with `./tools/validate.sh`).
- SHA-256: `ecb83652c651d6b668cffa7d4d8bae9d15991b804c65cb177a84be3455fbff01`.
- Menu text block centering and equal male/female head anchors are asserted in
  the host tests, including one-line and wrapped menu entries.
- The wider ponytail portrait adds oval-eye/blink/smile pixel assertions and
  separate home/office camera checks. Standing and home previews use production art.
- Visual checks cover original pixel icons, eight expression states, battery
  cells, rounded safe areas and both languages. UI test heap peak: 19,552 bytes.
- Protected partition addresses, partition-table MD5 and merged-byte ranges passed.
- Build: PASS. Host tests: PASS. Device tests: NOT RUN.
- Unverified on hardware: boot/display refresh, ADC button feel and two-second
  holds, speaker volume/melody alongside rendering, battery readings, and physical
  power interruption during NVS writes. Obtain separate permission before flashing.

## Career direction rules

Switch before using daily action slots, not during the evening. Existing skills,
collections and first-entry dates persist. Returning to ordinary office life is
always available at that time; joining an employer resets the active direction.

| Direction | Entry conditions | Continuing effect |
| --- | --- | --- |
| Expert | Employed, technical skill 120, XP 240 | Work grants 6 extra XP |
| Manager | Employed, communication 120, mentor bond 60 | Work improves all three bonds by 1 |
| Freelance | Resigned, technical skill 96, savings 100 | 12 coins per work action, no base salary or overtime |
| Small shop | Resigned, life skill 96, savings 200 | 10 coins per work action, no base salary or overtime |
| Comfortable life | 10 on-time days, life skill 48 | Rest/walk adds 5 extra mood and removes 5 extra stress |

Savings requirements are reserves, not purchase prices. Freelance/shop work
keeps the five-workday/two-weekend rhythm and settles income once at closing.
