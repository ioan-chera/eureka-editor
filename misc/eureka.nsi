;----------------------------------------------------------------
;
;  NSIS script for Eureka Level Editor
;
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
RequestExecutionLevel user

;--------------------------------

; Pages

Page directory
Page instfiles

;--------------------------------

; The stuff to install

Section "" ;No components page, name is not important

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File Eureka.exe
  File GPL.txt
  File AUTHORS.txt
  File README.txt
  File TODO.txt
  File /r /x .svn games
  File /r /x .svn common
  File /r /x .svn ports
  File /r /x .svn mods

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\EurekaEditor" "Install_Dir" "$INSTDIR"
  
SectionEnd
