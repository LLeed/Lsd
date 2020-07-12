@echo off
rem *************************************************************
rem
rem	 LSD 7.3 - December 2020
rem	 written by Marco Valente, Universita' dell'Aquila
rem	 and by Marcelo Pereira, University of Campinas
rem
rem	 Copyright Marco Valente and Marcelo Pereira
rem	 LSD is distributed under the GNU General Public License
rem	
rem *************************************************************

rem *************************************************************
rem  ADD-SHORTCUT-WINDOWS.BAT
rem  Add a shortcut to LSD LMM in the Windows desktop.
rem *************************************************************

if "%1"=="/?" (
	echo Add a shortcut to LSD LMM in the desktop
	echo Usage: add-shortcut-windows
	goto end
)
"%CD%\gnu64\bin\Shortcut.exe" /f:"%USERPROFILE%\Desktop\LMM.lnk" /a:c /t:"%CD%\run.bat" /w:%CD% /r:7 /i:%CD%\src\icons\lsd.ico /d:"Lsd Model Manager"
:end