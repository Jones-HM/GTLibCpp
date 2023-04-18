#pragma once
#ifndef GTLIBC_H
#define GTLIBC_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <ostream>
#include <regex>
#include <variant>
#include <thread>
#include <optional>
#include <windows.h>
#include <tlhelp32.h>
#include "CEParser.h"

#define to_hex_string(hex_val) (static_cast<std::stringstream const &>(std::stringstream() << std::uppercase << std::hex << hex_val)).str()
using DataType = std::variant<std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, float, double, long double, std::string>;

namespace GTLIBC
{
    class GTLibc
    {
    public:
        GTLibc();                            // Default constructor
        GTLibc(bool enableLogs);             // Constructor
        GTLibc(const std::string &gameName); // Constructor
        ~GTLibc();                           // Destructor
        GTLibc(const GTLibc &) = default;    // Copy constructor
        GTLibc(GTLibc &&) = default;         // Move constructor

        // Find game process and window.
        bool FindGameProcess(const std::string &gameName);
        HWND FindGameWindow(const std::string &windowName);

        // Read and write memory.
        template <typename T>
        T ReadAddress(DWORD address);
        template <typename T>
        bool WriteAddress(DWORD address, const T &value);

        // Read  and write memory with offsets.
        template <typename T>
        T ReadAddressOffset(DWORD address, const DWORD offset);
        template <typename T>
        bool WriteAddressOffset(DWORD address, DWORD offset, const T &value);
        template <typename T>
        T ReadAddressOffsets(DWORD address, const std::vector<DWORD> &offsets);
        template <typename T>
        bool WriteAddressOffsets(DWORD address, const std::vector<DWORD> &offsets, const T &value);

        // Read and write pointers with offsets.
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

        // Reading and writing strings.
        std::string ReadString(DWORD address, SIZE_T size);
        bool WriteString(DWORD address, const std::string &str);

        // Detecting Hotkeys.
        bool HotKeysDown(const std::vector<int> &keys);
        bool IsKeyPressed(int keycode);
        bool IsKeyToggled(int keycode);

        std::string GetGameName();
        DWORD GetProcessID();
        HANDLE GetGameHandle4mHWND(HWND hwnd);
        DWORD GetProcessID4mHWND(HWND hwnd);
        HANDLE GetGameHandle();
        DWORD GetGameBaseAddress();

        bool SuspendResumeProcess(bool suspend);
        bool Is64bitGame();

        void EnableLogs(bool status);

        // Cheat Engine variables.
        CheatTable ReadCheatTable(const std::string &filename, int entries);
        void PrintCheatTable();
        void ReadCheatTableEntries();
        template <typename T>
        void AddCheatEntry(const string &description, const string &dataType, const DWORD address,
                           const vector<DWORD> &offsets, const std::vector<int> &hotkeys, const std::string &hotkeyAction,
                           T hotkeyValue);
        void ActivateCheatEntries(const std::vector<int> &cheatEntryIndex);
        void ExecuteCheatTable();

        template <typename T>
        void CheatAction_SetValue(DWORD address, T value);

        template <typename T>
        void CheatAction_IncreaseValue(DWORD address, T value);

        template <typename T>
        void CheatAction_DecreaseValue(DWORD address, T value);

    private:
        void AddLog(const std::string &methodName, const std::string &logMessage);
        void ShowError(const std::string &errorMessage);
        void ShowWarning(const std::string &warningMessage);
        void ShowInfo(const std::string &infoMessage);

        // Cheat Engine variables.
        void PrintValue(const DataType &value);
        template <typename T>
        T ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList = {});
        DataType ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList = {});
        DataType ReadAddressGenericWrapper(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList = {});
        DWORD ResolveAddressGeneric(DWORD address, const std::vector<DWORD> &offsetsList);
        bool IsValidCheatTable(const std::string &xmlData);
        void PrintCheatTableMenu();
        void ExecuteCheatAction(string &cheatAction, const DWORD &address, DataType &value);
        template <typename T>
        void ExecuteCheatAction(string &cheatAction, const DWORD &address, T &value);

        // Utility functions
        template <typename T>
        std::optional<T> TryParse(const std::string &str);
        DataType ConvertStringToDataType(const std::string &str);
        string GetHotKeysName(const vector<int> &keys);

        std::string gameName;
        HWND gameWindow;
        DWORD processId;
        HANDLE gameHandle;
        bool enableLogs;
        DWORD gameBaseAddress;
        std::string logFile;
    };
    inline GTLibc *g_GTLibc{};
    inline static CheatTable g_CheatTable;
}

#endif