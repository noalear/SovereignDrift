; Windows installer for the Stellar Frontier remake.  Built on Linux by
; tools/publish_update.sh when it cuts a new base, which passes VERSION,
; BUILD, EXE and OUT:
;   makensis -DVERSION=0.1.0 -DBUILD=e94d84c -DEXE=build/windows/StellarFrontier.exe \
;            -DOUT=build/StellarFrontierRemake-Setup.exe tools/installer/StellarFrontier.nsi
; Installs for the current user only (no admin prompt), with Start menu
; and desktop shortcuts and an entry in Windows' installed apps list.
; Settings, careers and saves live in %APPDATA%\Godot and are never touched.

Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user

!include "MUI2.nsh"

!define APP "Stellar Frontier Remake"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\StellarFrontierRemake"

Name "${APP}"
OutFile "${OUT}"
InstallDir "$LOCALAPPDATA\Programs\${APP}"
InstallDirRegKey HKCU "${UNINST_KEY}" "InstallLocation"
BrandingText "${APP} ${VERSION} (${BUILD})"
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APP}"
VIAddVersionKey "FileDescription" "${APP} installer"
VIAddVersionKey "ProductVersion" "${VERSION} (${BUILD})"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "Non-commercial fan remake"

!define MUI_ICON "icon.ico"
!define MUI_UNICON "icon.ico"
!define MUI_ABORTWARNING
ShowInstDetails show
ShowUninstDetails show
!define MUI_FINISHPAGE_RUN "$INSTDIR\StellarFrontier.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Play ${APP} now"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
	SetOutPath "$INSTDIR"
	File "/oname=StellarFrontier.exe" "${EXE}"
	File "icon.ico"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	CreateDirectory "$SMPROGRAMS\${APP}"
	CreateShortcut "$SMPROGRAMS\${APP}\${APP}.lnk" "$INSTDIR\StellarFrontier.exe" "" "$INSTDIR\icon.ico"
	CreateShortcut "$SMPROGRAMS\${APP}\Uninstall ${APP}.lnk" "$INSTDIR\Uninstall.exe"
	CreateShortcut "$DESKTOP\${APP}.lnk" "$INSTDIR\StellarFrontier.exe" "" "$INSTDIR\icon.ico"
	WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "${APP}"
	WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${VERSION} (${BUILD})"
	WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\icon.ico"
	WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
	WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
	WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
	Delete "$INSTDIR\StellarFrontier.exe"
	Delete "$INSTDIR\icon.ico"
	Delete "$INSTDIR\Uninstall.exe"
	RMDir "$INSTDIR"
	Delete "$SMPROGRAMS\${APP}\${APP}.lnk"
	Delete "$SMPROGRAMS\${APP}\Uninstall ${APP}.lnk"
	RMDir "$SMPROGRAMS\${APP}"
	Delete "$DESKTOP\${APP}.lnk"
	DeleteRegKey HKCU "${UNINST_KEY}"
SectionEnd
