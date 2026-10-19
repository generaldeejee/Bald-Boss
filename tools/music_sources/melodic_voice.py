#!/usr/bin/env python3
"""Melody synthesis from Piper speech using formant-preserving Praat TD-PSOLA.

This is pitch-controlled synthesized singing, not a recording of a human singer.
The original speaker's consonants/timbre are preserved as well as possible.
Used only offline during audio generation; no dependency runs on a PSP.
"""
import numpy as np
import parselmouth
from parselmouth.praat import call
from scipy.ndimage import gaussian_filter1d, maximum_filter1d

NOTE_SEMITONES = {
    "C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5,
    "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11,
}


def note_frequency(note):
    midi = (int(note[-1]) + 1) * 12 + NOTE_SEMITONES[note[:-1]]
    return 440.0 * 2.0 ** ((midi - 69) / 12.0)


def sing_melody(signal, sample_rate, notes):
    """Fit speech syllables approximately across specified melodic notes.

    Each note gets an expressive onset scoop and restrained vowel vibrato.
    Overlap-add keeps unvoiced consonants and spectral formants much more
    natural than simple resampling, which changes playback speed and timbre.
    """
    signal = np.asarray(signal, dtype=np.float32)
    if len(signal) < int(sample_rate * .20) or not notes:
        return signal
    sound = parselmouth.Sound(signal.astype(np.float64), sample_rate)
    manipulation = call(sound, "To Manipulation", .01, 70, 400)
    tier = call(manipulation, "Extract pitch tier")
    duration = len(signal) / float(sample_rate)
    call(tier, "Remove points between", 0.0, duration)
    note_time = duration / len(notes)
    for j, note in enumerate(notes):
        base = note_frequency(note)
        start = j * note_time
        end = (j + 1) * note_time
        if j == 0:
            call(tier, "Add point", .002, base * .974)
        else:
            call(tier, "Add point", max(.001, start - .040),
                 note_frequency(notes[j - 1]))
        for t in np.arange(start + .018, end, .025):
            progress = (t - start) / note_time
            scoop = 1.0 - .014 * max(0.0, 1.0 - progress / .23)
            vibrato = 1.0 + .009 * np.sin(
                2.0 * np.pi * 5.1 * (t - start)
            ) * max(0.0, (progress - .50) * 2.0)
            call(tier, "Add point", min(t, duration - .001),
                 base * scoop * vibrato)
    call(tier, "Add point", duration - .001, note_frequency(notes[-1]))
    call([manipulation, tier], "Replace pitch tier")
    shifted = call(manipulation, "Get resynthesis (overlap-add)").values[0]
    if len(shifted) != len(signal):
        shifted = np.interp(
            np.linspace(0, 1, len(signal)),
            np.linspace(0, 1, len(shifted)), shifted
        )
    # Very little of the spoken fundamental may bleed through, or it
    # destabilizes the composed notes. TD-PSOLA already preserves consonants.
    return (.03 * signal + .97 * shifted).astype(np.float32)


def foreground_vocal_mix(instrumental, vocal, sample_rate):
    """Side-chain the backing to the voice; keep the instrumental master intact.

    Smooth envelope on a 10 ms grid minimizes pump/click artefacts. This
    creates an audible space for lyrics in the PSP's 11 kHz mono reduction.
    """
    assert instrumental.shape == vocal.shape and instrumental.ndim == 2
    block = max(1, int(sample_rate * .01))
    length = len(vocal)
    envelope = np.max(np.abs(vocal), axis=1)
    extra = (-length) % block
    if extra:
        envelope = np.pad(envelope, (0, extra))
    coarse = envelope.reshape(-1, block).max(axis=1)
    coarse = maximum_filter1d(coarse, size=13, mode="nearest")
    coarse = gaussian_filter1d(coarse, sigma=9.0)
    amount = np.clip((coarse - .007) / .024, 0.0, 1.0)
    # Up to roughly 9 dB of background reduction while a phrase is active.
    duck = 1.0 - .65 * amount
    smooth = np.interp(
        np.arange(length) / block,
        np.arange(len(duck)), duck
    ).astype(np.float32)
    # Vocal-forward mix with soft overload protection in the existing master.
    song = instrumental * (.81 * smooth[:, None]) + vocal * 2.40
    return song.astype(np.float32), {
        "min_instrument_gain": round(float(smooth.min() * .81), 3),
        "peak_pre_master": round(float(np.max(np.abs(song))), 3),
        "voice_active_fraction": round(float(np.mean(amount > .3)), 3),
    }
