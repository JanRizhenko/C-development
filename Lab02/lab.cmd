@echo off
chcp 65001 > NUL
setlocal enabledelayedexpansion

REM === Input Parameters ===
set "email_file_path=email.txt"
set "ipon_file_path=ipon.txt"
set "ntp_server=pool.ntp.org"
set "log_file_path=%~1"
set "base_path=%~2"
set "process_to_kill=%~3"
set "archive_path=%~4"
set "computer_ip=%~5"
set "max_log_file_size=%~6"

REM === Check for required log path ===
if "%log_file_path%"=="" (
    echo Usage: <log_file_path> [base_path] [process_to_kill] [archive_path] [computer_ip] [max_log_file_size]
    exit /b
)

REM === Time Sync ===
w32tm /config /manualpeerlist:"%ntp_server%" /syncfromflags:manual /update > NUL
w32tm /resync > NUL
call :add_log "Time synced to %ntp_server%"

REM === Tasklist + Kill process ===
call :add_log "Process list:"
tasklist >> "%log_file_path%"

if defined process_to_kill (
    taskkill /im "%process_to_kill%" > NUL 2>&1
    if !ERRORLEVEL! == 0 (
        call :add_log "Process %process_to_kill% killed successfully."
    ) else if !ERRORLEVEL! == 128 (
        call :add_log "Process %process_to_kill% not found."
    )
)

REM === Cleanup Temp Files and Archive ===
if defined base_path (
    if exist "%base_path%" (
        for /f %%A in ('dir /b /a-d "%base_path%" ^| find /c /v ""') do set "file_count_before=%%A"
        del /q "%base_path%\*.tmp" > NUL 2>&1
        del /q "%base_path%\temp*" > NUL 2>&1
        for /f %%A in ('dir /b /a-d "%base_path%" ^| find /c /v ""') do set "file_count_after=%%A"
        set /a files_deleted=!file_count_before!-!file_count_after!
        call :add_log "Temporary files deleted: !files_deleted!."

        REM === Create Archive ===
        if exist "%archive_path%" (
            if not "%archive_path:~-1%"=="\" set "archive_path=%archive_path%\"
            for /f %%a in ('powershell -nologo -noprofile -command "(Get-Date).ToString(\"yyyy-MM-dd_HH-mm-ss\")"') do set "datetime=%%a"
            set "valid_archive_name=archive_!datetime!.zip"
            7z a "!valid_archive_name!" "%base_path%\*.*" > NUL
            move "!valid_archive_name!" "%archive_path%" > NUL
            call :add_log "Created archive: %archive_path%!valid_archive_name!"
        )
    )
)

REM === Check for Yesterday's Archive ===
for /f %%a in ('powershell -nologo -noprofile -command "[datetime]::Today.AddDays(-1).ToString(\"yyyy-MM-dd\")"') do set "YESTERDAY=%%a"
set "found=false"
for %%F in ("%archive_path%\%YESTERDAY%*") do (
    set "found=true"
)

if "!found!"=="false" (
    call :send_email "No archive found for %YESTERDAY%!"
    call :add_log "No archive backup for %YESTERDAY%. Email sent."
)

REM === Delete Old Archives (older than 30 days) ===
for /f %%A in ('dir /b /a-d "%archive_path%" ^| find /c /v ""') do set "file_count_before=%%A"
forfiles /p "%archive_path%" /m *.zip /d -30 /c "cmd /c del /q @path" > NUL 2>&1
for /f %%A in ('dir /b /a-d "%archive_path%" ^| find /c /v ""') do set "file_count_after=%%A"
set /a files_deleted=!file_count_before!-!file_count_after!
call :add_log "Old archives deleted: !files_deleted!."

REM === Network Check ===
ping -n 1 google.com > NUL 2>&1
if errorlevel 1 (
    call :add_log "No internet connection!"
) else (
    call :add_log "Internet connection: OK."
)

REM === Remote Shutdown ===
ping -n 1 %computer_ip% > NUL
if !ERRORLEVEL! == 0 (
    call :add_log "%computer_ip% is online. Attempting shutdown..."
    shutdown /s /f /m \\%computer_ip% /t 0
    if !ERRORLEVEL! == 0 (
        call :add_log "Shutdown command sent to %computer_ip%"
    ) else (
        call :add_log "Failed to shutdown %computer_ip%"
    )
) else (
    call :add_log "%computer_ip% is not reachable."
)

REM === ARP Table ===
call :add_log "IP Address Table:"
arp -a >> "%log_file_path%"

REM === Check IPs from ipon.txt ===
if exist "%ipon_file_path%" (
    for /f "usebackq tokens=* delims=" %%i in ("%ipon_file_path%") do (
        ping -n 1 %%i > NUL 2>&1
        if !ERRORLEVEL! == 1 (
            call :add_log "IP %%i from ipon.txt not reachable!"
            call :send_email "Warning: IP %%i from ipon.txt is not reachable!"
            goto :after_ipon_loop
        )
    )
)
:after_ipon_loop

REM === Log File Size Check ===
if defined max_log_file_size (
    for %%f in ("%log_file_path%") do (
        set "filesize=%%~zf"
    )
    if !filesize! GTR !max_log_file_size! (
        call :send_email "Log file too large: !filesize! bytes."
        call :add_log "WARNING: Log file size exceeds limit: !filesize! bytes."
    )
)

REM === Disk Space Report ===
for /f %%a in ('powershell -command "Get-PSDrive -PSProvider FileSystem | ForEach-Object { $_.Name + ':' }"') do (
    call :add_log "Drive %%a:"
    fsutil volume diskfree %%a >> "%log_file_path%"
    echo. >> "%log_file_path%"
)

REM === System Info Snapshot ===
for /f %%a in ('powershell -nologo -noprofile -command "(Get-Date).ToString(\"yyyy-MM-dd_HH-mm-ss\")"') do set "sysinfo=%%a"
systeminfo > "sysinfo_!sysinfo!.txt" 2>&1

exit /b

REM === LOG FUNCTION ===
:add_log
if not exist "%log_file_path%" (
    echo [%date% %time%] Log created >> "%log_file_path%"
)
if not "%~1"=="" (
    echo [%date% %time%] %~1 >> "%log_file_path%"
)
goto :EOF

REM === SIMULATED EMAIL FUNCTION ===
:send_email
if not exist "%email_file_path%" (
    break > "%email_file_path%"
)
echo %~1 >> "%email_file_path%"
echo Sent: %date% %time% >> "%email_file_path%"
echo. >> "%email_file_path%"
goto :EOF
