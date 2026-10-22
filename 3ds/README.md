# Bald Elite — Nintendo 3DS Arcade Edition (playable beta)

A **native Nintendo 3DS** port of the PSP homebrew Bald Elite, targeting the
**original (old) Nintendo 3DS**. This edition builds separately to a `.3dsx`
using devkitARM, libctru, citro3d and the PICA200 GPU. The PSP sources and PSP
EBOOT build remain unchanged.

## What's playable in the source

- **Three applicants:** The Businessman (short brown hair), The Punk (pink
  mohawk), The Professor (thinning silver hair).
- **Actual 3D shaving:** remove individual strands by dragging your finger or
  stylus over the **bottom touchpad**; the clipper crosshair appears on the
  **top** 3D screen. Use the **circle pad + hold A** as another control option.
  Swift strokes sweep intervening hair rather than requiring a tap per strand.
- **Rotate / tilt / zoom:** L/R rotates the head, Up/Down tilts it, X zooms
  between three camera distances. Helps reach the scalp's far side.
- **Two modes:** timed **90-second Challenge** and untimed **Zen**.
- **Progress, score and combos:** displayed on the bottom screen; the top
  screen also has a progress bar and a visible aiming cursor.
- **Last-hair assist:** when 8 or fewer strands remain, tap Y to locate and
  shave a difficult strand. This prevents an endless hunt for invisible hairs.
- **Victory / failure:** stars, new Bald Boss membership ceremony, golden
  on-screen accents, replay, next applicant, and level selection.
- **Sound:** optional NDSP synthesised arcade instrumental loop, clipper buzz,
  and victory fanfare. If DSP initialization fails, the game runs silently.
- **High scores:** saves per applicant and per mode to the SD card at
  `sdmc:/3ds/bald_elite_3ds/scores.txt`. If the SD is read-only, the game
  still runs and remembers scores until exit.
- **Anti-flicker interface:** unlike the earlier prototype, the bottom console
  does **not** clear and repaint the entire screen every few frames; only
  changed lines are updated, including during gameplay.

## Controls

| When | Button | Action |
| --- | --- | --- |
| Level select | Left/Right | Choose applicant |
| Level select | Up/Down | Choose Challenge / Zen |
| Level select | A or X | Start |
| Level select | Start | Exit the Homebrew Launcher application |
| Playing | Touch and drag bottom screen | Move the clipper on the top screen and shave |
| Playing | Circle pad | Move clipper |
| Playing | Hold A | Shave at cursor |
| Playing | L/R | Turn head |
| Playing | Up/Down | Tilt head |
| Playing | X | Cycle camera zoom |
| Playing | Y, when 8 or fewer hairs remain | Final-hair assist |
| Playing | B | Pause or resume |
| Playing | Start | Return to level select |
| Paused | A | Restart candidate |
| Victory | A / X | Replay / Next candidate |
| Failure | A / X | Retry |
| Victory / Failure | B or Start | Return to selection |
| Any screen | Select | Mute/unmute arcade music & buzz |

The touch screen is a **clipper-control pad**, not a mirror of the 3D view. Its
320x240 touch coordinates are mapped to the 400x240 top-screen crosshair.

## Building

Install devkitPro's `3ds-dev` packages, including devkitARM, libctru,
citro3d, picasso and `3dsxtool`. Build from the repository root:

```sh
make -C 3ds clean
make -C 3ds -j2
```

The result should be `3ds/bald_elite_3ds.3dsx`. Copy it to
`sdmc:/3ds/bald_elite_3ds/bald_elite_3ds.3dsx` on a homebrew-capable 3DS,
then launch it using Homebrew Launcher.

GitHub Actions performs **separate host C geometry/gameplay regressions** and
the full devkitARM 3DS cross-build. From a successful
[Build Bald Elite Nintendo 3DS](https://github.com/generaldeejee/Bald-Boss/actions/workflows/build-3ds.yml)
run, download the `BaldElite-3DS-Playable` artifact. A green workflow and
in-game testing must still be checked before treating the version as released.

Host regression tests (without the 3DS SDK):

```sh
gcc -std=c99 -O2 -Wall -Wextra -Werror \
  3ds/tests/test_mesh.c 3ds/source/mesh.c -lm -o /tmp/be-mesh
/tmp/be-mesh
gcc -std=c99 -O2 -Wall -Wextra -Werror \
  3ds/tests/test_game.c 3ds/source/game.c 3ds/source/mesh.c \
  -lm -o /tmp/be-game
/tmp/be-game
```

## Remaining differences from the PSP version

This is now a **playable beta**, but not yet feature-identical to the PSP
version. In particular, the original PSP **Piper vocal song and announcer**,
the complete face texture atlas and shiny shaved-skin painting, and a highly
polished graphical bottom-screen interface have not been ported. The 3DS audio
is newly synthesised music, not a remix of the original PSP vocal recording.

The frame rate, dual-screen presentation, touch alignment, DSP availability,
score saving and overall feel must be tested in the emulator or on physical
**old 3DS hardware**. Source and CI checks alone do not guarantee that the
finished game behaves perfectly on the console.

The head profile is ported from the project's original PSP head geometry,
and the basic GPU shader/render conventions follow devkitPro's 3DS examples.
