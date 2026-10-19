#!/usr/bin/env python3
"""Generate the original Bald Boss announcement as PSP-ready PCM8 C bytes.
Needs free espeak installed on the build host, not on the PSP.
"""
from array import array
from pathlib import Path
import subprocess
import sys
import tempfile
import wave

root = Path(__file__).resolve().parents[1]
destination = root / "src" / "voice_sample.h"
with tempfile.TemporaryDirectory() as folder:
    wav = Path(folder) / "announcer.wav"
    subprocess.run(["espeak", "-v", "en-us", "-p", "36", "-s", "138",
                    "-a", "150", "-w", str(wav), "Another bald boss!"],
                   check=True)
    with wave.open(str(wav), "rb") as input_file:
        rate = input_file.getframerate()
        if input_file.getnchannels() != 1 or input_file.getsampwidth() != 2:
            raise ValueError("Expected 16-bit mono eSpeak PCM")
        audio = array("h")
        audio.frombytes(input_file.readframes(input_file.getnframes()))
        if sys.byteorder != "little":
            audio.byteswap()
    if rate not in (22050, 11025):
        raise ValueError(f"Unexpected eSpeak rate {rate}")
    if rate == 22050:
        audio = array("h", (int((audio[i] + audio[i+1]) / 2)
                            for i in range(0, len(audio)-1, 2)))
    count = len(audio)
    pcm8 = []
    for i, value in enumerate(audio):
        fade = min(1.0, i / 220.0, (count - 1 - i) / 1000.0)
        sample = int(round(128.0 + value * 0.0031 * max(0.0, fade)))
        pcm8.append(max(0, min(255, sample)))

rows = ["/* Original announcement, synthesized off-device using free eSpeak. */",
        "#ifndef BALD_BOSS_VOICE_SAMPLE_H", "#define BALD_BOSS_VOICE_SAMPLE_H",
        "#define VOICE_SAMPLE_RATE 11025", f"#define VOICE_SAMPLE_LEN {len(pcm8)}",
        "static const unsigned char voice_sample[VOICE_SAMPLE_LEN] __attribute__((aligned(16))) = {"]
for j in range(0, len(pcm8), 32):
    rows.append("  " + ",".join(f"0x{x:02x}" for x in pcm8[j:j+32]) + ",")
rows.extend(["};", "#endif", ""])
destination.write_text("\n".join(rows), encoding="utf8")
print(f"Generated {destination}, {len(pcm8)} samples at 11.025 kHz")
