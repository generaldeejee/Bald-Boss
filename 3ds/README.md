# Bald Elite — Nintendo 3DS port (milestone 1)

This is **a real native 3DS homebrew prototype**, not an emulated PSP/PS1 build.
It targets the **original 2011 Nintendo 3DS** using devkitARM, libctru,
citro3d and the PICA200 graphics processor. It lives in a separate `3ds/`
directory: **the existing PSP game and its Makefile are unchanged**.

## Implemented in source

- **Top 3D screen:** a rotating 3D head with the *same mathematical face
  profile* used by the PSP game's `src/bald_head_v2.h` (nose, brow, eyes,
  lips and ears). The head uses temporary vertex colors rather than the PSP
  texture atlas in this first milestone.
- **Hair:** separate GPU geometry for the Businessman's neat cut, the Punk's
  pink mohawk, and the Professor's thinning silver hair. All are generated
  with the same deterministic distribution each time they are selected.
- **Bottom screen:** text/status console with character name, controls and
  the current milestone. No GPU GUI widgets yet.
- **Buttons:** Left/Right changes applicant; L/R rotates the head;
  Up/Down tilts it; X toggles automatic rotation; START quits to the
  Homebrew Launcher.
- **Build:** produces `3ds/bald_elite_3ds.3dsx` and
  `3ds/bald_elite_3ds.smdh`; an independent GitHub Actions workflow
  also runs host-side mesh tests.

## Not ported yet

This is **not yet the complete Bald Elite game**. In particular,
shaving collision detection/touchscreen interactions, score, timer,
Zen/Challenge modes, victory membership cards, the PSP's original face texture,
and audio/soundtrack **are not implemented on 3DS** at this milestone.
The controls and preview demonstrate the rendering/input foundation, not a
finished playable 3DS game.

## Building (devkitPro)

Install the official devkitPro **3ds-dev** toolchain (including devkitARM,
libctru and citro3d) and `picasso`, then from the repo root:

```sh
make -C 3ds
```

For the standalone mesh test (without any Nintendo SDK):

```sh
gcc -std=c99 -O2 -Wall -Wextra -Werror \
  3ds/tests/test_mesh.c 3ds/source/mesh.c -lm -o /tmp/bald-elite-mesh-test
/tmp/bald-elite-mesh-test
```

You can also download the `BaldElite-3DS-Prototype` artifact from a **green**
[Build Bald Elite Nintendo 3DS Actions run](https://github.com/generaldeejee/Bald-Boss/actions).
If the workflow fails there is **no verified binary** yet; read the failing log.

## Running

A standard SD-card Homebrew Launcher layout:

```text
sdmc:/3ds/bald_elite_3ds/bald_elite_3ds.3dsx
```

Copy the produced `.3dsx` onto the 3DS SD card. A prepared 3DS capable of
launching homebrew is required; a Nintendo DS R4 flashcart is not the build
target. Running this early prototype in a 3DS emulator or on a physical
original 3DS is still necessary to establish frame rate and graphics quality.

## Next engineering milestones

1. Port the hair-hit projection and implement purposeful clipper movement
   using the touch screen/circle pad.
2. Reuse the PSP's original skin-texture painting and shaving coverage.
3. Port score/time rules, Zen & Challenge, applicant progression, then music
   and the golden club membership approval ceremony.
4. Evaluate stereo 3D **only after** a stable, responsive original-3DS build.

The head profile is an original part of the Bald Elite/PSP project.
The shader/build conventions follow devkitPro's open 3ds-examples.
