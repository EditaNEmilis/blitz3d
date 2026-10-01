@echo off
REM Configure and build Blitz3D with the modern MSVC toolchain.
REM
REM   tools\build.cmd                 release build into build\ and _release\
REM   tools\build.cmd --debug         debug build
REM   tools\build.cmd --clean         wipe the build tree first
REM
REM The 32-bit environment is not optional: Blitz3D is a 32-bit app throughout.
setlocal EnableExtensions

REM Captured before anything else: SHIFT rewrites %0 as well as %1..%9, so it
REM would silently eat the last component of %~dp0.
set "REPO=%~dp0.."
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCVARS%" (
  echo Could not find vcvarsall.bat at "%VCVARS%".
  echo Edit this script if Visual Studio lives somewhere else.
  exit /b 1
)

set CLEAN=
set CONFIG=Release
for %%A in (%*) do call :arg "%%~A"
if errorlevel 1 exit /b 1
goto parsed

:arg
if /i "%~1"=="--clean"   set "CLEAN=1"   & goto :eof
if /i "%~1"=="--debug"   set "CONFIG=Debug"   & goto :eof
if /i "%~1"=="--release" set "CONFIG=Release" & goto :eof
echo Unknown argument "%~1"
exit /b 1

:parsed
call "%VCVARS%" x86 >nul || exit /b 1
cd /d "%REPO%" || exit /b 1

if defined CLEAN if exist build rmdir /s /q build

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG% || exit /b 1
cmake --build build --parallel || exit /b 1

echo.
echo Built into _release:
dir /b _release\*.exe 2>nul
dir /b _release\bin 2>nul
endlocal
