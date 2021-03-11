@echo off

taskkill /F /IM PPSSPPWindows64.exe > nul 2>&1
taskkill /F /IM WerFault.exe > nul 2>&1

start PPSSPPWindows64.exe
exit