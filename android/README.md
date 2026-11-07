# Bald Elite — native Android edition (v1.1)

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
