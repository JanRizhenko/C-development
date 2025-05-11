#include <Windows.h>
#include <cstdlib>
#include <fileapi.h>
#include <iostream>
#include <memoryapi.h>
#include <string>
#include <synchapi.h>
#include <winbase.h>
#include <winnt.h>
#include <conio.h>
#include <chrono>
#include <thread>

HANDLE h_mutex;
std::string file_name = "data.dat";

int bubble_sort(int* array, int length)
{
    for (int i = 0; i < length - 1; i++)
    {
        for (int j = 0; j < length - 1 - i; j++)
        {
            if (array[j] < array[j + 1])
            {
                if(WaitForSingleObject(h_mutex, INFINITE) == WAIT_OBJECT_0)
                {
                    std::swap(array[j], array[j + 1]);
                    ReleaseMutex(h_mutex);
                }

                else
                {
                    std::cerr << "Error getting a mutex!" << std::endl;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    }

    return 0;
}

int sort_file(std::string file_name)
{
    HANDLE h_file = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h_file == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error opening the file!" << std::endl;
        return 1;
    }

    HANDLE h_mapping = CreateFileMapping(h_file, NULL, PAGE_READWRITE, 0, 0, NULL);

    if (!h_mapping)
    {
        std::cerr << "Error creating a file mapping object!" << std::endl;
        return 1;
    }

    int* numbers = (int*)MapViewOfFile(h_mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (!numbers)
    {
        std::cerr << "Error mapping the file!" << std::endl;
        return 1;
    }

    int file_size = GetFileSize(h_file, NULL);
    int count = file_size / sizeof(int);

    int success = bubble_sort(numbers, count);

    UnmapViewOfFile(numbers);
    CloseHandle(h_file);
    CloseHandle(h_mapping);

    return success;
}

int main(int argc, char* argv[])
{
    h_mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "Global\\SortMutex");

    std::cout << "Press space to sort the file." << std::endl;

    while (true)
    {
        char character = _getch();

        if (character == 32)
        {
            std::cout << "Sorting the file..." << std::endl;

            BOOL result = !sort_file(file_name);

            if (result) std::cout << "Array sorted successfully!" << std::endl;
        }

        else if (character == 'q')
        {
            break;
        }
    }

    return 0;
}
