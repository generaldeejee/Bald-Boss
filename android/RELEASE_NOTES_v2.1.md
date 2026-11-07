# Bald Elite v2.1 — The Bald Man Club

**Tagline:** NO HAIR. NO FEAR. NO UNAUTHORIZED FOLLICLES.

## What's new

The club now has a proper identity. Before playing, guests receive a two-page
invitation to the exclusive Bald Man Club. The Council of Polished Scalp's
four rules explain why only perfectly bald men are allowed past the velvet
rope. Applicants can read the club charter again via **ABOUT THE CLUB**.

The character-select menu rotates eight satirical slogans, including
"We do not negotiate with follicles" and "Combs are contraband". Victories
display a random membership approval, while missed deadlines produce
one of six very polite (and very bald) rejection notices.

## Bigger and better-looking heads

| Feature | v2.0 Deluxe | v2.1 |
|---|---|---|
| Face atlas, HD | 1024 x 1024 | 2048 x 2048 |
| Face atlas, Standard | 512 x 512 | 1024 x 1024 |
| Face rings x slices, HD | 86 x 120 | 110 x 160 |
| Face rings x slices, Standard | 48 x 72 | 64 x 96 |
| Ear geometry | Simple scalp protrusion | Curved paired ear shells |
| Facial sculpt | General-purpose oval | Stronger cheekbone, jaw, chin, brow, nose |
| Complexion | One shared procedural texture | Applicant-specific face details |
| Lighting | Specular + rim | Crown-dependent specular + rim and fill |

The original OpenGL ES 2.0 graphics and tested touch shave mechanics remain
the foundation. The high-poly head stays responsive because only non-removed
hairs are redrawn when shaving. Use **HD OFF** on slower devices.

## Original soundtrack and club atmosphere

The original PSP music is still included as 44.1kHz stereo Ogg, reconstructed
from the game's freely included generation scripts (not commercial audio).
The PSP victory announcement and shaving sounds, new UI pings, and crowd
ambience accompany the intro and victory scenes. The previously commissioned
club artwork remains the intro, menu backdrop, victory artwork and app icon
in the user-facing packaged build.

## Installation

This package has app ID `com.baldelite.club`, so it installs next to
`com.baldelite.deluxe` v2.0. You don't need to remove either app. Records
remain in their respective apps, and aren't automatically transferred.

**Hardware status:** Built and automatically APK-checked in CI. The 2.1
graphics and user interaction need a physical Android-device playtest before
any claims about FPS or visual fidelity can be treated as confirmed.

## Original humour used throughout the game

- "Only bald men may join the inner circle."
- "Every last strand must go."
- "A polished scalp is a badge of honour."
- "Combs are strictly forbidden."
- "No hair. No fear. No refunds on shampoo."
- "This scalp is inner-circle material."
- "Too much hair. The council is not amused."

## Build information

Android 6+ compatibility, OpenGL ES 2.0, Android 15 target SDK.
No internet, tracking, microphone or account required.
