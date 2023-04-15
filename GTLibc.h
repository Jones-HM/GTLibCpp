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
#include <iomanip>
#include <chrono>
#include <fstream>
#include <ostream>
#include <regex>
#include <variant>
#include "CEParser.h"

#define to_hex_str(hex_val) (static_cast<std::stringstream const &>(std::stringstream() << "0x" << std::uppercase << std::hex << hex_val)).str()
using DataType = std::variant<BYTE, int16_t, int32_t, int64_t, float, double, std::string>;

namespace GTLIBC
{
    class GTLibc
    {
    public:
        GTLibc(); // Default constructor
        GTLibc(bool enableLogs); // Constructor
        GTLibc(const std::string &gameName); // Constructor
        ~GTLibc(); // Destructor
        GTLibc(const GTLibc&) = default; // Copy constructor
		GTLibc(GTLibc&&) = default; // Move constructor

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
        T ReadPointer(DWORD address);
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
        DWORD GetStaticAddress(DWORD processId, DWORD moduleName);
        HMODULE GetProcessModule(DWORD processId, const std::string &moduleName);

        bool SuspendResumeProcess(bool suspend);
        bool Is64bitGame();
        bool HotKeysDown(int keycode);
        bool IsKeyPressed(int keycode);
        bool IsKeyToggled(int keycode);
        void EnableLogs(bool status);

        // Cheat Engine variables.
        CheatTable ReadCheatTable(const std::string &filename);
        void PrintCheatTable(CheatTable &cheatTable);
        void ReadCheatTableEntries(CheatTable &cheatTable);

    private:
        void AddLog(const std::string &methodName, const std::string &logMessage);
        void ShowError(const std::string &errorMessage);
        void ShowWarning(const std::string &warningMessage);
        void ShowInfo(const std::string &infoMessage);

        // Cheat Engine variables.
        void PrintValue(const DataType &value);
        DataType ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList = {});
        DWORD ReadPointerOffsetsUntilLast(DWORD address, const std::vector<DWORD> &offsetsList);
        bool IsValidCheatTable(const std::string &xmlData);

        std::string gameName;
        HWND gameWindow;
        DWORD processId;
        HANDLE gameHandle;
        bool enableLogs;
        DWORD gameBaseAddress;
        std::string logFile;
    };
    inline GTLibc* g_GTLibc{};
}

#endif