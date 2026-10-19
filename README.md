# Bald Boss 3D — PSP Edition v0.9.3 (public beta candidate)

**Bald Boss 3D** is an original, humorous, native PlayStation Portable (PSP) homebrew game. You play the **Club Barber**: shave a rotating 3D head with an electric clipper and help applicants earn the title **Bald Boss**.

### The Elite Bald Man Club

> **Only truly bald men may join. Shave these heads. Turn them into BALD BOSSES!**

As the club's barber, your job is to transform hairy applicants into bona fide bald bosses. A completed shave earns the candidate **MEMBERSHIP APPROVED!**; running out of time in Challenge Mode means **MEMBERSHIP DENIED!**. The victory announcer celebrates each new recruit with **"Another bald boss!"**

### Official naming guide

| Name | Meaning |
| --- | --- |
| **Bald Boss 3D** | The name of the video game (and PSP EBOOT) |
| **The Elite Bald Man Club** | The one and only fictional club |
| **Club Barber** | You, the player: the person who shaves applicants |
| **Bald Boss** | The honorary rank of each newly accepted bald club member |
| **Membership Approved!** | An applicant has been completely shaved and accepted |
| **Membership Denied!** | The applicant failed the timed Challenge; Zen Mode has no timeout |

In the title screen, the barber's workspace is called **Club Barber's Chair**, not a second business or club. **“Another bald boss!”** refers to the newly admitted member, not a new club name.

Built in C with the open-source PSPDEV/PSPSDK toolchain and PSP GU/GUM rendering. It is **not** a PS1 executable and needs **no PS1 BIOS**.

## What's new in v0.9.3

- **Consistent theme (v0.9.3):** game title *Bald Boss 3D*, club *The Elite Bald Man Club*, player *Club Barber*, accepted member *Bald Boss*. Replaced the confusing *Boss Barbers* and *Elite Bald Club* aliases across the in-game menus. Win voice and music remain unchanged.


- **More natural victory announcer (v0.9.2):** builds use the Piper **en_US-john-medium** neural male voice instead of the original robotic eSpeak rendering for “Another bald boss!”. The 22,050 Hz mono PCM8 recording is embedded in the EBOOT; **no neural-network runtime or model is shipped on PSP**. Original music and all shaving sound effects are retained.
- **Reproducible speech generation:** the workflow downloads and SHA-256 verifies the model, caches it between builds and verifies the new header was actually generated. Requires network access to the public Piper model *only when building*. Run `python3 tools/generate_voice.py --legacy` only if you specifically want the old eSpeak voice for troubleshooting.


- **Elite Bald Man Club theme (v0.9.1):** rewritten start-menu mission, club-themed tutorial, **Membership Approved** victory and **Membership Denied** timed-out results.
- **90 Seconds Challenge:** the classic timed shave, with a 30-second cleanup bonus near the end and protection against losing to one inaccessible final hair.
- **Zen Mode:** the same shaving, hair particles, sound effects and free rotation, but without a time limit. The clock tracks elapsed time.
- **Victory celebration:** an automatic slow 360° head orbit to showcase your freshly polished scalp, a score, a one-to-three-star rating, a **Membership Approved** banner, and the original **“Another bald boss!”** announcer.
- **In-game tutorial:** displayed on the first round; also accessible from the title screen with **Triangle**, or from **Pause → How to Play**.
- Preserves the procedurally modeled 3D head, 256×256 textured skin, hair-finding help, dynamic scalp gloss, zoom, vertical head tilt and the original Deep Voice Edition soundtrack.

### Two ways to play

| Mode | Objective | Clock | Stars |
| --- | --- | --- | --- |
| 90 Seconds Challenge | Shave every strand before the timer expires | 90 seconds initially; a cleanup-time bonus near the end | 3 stars for finishing with a displayed time of 50 seconds or less elapsed, 2 for 75 seconds or less, otherwise 1 |
| Zen Mode | Shave at your own pace | No time limit; elapsed time is displayed | 3 stars on completion (speed is not scored) |

The last-hair locator and close-up assistance work in **both** modes.

## PSP controls

| Control | Action |
| --- | --- |
| **D-pad Up/Down** on title screen | Choose Challenge or Zen Mode |
| **X** on title screen | Start the selected mode |
| **Triangle** on title screen | Open controls tutorial |
| **D-pad / analog stick** in-game | Move the clipper |
| **Hold X or Square** | Shave the hair under the clipper; quick movement shaves across the travelled path |
| **L / R** | Rotate the head left / right |
| **Hold L + Up/Down** | Tilt the head to reveal the crown and the underside |
| **Circle** | Switch camera zoom |
| **Triangle** when ≤3 hairs remain | Aim the camera and clipper at a remaining hair |
| **Select** | Toggle background music (clipper and voice remain audible) |
| **Start** | Pause / open the pause menu |
| **Up/Down, X** in pause menu | Choose Resume, Restart Run, Return to Title, or How to Play |
| **Circle** in pause menu | Return to gameplay |
| **X or Circle** in tutorial | Close tutorial |
| **X** on victory / timeout | Play again in the currently selected mode |
| **HOME** on physical PSP | System exit menu |

The game uses on-screen tracking markers to help you find the last hairs and stops the Challenge clock when only **one hair** remains.

## Installation and playing

Download the compiled **EBOOT.PBP** from the [GitHub Actions build page](https://github.com/generaldeejee/Bald-Boss/actions) when the newest build has passed.

**PPSSPP emulator:** open **EBOOT.PBP** using *Load*.

**Physical PSP:** use a homebrew-capable PSP, create the folder `/PSP/GAME/BALDBOSS/` on the Memory Stick, and copy the file as `/PSP/GAME/BALDBOSS/EBOOT.PBP`. Launch it from the PSP Game menu.

A **GitHub Actions artifact is not a permanent public release**. For a public homebrew-site submission, attach a tested installation ZIP to a proper GitHub Release rather than directing users to a short-lived Actions artifact.

## Building from source

The GitHub workflow [Build PSP EBOOT](https://github.com/generaldeejee/Bald-Boss/actions) generates its audio data and compiles the game. It is triggered automatically for relevant code/asset changes, or manually through *Run workflow*.

For a local PSPSDK installation, install the free **piper-tts==1.3.0** build-time package along with NumPy and SciPy. The speech model is about 63.5 MB and is cached only on the build computer:

```sh
python3 -m pip install numpy scipy soundfile piper-tts==1.3.0
```

The Piper model is downloaded and SHA-256-verified automatically by the announcer generator. If model download fails, the normal build stops rather than silently substituting robotic speech.

For a local PSPSDK installation:

```sh
python3 tools/generate_voice.py
python3 tools/generate_psp_music.py
make
```

The audio generators require their documented Python dependencies (NumPy, SciPy and SoundFile for the music workflow) and ffmpeg/eSpeak where used. You may alternatively follow `.github/workflows/build-psp.yml` for the exact dependencies and use the `pspdev/pspdev` toolchain container.

The game renders at **480×272**, uses its own PSP GU/GUM renderer, and generates the head mesh and base face texture on startup.

## Credits and licensing

- **Source code:** MIT; see [LICENSE](LICENSE).
- **Original 3D head and UV paint generator:** project-authored and marked CC0 in `src/bald_head_v2.h`.
- **Soundtrack:** original/AI-assisted procedural compositions and eSpeak-derived vocals made using the repository's music generators. The title/results use the Deep Voice track; gameplay uses its instrumental.
- **Victory announcer:** synthetic offline speech generated with [Piper](https://github.com/rhasspy/piper) using the [John medium English male voice](https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_US/john/medium), developed from LibriVox public-domain audio (see its [model card](https://huggingface.co/rhasspy/piper-voices/blob/main/en/en_US/john/medium/MODEL_CARD)). Piper is used only at build time; audio samples alone are distributed in the game, not the model or Piper executable. The old eSpeak announcer is available through the explicit `--legacy` generator option.
- **Development:** project concept and coding work developed in collaboration with ChatGPT. This disclosure is useful when submitting to community homebrew indexes.

No copyrighted Sony game, BIOS or firmware files are distributed.

## Beta release checklist

Before publishing a download on a homebrew site, verify:

- [ ] The latest GitHub Actions **PSP EBOOT** build succeeds and its artifact contains `EBOOT.PBP`.
- [ ] Both modes can finish a full shave in PPSSPP without crashes, audio glitches or missing hairs.
- [ ] Zoom, head pitch, final-hair locator, pause/resume, restart, title navigation and tutorial work.
- [ ] The victory orbit, stars, score and announcer behave properly after the win.
- [ ] Test on physical PSP hardware if available, especially PSP-1000 (lower memory), or clearly state if only PPSSPP was tested.
- [ ] Take genuine in-game screenshots and include them in a GitHub Release.
- [ ] Publish a ZIP containing `PSP/GAME/BALDBOSS/EBOOT.PBP` with version, changelog and license.

**Verification status:** Source-level integration checks have been performed for v0.9.3. The Piper neural voice generator and changed PSP sample rate still need confirmation from the GitHub Actions build and a PPSSPP listening test. A successful PSPDEV cross-compilation, PPSSPP playthrough of v0.9.0 and physical PSP testing still need confirmation. Do not label this build “fully tested on PSP” until it has been verified.

## Reporting issues

Include your PSP model or PPSSPP version, the mode (Challenge or Zen), the steps to reproduce, and a screenshot or the GitHub Actions error log. Report problems through the repository’s GitHub Issues tab.

---

*No hair. No fear. Just baldness.*
