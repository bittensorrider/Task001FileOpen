// Task001FileOpen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// used for turning OFF warnings regarding CRT
#define _CRT_SECURE_NO_WARNINGS

//#include <iostream>
#include <Windows.h>    // CreateFileW, WriteFile, ReadFile, etc.
#include <cstdio>       // fopen, fprintf, fclose, etc.
#include <cstring>      // strlen, etc.

static void MyLogger(const char* msg)
{
    printf("\n================================\n");
    printf("%s\n", msg);
    printf("================================\n");
}

static void PrintLastError(const char* errWhere)
{
    DWORD errCode = GetLastError();
    char errMsg[512] = {};
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errCode,
        0,
        errMsg,
        sizeof(errMsg),
        nullptr
    );
    printf("[Error] %s failed: code=%lu, stack=%s", errWhere, errCode, errMsg);
}

// file Create, Write, Close using Win32 API
static bool Win32_CreateAndWrite(const wchar_t* path)
{
    MyLogger("make file using CreateFileW, and write");

    HANDLE hFile = CreateFileW(
        path,
        GENERIC_WRITE,
        0,  // do not share while I occupy the file
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFileW");
        return false;
    }
    printf("create file OK: HANDLE = %p\n", hFile);

    // contents to write
    const char* txtLines[] = {
        "Line 01: This text is written by Win32 API.\r\n",
        "Line 02: WriteFile writes characters per byte unit.\r\n",
        "Line 03: That is all. End.\r\n"
    };

    for (const char* line : txtLines)
    {
        DWORD toWrite = (DWORD)strlen(line);
        DWORD written = 0;

        bool writeRes = WriteFile(
            hFile,
            line,
            toWrite,
            &written,
            nullptr
        );
        if (!writeRes)
        {
            PrintLastError("WriteFile");
            CloseHandle(hFile);     // must have
            return false;
        }

        printf("%lu bytes requested ... %lu bytes recorded\n", toWrite, written);
    }

    CloseHandle(hFile);     // must have
    printf("CloseHandle done.\n");
    // TODO

    return true;
}

static bool Win32_OpenAndRead(const wchar_t* path)
{
    MyLogger("open file using CreateFileW(OPEN_EXISTING), and read");

    HANDLE hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFileW(OPEN_EXISTING)");
        return false;
    }

    // check file size
    LARGE_INTEGER size{};
    GetFileSizeEx(hFile, &size);
    printf("file size = %lld bytes\n", size.QuadPart);

    // read file by buffer chunks
    char chunk[32];
    char all[1024] = {};
    DWORD total = 0;
    DWORD bytesRead = 0;
    int count = 0;

    for (;;)
    {
        bool readRes = ReadFile(
            hFile,
            chunk,
            sizeof(chunk),
            &bytesRead,
            nullptr
        );
        if (!readRes)
        {
            PrintLastError("ReadFile");
            break;
        }
        if (bytesRead == 0)
        {
            // 0 byte read means EOF
            break;
        }

        printf("ReadFile #%d : %lu bytes\n", ++count, bytesRead);

        if (total + bytesRead < sizeof(all))
        {
            memcpy(all + total, chunk, bytesRead);
            total += bytesRead;
        }
    }

    // make result into string by attaching '\0' at the end
    all[total] = '\0';

    printf("file content (%lu bytes):\n%s", total, all);
    printf("-----------------------------\n");

    CloseHandle(hFile);     // must have

    return true;
}

static bool Win32_SeekAndAppend(const wchar_t* path)
{
    MyLogger("append at the end of file via SetFilePointerEx");

    HANDLE hFile = CreateFileW(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFileW");
        return false;
    }

    // append to file by moving file pointer
    LARGE_INTEGER zero{};
    LARGE_INTEGER newPos{};
    SetFilePointerEx(
        hFile,
        zero,
        &newPos,
        FILE_END
    );
    printf("file pointer is moved to the last: position = %lld\n", newPos.QuadPart);

    // contents to append
    const char* extraMsg = "Line 04: attached later.\r\n";
    DWORD written = 0;
    WriteFile(
        hFile,
        extraMsg,
        (DWORD)strlen(extraMsg),
        &written,
        nullptr
    );
    printf("%lu bytes appended\n", written);

    // go back to the beginning to read 1st 10 bytes again for test
    SetFilePointerEx(
        hFile,
        zero,
        nullptr,
        FILE_BEGIN
    );
    char fileHead[11] = {};
    DWORD bytesRead = 0;
    ReadFile(
        hFile,
        fileHead,
        10,
        &bytesRead,
        nullptr
    );
    printf("1st beginning 10 bytes = [%s]\n", fileHead);

    CloseHandle(hFile);     // must have

    return true;
}

// file Create, Write, Close using C Runtime Library
static bool CRT_CreateAndWrite(const char* path)
{
    MyLogger("make file using fopen, and write");

    FILE* fp = fopen(path, "w");
    if (fp == nullptr)
    {
        perror("[Error] fopen");
        return false;
    }
    printf("fopen OK, FILE* value = %p\n", (void*)fp);

    fputs("1st Line: written by fputs.\n", fp);
    fprintf(fp, "2nd Line: fprintf() uses %s just like printf()\n", "format");
    for (int i = 1; i <= 3; i++)
    {
        fprintf(fp, "Number Line %d : %d x %d = %d\n", i, i, i, i * i);
    }

    fflush(fp);
    fclose(fp);

    printf("fclose Completed.\n");

    return true;
}


int main()
{
    //std::cout << "Hello World!\n";

    SetConsoleOutputCP(CP_UTF8);    // set output encoding as UTF-8

    wchar_t cwd[MAX_PATH] = {};
    GetCurrentDirectoryW(MAX_PATH, cwd);    // file will be created in the current working directory
    char cwdUtf8[MAX_PATH * 3] = {};        // UTF-16 character can be 3 bytes in UTF-8
    WideCharToMultiByte(
        CP_UTF8,
        0,
        cwd,
        -1,
        cwdUtf8,
        sizeof(cwdUtf8),
        nullptr,
        nullptr
    );
    printf("Working directory: %s\n", cwdUtf8);

    MyLogger("Win32 API methods");
    const wchar_t* filePathWin32 = L"C:\\Users\\weimi\\Documents\\myCppWork\\task001\\sample_text_Win32_mode.txt";

    if (Win32_CreateAndWrite(filePathWin32))
    {
        Win32_OpenAndRead(filePathWin32);
        Win32_SeekAndAppend(filePathWin32);
        Win32_OpenAndRead(filePathWin32);
    }

    // TODO

    printf("\nCompleted!\nPlease open [sample_text_Win32_mode.txt] for confirmation.");
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
