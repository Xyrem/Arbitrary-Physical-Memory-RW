@echo off

xcopy %~dp0RwDrv.sys C:\
sc create RwDrv binpath=C:\RwDrv.sys type=kernel
sc start RwDrv

echo Press Enter To Unload
pause

sc stop RwDrv
sc delete RwDrv
del /f C:\RwDrv.sys

pause