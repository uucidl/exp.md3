REM User Configuration
REM ==================
@echo off
set HereDir=%~d0%~p0.
if not defined OutputDir set OutputDir=%HereDir%\output
if not defined ObjDir set ObjDir=%OutputDir%\obj
if not defined CLExe set CLExe=cl.exe
if not defined LinkExe set LinkExe=link.exe
if not defined IonExe set IonExe="%OutputDir%\ion.exe"
setlocal

if not exist "%OutputDir%" mkdir "%OutputDir%"
if not exist "%ObjDir%" mkdir "%ObjDir%"
if %errorlevel% neq 0 exit /b 1

set CLCommonFlags="-I%HereDir%" -nologo -Z7 -W3 -wd4244 -wd4267 -wd4204 -wd4201 -D_CRT_SECURE_NO_WARNINGS -Fo:"%ObjDir%"\ 
set IonCommonFlags=
set IONHOME=%HereDir%\deps\bitwise\ion\

call :build_program_module docking_demo_program
if %errorlevel% neq 0 exit /b 1

xcopy /y /s "%HereDir%\assets" "%OutputDir%"\assets\

exit /b 0

:build_program_module
set ProgramModuleName=%1
set ProgramModuleCFile="%ObjDir%\out_%ProgramModuleName%.c"
pushd %HereDir%
"%IonExe%" %IonCommonFlags% -o "%ProgramModuleCFile%" %ProgramModuleName%
if %errorlevel% neq 0 exit /b 1
popd

set O=%OutputDir%\%ProgramModuleName%.exe
"%CLExe%" -Fe:"%O%" -Fo:"%ObjDir%\\" -I"%ObjDir%" "%ProgramModuleCFile%"
if %errorlevel% neq 0 exit /b 1
echo PROGRAM    %O%
exit /b 0



