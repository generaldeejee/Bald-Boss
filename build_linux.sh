#!/usr/bin/env bash
set -euo pipefail
if ! command -v psp-config >/dev/null 2>&1; then
  echo 'PSPDEV not detected. Install PSPDEV from https://pspdev.github.io/' >&2
  exit 1
fi
make clean && make
mkdir -p release/PSP/GAME/BALDBOSS
cp -f EBOOT.PBP release/PSP/GAME/BALDBOSS/EBOOT.PBP
echo 'Ready: release/PSP/GAME/BALDBOSS/EBOOT.PBP'
