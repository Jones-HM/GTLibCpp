#include "GTLibc.h"
using namespace GTLIBC;

// Constructors and Destructors
GTLibc::GTLibc()
    : GTLibc(false)
{
    g_GTLibc = this;
}

GTLibc::GTLibc(bool enableLogs)
    : GTLibc("", enableLogs)
{
    g_GTLibc = this;
}

GTLibc::GTLibc(const std::string &gameName)
    : GTLibc(gameName, false)
{
    g_GTLibc = this;
}

GTLibc::GTLibc(const std::string &gameName, bool enableLogs)
{
    logFile = "GTLibc.log";
    this->enableLogs = enableLogs;
    if (!gameName.empty())
    {
        FindGameProcess(gameName);
    }
    g_GTLibc = this;
}

GTLibc::~GTLibc()
{
    if (gameHandle)
    {
        CloseHandle(gameHandle);
    }
    g_GTLibc = nullptr;
}

/*
--------------------------------------------------------------------------------
------------------------------ GTLibc Functions --------------------------------
--------------------------------------------------------------------------------
*/

/*
 * @brief Find game process by name
 * @param gameName - Name of the game process
 * @return bool - True if game process found, false otherwise.
 *
 */

bool GTLibc::FindGameProcess(const std::string &gameName)
{
    AddLog("FindGameProcess", "Trying to find game process: " + gameName);

    auto EndsWith = [](const std::string &str, const std::string &suffix)
    {
        if (suffix.size() > str.size())
        {
            return false;
        }
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    };

    // Check if game name ends with .exe
    std::string gameNameExe = gameName;
    if (!EndsWith(gameNameExe, ".exe"))
    {
        gameNameExe += ".exe";
        AddLog("FindGameProcess", "Game name appended with .exe: " + gameNameExe);
    }

    PROCESSENTRY32 pe;
    HANDLE hSnapshot;

    try
    {
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        AddLog("FindGameProcess", "Snapshot handle: " + to_hex_string(hSnapshot));

        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            std::string errMsg = "Failed to create process snapshot";
            AddLog("FindGameProcess", "Error: " + errMsg);
            ShowError(errMsg);
            return false;
        }

        pe.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &pe))
        {
            std::string errMsg = "Failed to get first process entry";
            AddLog("FindGameProcess", "Error: " + errMsg);
            ShowError(errMsg.c_str());
            CloseHandle(hSnapshot);
            return false;
        }

        do
        {
            if (!lstrcmpi(pe.szExeFile, gameNameExe.c_str()))
            {
                CloseHandle(hSnapshot);
                processId = pe.th32ProcessID;
                gameHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

                /*set current process info.*/
                this->gameName = gameName;
                this->processId = pe.th32ProcessID;
                this->gameWindow = FindGameWindow(gameName);
                AddLog("FindGameProcess", "Game process found: " + gameName);
                AddLog("FindGameProcess", "Game process id: " + to_hex_string(processId));
                AddLog("FindGameProcess", "Game process handle: " + to_hex_string(gameHandle));
                AddLog("FindGameProcess", "Game window handle: " + to_hex_string(gameWindow));

                // Check game trainer architecture.
                CheckGameTrainerArch();
                return true;
            }
        } while (Process32Next(hSnapshot, &pe));

        std::string errMsg = "Game process not found '" + gameName + "'";
        AddLog("FindGameProcess", "Error: " + errMsg);
        ShowError(errMsg);
    }
    catch (const std::exception &e)
    {
        AddLog("FindGameProcess", "Error: " + std::string(e.what()));
        ShowError(e.what());
    }

    CloseHandle(hSnapshot);
    return false;
}

/*
 * @brief Find game window by name
 * @param windowName - Name of the game window
 * @return HWND - Handle to the game window
 *
 */

HWND GTLibc::FindGameWindow(const std::string &windowName)
{
    AddLog("FindGameWindow", "Trying to find game window: " + windowName);
    return FindWindowA(NULL, windowName.c_str());
}

/*
 * @brief Read address with offset
 * @param address - Address to read
 * @param offset - Offset to add to address
 * @return T - Value read from address
 *
 */
template <typename T>
T GTLibc::ReadAddressOffset(DWORD address, DWORD offset)
{
    AddLog("ReadAddressOffset", "Trying to read address with offset: " + to_hex_string(address) + " + " + to_hex_string(offset));
    try
    {
        T value;
        SIZE_T bytesRead;
        DWORD newAddress = address + offset;
        if (ReadProcessMemory(gameHandle, (LPVOID)newAddress, &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T))
        {
            AddLog("ReadAddressOffset", "Read value: " + to_hex_string(value));
            return value;
        }
        throw std::runtime_error("Failed to read address with offset");
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadAddressOffset", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return T();
    }
    // return default value for type T
    return T();
}

/*
 * @brief Read address with offsets
 * @param address - Address to read
 * @param offsets - Offsets to add to address
 * @return T - Value read from address
 *
 */
template <typename T>
T GTLibc::ReadAddressOffsets(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ReadAddressOffsets", "Trying to read address with offsets: " + to_hex_string(address));

    std::ostringstream oss;
    for (auto d : offsets)
        oss << to_hex_string(d) << ",";

    std::string result = oss.str();
    result.pop_back();
    AddLog("ReadPointerOffsets", "Offsets: " + result);

    try
    {
        DWORD currentAddress = address;
        for (DWORD offset : offsets)
        {
            currentAddress = ReadAddress<DWORD>(currentAddress + offset);
        }
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadAddressOffsets", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return T();
    }

    return T();
}

/*
 * @brief Read address
 * @param address - Address to read
 * @return T - Value read from address
 *
 */
template <typename T>
T GTLibc::ReadAddress(DWORD address)
{
    AddLog("ReadAddress", "Trying to read address: " + to_hex_string(address));
    try
    {
        T value;
        SIZE_T bytesRead;
        if (ReadProcessMemory(gameHandle, (LPVOID)address, &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T))
        {
            AddLog("ReadAddress", "Read value: " + to_hex_string(value));
            return value;
        }
        throw std::runtime_error("Failed to read address");
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadAddress", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return T();
    }
    // return default value for type T
    return T();
}

/*
 * @brief Write address
 * @param address - Address to write
 * @param value - Value to write to address
 * @return bool - True if write was successful else false
 *
 */

template <typename T>
bool GTLibc::WriteAddress(DWORD address, const T &value)
{
    AddLog("WriteAddress", "Trying to write to address: " + to_hex_string(address) + " with value: " + to_hex_string(value));
    try
    {
        SIZE_T bytesWritten;
        if (WriteProcessMemory(gameHandle, (LPVOID)address, &value, sizeof(T), &bytesWritten) && bytesWritten == sizeof(T))
        {
            return true;
        }
        throw std::runtime_error("Failed to write to address");
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WriteAddress", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Write address with offset
 * @param address - Address to write
 * @param offset - Offset to add to address
 * @param value - Value to write to address
 * @return bool - True if write was successful else false
 *
 */

template <typename T>
bool GTLibc::WriteAddressOffset(DWORD address, DWORD offset, const T &value)
{
    AddLog("WriteAddressOffset", "Trying to write to address: " + to_hex_string(address) + " with offset: " + to_hex_string(offset) + " with value: " + std::to_string(value));
    try
    {
        DWORD newAddress = address + offset;
        return WriteAddress(newAddress, value);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WriteAddressOffset", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Write address with offsets
 * @param address - Address to write
 * @param offsets - Offsets to add to address
 * @param value - Value to write to address
 * @return bool - True if write was successful else false
 *
 */

template <typename T>
bool GTLibc::WriteAddressOffsets(DWORD address, const std::vector<DWORD> &offsets, const T &value)
{
    AddLog("WriteAddressOffsets", "Trying to write to address: " + to_hex_string(address) + " with offsets: " + to_hex_string(offsets.size()) + " with value: " + std::to_string(value));
    try
    {
        for (const DWORD &offset : offsets)
        {
            WriteAddressOffset(address, offset, value);
        }
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WriteAddressOffsets", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Write pointer to address
 * @param address - base address
 * @param offset - offset from pointer address
 * @param value - value to write
 * @return bool - true if successful or false if not
 *
 */
template <typename T>
bool GTLibc::WritePointer(DWORD address, DWORD offset, const T &value)
{
    AddLog("WritePointer", "Trying to write to pointer at base address: " + to_hex_string(address) + " with offset: " + to_hex_string(offset) + " with value: " + std::to_string(value));
    try
    {
        DWORD pointerAddress = ReadAddress<DWORD>(address);
        return WriteAddressOffset(pointerAddress, offset, value);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WritePointer", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Read pointer from address
 * @param address - base address
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadPointer(DWORD address)
{
    const DWORD offset = 0;
    AddLog("ReadPointer", "Trying to read pointer at base address: " + to_hex_string(address) + " with offset: " + to_hex_string(offset));
    try
    {
        DWORD pointerAddress = ReadAddress<DWORD>(address);
        T returnValue = ReadPointerOffset<T>(pointerAddress, offset);
        AddLog("ReadPointer", "Return value: " + to_hex_string(returnValue));
        return returnValue;
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadPointer", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return T();
    }
    // return default value for type T
    return T();
}

/*
 * @brief Read pointer with offset
 * @param address - base address
 * @param offset - offset from pointer address
 * @return T - value at pointer address
 *
 */
template <typename T>
T GTLibc::ReadPointerOffset(DWORD address, const DWORD offset)
{
    AddLog("ReadPointerOffset", "Trying to read pointer at base address: " + to_hex_string(address) + " with offset: " + to_hex_string(offset));
    try
    {
        DWORD pointerAddress = address + offset;
        pointerAddress = ReadAddress<DWORD>(pointerAddress);
        AddLog("ReadPointerOffset", "Return value: " + to_hex_string(pointerAddress));
        return pointerAddress;
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadPointerOffset", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return T();
    }
    // return default value for type T
    return T();
}

/*
 * @brief Read pointer with offsets
 * @param address - base address
 * @param offsets - offsets from pointer address
 * @return T - value at pointer address
 *
 */

template <typename T>
T GTLibc::ReadPointerOffsets(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ReadPointerOffsets", "Trying to read pointer at base address: " + to_hex_string(address) + " with offsets size: " + std::to_string(offsets.size()));

    std::ostringstream oss;
    for (auto d : offsets)
        oss << std::hex << to_hex_string(d) << ",";

    std::string result = oss.str();
    result.pop_back();
    AddLog("ReadPointerOffsets", "Offsets: " + result);

    try
    {
        DWORD pointerAddress = address;
        for (const DWORD offsets : offsets)
        {
            pointerAddress = ReadPointerOffset<DWORD>(pointerAddress, offsets);
        }
        AddLog("ReadPointerOffsets", "Return value: " + to_hex_string(pointerAddress));
        return pointerAddress;
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadPointerOffsets", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return T();
    }
    // return default value for type T
    return T();
}

/*
 * @brief Write pointer with offset
 * @param address - base address
 * @param offset - offset from pointer address
 * @param value - value to write
 * @return bool
 *
 */
template <typename T>
bool GTLibc::WritePointerOffset(DWORD address, const std::vector<DWORD> &offsets, const T &value)
{
    AddLog("WritePointerOffset", "Trying to write to pointer at base address: " + to_hex_string(address) + " with offsets: " + std::to_string(offsets.size()) + " with value: " + std::to_string(value));
    try
    {
        DWORD pointerAddress = ReadPointerOffsets<DWORD>(address, {offsets});
        return WriteAddress(pointerAddress, value);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WritePointerOffset", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Write pointer with offsets
 * @param address - base address
 * @param offsets - offsets from pointer address
 * @param value - value to write
 * @return bool - true if write was successful
 *
 */
template <typename T>
bool GTLibc::WritePointerOffsets(DWORD address, const std::vector<DWORD> &offsets, const T &value)
{
    AddLog("WritePointerOffsets", "Trying to write to pointer at base address: " + to_hex_string(address) + " with offsets: " + std::to_string(offsets.size()) + " with value: " + std::to_string(value));
    try
    {
        DWORD pointerAddress = ReadPointerOffsets<DWORD>(address, offsets);
        return WriteAddress(pointerAddress, value);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WritePointerOffsets", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Read string from address
 * @param address - address to read from
 * @param length - length of string
 * @return string
 *
 */
std::string GTLibc::ReadString(DWORD address, size_t length)
{
    AddLog("ReadString", "Trying to read string at address: " + to_hex_string(address) + " with length: " + std::to_string(length));
    try
    {
        char *buffer = new char[length + 1];
        ReadProcessMemory(gameHandle, (LPVOID)address, buffer, length, NULL);
        buffer[length] = '\0';
        std::string result(buffer);
        delete[] buffer;
        AddLog("ReadString", "Return value: " + result);
        return result;
    }
    catch (const std::runtime_error &e)
    {
        AddLog("ReadString", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Return a default value or handle the error as needed
        return "";
    }
    // return default value for type string
    return "";
}

/*
 * @brief Write string to address
 * @param address - address to write to
 * @param value - value to write
 * @return bool - true if write was successful or false if not
 *
 */
bool GTLibc::WriteString(DWORD address, const std::string &value)
{
    AddLog("WriteString", "Trying to write string to address: " + to_hex_string(address));
    try
    {
        return WriteProcessMemory(gameHandle, (LPVOID)address, value.c_str(), value.length() + 1, NULL);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("WriteString", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
 * @brief Get the game name of current process.
 * @param None
 * @return string - game name if found or empty string if not
 *
 */

std::string GTLibc::GetGameName()
{
    AddLog("GetGameName", "Trying to get game name");
    try
    {
        // check if gameName is not empty
        if (!this->gameName.empty())
        {
            return this->gameName;
        }
        else
        {
            AddLog("GetGameName", "Error: game name is empty");
        }
    }
    catch (const std::runtime_error &e)
    {
        AddLog("GetGameName", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Handle error and return default value as needed for string
        return "";
    }
    return "";
}

/*
 * @brief Get process Id of the game
 * @param None
 * @return Process ID in DWORD
 *
 */

DWORD GTLibc::GetProcessID()
{
    AddLog("GetProcessID", "Trying to get process ID");
    try
    {
        if (this->processId != 0)
        {
            return this->processId;
        }
        else
        {
            AddLog("GetProcessID", "Error: process ID is 0");
        }
    }
    catch (const std::runtime_error &e)
    {
        AddLog("GetProcessID", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Handle error and return default value as needed
        return 0;
    }
    return 0;
}

/*
 * @brief Get the game handle
 * @param None
 * @return Game handle in HANDLE
 *
 */
HANDLE GTLibc::GetGameHandle()
{
    AddLog("GetGameHandle", "Trying to get game handle");
    try
    {
        if (this->gameHandle != nullptr)
        {
            return this->gameHandle;
        }
        else
        {
            AddLog("GetGameHandle", "Error: game handle is 0");
        }
    }
    catch (const std::runtime_error &e)
    {
        AddLog("GetGameHandle", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Handle error and return default value as needed
        return 0;
    }
    return 0;
}

/*
 * @brief Get the game base address
 * @param None
 * @return Game base address in DWORD
 *
 */
DWORD GTLibc::GetGameBaseAddress()
{
    AddLog("GetGameBaseAddress", "Trying to get game base address");
    try
    {
        MODULEENTRY32 module;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, this->processId);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            AddLog("GetGameBaseAddress", "Error: snapshot is invalid");
            return 0;
        }
        module.dwSize = sizeof(MODULEENTRY32);
        if (!Module32First(snapshot, &module))
        {
            AddLog("GetGameBaseAddress", "Error: module32first failed");
            CloseHandle(snapshot);
            return 0;
        }

        else
        {
            module.dwSize = sizeof(MODULEENTRY32);
            Module32First(snapshot, &module);
            CloseHandle(snapshot);
            uintptr_t modBaseAddr = reinterpret_cast<uintptr_t>(module.modBaseAddr);
            this->gameBaseAddress = static_cast<DWORD>(modBaseAddr);
            AddLog("GetGameBaseAddress", "Return value: " + to_hex_string(this->gameBaseAddress));
        }
        return this->gameBaseAddress;
    }
    catch (const std::runtime_error &e)
    {
        AddLog("GetGameBaseAddress", "Error: " + std::string(e.what()));
        ShowError(e.what());
        // Handle error and return default value as needed
        return 0;
    }
    return 0;
}

/*
@brief Get the game handle from game window handle.
@param Game window handle
@return Game handle in HANDLE
*/

HANDLE GTLibc::GetGameHandle4mHWND(HWND hwnd)
{
    AddLog("GetGameHandle4mHWND", "Trying to get game handle");
    DWORD processId;
    if (!GetWindowThreadProcessId(hwnd, &processId))
    {
        AddLog("GetGameHandle4mHWND", "Error: GetWindowThreadProcessId failed");
        return NULL;
    }
    HANDLE gameHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

    if (!gameHandle)
    {
        AddLog("GetGameHandle4mHWND", "Error: OpenProcess failed");
        return NULL;
    }
    else
    {
        AddLog("GetGameHandle4mHWND", "Return value: " + to_hex_string(gameHandle));
        this->gameHandle = gameHandle;
    }
    return gameHandle;
}

/*
@brief Get the process ID from the game window handle
@param Game window handle
@return Process ID in DWORD
*/

DWORD GTLibc::GetProcessID4mHWND(HWND hwnd)
{
    AddLog("GetProcessID4mHWND", "Trying to get process ID");
    DWORD processId;

    if (!GetWindowThreadProcessId(hwnd, &processId))
    {
        AddLog("GetProcessID4mHWND", "Error: GetWindowThreadProcessId failed");
        return 0;
    }
    else
    {
        AddLog("GetProcessID4mHWND", "Return value: " + to_hex_string(processId));
        this->processId = processId;
    }

    return processId;
}

/*
@brief Check hotkeys are pressed
@param Vector of hotkeys - Ex : {VK_CTRL,VK_SHIFT,VK_F1}
@return bool - true if all hotkeys are pressed else false
*/

bool GTLibc::HotKeysDown(const std::vector<int> &keys)
{
    short result;

    result = GetAsyncKeyState(keys[0]);
    for (auto &key : keys)
        result &= GetAsyncKeyState(key);

    return ((result & 0x8000) != 0);
}
/*
 * @brief Checks if Hotkey is pressed
 * @param keycode
 * @return bool
 *
 */
bool GTLibc::IsKeyPressed(int keycode)
{
    try
    {
        return (GetAsyncKeyState(keycode) & 0x8000);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("IsKeyPressed", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
}
/*
 * @brief Checks if Hotkey is toggled
 * @param keycode
 * @return bool
 *
 */
bool GTLibc::IsKeyToggled(int keycode)
{
    try
    {
        return (GetAsyncKeyState(keycode) & 0x1);
    }
    catch (const std::runtime_error &e)
    {
        AddLog("IsKeyToggled", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
}

/*
--------------------------------------------------------------------------------
------------------------- CheatTable Public Functions --------------------------
--------------------------------------------------------------------------------
*/

#ifdef GT_USE_CE_PARSER
/*
 * @brief Read cheat table from file
 * @param cheatTableFile - File path
 * @param entries - Number of entries in the cheat table
 * @return CheatTable
 *
 */
CheatTable GTLibc::ReadCheatTable(const std::string &cheatTableFile, int entries)
{
    std::ifstream ifs(cheatTableFile);
    std::string cheatTableData((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

    if (cheatTableData.empty())
    {
        AddLog("ReadCheatTable", "Error: Could not read file: " + cheatTableFile);
        ShowError("Could not read file: " + cheatTableFile);
        return CheatTable();
    }

    else if (IsValidCheatTable(cheatTableData))
    {
        AddLog("ReadCheatTable", "Successfully read file: " + cheatTableFile);
        // Show Error if gameBaseAddress is not set and is trying to read cheat table.
        if (gameBaseAddress == 0)
        {
            AddLog("ReadCheatTable", "Error: GameBaseAddress is invalid, Try finding the game using FindGameProcess() first.");
            ShowError("GameBaseAddress is invalid, Try finding the game using FindGameProcess() first.");
        }
        else
        {
            g_CheatTable.SetGameBaseAddress(gameBaseAddress);
            g_CheatTable = g_CheatTable.ParseCheatTable(cheatTableData, entries);
            return g_CheatTable;
        }
    }

    return CheatTable();
}

/*
@brief Execute the cheat table and run it as Generic trainer.
@param None
@return None
*/

void GTLibc::ExecuteCheatTable()
{
    // 1. Resolve the address and values.
    for (auto &entry : g_CheatTable.cheatEntries)
    {
        // Resolving the address with offsets.
        std::vector<DWORD> offsets = entry->Offsets;
        std::reverse(offsets.begin(), offsets.end());
        AddLog("ExecuteCheatTable", "Trying to resolve address for cheat entry: " + entry->Description);
        DWORD address = 0;

        // Update the address in the cheat table.
        if (entry->Address != 0)
        {
            address = ResolveAddressGeneric(entry->Address, offsets);
            entry->Address = address;
        }
        else
        {
            AddLog("ExecuteCheatTable", "Failed to resolve address for cheat entry: " + entry->Description);
            continue;
        }

        // Resolving the value and Hotkeys Ids.
        if (entry->Hotkeys.size() > 0)
        {
            std::string cheatActionValue = std::get<2>(entry->Hotkeys[0]);
            std::string cheatAction = std::get<0>(entry->Hotkeys[0]);
            // Resolving the Hotkeys Ids.
            entry->HotkeyIds = std::get<1>(entry->Hotkeys[0]);

            //  Check if cheatActionValue is not empty string.
            if (!cheatActionValue.empty())
            {
                DataType cheatValue = ConvertStringToDataType(cheatActionValue);

                // Update the value and action in the cheat table.
                entry->Value = cheatValue;
            }
            else
            {
                AddLog("ExecuteCheatTable", "Failed to resolve value for cheat entry: " + entry->Description + " with action: " + cheatAction);
            }

            if (!cheatAction.empty())
            {
                entry->Action = cheatAction;
            }
            else
            {
                AddLog("ExecuteCheatTable", "Failed to resolve action for cheat entry: " + entry->Description);
            }

            AddLog("ExecuteCheatTable", "Resolved address: " + to_hex_string(address) + " for cheat entry: " + entry->Description);
        }
        else
        {
            AddLog("ExecuteCheatTable", "Failed to resolve hotkeys for cheat entry: " + entry->Description);
        }
    }

    // 2. Refine the Cheat table with unresolved entries.
    g_CheatTable.cheatEntries.erase(std::remove_if(g_CheatTable.cheatEntries.begin(), g_CheatTable.cheatEntries.end(), [](auto &entry)
                                                   { return entry->HotkeyIds.size() == 0; }),
                                    g_CheatTable.cheatEntries.end());

    // 3. Print Cheat table menu.
    PrintCheatTableMenu();

    // 4. Execute the cheat table.
    while (true)
    {
        for (auto &entry : g_CheatTable.cheatEntries)
        {
            if (HotKeysDown(entry->HotkeyIds))
            {
                ExecuteCheatActionForType(entry->Action, entry->Address, entry->Value, entry->VariableType);
            }
            if (IsKeyToggled(VK_F5))
            {
                break;
            }
        }
        // sleep for 100 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/*
 * @brief Add Cheat Entry to cheat table
 * @param description - Description of the cheat entry
 * @param dataType - Data type of the cheat entry
 * @param address - Address of the cheat entry
 * @param offsets - Offsets of the cheat entry
 * @param hotkeys - Hotkeys of the cheat entry
 * @param hotkeyAction - Hotkey action of the cheat entry
 * @param hotkeyValue - Hotkey value of the cheat entry
 * @return void
 *
 */
void GTLibc::AddCheatTableEntry(const std::string &description, const std::string &dataType, const DWORD address, const std::vector<DWORD> &offsets, const std::vector<int> &hotkeys, const std::string &hotkeyAction, const std::string hotkeyValue)
{
    int id = g_CheatTable.cheatEntries.size();
    const HOTKEYS hotkey = {make_tuple(hotkeyAction, hotkeys, hotkeyValue, 0)};
    g_CheatTable.AddCheatEntry(description, id, dataType, address, offsets, hotkey);
}

/*
 * @brief Activate cheat table entries from cheat table
 * @param cheatEntryIds - Vector of cheat entry IDs
 * @return void
 *
 */

void GTLibc::ActivateCheatTableEntries(const std::vector<int> &cheatEntryIds)
{
    g_CheatTable.cheatEntries.erase(
        std::remove_if(g_CheatTable.cheatEntries.begin(), g_CheatTable.cheatEntries.end(),
                       [&](const auto &entry)
                       {
                           return std::none_of(cheatEntryIds.begin(), cheatEntryIds.end(),
                                               [&](const auto &index)
                                               { return g_CheatTable.cheatEntries[index] == entry; });
                       }),
        g_CheatTable.cheatEntries.end());
}

/*
 * @brief Print current cheat table values.
 * @return void
 *
 */

void GTLibc::PrintCheatTable()
{
    for (auto &entry : g_CheatTable.cheatEntries)
    {
        std::cout << "Description: " << entry->Description << std::endl;
        std::cout << "ID: " << entry->Id << std::endl;
        std::cout << "VariableType: " << entry->VariableType << std::endl;
        std::cout << "Address: " << entry->Address << std::endl;
        std::cout << "Offsets: ";
        for (auto &offset : entry->Offsets)
        {
            std::cout << offset << " ";
        }
        std::cout << std::endl;
        std::cout << "Hotkeys: " << std::endl;
        for (auto &hotkey : entry->Hotkeys)
        {
            std::cout << "  Action: " << std::get<0>(hotkey) << std::endl;
            std::cout << "  Keys: [";
            for (auto &key : std::get<1>(hotkey))
            {
                std::cout << KeyCodeToName(key) << " ";
            }
            std::cout << "]" << std::endl;
            std::cout << "  Value: " << std::get<2>(hotkey) << std::endl;
            std::cout << "  ID: " << std::get<3>(hotkey) << std::endl;
        }
        std::cout << std::endl;
    }
}

/*
@brief Read all cheat table entries and print their values.
@return void
*/

void GTLibc::ReadCheatTableEntries()
{
    for (auto &entry : g_CheatTable.cheatEntries)
    {
        const DWORD address = entry->Address;
        const std::vector<DWORD> offsets = entry->Offsets;

        std::vector<DWORD> offsetsSorted = offsets;
        std::reverse(offsetsSorted.begin(), offsetsSorted.end());

        if (offsets.size() >= 1)
        {
            std::cout << "Description: " << entry->Description;
            std::cout << " Address: " << to_hex_string(address);
            std::cout << " Offsets: ";
            for (auto &offset : offsetsSorted)
            {
                std::cout << to_hex_string(offset) << ",";
            }

            auto result = ReadAddressGeneric(entry->VariableType, address, offsetsSorted);
            PrintCheatValue(result);
        }

        if (offsets.size() == 0 && address != 0)
        {
            std::cout << "Description: " << entry->Description << " ";
            auto result = ReadAddressGeneric(entry->VariableType, address);
            PrintCheatValue(result);
        }
    }
}

/*
--------------------------------------------------------------------------------
------------------ CheatTable Private Functions --------------------------------
--------------------------------------------------------------------------------
*/

void GTLibc::PrintCheatTableMenu()
{
    int cheatIndex = 1;

    // Loop through all the cheat entries.
    std::cout << "Index. "
              << "\t"
              << "Description"
              << "\t"
              << "Action"
              << "\t"
              << "Hotkeys" << std::endl;

    for (auto &entry : g_CheatTable.cheatEntries)
    {
        // Print the description.
        std::cout << cheatIndex << ". \t" << entry->Description << "\t";

        // Loop through all the hotkeys.
        for (auto &hotkey : entry->Hotkeys)
        {
            std::cout << " " << std::get<0>(hotkey) << "\t";
            std::cout << " [";
            for (auto &key : std::get<1>(hotkey))
            {
                std::cout << KeyCodeToName(key) << " ";
            }
            std::cout << "] ";
        }
        std::cout << "\n"
                  << std::endl;
        cheatIndex++;
    }
}

template <typename T>
void GTLibc::CheatAction_SetValue(DWORD address, T value)
{
    AddLog("CheatAction_SetValue", "trying to write value: " + ValueToString(value) + " at address: " + to_hex_string(address) + " of type: " + GetDataTypeInfo(value));
    if constexpr (std::is_same_v<T, std::string>)
    {
        WriteString(address, value);
    }
    else
    {
        WriteAddress<T>(address, value);
    }
}

template <typename T>
void GTLibc::CheatAction_IncreaseValue(DWORD address, T value)
{
    AddLog("CheatAction_IncreaseValue", "trying to write value: " + ValueToString(value) + " at address: " + to_hex_string(address) + " of type: " + GetDataTypeInfo(value));

    if constexpr (std::is_same_v<T, std::string>)
    {
        std::string currentValue = ReadString(address, value.length());
        WriteString(address, currentValue + value);
    }
    else
    {
        T currentValue = ReadAddress<T>(address);
        WriteAddress<T>(address, currentValue + value);
    }
}

template <typename T>
void GTLibc::CheatAction_DecreaseValue(DWORD address, T value)
{
    AddLog("CheatAction_DecreaseValue", "trying to write value: " + ValueToString(value) + " at address: " + to_hex_string(address) + " of type: " + GetDataTypeInfo(value));

    if constexpr (std::is_same_v<T, std::string>)
    {
        std::string currentValue = ReadString(address, value.length());
        WriteString(address, currentValue - value);
    }
    else
    {
        T currentValue = ReadAddress<T>(address);
        WriteAddress<T>(address, currentValue - value);
    }
}

template <>
void GTLibc::CheatAction_DecreaseValue<std::string>(DWORD address, std::string value)
{
    throw std::runtime_error("Decrease action is not supported for strings");
}

template <typename T>
void GTLibc::ExecuteCheatAction(const std::string &cheatAction, DWORD &address, const T &value)
{
    if (cheatAction == CheatActions.SetValue)
    {
        CheatAction_SetValue<T>(address, value);
    }
    else if (cheatAction == CheatActions.IncreaseValue)
    {
        CheatAction_IncreaseValue<T>(address, value);
    }
    else if (cheatAction == CheatActions.DecreaseValue)
    {
        CheatAction_DecreaseValue<T>(address, value);
    }
}

template <typename T>
void GTLibc::ExecuteCheatActionType(const std::string &cheatAction, DWORD &address, const std::string &valueStr)
{
    AddLog("ExecuteCheatActionTypeHelper", "Trying to execute cheat action: " + cheatAction + " at address: " + to_hex_string(address) + " with value: " + valueStr);
    T value;
    std::istringstream iss(valueStr);

    if (iss >> value)
    {
        ExecuteCheatAction<T>(cheatAction, address, value);
    }
}

void GTLibc::ExecuteCheatActionType(const std::string &cheatAction, DWORD &address, const std::string &value, const std::string &variableType)
{
    AddLog("ExecuteCheatActionType", "Trying to execute cheat action: " + cheatAction + " at address: " + to_hex_string(address) + " with value: " + value + " of type: " + variableType);

    const std::map<std::string, std::function<void(const std::string &, DWORD &, const std::string &)>> typeMap =
        {
            {CheatTypes.Byte, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<std::uint8_t>(action, addr, val); }},
            {CheatTypes.Short, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<std::uint16_t>(action, addr, val); }},
            {CheatTypes.Integer, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<std::uint32_t>(action, addr, val); }},
            {CheatTypes.Long, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<std::uint64_t>(action, addr, val); }},
            {CheatTypes.Float, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<float>(action, addr, val); }},
            {CheatTypes.Double, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<double>(action, addr, val); }},
            {CheatTypes.String, [&](const std::string &action, DWORD &addr, const std::string &val)
             { ExecuteCheatActionType<std::string>(action, addr, val); }}};

    auto iter = typeMap.find(variableType);
    if (iter != typeMap.end())
    {
        iter->second(cheatAction, address, value);
    }
    else
    {
        // Handle invalid variableType.
        AddLog("ExecuteCheatActionType", "Invalid variable type: " + variableType);
    }
}

// Helper function to convert any type to string.
template <typename T>
auto ValueToString(const T &value)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_string(value);
    }
    else
    {
        return value;
    }
}

void GTLibc::ExecuteCheatActionForType(const string &cheatAction, DWORD &address, DataType &value, const string &variableType)
{
    AddLog("ExecuteCheatActionForType", "Trying to execute action: " + cheatAction + " at address: " + to_hex_string(address) + " of type: " + variableType);
    try
    {
        std::visit([this, &cheatAction, &address, &variableType](auto &&arg)
                   {
                       auto valueStr = ValueToString(arg);
                       ExecuteCheatActionType(cheatAction, address, valueStr, variableType); },
                   value);
    }
    catch (const std::exception &e)
    {
        AddLog("ExecuteCheatActionForType", "Exception: " + std::string(e.what()));
    }
}

// Check if cheat table is valid XML check for tags.
bool GTLibc::IsValidCheatTable(const std::string &xmlData)
{
    std::string_view xmlDataView = xmlData;
    if (xmlDataView.find("<CheatEntries>") == std::string::npos)
    {
        return false;
    }
    if (xmlDataView.find("</CheatEntries>") == std::string::npos)
    {
        return false;
    }
    return true;
}

DataType GTLibc::ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsets)
{
    static const std::map<std::string, std::function<DataType(DWORD, const std::vector<DWORD> &)>> typeMap =
        {
            {CheatTypes.Byte, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<std::uint8_t>(addr) : ReadAddress<std::uint8_t>(ResolveAddressGeneric(addr, offs)); }},
            {CheatTypes.Short, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<std::uint16_t>(addr) : ReadAddress<std::uint16_t>(ResolveAddressGeneric(addr, offs)); }},
            {CheatTypes.Integer, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<std::uint32_t>(addr) : ReadAddress<std::uint32_t>(ResolveAddressGeneric(addr, offs)); }},
            {CheatTypes.Long, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<std::uint64_t>(addr) : ReadAddress<std::uint64_t>(ResolveAddressGeneric(addr, offs)); }},
            {CheatTypes.Float, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<float>(addr) : ReadAddress<float>(ResolveAddressGeneric(addr, offs)); }},
            {CheatTypes.Double, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<double>(addr) : ReadAddress<double>(ResolveAddressGeneric(addr, offs)); }},
            {CheatTypes.String, [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? std::string(ReadString(addr, MAX_PATH)) : std::string(ReadString(ResolveAddressGeneric(addr, offs), MAX_PATH)); }},
        };

    const auto it = typeMap.find(dataType);
    if (it == typeMap.end())
    {
        AddLog("ReadAddressGeneric", "Invalid data type specified: " + dataType);
        return {};
    }
    return it->second(address, offsets);
}

DWORD GTLibc::ResolveAddressGeneric(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ResolveAddressGeneric", "Parameters: " + to_hex_string(address) + " " + to_hex_string(gameBaseAddress));

    if (offsets.size() == 0)
    {
        AddLog("ResolveAddressGeneric", "No offsets provided, returning address: " + to_hex_string(address));
        return address;
    }

    DWORD staticAddress = address - gameBaseAddress;
    AddLog("ResolveAddressGeneric", "Static address: " + to_hex_string(staticAddress));
    DWORD result = ReadPointerOffset<DWORD>(gameBaseAddress, staticAddress);
    AddLog("ResolveAddressGeneric", "Startred resolving address: " + to_hex_string(result));

    if (offsets.size() > 1)
    {
        for (size_t i = 0; i < offsets.size() - 1; ++i)
        {
            result = ReadPointerOffset<DWORD>(result, offsets[i]);
        }
    }

    // Add the last offset to the result
    result += offsets.back();
    AddLog("ResolveAddressGeneric", "Resolved address: " + to_hex_string(result));
    return result;
}
#endif

/*
--------------------------------------------------------------------------------
------------------ GTLibc Private Functions ------------------------------------
--------------------------------------------------------------------------------
*/

void GTLibc::AddLog(const std::string &methodName, const std::string &message)
{
    if (!enableLogs)
        return;

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%T") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << methodName << "] " << message;

    std::ofstream ofs(logFile, std::ios_base::out | std::ios_base::app);
    ofs << ss.str() << std::endl;
    ofs.close();
}

void GTLibc::EnableLogs(bool status)
{
    AddLog("EnableLogs", "Trying to enable logs");
    enableLogs = status;
}

void GTLibc::ShowError(const std::string &errorMessage)
{
    MessageBox(NULL, errorMessage.c_str(), "ERROR!", MB_ICONERROR);
}

void GTLibc::ShowWarning(const std::string &warningMessage)
{
    MessageBox(NULL, warningMessage.c_str(), "WARNING!", MB_ICONWARNING);
}

void GTLibc::ShowInfo(const std::string &infoMessage)
{
    MessageBox(NULL, infoMessage.c_str(), "INFO!", MB_ICONINFORMATION);
}

// Method to Execute system commands.
std::string GTLibc::ShellExec(const std::string &cmdArgs, bool runAsAdmin, bool waitForExit, const std::string &shell)
{
    std::string command;
    std::array<char, 128> buffer;
    std::string result;

#if defined(__linux__)
    if (runAsAdmin)
    {
        command = "sudo " + shell + " -c \"" + cmdArgs + "\"";
    }
    else
    {
        command = shell + " -c \"" + cmdArgs + "\"";
    }
#elif defined(_WIN32) || defined(_WIN64)
    command = shell + " /c " + cmdArgs;
    if (runAsAdmin)
    {
        SHELLEXECUTEINFO shExInfo = {0};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = 0;
        shExInfo.lpVerb = ("runas");
        shExInfo.lpFile = (shell.c_str());
        shExInfo.lpParameters = (cmdArgs.c_str());
        shExInfo.lpDirectory = 0;
        shExInfo.nShow = SW_HIDE;
        shExInfo.hInstApp = 0;
        if (!ShellExecuteEx(&shExInfo))
        {
            throw std::system_error(GetLastError(), std::system_category(), "Error executing command as admin.");
        }
        if (waitForExit)
        {
            WaitForSingleObject(shExInfo.hProcess, INFINITE);
        }
        CloseHandle(shExInfo.hProcess);
        return "";
    }
#endif

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

template <typename T>
std::string GTLibc::GetDataTypeInfo(T type)
{
    auto name = typeid(type).name();
    std::string cmd_str = "echo '" + std::string(name) + "' | c++filt -t";
    std::string result = ShellExec(cmd_str);
    return result;
}

bool GTLibc::CheckGameTrainerArch()
{
    SYSTEM_INFO siCurrent = {};
    SYSTEM_INFO siRemote = {};
    GetNativeSystemInfo(&siCurrent);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(this->gameHandle));
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 me = {};
        me.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &me))
        {
            siRemote.wProcessorArchitecture = (me.modBaseAddr >= (LPBYTE)0x80000000) ? PROCESSOR_ARCHITECTURE_AMD64 : PROCESSOR_ARCHITECTURE_INTEL;
        }
        CloseHandle(hSnapshot);
    }

    if (siCurrent.wProcessorArchitecture != siRemote.wProcessorArchitecture)
    {
        if (siCurrent.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL && siRemote.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            AddLog("CheckGameTrainerArch", "The Trainer is 32-bit and the game is 64-bit.");
            ShowError("The Trainer is 32-bit and the game is 64-bit.");
            std::exit(EXIT_FAILURE);
        }
        else
        {
            AddLog("CheckGameTrainerArch", "Trainer has architecture value of " + GetArchitectureString(siCurrent.wProcessorArchitecture) + " and game has architecture value of " + GetArchitectureString(siRemote.wProcessorArchitecture));
            return false;
        }
    }
    else
    {
        AddLog("CheckGameTrainerArch", "Trainer has architecture value of " + GetArchitectureString(siCurrent.wProcessorArchitecture) + " and game has architecture value of " + GetArchitectureString(siRemote.wProcessorArchitecture));
        return true;
    }
}

// Helper function that converts the wProcessorArchitecture value to a string representation.
std::string GTLibc::GetArchitectureString(WORD wProcessorArchitecture)
{
    switch (wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    case PROCESSOR_ARCHITECTURE_ARM:
        return "ARM";
    case PROCESSOR_ARCHITECTURE_IA64:
        return "IA-64";
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    case PROCESSOR_ARCHITECTURE_ALPHA64:
        return "Alpha64";
    case PROCESSOR_ARCHITECTURE_SHX:
        return "SHX";
    case PROCESSOR_ARCHITECTURE_MIPS:
        return "MIPS";
    case PROCESSOR_ARCHITECTURE_PPC:
        return "PPC";
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
        return "unknown";
    }
}

// Method to convert single keycode to Key name.
std::string GTLibc::KeyCodeToName(int keyCode)
{
    // Map the key code to a scan code.
    UINT scanCode = MapVirtualKey(keyCode, MAPVK_VK_TO_VSC);
    if (scanCode == 0)
        return std::string();

    // Map the scan code to a key name.
    char keyName[256];
    int result = GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName));
    if (result == 0)
        return std::string();

    return std::string(keyName);
}

// Method to convert multiple keycode to Key name.
std::string GTLibc::GetHotKeysName(const std::vector<int> &keys)
{
    std::string hotkeysName = "";
    for (auto &key : keys)
    {
        hotkeysName += KeyCodeToName(key) + " ";
    }
    return hotkeysName;
}

DataType GTLibc::ConvertStringToDataType(const std::string &str)
{
    if (auto value = TryParse<std::uint8_t>(str))
        return *value;
    if (auto value = TryParse<std::uint16_t>(str))
        return *value;
    if (auto value = TryParse<std::uint32_t>(str))
        return *value;
    if (auto value = TryParse<std::uint64_t>(str))
        return *value;
    if (auto value = TryParse<float>(str))
        return *value;
    if (auto value = TryParse<double>(str))
        return *value;
    if (auto value = TryParse<std::string>(str))
        return *value;

    else
    {
        std::cout << "Unknown data type: " << str << std::endl;
        AddLog("ConvertStringToDataType", "Unknown data type: " + str);
        return {};
    }
}

template <typename T>
std::optional<T> GTLibc::TryParse(const std::string &str)
{
    std::stringstream ss(str);
    T value;
    if (ss >> value)
    {
        if (ss.eof())
        {
            return value;
        }
    }
    return {};
}

template <typename T>
std::string GTLibc::ValueToString(const T &value)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_string(value);
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        return value;
    }
    else
    {
        // Handle other types if necessary, or throw an exception if unsupported type
        throw std::invalid_argument("Unsupported data type for value_to_string.");
    }
}

void GTLibc::PrintCheatValue(const DataType &value)
{
    std::visit([](const auto &item)
               { std::cout << "Value: " << item << std::endl; },
               value);
}
