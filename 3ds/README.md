# Bald Elite for Nintendo 3DS — native playable beta

This is the **Nintendo 3DS homebrew edition** of Bald Elite, running on the
original 2011 3DS through devkitARM, libctru, and Citro3D/PICA200.
The independent **PSP build is unchanged**. You play the Club Barber,
shaving applicants for **The Elite Bald Man Club**. Each freshly bald
applicant earns the *Bald Boss* rank.

This is a playable arcade port, **not** an exact audiovisual copy of the
PSP version. The original PSP face texture/skin painting, vocal soundtrack,
and full-screen graphical fonts/tutorial are not yet reproduced on 3DS.
A GitHub Actions cross-compile passing is not a physical 3DS playtest.

## Playable game features

- Three applicants: **The Businessman**, **The Punk**, **The Professor**,
  with the PSP-inspired 3D facial silhouette and three distinct hair styles.
- Native top-screen 3D rendering; interactive hair mesh with individual
  strand removal; touch and Circle Pad clipping; continuous *swipe* hair
  collision; head rotation and pitch; three camera zoom distances.
- **Challenge**: shave all hair within 90 seconds. **Zen**: untimed.
- Score and combo, shave percentage/progress bar, a last-hair locator,
  timeout / retry, automatic victory orbit, medal stars and golden approval
  ceremony. The club rank is **Bald Boss**; the game's title is Bald Elite.
- Optional NDSP music, clipper buzz, and success jingle. The 3DS music is a
  small original synthesized instrumental loop, **not** the PSP vocal song.
  The game remains playable without DSP support.
- High scores saved per applicant and per mode to
  `sdmc:/3ds/bald_elite_3ds/scores.txt` (when writable).
- Bottom-screen text is updated only when necessary. Bottom LCD double
  buffering is explicitly disabled to prevent old/new console-page flicker.

## Controls

| State | Control | Action |
|---|---|---|
| Title | D-pad Left / Right | Previous / next applicant |
| Title | D-pad Up / Down | Challenge / Zen |
| Title | A or X | Start shaving |
| Title | L / R | Preview head rotation |
| Gameplay | Drag on touchscreen | Move clippers and shave through the corresponding point on the **top** screen |
| Gameplay | Circle Pad | Move clippers without shaving |
| Gameplay | Hold A | Shave at the clipper position, including swept motion |
| Gameplay | L / R | Rotate head left / right |
| Gameplay | D-pad Up / Down | Tilt to inspect scalp and crown |
| Gameplay | X | Cycle 1x / medium / close-up camera |
| Gameplay | Y | Remove/focus a remaining strand when at most eight remain |
| Gameplay | B | Pause / resume |
| Paused | A | Restart same applicant |
| Gameplay | START | Return to applicant selection |
| Victory | A | Replay the applicant |
| Victory | X | Start next applicant |
| Victory/fail | B or START | Return to selection |
| Fail | A or X | Retry |
| Any screen | SELECT | Mute/unmute music |
| Title | START | Quit to Homebrew Launcher |

The 320×240 bottom touchscreen maps to the 400×240 top preview. When
shaving with the touchscreen, move your finger while watching the crosshair
on the top screen. The separate physical game buttons remain usable.

## Download and install

1. Open [GitHub Actions: Build Bald Elite Nintendo 3DS](https://github.com/generaldeejee/Bald-Boss/actions/workflows/build-3ds.yml).
2. Select the newest **green** workflow run, then download the
   **BaldElite-3DS-Playable** artifact. Do not use the older prototype artifact
   from the 3DS milestone-one build.
3. Extract `bald_elite_3ds.3dsx`. Copy it to the SD card as:
   `sdmc:/3ds/bald_elite_3ds/bald_elite_3ds.3dsx`.
4. Launch using the Homebrew Launcher on a homebrew-capable 3DS.
   A DS-only R4 cartridge cannot run the native 3DS executable.

If all you can do is rotate a head, you are **running the older preview build**.
Delete the old `.3dsx`, copy the new file, and relaunch. A blue/changing
console on the bottom screen can also indicate the old double-buffered build.

## Building and testing

Install official devkitPro's `3ds-dev` package (including devkitARM,
Citro3D, libctru, picasso, and 3dsxtool). Set `DEVKITARM`, `DEVKITPRO`
and include `$DEVKITARM/bin` in `PATH`:

```sh
make -C 3ds clean
make -C 3ds -j2
```

Use regular desktop GCC for portable mesh and game-state tests:

```sh
gcc -std=c99 -O2 -Wall -Wextra -Werror \
  3ds/tests/test_mesh.c 3ds/source/mesh.c -lm -o /tmp/bald-elite-mesh
/tmp/bald-elite-mesh
gcc -std=c99 -O2 -Wall -Wextra -Werror \
  3ds/tests/test_game.c 3ds/source/game.c 3ds/source/mesh.c \
  -lm -o /tmp/bald-elite-game
/tmp/bald-elite-game
```

The GitHub Actions workflow runs both tests and compiles the native
`.3dsx` automatically after commits under `3ds/`.

## Outstanding differences and validation

- Restore the PSP's actual textured face, polish effect, skin atlas and
  more detailed clipper/particle effects.
- Port the original sung **No Hair, No Fear!** soundtrack and the
  **“Another bald boss!”** neural announcer while respecting 3DS memory/DSP.
- Replace text-console results with a graphical touch HUD, applicant outfits,
  tutorial and genuinely styled 3D membership card.
- Test on original hardware and in a 3DS emulator: FPS, touch sensitivity,
  all applicants, full end-to-end victories, memory usage and optional audio.

The repository ships original project code and MIT licensing.
