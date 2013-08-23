;----------------------------------------------------------------
;
;  NSIS script for Eureka Level Editor
;
;----------------------------------------------------------------

; use the Modern UI
!include "MUI2.nsh"


;----------------------------------------------------------------

; The name of the installer
Name "Eureka Setup"

; The file to write
OutFile "Eureka_setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\EurekaEditor

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\EurekaEditor" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin


;----------------------------------------------------------------

; Pages

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES


;----------------------------------------------------------------

; The stuff to install

Section

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File Eureka.exe
  File *.txt
  File bindings.cfg
  File about_logo.png
  File core_defs.up

  File /r games
  File /r common
  File /r ports
  File /nonfatal /r mods
  File /nonfatal /r ups

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\EurekaEditor" "Install_Dir" "$INSTDIR"

  ; Create the Start Menu link
  createShortCut "$SMPROGRAMS\Eureka Doom Editor.lnk" "$INSTDIR\Eureka.exe"
  
SectionEnd
