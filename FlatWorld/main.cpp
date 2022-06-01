#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>


template<typename T>
void alert(T item)
{
    std::ostringstream os;
    os << item;
    MessageBoxA(NULL, os.str().c_str(), "Error", MB_OK | MB_ICONERROR);
}

void alert(const wchar_t* item)
{
    MessageBoxW(NULL, item, L"Message", MB_OK | MB_ICONERROR);
}

bool exists(const char* name) {
    std::ifstream f(name);
    return f.good();
}

struct Proc {
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;

    Proc() {
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&processInfo, sizeof(processInfo));
    }

    BOOL start(const char* path) {
        return CreateProcessA(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
    }

    BOOL inject(const char* path) {
        auto loc = VirtualAllocEx(processInfo.hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!loc)
        {
            alert("Unable to allocate virtual memory");
            return FALSE;
        }
        auto writeSucess = WriteProcessMemory(processInfo.hProcess, loc, path, strlen(path) + 1, 0);
        if (!writeSucess)
        {
            alert("Unable to write process memory");
            return FALSE;
        }
        HANDLE hThread = CreateRemoteThread(processInfo.hProcess, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA), loc, 0, 0);
        if (!hThread)
        {
            VirtualFree(loc, 0, MEM_RELEASE);
            alert("Dll loading failed");
            return FALSE;
        }
        CloseHandle(hThread);
        VirtualFree(loc, 0, MEM_RELEASE);
        return TRUE;
    }

    ~Proc() {
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
};

int wmain(void)
{
    auto exePath = "./4D Miner.exe";
    auto dllPath = "./FlatWorld.dll";

    FreeConsole();
    auto exeFound = exists(exePath);
    if (!exeFound) {
        alert("4D Miner.exe was not found in directory");
        return EXIT_FAILURE;
    }
    auto dllFound = exists(dllPath);
    if (!dllFound) {
        alert("FlatWorld.dll was not found in directory");
        return EXIT_FAILURE;
    }

    auto proc = Proc();
    auto processStarted = proc.start(exePath);
    if (!processStarted) {
        alert("Unable to run 4D miner");
        return EXIT_FAILURE;
    }
    auto injectionSuccess = proc.inject(dllPath);
    if (!injectionSuccess) {
        alert("Unable to inject dll");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}