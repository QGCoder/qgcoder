!include "MUI2.nsh"
!include "FileFunc.nsh"

!define APPNAME "QGCoder"

; Version is passed in via command line: makensis -DVERSION=...
!ifndef VERSION
  !define VERSION "0.0.0"
!endif

Name "${APPNAME} ${VERSION}"
OutFile "qgcoder-${VERSION}-windows-installer.exe"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" "InstallDir"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"
    ; The interpreter and the 3D view are pure Qt Widgets/OpenGL, so the whole
    ; runtime has to come with the installer: this machine may not have Qt.
    SetOutPath "$INSTDIR\bin"
    File "..\dist\bin\qgcoder.exe"
    File "..\dist\bin\qt.conf"
    ; Every DLL windeployqt laid out next to the exe: the Qt6 runtime
    ; (Core/Gui/Network/OpenGL/OpenGLWidgets/Widgets) plus the MinGW toolchain
    ; runtime (libgcc_s_seh, libstdc++, libwinpthread) that windeployqt does
    ; not know about. They have to stay next to the exe for Qt to find them.
    File "..\dist\bin\*.dll"

    ; Qt's platform and helper plugins. bin\qt.conf points Qt at this exact
    ; tree relative to the install prefix, so it must mirror the layout
    ; windeployqt used - platforms\qwindows.dll is the one without which the
    ; window will not open at all.
    SetOutPath "$INSTDIR\share\qt6\plugins"
    File /r "..\dist\share\qt6\plugins\*.*"

    WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"

    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\bin\${APPNAME}.exe"
    CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    !define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
    WriteRegStr HKLM "${UNINST_KEY}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKLM "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${UNINST_KEY}" "Publisher" "${APPNAME}"
    WriteRegStr HKLM "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "${UNINST_KEY}" "URLInfoAbout" "https://github.com/QGCoder/qgcoder"
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\${APPNAME}"
    DeleteRegKey HKLM "${UNINST_KEY}"
    DeleteRegKey HKLM "Software\${APPNAME}"
SectionEnd