@echo off
setlocal

set "TEMP_FILE=%TEMP%\~linecount.tmp"
rem Tao bien dem de can chinh
set "PAD_FN=                                        "
set "PAD_NUM=          "

:menu
cls
echo ===================================
echo     CHUONG TRINH DEM DONG CODE
echo ===================================
echo.
echo   Chon mot tuy chon:
echo.
echo   1. Dem TOAN BO dong (bao gom dong trong, comment)
echo   2. Chi dem dong CODE (Loai bo dong trong, comment)
echo.
echo   Nhan phim 1 hoac 2 (nhan Q de thoat)...
echo.

choice /C 12Q /N /M "> "
if errorlevel 3 goto :eof
if errorlevel 2 goto :count_code
if errorlevel 1 goto :count_all
goto :menu

:count_all
cls
echo Dang tinh...
setlocal enabledelayedexpansion
set total_lines=0
if exist "%TEMP_FILE%" del "%TEMP_FILE%"

for /R %%F in (*.h *.cpp) do (
    for /f %%C in ('type "%%F" ^| find /c /v ""') do (
        set "count=%%C"
        
        rem Them vao 10 so 0 de dam bao sap xep so dung
        set "padded_count=0000000000!count!"
        set "padded_count=!padded_count:~-10!"
        
        rem Chi luu ten file (%%~nxF) thay vi duong dan day du (%%F)
        echo !padded_count! "%%~nxF" >> "%TEMP_FILE%"
    )
)

echo.
echo KET QUA (TOAN BO DONG):
echo =======================================================
for /f "tokens=1,*" %%A in ('sort /R "%TEMP_FILE%"') do (
    set "filename=%%B"
    set "filename=!filename:"=!"
    set "line_count_str=%%A"
    
    rem Bo so 0 o dau
    for /l %%N in (1,1,9) do (
        if "!line_count_str:~0,1!"=="0" set "line_count_str=!line_count_str:~1!"
    )
    if "!line_count_str!"=="" set "line_count_str=0"
    
    rem Can chinh ten file (trai)
    set "display_name=!filename!!PAD_FN!"
    set "display_name=!display_name:~0,40!"
    
    rem Can chinh so dong (phai)
    set "display_count=!PAD_NUM!!line_count_str!"
    set "display_count=!display_count:~-10!"
    
    echo !display_name! !display_count! dong
    set /a total_lines+=!line_count_str!
)

echo =======================================================
set "display_total=!PAD_NUM!!total_lines!"
set "display_total=!display_total:~-10!"
echo TONG CONG:                                !display_total! dong
echo =======================================================

if exist "%TEMP_FILE%" del "%TEMP_FILE%"
endlocal
echo.
pause
goto :menu

:count_code
cls
echo Dang tinh (loai bo dong trong, comments)...
setlocal enabledelayedexpansion
set total_lines=0
if exist "%TEMP_FILE%" del "%TEMP_FILE%"

for /R %%F in (*.h *.cpp) do (
    for /f %%C in ('type "%%F" ^| findstr /v /r /i /c:"^$" /c:"^[ \t]*//" /c:"^[ \t]*/\*" /c:"^[ \t]*\*" ^| find /c /v ""') do (
        set "count=%%C"

        rem Them vao 10 so 0 de dam bao sap xep so dung
        set "padded_count=0000000000!count!"
        set "padded_count=!padded_count:~-10!"
        
        rem Chi luu ten file (%%~nxF)
        echo !padded_count! "%%~nxF" >> "%TEMP_FILE%"
    )
)

echo.
echo KET QUA (CHI DONG CODE):
echo =======================================================
for /f "tokens=1,*" %%A in ('sort /R "%TEMP_FILE%"') do (
    set "filename=%%B"
    set "filename=!filename:"=!"
    set "line_count_str=%%A"
    
    rem Bo so 0 o dau
    for /l %%N in (1,1,9) do (
        if "!line_count_str:~0,1!"=="0" set "line_count_str=!line_count_str:~1!"
    )
    if "!line_count_str!"=="" set "line_count_str=0"

    rem Can chinh ten file (trai)
    set "display_name=!filename!!PAD_FN!"
    set "display_name=!display_name:~0,40!"
    
    rem Can chinh so dong (phai)
    set "display_count=!PAD_NUM!!line_count_str!"
    set "display_count=!display_count:~-10!"
    
    echo !display_name! !display_count! dong
    set /a total_lines+=!line_count_str!
)

echo =======================================================
set "display_total=!PAD_NUM!!total_lines!"
set "display_total=!display_total:~-10!"
echo TONG CONG (CODE):                         !display_total! dong
echo =======================================================

if exist "%TEMP_FILE%" del "%TEMP_FILE%"
endlocal
echo.
pause
goto :menu

:eof
endlocal