#define UNICODE
#define _UNICODE
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <Lmcons.h>
#include <thread>
#include <iomanip>
#include <conio.h>
#pragma comment(lib, "Advapi32.lib")

void DebugMessage(const wchar_t* message) {
    std::wcout << L"[DEBUG] " << message << L"\n";
    std::wcout.flush();
}
void PrintLastError(const wchar_t* functionName) {
    DWORD error = GetLastError();
    if (error != 0) {
        wchar_t errorMsg[256];
        FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            0,
            errorMsg,
            256,
            NULL
        );
        std::wcerr << L"Error in " << functionName << L": " << errorMsg << L" (code: " << error << L")\n";
    }
}

bool ListLogicalDrives() {
    try {
        std::wcout << L"\n===== LOGICAL DRIVES =====\n";
        DWORD drives = GetLogicalDrives();
        if (drives == 0) {
            PrintLastError(L"GetLogicalDrives");
            return false;
        }

        bool found = false;
        for (wchar_t drive = L'A'; drive <= L'Z'; ++drive) {
            if (drives & 1) {
                std::wcout << L"Drive " << drive << L":\\ is present in the system.\n";
                found = true;
            }
            drives >>= 1;
        }

        if (!found) {
            std::wcout << L"No drives found.\n";
        }
        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in ListLogicalDrives: " << e.what() << L"\n";
        return false;
    }
}

bool GetDriveTypeInfo() {
    try {
        std::wcout << L"\n===== DRIVE TYPES =====\n";
        std::wcout.flush();
        DebugMessage(L"Starting GetDriveTypeInfo function");

        for (wchar_t drive = L'A'; drive <= L'Z'; ++drive) {
            try {
                std::wstring drivePath = std::wstring(1, drive) + L":\\";
                DebugMessage((L"Checking drive: " + drivePath).c_str());

                UINT driveType = GetDriveTypeW(drivePath.c_str());
                DebugMessage((L"Drive " + drivePath + L" type code: " + std::to_wstring(driveType)).c_str());

                if (driveType != DRIVE_NO_ROOT_DIR && driveType != DRIVE_UNKNOWN) {
                    std::wcout << L"Drive " << drivePath << L" - ";
                    std::wcout.flush();

                    switch (driveType) {
                    case DRIVE_REMOVABLE:
                        std::wcout << L"Removable drive (e.g., USB)";
                        break;
                    case DRIVE_FIXED:
                        std::wcout << L"Fixed drive (HDD or SSD)";
                        break;
                    case DRIVE_REMOTE:
                        std::wcout << L"Network drive";
                        break;
                    case DRIVE_CDROM:
                        std::wcout << L"CD/DVD drive";
                        break;
                    case DRIVE_RAMDISK:
                        std::wcout << L"RAM disk";
                        break;
                    default:
                        std::wcout << L"Unknown type";
                        break;
                    }

                    std::wcout << L"\n";
                    std::wcout.flush();
                }
            }
            catch (const std::exception& e) {
                std::wcerr << L"Exception processing drive " << drive << L": " << e.what() << L"\n";
                std::wcerr.flush();
            }
            catch (...) {
                std::wcerr << L"Unknown exception processing drive " << drive << L"\n";
                std::wcerr.flush();
            }
        }

        DebugMessage(L"Completed drive type check");
        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetDriveTypeInfo: " << e.what() << L"\n";
        std::wcerr.flush();
        return false;
    }
    catch (...) {
        std::wcerr << L"Unknown exception in GetDriveTypeInfo\n";
        std::wcerr.flush();
        return false;
    }
}

bool GetDiskAndFileSystemInfo() {
    try {
        std::wcout << L"\n===== FILE SYSTEMS =====\n";
        bool found = false;

        for (wchar_t drive = L'A'; drive <= L'Z'; ++drive) {
            std::wstring drivePath = std::wstring(1, drive) + L":\\";
            UINT type = GetDriveTypeW(drivePath.c_str());

            if (type != DRIVE_NO_ROOT_DIR && type != DRIVE_UNKNOWN) {
                wchar_t volumeName[MAX_PATH] = L"";
                wchar_t fileSystemName[MAX_PATH] = L"";
                DWORD serialNumber = 0;
                DWORD maxComponentLength = 0;
                DWORD fileSystemFlags = 0;

                BOOL success = GetVolumeInformationW(
                    drivePath.c_str(),
                    volumeName,
                    MAX_PATH,
                    &serialNumber,
                    &maxComponentLength,
                    &fileSystemFlags,
                    fileSystemName,
                    MAX_PATH
                );

                if (success) {
                    found = true;
                    std::wcout << L"Drive " << drivePath << L"\n";
                    std::wcout << L"  Volume Name: " << (wcslen(volumeName) > 0 ? volumeName : L"<No Label>") << L"\n";
                    std::wcout << L"  File System: " << fileSystemName << L"\n";
                    std::wcout << L"  Serial Number: " << std::hex << std::uppercase << serialNumber << std::dec << L"\n";
                }
            }
        }

        if (!found) {
            std::wcout << L"No file system information available.\n";
        }
        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetDiskAndFileSystemInfo: " << e.what() << L"\n";
        return false;
    }
}

bool GetDiskFreeSpaceInfo() {
    try {
        std::wcout << L"\n===== DISK SPACE =====\n";
        bool found = false;

        for (wchar_t drive = L'A'; drive <= L'Z'; ++drive) {
            std::wstring drivePath = std::wstring(1, drive) + L":\\";
            UINT type = GetDriveTypeW(drivePath.c_str());

            if (type != DRIVE_NO_ROOT_DIR && type != DRIVE_UNKNOWN) {
                ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

                if (GetDiskFreeSpaceExW(drivePath.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
                    found = true;
                    double totalGB = totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
                    double freeGB = totalFreeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
                    double usedGB = totalGB - freeGB;
                    double usagePercentage = (totalGB > 0) ? (usedGB / totalGB * 100.0) : 0;

                    std::wcout << L"Drive " << drivePath << L"\n";
                    std::wcout << std::fixed << std::setprecision(2);
                    std::wcout << L"  Total: " << totalGB << L" GB\n";
                    std::wcout << L"  Free:  " << freeGB << L" GB\n";
                    std::wcout << L"  Used:  " << usedGB << L" GB (" << usagePercentage << L"%)\n";
                }
            }
        }

        if (!found) {
            std::wcout << L"No disk space information available.\n";
        }
        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetDiskFreeSpaceInfo: " << e.what() << L"\n";
        return false;
    }
}

bool GetSystemMemoryInfo() {
    try {
        std::wcout << L"\n===== SYSTEM MEMORY =====\n";

        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);

        if (GlobalMemoryStatusEx(&statex)) {
            double totalPhysMB = statex.ullTotalPhys / (1024.0 * 1024.0);
            double availPhysMB = statex.ullAvailPhys / (1024.0 * 1024.0);
            double totalVirtualMB = statex.ullTotalVirtual / (1024.0 * 1024.0);
            double availVirtualMB = statex.ullAvailVirtual / (1024.0 * 1024.0);

            std::wcout << std::fixed << std::setprecision(2);
            std::wcout << L"Memory usage: " << statex.dwMemoryLoad << L"%\n";
            std::wcout << L"Total physical memory: " << totalPhysMB << L" MB ("
                << (totalPhysMB / 1024.0) << L" GB)\n";
            std::wcout << L"Available physical memory: " << availPhysMB << L" MB ("
                << (availPhysMB / 1024.0) << L" GB)\n";
            std::wcout << L"Total virtual memory: " << totalVirtualMB << L" MB ("
                << (totalVirtualMB / 1024.0) << L" GB)\n";
            std::wcout << L"Available virtual memory: " << availVirtualMB << L" MB ("
                << (availVirtualMB / 1024.0) << L" GB)\n";

            return true;
        }
        else {
            PrintLastError(L"GlobalMemoryStatusEx");
            return false;
        }
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetSystemMemoryInfo: " << e.what() << L"\n";
        return false;
    }
}

bool GetComputerNameInfo() {
    try {
        std::wcout << L"\n===== COMPUTER INFO =====\n";

        wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName) / sizeof(computerName[0]);

        if (GetComputerNameW(computerName, &size)) {
            std::wcout << L"Computer name: " << computerName << L"\n";
            return true;
        }
        else {
            PrintLastError(L"GetComputerNameW");
            return false;
        }
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetComputerNameInfo: " << e.what() << L"\n";
        return false;
    }
}

bool GetUserNameInfo() {
    try {
        wchar_t userName[UNLEN + 1];
        DWORD size = UNLEN + 1;

        if (GetUserNameW(userName, &size)) {
            std::wcout << L"Current user: " << userName << L"\n";
            return true;
        }
        else {
            PrintLastError(L"GetUserNameW");
            return false;
        }
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetUserNameInfo: " << e.what() << L"\n";
        return false;
    }
}

bool GetDirectoryInfo() {
    try {
        std::wcout << L"\n===== DIRECTORIES =====\n";

        wchar_t systemDir[MAX_PATH];
        wchar_t tempDir[MAX_PATH];
        wchar_t currentDir[MAX_PATH];
        wchar_t userProfileDir[MAX_PATH];

        bool success = true;

        if (!GetSystemDirectoryW(systemDir, MAX_PATH)) {
            PrintLastError(L"GetSystemDirectoryW");
            success = false;
        }
        else {
            std::wcout << L"System Directory: " << systemDir << L"\n";
        }

        if (!GetTempPathW(MAX_PATH, tempDir)) {
            PrintLastError(L"GetTempPathW");
            success = false;
        }
        else {
            std::wcout << L"Temporary Directory: " << tempDir << L"\n";
        }

        if (!GetCurrentDirectoryW(MAX_PATH, currentDir)) {
            PrintLastError(L"GetCurrentDirectoryW");
            success = false;
        }
        else {
            std::wcout << L"Current Working Directory: " << currentDir << L"\n";
        }

        if (ExpandEnvironmentStringsW(L"%USERPROFILE%", userProfileDir, MAX_PATH) == 0) {
            PrintLastError(L"ExpandEnvironmentStringsW");
            success = false;
        }
        else {
            std::wcout << L"User Profile Directory: " << userProfileDir << L"\n";
        }

        return success;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in GetDirectoryInfo: " << e.what() << L"\n";
        return false;
    }
}

bool MonitorDirectory(const std::wstring& directory) {
    try {
        std::wcout << L"\n===== DIRECTORY MONITORING =====\n";
        std::wcout << L"Starting to monitor: " << directory << L"\n";
        std::wcout << L"Changes will be logged to changes.log\n";
        std::wcout << L"Press 'C' to terminate monitoring.\n\n";

        HANDLE hDir = CreateFileW(
            directory.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (hDir == INVALID_HANDLE_VALUE) {
            PrintLastError(L"CreateFileW");
            std::wcerr << L"Failed to open directory for monitoring: " << directory << L"\n";
            std::wcerr << L"Try running the program with administrator privileges.\n";
            return false;
        }

        SYSTEMTIME st;
        GetLocalTime(&st);

        std::wstring logFileName = L"changes_";
        logFileName += std::to_wstring(st.wYear) + L"_" +
            std::to_wstring(st.wMonth) + L"_" +
            std::to_wstring(st.wDay) + L"_" +
            std::to_wstring(st.wHour) + L"_" +
            std::to_wstring(st.wMinute) + L".log";

        std::wofstream log(logFileName, std::ios::out);
        if (!log.is_open()) {
            std::wcerr << L"Failed to open log file: " << logFileName << L"\n";
            CloseHandle(hDir);
            return false;
        }

        log << L"=== Directory Monitoring Started ===\n";
        log << L"Monitoring: " << directory << L"\n";
        log << L"Started at: " << st.wYear << L"-" << st.wMonth << L"-" << st.wDay
            << L" " << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"\n\n";
        log.flush();

        char buffer[4096];
        DWORD bytesReturned;

        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (overlapped.hEvent == NULL) {
            PrintLastError(L"CreateEvent");
            CloseHandle(hDir);
            log.close();
            return false;
        }

        BOOL readStarted = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            &overlapped,
            NULL
        );

        if (!readStarted) {
            PrintLastError(L"ReadDirectoryChangesW");
            CloseHandle(overlapped.hEvent);
            CloseHandle(hDir);
            log.close();
            return false;
        }

        bool monitoring = true;

        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD originalConsoleMode;
        GetConsoleMode(hStdin, &originalConsoleMode);
        SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

        while (monitoring) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 'c' || key == 'C') {
                    std::wcout << L"\nTerminating directory monitoring...\n";
                    monitoring = false;
                    continue;
                }
            }

            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, 100);

            if (waitResult == WAIT_OBJECT_0) {
                if (GetOverlappedResult(hDir, &overlapped, &bytesReturned, FALSE) && bytesReturned > 0) {
                    FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)buffer;

                    do {
                        std::wstring fileName(info->FileName, info->FileNameLength / sizeof(WCHAR));
                        std::wstring action;

                        switch (info->Action) {
                        case FILE_ACTION_ADDED:
                            action = L"Added";
                            break;
                        case FILE_ACTION_REMOVED:
                            action = L"Removed";
                            break;
                        case FILE_ACTION_MODIFIED:
                            action = L"Modified";
                            break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                            action = L"Renamed (old name)";
                            break;
                        case FILE_ACTION_RENAMED_NEW_NAME:
                            action = L"Renamed (new name)";
                            break;
                        default:
                            action = L"Unknown action";
                            break;
                        }

                        GetLocalTime(&st);
                        log << st.wHour << L":" << st.wMinute << L":" << st.wSecond
                            << L" - " << action << L": " << fileName << L"\n";
                        log.flush();

                        std::wcout << st.wHour << L":" << st.wMinute << L":" << st.wSecond
                            << L" - " << action << L": " << fileName << std::endl;

                        if (info->NextEntryOffset == 0)
                            break;

                        info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);

                    } while (true);

                    ResetEvent(overlapped.hEvent);
                    ZeroMemory(buffer, sizeof(buffer));

                    if (!ReadDirectoryChangesW(
                        hDir,
                        buffer,
                        sizeof(buffer),
                        TRUE,
                        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                        FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                        FILE_NOTIFY_CHANGE_LAST_WRITE,
                        &bytesReturned,
                        &overlapped,
                        NULL)) {
                        PrintLastError(L"ReadDirectoryChangesW");
                        monitoring = false;
                    }
                }
            }
            else if (waitResult == WAIT_FAILED) {
                PrintLastError(L"WaitForSingleObject");
                monitoring = false;
            }

            Sleep(10);
        }

        SetConsoleMode(hStdin, originalConsoleMode);

        CancelIo(hDir);
        CloseHandle(overlapped.hEvent);
        CloseHandle(hDir);

        GetLocalTime(&st);
        log << L"\n=== Directory Monitoring Ended ===\n";
        log << L"Ended at: " << st.wYear << L"-" << st.wMonth << L"-" << st.wDay
            << L" " << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"\n";
        log.close();

        return true;
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception in MonitorDirectory: " << e.what() << L"\n";
        return false;
    }
}

bool EnsureDirectoryExists(const std::wstring& directoryPath) {
    DWORD attrs = GetFileAttributesW(directoryPath.c_str());

    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }

    if (CreateDirectoryW(directoryPath.c_str(), NULL) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }

    PrintLastError(L"CreateDirectoryW");
    return false;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    try {
        std::wcout << L"===== SYSTEM INFORMATION TOOL =====\n";
        std::wcout << L"Running all checks...\n";
        std::wcout.flush();

        DebugMessage(L"Starting ListLogicalDrives");
        ListLogicalDrives();

        DebugMessage(L"Starting GetDriveTypeInfo");
        if (!GetDriveTypeInfo()) {
            std::wcerr << L"Error in GetDriveTypeInfo, continuing with next function\n";
        }

        DebugMessage(L"Starting GetDiskAndFileSystemInfo");
        if (!GetDiskAndFileSystemInfo()) {
            std::wcerr << L"Error in GetDiskAndFileSystemInfo, continuing with next function\n";
        }

        DebugMessage(L"Starting GetDiskFreeSpaceInfo");
        if (!GetDiskFreeSpaceInfo()) {
            std::wcerr << L"Error in GetDiskFreeSpaceInfo, continuing with next function\n";
        }

        DebugMessage(L"Starting GetSystemMemoryInfo");
        if (!GetSystemMemoryInfo()) {
            std::wcerr << L"Error in GetSystemMemoryInfo, continuing with next function\n";
        }

        DebugMessage(L"Starting GetComputerNameInfo");
        if (!GetComputerNameInfo()) {
            std::wcerr << L"Error in GetComputerNameInfo, continuing with next function\n";
        }

        DebugMessage(L"Starting GetUserNameInfo");
        if (!GetUserNameInfo()) {
            std::wcerr << L"Error in GetUserNameInfo, continuing with next function\n";
        }

        DebugMessage(L"Starting GetDirectoryInfo");
        if (!GetDirectoryInfo()) {
            std::wcerr << L"Error in GetDirectoryInfo, continuing with next function\n";
        }

        std::wstring targetDir = L"C:\\Users\\Admin\\Desktop\\VisualStudio\\C++\\Lab03\\target_folder";

        DebugMessage(L"Checking if target directory exists");
        if (!EnsureDirectoryExists(targetDir)) {
            std::wcerr << L"Failed to ensure target directory exists: " << targetDir << L"\n";
            std::wcerr << L"Please check the path and try again.\n";
        }
        else {
            DebugMessage(L"Starting MonitorDirectory");
            if (!MonitorDirectory(targetDir)) {
                std::wcerr << L"Error in MonitorDirectory\n";
            }
        }

        std::wcout << L"\n===== PROGRAM COMPLETE =====\n";
        std::wcout.flush();
    }
    catch (const std::exception& e) {
        std::wcerr << L"Unhandled exception: " << e.what() << L"\n";
        std::wcerr.flush();
    }
    catch (...) {
        std::wcerr << L"Unknown unhandled exception occurred\n";
        std::wcerr.flush();
    }

    std::wcout << L"\nPress any key to exit...\n";
    std::wcin.get();
    return 0;
}