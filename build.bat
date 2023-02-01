@echo off
cd "%~dp0"
gcc -mwindows -Ofast -s src/SndVolTray.c -o SndVolTray.exe
upx --best --ultra-brute SndVolTray.exe>nul 2>&1