#include <Windows.h>
#include <string>
#include <iostream>
#include <random>

int count = 30;
int min = 10;
int max = 100;

std::string file_name = "data.dat";

int random_integer(int min, int max)
{
    std::random_device rd;
    std::uniform_int_distribution<int> dis(min, max);
    return dis(rd);
}

int main(int argc, char* argv[])
{
    HANDLE h_file = CreateFile(file_name.c_str(), GENERIC_WRITE | GENERIC_READ,  FILE_SHARE_READ |
        FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (h_file == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Error creating the file!" << std::endl;
        return 1;
    }

    int* numbers = new int[count];

    for (int i = 0; i < count; i++)
    {
        numbers[i] = random_integer(min, max);
    }

    DWORD bytes_written;

    BOOL success = WriteFile(h_file, numbers, sizeof(int) * count, &bytes_written, NULL);

    if (!success)
    {
        std::cerr << "Error writing to the file!" << std::endl;
        return 1;
    }

    CloseHandle(h_file);

    return 0;
}
