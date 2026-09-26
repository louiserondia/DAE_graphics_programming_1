@echo off
set __VSCMD_ARG_NO_LOGO=1
set VSCMD_SKIP_SENDTELEMETRY=1
echo conanvcvars.bat: Activating environment Visual Studio 18 - amd64 - winsdk_version=None - vcvars_ver=14.5
set "VSCMD_START_DIR=%CD%" && call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"  amd64 -vcvars_ver=14.5
