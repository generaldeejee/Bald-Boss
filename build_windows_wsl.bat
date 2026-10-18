@echo off
REM Run inside the extracted project directory using WSL + installed PSPDEV.
where wsl >nul 2>&1
if errorlevel 1 (
  echo Install Windows Subsystem for Linux and PSPDEV first.
  pause
  exit /b 1
)
wsl bash -lc "cd \"$(wslpath '%CD%')\" && make clean && make"
echo If successful, copy EBOOT.PBP to PSP/GAME/BALDBOSS/.
pause
