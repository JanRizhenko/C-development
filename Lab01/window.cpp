#include <Windows.h>
#include <cstring>
#include <fileapi.h>
#include <handleapi.h>
#include <iostream>
#include <memoryapi.h>
#include <minwindef.h>
#include <synchapi.h>
#include <winnt.h>
#include <winuser.h>

std::string file_name = "data.dat";
char character = '*';
int gap = 20;
int timer_id = 1;
int timer_interval = 500;
HANDLE h_mutex;

std::string get_characters(char character, int count)
{
    std::string result = "";

    for (int i = 0; i < count; i++)
    {
        result = result + character;
    }

    return result;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            HANDLE h_file = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (h_file == INVALID_HANDLE_VALUE)
            {
                std::cerr << "Error reading the file!" << std::endl;
                return 1;
            }

            WaitForSingleObject(h_mutex, INFINITE);
            int file_size = GetFileSize(h_file, NULL);

            HANDLE h_mapping = CreateFileMapping(h_file, NULL, PAGE_READWRITE, 0, 0, NULL);

            if (h_mapping == NULL)
            {
                ReleaseMutex(h_file);
                std::cerr << "Error creating a file mapping object!" << std::endl;
                return 1;
            }

            int* numbers = (int*)MapViewOfFile(h_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

            if (!numbers)
            {
                std::cerr << "Error mapping the file!" << std::endl;
                CloseHandle(h_mapping);
                ReleaseMutex(h_file);
                return 1;
            }

            int count = file_size / sizeof(int);

            HDC hdc = BeginPaint(hwnd, &ps);

            for (int i = 0; i < count; i++)
            {
                TextOut(hdc, 0, i * gap, get_characters(character, numbers[i]).c_str(), numbers[i]);
            }

            ReleaseMutex(h_file);
            CloseHandle(h_file);
            CloseHandle(h_mapping);
            
            SetTimer(hwnd, timer_id, timer_interval, NULL);
            EndPaint(hwnd, &ps);

            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == timer_id)
            {
                InvalidateRect(hwnd, NULL, TRUE);
            }

            return 0;
        }

        case WM_DESTROY:
        {
            CloseHandle(h_mutex);
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    h_mutex = CreateMutex(NULL, FALSE, "Global\\SortMutex");

    if (h_mutex == NULL)
    {
        std::cerr << "Error creating a mutex!" << std::endl;
        return 1;
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {0};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SomeWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
    {
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        "Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        300,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        std::cerr << "Error  creating a window!" << std::endl;
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
