@echo off
rem Abre la terminal serie para jugar Road Rocks (9600 baudios, 8N1).
rem Busca sola el puerto COM de la tarjeta; para fijarlo:  jugar.bat COM5
title Road Rocks - terminal serie
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0terminal.ps1" %*
echo.
pause
