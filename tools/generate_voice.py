#!/usr/bin/env python3
"""Build the Bald Boss victory line with a natural offline Piper male voice.

The 63 MB model lives only in the GitHub Actions build cache, NOT on PSP.
John medium uses public-domain LibriVox recordings (see MODEL_CARD).
For an explicit emergency fallback, pass --legacy for the original eSpeak.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import urllib.request
import wave

import numpy as np
from scipy.signal import butter, resample_poly, sosfilt

ROOT = Path(__file__).resolve().parents[1]
DST = ROOT / "src" / "voice_sample.h"
MODEL = "en_US-john-medium.onnx"
MODEL_HASH = "789c6c875726e627ddee93d51d8727859abe9c091c3d141591f4b83c2072e988"
MODEL_URL = ("https://huggingface.co/rhasspy/piper-voices/resolve/main/"
             "en/en_US/john/medium/")
SAMPLE_RATE = 22050


def checksum(path):
    h = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def get_voice(cache):
    cache.mkdir(parents=True, exist_ok=True)
    model = cache / MODEL
    cfg = cache / (MODEL + ".json")
    if not model.is_file() or checksum(model) != MODEL_HASH:
        print("Downloading and verifying the Piper John neural voice model...")
        tmp = cache / (MODEL + ".partial")
        try:
            with urllib.request.urlopen(MODEL_URL + MODEL, timeout=180) as remote:
                with tmp.open("wb") as output:
                    while True:
                        data = remote.read(1 << 20)
                        if not data:
                            break
                        output.write(data)
            if checksum(tmp) != MODEL_HASH:
                raise ValueError("Voice model SHA256 mismatch; refusing unverified download")
            tmp.replace(model)
        finally:
            tmp.unlink(missing_ok=True)
    if not cfg.is_file():
        tmp = cache / (MODEL + ".json.partial")
        try:
            with urllib.request.urlopen(MODEL_URL + MODEL + ".json", timeout=60) as response:
                tmp.write_bytes(response.read())
            meta = json.loads(tmp.read_text(encoding="utf-8"))
            if meta.get("dataset") != "john" or meta["audio"]["sample_rate"] != 22050:
                raise ValueError("Unexpected neural model configuration")
            tmp.replace(cfg)
        finally:
            tmp.unlink(missing_ok=True)
    return model, cfg


def synthesize(destination, cache, legacy=False):
    if legacy:
        subprocess.run(["espeak", "-v", "en-us", "-p", "36", "-s", "138",
                        "-a", "150", "-w", str(destination), "Another bald boss!"],
                       check=True)
    else:
        model, cfg = get_voice(cache)
        subprocess.run(
            ["piper", "--model", str(model), "--config", str(cfg),
             "--output_file", str(destination)],
            input="Another bald boss!\n", text=True, check=True, timeout=180,
        )
    if not destination.is_file() or destination.stat().st_size < 5000:
        raise RuntimeError("The voice generator did not produce the expected WAV")


def voice_pcm8(path):
    with wave.open(str(path), "rb") as recording:
        rate = recording.getframerate()
        channels = recording.getnchannels()
        if recording.getsampwidth() != 2 or channels not in (1, 2):
            raise ValueError("Expected PCM16 WAV (mono or stereo)")
        samples = np.frombuffer(recording.readframes(recording.getnframes()),
                                dtype="<i2").astype(np.float32) / 32768.0
        if channels == 2:
            samples = samples.reshape(-1, 2).mean(axis=1)
    if rate < 11025 or len(samples) < rate // 2:
        raise ValueError("Voice WAV too short or sample rate too low")

    # Remove excessive initial silence, but keep a natural lead-in and tail.
    voiced = np.where(np.abs(samples) > 0.012)[0]
    if len(voiced):
        left = max(0, int(voiced[0]) - int(rate * 0.08))
        right = min(len(samples), int(voiced[-1]) + int(rate * 0.17))
        samples = samples[left:right]
    samples = sosfilt(butter(2, 85, fs=rate, btype="highpass", output="sos"), samples)
    from math import gcd
    divisor = gcd(rate, SAMPLE_RATE)
    samples = resample_poly(samples, SAMPLE_RATE // divisor,
                             rate // divisor).astype(np.float32)
    peak = float(np.quantile(np.abs(samples), .997))
    gain = min(3.0, .80 / max(peak, .05))
    samples = np.tanh(samples * gain) / np.tanh(1.0)
    samples = np.clip(samples, -.96, .96)
    fade_in = min(len(samples) // 3, int(SAMPLE_RATE * .012))
    fade_out = min(len(samples) // 3, int(SAMPLE_RATE * .075))
    samples[:fade_in] *= np.linspace(0, 1, fade_in)
    samples[-fade_out:] *= np.linspace(1, 0, fade_out)
    if not SAMPLE_RATE * .6 <= len(samples) <= SAMPLE_RATE * 7:
        raise ValueError("Unexpected announcer duration")
    return np.clip(np.rint(samples * 123 + 128), 0, 255).astype(np.uint8)


def write_header(samples, neural):
    lines = [
        "/* Bald Boss announcer: 22.05 kHz PCM8; synthesized with "
        + ("Piper John / public-domain LibriVox." if neural else "legacy eSpeak.") + " */",
        "#ifndef BALD_BOSS_VOICE_SAMPLE_H",
        "#define BALD_BOSS_VOICE_SAMPLE_H",
        "#define VOICE_SAMPLE_RATE 22050",
        "#define BALD_BOSS_VOICE_ENGINE_PIPER " + ("1" if neural else "0"),
        f"#define VOICE_SAMPLE_LEN {len(samples)}",
        "static const unsigned char voice_sample[VOICE_SAMPLE_LEN] __attribute__((aligned(16))) = {",
    ]
    for offset in range(0, len(samples), 32):
        lines.append("  " + ",".join(f"0x{int(b):02x}" for b in samples[offset:offset+32]) + ",")
    lines.extend(["};", "#endif", ""])
    DST.parent.mkdir(parents=True, exist_ok=True)
    DST.write_text("\n".join(lines), encoding="ascii")
    print("Voice generated:", len(samples), "samples at 22,050 Hz, PCM8.",
          "Piper neural" if neural else "legacy eSpeak")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy", action="store_true", help="Explicit old espeak fallback")
    args = parser.parse_args()
    cache = Path(os.environ.get("BALD_BOSS_PIPER_CACHE",
                                str(ROOT / ".cache" / "piper")))
    with tempfile.TemporaryDirectory() as folder:
        wav = Path(folder) / "announcement.wav"
        synthesize(wav, cache, legacy=args.legacy)
        write_header(voice_pcm8(wav), neural=not args.legacy)


if __name__ == "__main__":
    main()
