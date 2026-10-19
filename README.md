# BALD BOSS 3D — PSP Edition (source prototype v0.1)

A native PlayStation Portable homebrew rewrite of the PS1 Bald Boss 3D mini-game.
**This is PSP C source for PSPSDK/libGU, not a PS1 EXE or a Windows program.**

## Improvements over the PS1 game

- **480 × 272** widescreen output, genuine PSP GU/GUM perspective 3D graphics
- Smooth lighting, textured skin, rounded ears, nose, lips, and 3D sunglasses
- 28 × 44 segments / 2,464 triangles for the head alone (much smoother than PS1)
- Up to 1,250 seeded scalp hairs/tufts (some skipped for receding temples), each independently removable
- Real hardware depth buffer so back-facing/hidden features do not draw through the head
- Smooth 360-degree rotation; head, hairs and facial features rotate together
- 90-second shave challenge, percentage, score, victory/game-over screen
- Original PSP homebrew menu icon, analog controls and safe-area HUD

This is original replacement rendering/game code, **not** the PS1 MIPS EXE run through PSP's backwards compatibility.

## Gameplay controls

| Button | Action |
| --- | --- |
| **X** on title screen | Start game |
| D-pad / analog stick | Move clippers |
| **Hold X / Square** | Shave hair under the clippers |
| L / R | Rotate head (360 degrees) |
| X on results screen | New round |
| HOME | Exit through PSP system menu |

## How to create a real `EBOOT.PBP`

The binary is built with the open-source **PSPDEV / PSPSDK** toolchain:
https://pspdev.github.io/

### Option A — GitHub Actions (no PSP compiler needed on your own PC)

1. Create a new GitHub repository and upload **everything** from this project directory, including `.github/workflows/build-psp.yml`.
2. Open the repository's **Actions** tab.
3. Select **Build PSP EBOOT**, then **Run workflow**.
4. When the build finishes, download the `BaldBoss3D-PSP-EBOOT` artifact. Inside is `EBOOT.PBP`.
5. Start PPSSPP, choose **Load…**, and open `EBOOT.PBP` (or install it under `PSP/GAME/BALDBOSS/EBOOT.PBP`).

If you don't see workflows, enable Actions for the repository. GitHub Actions may have usage limits.

### Option B — Docker Desktop (Windows/Linux/macOS)

From the project directory (PowerShell or terminal), with Docker running:

```bash
docker run --rm -v "${PWD}:/src" -w /src pspdev/pspdev:latest make
```

On Windows CMD rather than PowerShell, replace `${PWD}` with `%cd%`.
When successful, `EBOOT.PBP` appears in the project directory.

### Option C — Existing PSPDEV installation (Linux/WSL/macOS)

```bash
make
# or:
./build_linux.sh
```

Do not double-click the `.c` file. Do not load PS1 `.EXE` binaries in PPSSPP. PSP games use `EBOOT.PBP`.

## Technical notes and limitations

- The renderer uses the PSP's hardware Graphics Utility (`sceGu*` and `sceGum*`), with independent draw, display, and 16-bit depth buffers.
- Hair is constructed procedurally and is independently tracked so it can be removed while preserving complete 360-degree rotation.
- The hair model is **stylized realism**, not photorealistic scanned hair or a photorealistic face.
- All assets are original and no Sony assets/BIOS/firmware are bundled.
- **Important validation status:** The C source has passed a local GCC syntax check (using API header shims) and native host-side gameplay/geometry tests, but a PSPDEV cross-compile and a PPSSPP or real-hardware run have **NOT** been possible in the creation environment. There is **no prebuilt or verified EBOOT.PBP** in this ZIP. A successful GitHub Actions or Docker build is needed before the program can be played.
- First hardware test should check texture visibility, head rotation, controller input, scalp visibility from all angles, and performance. Follow-up iterations may be needed.

## Folder structure

```text
Bald_Boss_3D_PSP/
├── src/main.c                 # Full playable game source
├── assets/ICON0.PNG           # 144×80 PSP menu icon
├── Makefile                   # PSPSDK PSP binary + EBOOT build
├── .github/workflows/build-psp.yml
├── build_linux.sh             # Linux/WSL build
├── build_windows_wsl.bat      # Windows WSL helper
└── tools/
    ├── syntax_check.py       # Host-only C syntax check
    ├── test_model.c           # Geometry/shave gameplay regression tests
    └── verify.sh
```

## Credits

Bald Boss 3D game concept and source generated in collaboration with ChatGPT. Original project started as a PS1 homebrew and is rewritten here for PSP. Licensed under MIT; see LICENSE.


## v0.6.5 – Tondeuse, zoom en stem

- **X of vierkant ingedrukt houden:** scheren met een gegenereerd tondeusegeluid.
- **Cirkel:** drie zoomstanden (1×, ongeveer 1,4× en 1,8×). De scheerhitbox beweegt mee met de zoom.
- **Volledig kaal vóór de tijd op is:** eenmalige stemmelding **“Another bald boss!”**.
- **L/R:** hoofd draaien. **D-pad/analoge stick:** tondeuse bewegen.
- **SELECT:** achtergrondmuziek aan/uit, geluidseffecten blijven hoorbaar.

De audiothread mengt de bestaande chiptune met een originele, programmatisch gemaakte tondeusebrom en een met de vrije eSpeak-synthesizer gemaakte Engelse stem. GitHub Actions genereert via \`tools/generate_voice.py\` eerst een PCM8-header en bouwt daarna \`EBOOT.PBP\`. Lokaal bouwen: installeer eSpeak, voer \`python3 tools/generate_voice.py\` uit en daarna \`make\` met PSPDEV.

De code heeft lokale host-syntax- en gameplaytests doorstaan. De werkelijke PSPDEV-build en PPSSPP-weergave moeten nog worden gecontroleerd.

## v0.7 - Deep Voice Edition soundtrack

The complete **No Hair, No Fear! (Deep Voice Edition)** song now plays on the
title screen and on the win/timeout results screens. During the 90-second
shave challenge, a matching **instrumental** plays instead. Starting a new
round automatically switches music. **SELECT** still toggles music only,
while the clipper motor and **"Another bald boss!"** announcer remain audible.

The full song and instrumental are **original, AI-assisted compositions** made
with procedural synthesizers and offline free eSpeak vocals. The vocals are a
single slow, low synthetic male voice (not a human performance). No Suno assets
or third-party recordings are included.

### Build details

GitHub Actions runs `tools/generate_psp_music.py`, which reproduces the exact
deep-voice and instrumental mixes from the committed Python generators in
`tools/music_sources/`, then embeds both as **11,025 Hz PCM8 mono** data in
`src/music_samples.h`. This deliberately trades hi-fi quality for PSP
compatibility and avoids an additional OGG decoder. The generated header is not
versioned; it is included in the PSP build artifact automatically.

For a local PSPDEV build, install Python 3, NumPy, SciPy, SoundFile, ffmpeg,
and eSpeak; then run `python3 tools/generate_voice.py` and
`python3 tools/generate_psp_music.py` before `make`.

The full-resolution OGG/WAV tracks are intended for future Windows, Linux,
macOS, and Android ports; this repository still builds the **PSP edition**, not
yet a native desktop/mobile game. No actual PPSSPP playback validation has
been performed for this music change.
