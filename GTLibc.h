#pragma once
#ifndef GTLIBC_H
#define GTLIBC_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdexcept>
#include <ctime>
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <ostream>

#define to_hex_str(hex_val) (static_cast<std::stringstream const &>(std::stringstream() << "0x" << std::uppercase << std::hex << hex_val)).str()
class GTLibc
{
public:
    GTLibc();
    GTLibc(bool enableLogs);
    GTLibc(const std::string &gameName);
    ~GTLibc();

    bool FindGameProcess(const std::string &gameName);
    HWND FindGameWindow(const std::string &windowName);

    template <typename T>
    T ReadAddress(DWORD address);
    template <typename T>
    bool WriteAddress(DWORD address, const T &value);

    template <typename T>
    T ReadAddressOffset(DWORD address, const DWORD offset);
    template <typename T>
    bool WriteAddressOffset(DWORD address, DWORD offset, const T &value);

    template <typename T>
    T ReadAddressOffsets(DWORD address, const std::vector<DWORD> &offsets);

    template <typename T>
    bool WriteAddressOffsets(DWORD address, const std::vector<DWORD> &offsets, const T &value);

    template <typename T>
    T ReadPointer(DWORD address, DWORD pointerOffset);
    template <typename T>
    bool WritePointer(DWORD address, DWORD pointerOffset, const T &value);

    template <typename T>
    T ReadPointerOffset(DWORD address, const DWORD offset);
    template <typename T>
    bool WritePointerOffset(DWORD address, const std::vector<DWORD> &offsets, const T &value);

    template <typename T>
    T ReadPointerOffsets(DWORD address, const std::vector<DWORD> &offsetsList);
    template <typename T>
    bool WritePointerOffsets(DWORD address, const std::vector<DWORD> &offsetsList, const T &value);

    std::string ReadString(DWORD address, SIZE_T size);
    bool WriteString(DWORD address, const std::string &str);

    std::string GetGameName();
    DWORD GetProcessID();
    HANDLE GetGameHandle4mHWND(HWND hwnd);
    HANDLE GetGameHandle();
    DWORD GetGameBaseAddress();
    DWORD GetProcessID4mHWND(HWND hwnd);
    DWORD GetGameBaseAddress(DWORD processId);
    DWORD GetStaticAddress(DWORD processId, DWORD moduleName);
    HMODULE GetProcessModule(DWORD processId, const std::string &moduleName);

    bool SuspendResumeProcess(bool suspend);
    bool Is64bitGame();
    bool HotKeysDown(int keycode);
    bool IsKeyPressed(int keycode);
    bool IsKeyToggled(int keycode);
    void EnableLogs(bool status);

private:
    void AddLog(const std::string &methodName, const std::string &logMessage);
    void ShowError(const std::string &errorMessage);
    void ShowWarning(const std::string &warningMessage);
    void ShowInfo(const std::string &infoMessage);

    std::string gameName;
    HWND gameWindow;
    DWORD processId;
    HANDLE gameHandle;
    bool enableLogs;
    DWORD gameBaseAddress;
    std::string logFile;
};

#endif