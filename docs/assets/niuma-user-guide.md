English | [简体中文](niuma-user-guide.zh_CN.md)

# NiuMa player guide

NiuMa is an offline office-pet game for AI Passport. Care for your colleague,
earn coins, learn skills and choose what a good working life means to you.
The firmware implementation is complete; automated tests do not constitute device acceptance.

## Start and controls

Choose a character and one of four names, then collect your employee badge.
Appearance does not change abilities. The badge shows employer, rank, base pay
and current career direction. Existing saves can be continued from the title page.

- UP/DOWN select an option; OK confirms; hold OK for two seconds to go back.
  On the separately opened reset confirmation page, that hold resets the story;
  a short OK click cancels instead.
- Menus with more than five options show the current/total page count beside
  the title. Keep moving to cross pages; first and last options wrap around.
- In a minigame, UP/DOWN move immediately. An OK press holds the game at that
  instant until click/hold is distinguished: a click acts at the captured time,
  while a hold opens pause without spending an attempt.
- Resume continues the paused round. Skip retains the base action but forfeits
  the extra minigame bonus. A missing gesture completion expires after three seconds.
- The first click after automatic screen blanking only wakes the display.

## Your day

The number beside the calendar icon is a simulated day, not a real-world date.
Each day has six action slots. Five workdays are followed by two weekend days.
Powering off does not reduce stats or advance time.

The header uses a speaker with four volume bars and a four-cell battery. A
dash inside the battery means the reading is unavailable; an exclamation mark
warns at 15% or below. Exact battery and volume values remain in Device status
and Sound settings. A disk indicates a pending save; a disk with an exclamation
mark indicates a save error.

Five large home icons select work, food, rest, chat and more; the selected
action's name appears underneath. The need strip shows food, energy, mood,
stress and health, from left to right, with values and meters. Higher stress is
worse. Employee stats pairs these same icons with names. Coins and six outlined
action slots sit below; filled slots are actions remaining. Character faces
respond to mood, energy, stress and the current scene without changing gameplay.
After sleep, a morning summary shows the week/day and restored needs. Start
today or check stats without advancing again. Burnout/recovery notices take
priority when that status changes overnight.

The office/home screen links to work, food, rest, friends and more actions.
All five needs are shown. On weekends, Walk replaces Work; between jobs, the
first entry opens career choices, except for active freelance/shop directions.
Closing work settles income once and takes you home without advancing the date.
Use remaining slots for food, rest, learning, care or socializing. Sleep starts
the next day with six slots. Once evening starts, work is closed for that day.

Watch satiety, energy, mood, stress and health. All range from 0 to 100; lower
stress is better. Every action preview shows its actual effect at your current
stats, including caps and career bonuses. Rejected actions do not spend money
or slots. Previews and receipts include actual skill, XP and friendship gains.
Free basic meals and recovery remain available when money runs out,
but still require an action slot. Finish the day and sleep when no slots remain.

## Care and growth

Insufficient coins offers free basic-meal or recovery previews; these still
need one action and explicit confirmation. When actions run out, choose the
payslip (or sleep if already home for the evening), view stats, or go back.
Failed attempts spend nothing and this route never pays a day's salary twice.

Rest includes Listen to music: confirm one normal rest action, then listen to
the original melody on its result screen. Leaving stops this temporary playback;
your global background-music preference is unchanged. Muting or unavailable
audio does not remove rest benefits, and automatic blanking silences playback.
Study includes a free Learning notebook showing current skills and career XP.
Overtime confirmation starts on Cancel; deliberately select Confirm to proceed.

- Eat a basic meal, bento or noodles; drink water, coffee or milk tea. Repeated
  coffee adds stress. Repeated noodles can reduce health. Life skill 24 unlocks cooking.
- Rest, take a tea break, exercise, tidy up, use the toilet, pay for treatment
  or take free recovery. Water, toilet and overtime each have a daily limit.
- Study technical, communication or life skills. Work builds career XP and
  daily performance. Chat with three friends or give an eight-coin snack gift.
- Resolve one daily event. Choices can change stats, skills or relationships;
  accepting an overtime request also consumes that action.
- Three days ending at stress 75 or higher cause burnout; two days ending at
  stress 35 or lower recover from it. Burnout blocks overtime, not the whole game.
  After sleep, a notice marks either transition. Recovery opens its normal
  confirmation page, while the friend shortcut opens social choices. Continuing
  a burnout save also shows the reminder; the reminder itself costs nothing.

## Six minigames

The guide appears before any action is charged. Choose Play, Base action only,
or Back. Games have up to eight rounds, up to eight extra coins, and one settlement.

| Game | Interaction | Base action |
| --- | --- | --- |
| Typing | Select the target row; press OK as the cursor enters the middle zone | Work |
| Meeting | Read the topic, choose nod/notes/question and confirm within eight seconds | Communication study |
| Commute | Select the highlighted free seat and confirm while available | Exercise |
| Tea break | Sip while safe, switch to busy at the visual warning; sip again next round | Tea break |
| Bento catch | Move the tray under the falling bento; confirm near the bottom | Basic meal |
| Going home | Wait for the lift to open, or take the stairs past a temporary meeting | Exercise |

Tea warnings appear 300 ms before danger. All important cues are visible;
sound is optional. Missing a round costs only that round's extra reward.

Meetings draw from six topics. A fitting nod earns Yang bond +2, taking notes
earns XP +3, and asking a useful question earns communication +2 at energy -1.
These effects apply only to fitting responses and settle once at the end,
alongside coins. A wrong response shows the suggested alternative. Skipping
forfeits these extra effects as well as coins; base study gains remain.

Going home starts three floors above the exit. Each six-second round allows one
confirmed route: an open lift descends two floors, stairs one. The lift opens for
1.5 seconds after its arrival countdown; a meeting by the lift blocks it for that
round, but never the stairs. Each descended floor earns two coins; exiting within
two rounds adds two more. Reaching the exit ends early, while the eighth round
ends with any partial reward. Skipping forfeits all extra coins. This is a walk
minigame, not the wage settlement or end-of-day command.

## Career, coins and collections

The payslip lists base pay, performance bonus, other rewards, spending and net
income. Employed weekdays have base pay; performance 20 unlocks a target bonus.
Project and flexible employers have variable bonuses or pay. Weekends have no wages.

Salary reviews are available every 28 simulated days. The first negotiation
fails; a later one can succeed with communication 24 and career XP 120.
The review page shows the remaining wait and actual pay/rank changes. Base pay
is capped at 100; no further raise can be claimed at the cap. Flexible-company
pay varies from base pay minus four to base pay plus eight, so later raises
still affect income when rank is already at its maximum.
Resignation keeps savings, skills, friendships and collections. Rehiring starts
a new entry-level role. After confirming resignation, the departure screen
offers rest at home or employer browsing. Five optional [career directions](niuma-development.md#career-direction-rules)
add expert, management, freelance, shop or comfortable-life effects.

The six play styles reflect recent actions and can change. They are not permanent classes.
Open Play style from Career to browse all six descriptions. It starts at your
current style; Next style cycles without changing it or spending an action.
The classification uses the most recent 14 simulated days, including today.
Basic meals and bento are style-neutral; cooking, learning and other deliberate
self-care can contribute to lifestyle. Eating normally does not force that style.
Buy four desk items, four home items and four clothing accessories. Previewing
never purchases or equips an item. Buying and equipping are separate confirmations;
decorations are cosmetic. Bento, noodles, coffee and tea can be stocked for later,
up to nine of each; eating stored food does not charge its price a second time.
The album records eight achievements and the day each was first earned.
`[+]` marks collected entries and `[-]` marks locked ones. Open an entry for its
story or unlock conditions. The life-expert milestone lets you continue playing;
it does not end or reset your character.

## Settings, saves and device status

Settings include volume, key sounds, optional original background music,
brightness, screen timeout, language, help, saves and device status. Confirm
volume/brightness changes to save them; holding OK cancels their preview.
The optional melody is original, not a recording of a commercial song.

The battery indicator is at the top right. `--` means unknown, not empty;
15% or below is highlighted as a charging reminder, not an automatic shutdown.
Device status reports audio readiness and offers a test sound. At volume zero
the test is silent. If audio remains unavailable, save progress before restarting.

Actions request background saves. `*` means pending and `!` indicates a storage
error. Save management gives live status and a retry option; do not power off
while an important change is pending or failed. Startup errors offer a read-only
retry. A default character is never automatically saved before creation/restore.
Starting over requires entering the reset page and holding OK; it cannot be undone.

The current development save format is NMA4. Earlier development formats are
not migrated. The firmware uses alternating validated slots, but an interrupted
save can still lose changes since the last durable snapshot. Actual power-loss,
audio and display behavior remain subject to on-device validation.
