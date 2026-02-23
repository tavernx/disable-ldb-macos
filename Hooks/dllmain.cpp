#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <tlhelp32.h>
#include <cstdlib>
#include <tchar.h>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <GL/gl.h>
#include <wincodec.h> 
#pragma comment(lib, "windowscodecs.lib")
#include <Shlobj.h> 
#pragma comment(lib, "Shell32.lib")

/*
TODO: REPLACE HOOKING AND DLL INJECTION BEHAVIOR WITH ONE THAT ACTUALLY WORKS
    */

std::wstring GetLogFilePath() {
    wchar_t* appdataPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdataPath))) {
        std::wstring logPath = std::wstring(appdataPath) + L"\\transcientclient\\transcient-hook.txt";
        CoTaskMemFree(appdataPath);

        // Create directory if it doesn't exist
        CreateDirectoryW((std::wstring(appdataPath) + L"\\transcientclient").c_str(), nullptr);
        return logPath;
    }
    return L"C:\\transcient-hook.txt";
}

void Log(const std::string& message) {
    std::wstring filePath = GetLogFilePath();
    std::ofstream logFile(filePath, std::ios::app);
    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.close();
    }
}

bool isFocusInstalled = false; // Global flag

// create global hWND variable
HWND focusHWND = NULL;
HWND bringWindowToTopHWND = NULL;
HWND setWindowFocusHWND = NULL;
HWND setWindowFocushWndInsertAfter = NULL;
int setWindowFocusX = 0;
int setWindowFocusY = 0;
int setWindowFocuscx = 0;
int setWindowFocuscy = 0;
UINT setWindowFocusuFlags = 0;

BYTE originalBytesForGetForeground[5] = { 0 };
BYTE originalBytesForShowWindow[5] = { 0 };
BYTE originalBytesForSetWindowPos[5] = { 0 };
BYTE orginalBytesForSetFocus[5] = { 0 };
BYTE originalBytesForEmptyClipboard[5] = { 0 };
BYTE originalBytesForSetClipboardData[5] = { 0 };
BYTE originalBytesForTerminateProcess[5] = { 0 };
BYTE originalBytesForExitProcess[5] = { 0 };

// My custom functions
HWND WINAPI MyGetForegroundWindow();
BOOL WINAPI MyShowWindow(HWND hWnd);
BOOL WINAPI MySetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
HWND WINAPI MySetFocus(HWND hWnd);
HWND WINAPI MyGetWindow(HWND hWnd, UINT uCmd);
int WINAPI MyGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);
BOOL WINAPI MyK32EnumProcesses(DWORD * pProcessIds, DWORD cb, DWORD * pBytesReturned);
HANDLE WINAPI MyOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
BOOL WINAPI MyTerminateProcess(HANDLE hProcess, UINT uExitCode);
VOID WINAPI MyExitProcess(UINT uExitCode);
BOOL WINAPI MyEmptyClipboard();
HANDLE WINAPI MySetClipboardData(UINT uFormat, HANDLE hMem);

// Implement MySetClipboardData which does nothing
HANDLE WINAPI MySetClipboardData(UINT uFormat, HANDLE hMem) {
    std::cout << "SetClipboardData hook called, been hooked, terminating chain" << std::endl;
    return NULL; // Indicate failure or that the data was not set
}

BOOL WINAPI MyEmptyClipboard() {
    Log("Empty clipboard data called");
    // This custom function pretends to clear the clipboard but does nothing
    std::cout << "EmptyClipboard hook called, but not clearing the clipboard." << std::endl;
    return TRUE; // Pretend success
}

HANDLE WINAPI MyOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
    std::cout << "OpenProcess hook called, but not opening process." << std::endl;
    return NULL;
}

BOOL WINAPI MyTerminateProcess(HANDLE hProcess, UINT uExitCode) {
    std::cout << "TerminateProcess hook called, but not terminating process." << std::endl;
    return TRUE; // Simulate success
}

VOID WINAPI MyExitProcess(UINT uExitCode) {
    std::cout << "ExitProcess hook called, but not exiting process." << std::endl;
}

BOOL WINAPI MyK32EnumProcesses(DWORD * pProcessIds, DWORD cb, DWORD * pBytesReturned) {
    Log("K32EnumProcesses hook called, but pretending no processes exist");
    // This custom function behaves as if no processes are running
    std::cout << "K32EnumProcesses hook called, but pretending no processes exist." << std::endl;
    if (pBytesReturned != NULL) {
        *pBytesReturned = 0; // Indicate no processes were written to the buffer
    }
    return TRUE; // Indicate the function succeeded
}

int WINAPI MyGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount) {
    Log("GetWindowTextW hook called, but not returning actual window text");
    // This custom function behaves as if no window title is retrieved
    std::cout << "GetWindowTextW hook called, but not returning actual window text." << std::endl;
    if (nMaxCount > 0) {
        lpString[0] = L'\0'; // Return an empty string
    }
    return 0; // Indicate that no characters were copied to the buffer
}
HWND WINAPI MyGetWindow(HWND hWnd, UINT uCmd) {
    // This custom function behaves as if there are no windows to return
    std::cout << "GetWindow hook called, but pretending no related window." << std::endl;
    return NULL; // Indicate no window found
}

void InstallHook() {
    Log("Installing hooks");
    std::cout << "Installing hooks..." << std::endl;
    DWORD oldProtect;

    // Hook EmptyClipboard
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");

    // Hook GetForegroundWindow
    void* targetGetForegroundWindow = GetProcAddress(hUser32, "GetForegroundWindow");

    // Hook TerminateProcess
    HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
    void* targetTerminateProcess = GetProcAddress(hKernel32, "TerminateProcess");

    // Hook ExitProcess
    void* targetExitProcess = GetProcAddress(hKernel32, "ExitProcess");
}

void InstallFocus() {
    if (isFocusInstalled) {
        return;
    }
    isFocusInstalled = true;
    std::cout << "Installing focus..." << std::endl;
    DWORD oldProtect;

    // Hook BringWindowToTop
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");

    // Hook SetWindowPos
    void* targetSetWindowPos = GetProcAddress(hUser32, "SetWindowPos");
}

void UninstallFocus() {
    if (!isFocusInstalled) {
        return;
    }
    isFocusInstalled = false;
    std::cout << "Uninstalling focus..." << std::endl;
    DWORD oldProtect;

    // Unhook BringWindowToTop
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");

    // Unhook SetWindowPos
    // TODO: New functionality thats undetected here

    // Unhook SetFocus
    // todo: new functionliaty thats undetected here
}

void UninstallHook() {
    DWORD oldProtect;

    // Unhook SetClipboardData
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");

    // Unhook EmptyClipboard

    // Unhook GetForegroundWindow
}

// Helper function to determine if the given window is the main window of the current process
BOOL IsMainWindow(HWND handle) {
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    DWORD processID = 0;
    GetWindowThreadProcessId(handle, &processID);
    if (GetCurrentProcessId() == processID && IsMainWindow(handle)) {
        // Stop enumeration if a main window is found, and return its handle
        *reinterpret_cast<HWND*>(lParam) = handle;
        return FALSE;
    }
    return TRUE;
}

// Function to find the main window of the current process
HWND FindMainWindow() {
    HWND mainWindow = NULL;
    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&mainWindow));
    return mainWindow;
}

// Your hook function implementation
HWND WINAPI MyGetForegroundWindow() {
    HWND hWnd = FindMainWindow();
    if (hWnd != NULL) {
        std::cout << "Returning the main window of the current application." << std::endl;
        return hWnd;
    }
    std::cout << "Main window not found, returning NULL." << std::endl;
    return NULL;
}

HWND WINAPI MySetFocus(HWND _hWnd) {
    focusHWND = _hWnd;
    HWND hWnd = FindMainWindow(); // Find the main window of the current process
    if (hWnd != NULL) {
        std::cout << "Returning the main window of the current application due to '[' key press." << std::endl;
        return hWnd; // Return the main window handle if found
    }
    else {
        std::cout << "Main window not found, returning NULL." << std::endl;
        return NULL; // If main window is not found, return NULL
    }
}

// Keyboard hook handle
HHOOK hKeyboardHook;

// Keyboard hook callback
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        if (wParam == WM_KEYDOWN) {
            switch (p->vkCode) {
                // VK_UP is the virtual key code for the Up arrow key
            case VK_UP:
                //CaptureOpenGLScreen();
                InstallFocus();
                std::cout << "Up arrow key pressed, installing focus hook." << std::endl;
                break;
                // VK_DOWN is the virtual key code for the Down arrow key
            case VK_DOWN:
                UninstallFocus();
                // call the set focus with the focusHWND
                if (focusHWND != NULL) {
                    SetFocus(focusHWND);
                }
                if (bringWindowToTopHWND != NULL) {
                    BringWindowToTop(bringWindowToTopHWND);
                }
                if (setWindowFocusHWND != NULL) {
                    SetWindowPos(setWindowFocusHWND, setWindowFocushWndInsertAfter, setWindowFocusX, setWindowFocusY, setWindowFocuscx, setWindowFocuscy, setWindowFocusuFlags);
                }
                std::cout << "Down arrow key pressed, uninstalling focus hook." << std::endl;
                break;
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// Place this in a suitable location in your existing code.
void SetupKeyboardHook() {
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Start the hook setup in a new thread to keep DllMain non-blocking
        InstallHook(); // Sets up your custom hooks
        std::thread([]() {
            SetupKeyboardHook(); // Sets up the keyboard hook
            }).detach();
            break;
    case DLL_PROCESS_DETACH:
        UninstallHook(); // Uninstall your custom hooks
        if (hKeyboardHook != nullptr) {
            UnhookWindowsHookEx(hKeyboardHook); // Uninstall the keyboard hook
        }
        break;
    }
    return TRUE;
}
