# Bald Elite — native Android edition (v1.2, PSP audio + launcher icon)

**Real Android APK, not an emulator.** Original OpenGL ES 2.0 3D renderer with a procedural, textured head and multiple haircuts. Uses the Bald Elite/3DS gameplay loop, rewritten in Java for Android devices.

## Controls
- **PLAY** — start an applicant, **ZEN / 90 SEC** — choose a mode.
- **Drag one finger across the head** — shave; the touch path cuts all crossed hair strands.
- **Two-finger drag** — orbit and tilt head.
- During gameplay, **TURN L / TURN R**, **UP / DOWN**, **ZOOM**, **PAUSE**, **MENU** are touch buttons.
- **NEXT** — switch applicant after victory.
- Scoring, combos, timed challenge and best results are included; best scores saved in Android preferences.
- No internet, tracking, account or special permissions required.

## Build
From the `android` directory, run `gradle assembleDebug` with Android SDK 35 and JDK 17.
The v1.1 APK targets Android 15 (API 35), eliminating the outdated-app warning on Android 16/17 while preserving Android 6+ installation compatibility. Because GitHub CI builds use fresh debug-signing certificates, Android may reject an update over v1.0 with INSTALL_FAILED_UPDATE_INCOMPATIBLE; if that happens, uninstall v1.0 first (note that locally saved scores are then removed). For a seamless upgrade path in later versions, use a persistent signing key held outside the public repository.

The generated APK is at `app/build/outputs/apk/debug/app-debug.apk`.
The repository workflow `Build Bald Elite Android` builds and uploads the APK automatically.

## Design
GPU-rendered 3D, 256px procedural face texture, batched colored hair, z-buffer, and responsive full-screen Android HUD. Only removed hair segments are updated. The Android version is independent from PSP and 3DS and does not overwrite those projects.

This is an initial Android port; the real-world GPU/FPS and device-specific touch feel require testing on hardware.


## v1.2: original PSP audio

The app ships all three original sounds directly from the PSP game:
- Title/result music: original generated Deep Voice vocal song (PCM8, 11,025 Hz).
- Gameplay: corresponding original instrumental mix (PCM8, 11,025 Hz).
- Victory: actual Piper synthesised PSP announcement, "Another bald boss!" (PCM8, 22,050 Hz).

Android's audio mixer also recreates the PSP clipper motor, contact/cut SFX and automatic music ducking. MUSIC ON/OFF only affects the soundtrack; clippers and announcer still play.

Launcher icons are generated at 5 densities using the project's original custom Bald Elite design and Pillow. The app does not load assets from the internet.

**Building:** GitHub Actions downloads the existing verified PSP audio generation artifact from run 39478827781. The artifact is subject to GitHub's retention period; once expired, rerun the PSP audio generation workflow and update the run-id in `.github/workflows/build-android.yml`. For offline development, provide those headers yourself and run `python3 android/tools/prepare_assets.py --music-header <path> --voice-header <path>` prior to Gradle.


## Bald Elite Android v2.0 DELUXE

- A separate app ID `com.baldelite.deluxe` allows installing alongside v1.2 without overwriting or deleting existing records. The Deluxe app has its own score storage.
- NEW INTRO: Club invitation and entry, with original deluxe club concept artwork.
- MENU: atmospheric barbershop background, three applicant previews, graphic quality toggle and music control.
- VICTORY: gold confetti, high-score display, champion announcement and cheering effect.
- RENDERER: 86x120 face mesh, new neck geometry, 1024x1024 portrait atlas, specular skin lighting, rim fill, ribbon hair cards, 512px and lower polygon Standard mode.
- AUDIO: the same original PSP music masters are now regenerated as 44.1kHz stereo Ogg Vorbis quality 7. The title and victory use vocals, gameplay uses the instrumental. Effects include classic clipper, enhanced shave, victory voice, menu stingers and synthesized crowd applause.
- ASSETS: The code creates reproducible club scene backgrounds with Pillow. A separately packaged Deluxe APK may substitute the high-resolution commissioned artwork (club_intro, club_menu, club_victory and club_crest) from the user's image-generation session. This repository deliberately does not claim that generated art is contained in its source tree.
- Android minSdk23 / targetSdk35; no internet permission and no telemetry.


## Bald Elite Android v2.1 — THE BALD MAN CLUB

This update adds a second chapter of the club invitation, an interactive
"About the Club" panel in the applicant menu, rotating original humour, new
victory/failure captions, and anatomy improvements to the actual 3D mesh.

* Club invitation: "Only the perfectly bald may enter"; four club rules
  and explicit acknowledgement before admission to the game menu.
* Club rules and lore: no visible hair; every follicle inspected; a glossy
  dome counts as a badge of honour; absolutely no combs.
* Changing jokes: "We do not negotiate with follicles", "Hair today. Gone today",
  "Shampoo? We retired it in 1987", and others.
* Victory: Council of Polished Scalps-approved quips, scores, confetti
  and the original sung PSP soundtrack.
* Failure: humorous rejection reasons, percent shaved and retry/menu.
* Face mesh: 110x160 segments in Deluxe HD, 64x96 in Standard,
  curved anatomical ears on both sides, improved cheekbones, jawline,
  chin, eyebrow ridge and nose; applicant-specific skin details.
* Face texture: 2048x2048 skin in HD, 1024x1024 Standard, extra pores,
  age details and glossy scalp shader. Haircutting still uses GameState
  as its source of truth, while the shader mesh is updated only as needed.
* All premium scene images and original 44.1 kHz stereo PSP audio are kept.

This version intentionally uses `com.baldelite.club` so it installs
**alongside** v2.0 Deluxe without deleting its local saved high scores.
High scores are not yet shared between different app IDs.

See `android/RELEASE_NOTES_v2.1.md` for complete release notes.

## Bald Elite v2.2 — Seamless Neck & Natural Skin

Corrects a significant anatomical error in the procedural 3D character.
The v2.1 neck was a cylinder centred behind the head, causing a visible
disconnected seam. The v2.2 neck now begins on the **exact last head ring**,
with matching positions, angles, UVs and normals, and gradually transitions
to the collar through a smooth throat curve. Both meshes use the same angular
tessellation so no hole is visible while orbiting the head.

The old Canvas circles/flat facial shapes have been superseded by a
reproducible **2048 x 2048** seamless skin UV atlas generated in
`android/tools/make_face_textures.py`. Each of the three applicants gets
an individual complexion with natural almond eyelids, iris fibres, fine brow
strands, anatomically softened nostrils and lips, cheeks, pores and stubble.
The generator also creates a packed tangent-detail/specular atlas used
by the ES 2.0 shader to give the crown smoother realistic skin highlights.
Standard graphics mode downscales the same real source skin atlas to 1024px.

CI verifies that all texture assets exist and uploads example images for
inspection. This build is published as `com.baldelite.club.hd` to install
alongside v2.1 without losing existing scores, since the prior APK was signed
with a temporary key. All club jokes, art, 44.1 kHz original PSP music,
gameplay, score saving and touch controls are preserved.

The software build confirms installation-package integrity. A physical
Android device is still required to validate appearance, shading and FPS.


## Bald Elite v2.3 — Profile, HUD & Hair Overhaul

The 2.2 sideways-profile screenshot exposed two actual problems:
the front portrait image was still sampled all the way around the
cylindrical 3D head, and the onscreen HUD covered much of the 3D model.
v2.3 addresses both in software (not just a mock-up).

**Face mapping:** Face shader now reads separate photographic face and
per-applicant, seamless side/back scalp atlases. Facial detail is confined
to the frontal section in UV-space, and smoothly blended to skin after
45 degrees, fully scalp at ~90 degrees. This prevents extraneous noses,
eyes and lips from stretching over side and rear surfaces.

**HUD:** Gameplay info now resides in a single 28px-high HUD strip with a
four-pixel progress bar at y34. The controller dock at y241 is only
28 pixels high, leaving the middle 39..240 region completely clear for
the 3D head. Existing functional buttons, pause, and 2-finger orbit remain.

**Hair:** The former single opaque triangular hair is replaced by
three-dimensional, nine-vertex curved/tapered ribbon geometry with
slightly individualized lengths, thickness, highlights, direction and
natural temple recession. Clipped follicles leave a very short temporary
stubble for half a second then vanish, without increasing the number of
touches needed to complete the 90-second mission.

**Installation:** v2.3 package `com.baldelite.club.profile` runs
alongside the previous version because Android debug-signing keys may differ.
The v2.2 save data remains untouched. No internet or microphone needed.

**Verification:** CI builds the Android APK and checks the portrait,
side-scalp, and music assets; physical-device profiling remains necessary.


## Bald Elite v2.4 — TRUE HEAD / PROFILE REBUILD

The v2.3 player screenshot showed that the ears and head profile still did not
look convincing, and that the number of hairs could be clipped on ultrawide
Samsung screens. v2.4 makes these functional corrections:

- **Sculpted skull**: recomputed occiput curvature and temple hollow on the
  gameplay's actual headPoint() surface, with a reduced jaw taper and removal
  of the previous fake painted-on ear swelling.
- **Real 3D ear shells**: curved anatomical cartilage attached at a sampled
  skull socket, with a dimensional antihelix, concha depression and matching
  back shell. Each ear has an independent 512px radial ear-material texture.
- **Four distinct shader regions**: explicit vertex-region attribute routes
  the face, side/back scalp, neck and ears through separate material controls.
  Front portrait details are restricted to the frontal 0.085–0.17 UV band,
  replacing the old 0.125–0.245 smear across the visible side profile.
- **Scalp and ear materials** are unique per candidate and created by the
  `make_profile_scalp.py` asset generator, so there can be no portrait eye
  or mouth graphics baked into the side of the head.
- **Gameplay HUD** respects extra-wide phone screens with a uniform UI scale
  and centered safe frame. The hair counter is now right-aligned instead of
  centered at the edge, avoiding clipped numbers. Touch hit targets are
  transformed using the identical coordinate transform.
- **v2.3's curved hair ribbons, temporary stubble, club lore, luxury art,
  original stereo PSP music and gameplay are preserved.**

The 2.4 application ID `com.baldelite.club.truehead` installs alongside
2.3 without replacing its saves. Actual visual fidelity still requires
photographs/screenshots from a physical Android device; successful CI does
not prove the ear silhouette and every texture seam look realistic.


## Bald Elite v2.5 — Instrument Band Soundtrack

Based on player feedback that the old synthesizer music sounded like MIDI,
the old 44.1 kHz tracks have been replaced with entirely new **offline,
sample-instrument arrangements**. There is no MIDI synthesizer on the phone.

- Menu: original club-lounge funk composition, **113 BPM**, approx 51 s.
- Gameplay: original energetic funk-rock composition, **140 BPM**, approx 69 s.
- Arrangement: sample-based fingered electric bass, clean and overdriven guitar,
  acoustic steel-string guitar, acoustic drum-kit samples, Rhodes electric
  piano, Hammond-style organ, saxo melody and short horn accents.
- Humanized micro-timing, varied velocities, offbeat guitar chops and
  room reflections make it sound less sequenced than v2.4.
- 44,100 Hz, stereo, Ogg Vorbis quality 7.
- **Not a live studio recording:** audio uses real-instrument *samples* from
  the TimGM6mb SoundFont, GPL-2 (see Debian/Ubuntu soundfont package notices).
  Source script: `android/tools/render_instrument_band.py`.
- The spoken PSP victory announcement, clipper motor, shave sounds, menu
  feedback, original art, and all gameplay mechanics remain unchanged.
- v2.5 retains app ID `com.baldelite.club.truehead` and raises versionCode
  from 24 to 25; an update preserves records if signed with the same key.
  Debug CI APKs are automatically signed with ephemeral keys, so a release
  should use a persistent signing key to support in-place upgrades.
