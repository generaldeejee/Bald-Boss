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
