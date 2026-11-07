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
