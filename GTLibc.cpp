#include "GTLibc.h"
using namespace GTLIBC;

GTLibc::GTLibc()
{
    logFile = "GTLibc.log";
    enableLogs = false;
    g_GTLibc = this;
}

GTLibc::GTLibc(bool enableLogs)
{
    logFile = "GTLibc.log";
    this->enableLogs = enableLogs;
    g_GTLibc = this;
}

GTLibc::GTLibc(const std::string &gameName)
{
    logFile = "GTLibc.log";
    enableLogs = false;
    FindGameProcess(gameName);
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
 * @brief Find game process by name
 * @param gameName
 * @return bool
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
        AddLog("FindGameProcess", "Snapshot handle: " + to_hex_str(hSnapshot));

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
                AddLog("FindGameProcess", "Game process id: " + std::to_string(processId));
                AddLog("FindGameProcess", "Game process handle: " + to_hex_str(gameHandle));
                AddLog("FindGameProcess", "Game window handle: " + to_hex_str(gameWindow));

                return true;
            }
        } while (Process32Next(hSnapshot, &pe));

        std::string errMsg = "Game process not found";
        AddLog("FindGameProcess", "Error: " + errMsg);
        ShowError(errMsg.c_str());
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
 * @param windowName
 * @return HWND
 *
 */

HWND GTLibc::FindGameWindow(const std::string &windowName)
{
    AddLog("FindGameWindow", "Trying to find game window: " + windowName);
    return FindWindowA(NULL, windowName.c_str());
}

/*
 * @brief Read address with offset
 * @param address
 * @param offset
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadAddressOffset(DWORD address, DWORD offset)
{
    AddLog("ReadAddressOffset", "Trying to read address with offset: " + to_hex_str(address) + " + " + to_hex_str(offset));
    try
    {
        T value;
        SIZE_T bytesRead;
        DWORD newAddress = address + offset;
        if (ReadProcessMemory(gameHandle, (LPVOID)newAddress, &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T))
        {
            AddLog("ReadAddressOffset", "Read value: " + to_hex_str(value));
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
 * @param address
 * @param offsets
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadAddressOffsets(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ReadAddressOffsets", "Trying to read address with offsets: " + to_hex_str(address));

    // Add log all the values of the offsets
    std::ostringstream oss;
    for (auto d : offsets)
        oss << to_hex_str(d) << ",";

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
 * @param address
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadAddress(DWORD address)
{
    AddLog("ReadAddress", "Trying to read address: " + to_hex_str(address));
    try
    {
        T value;
        SIZE_T bytesRead;
        if (ReadProcessMemory(gameHandle, (LPVOID)address, &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T))
        {
            AddLog("ReadAddress", "Read value: " + to_hex_str(value));
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
 * @param address
 * @param value
 * @return bool
 *
 */

template <typename T>
bool GTLibc::WriteAddress(DWORD address, const T &value)
{
    AddLog("WriteAddress", "Trying to write to address: " + to_hex_str(address) + " with value: " + std::to_string(value));
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
 * @param address
 * @param offset
 * @param value
 * @return bool
 *
 */

template <typename T>
bool GTLibc::WriteAddressOffset(DWORD address, DWORD offset, const T &value)
{
    AddLog("WriteAddressOffset", "Trying to write to address: " + to_hex_str(address) + " with offset: " + to_hex_str(offset) + " with value: " + std::to_string(value));
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
 * @param address
 * @param offsets
 * @param value
 * @return bool
 *
 */

template <typename T>
bool GTLibc::WriteAddressOffsets(DWORD address, const std::vector<DWORD> &offsets, const T &value)
{
    AddLog("WriteAddressOffsets", "Trying to write to address: " + to_hex_str(address) + " with offsets: " + std::to_string(offsets.size()) + " with value: " + std::to_string(value));
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
 * @brief Read pointer
 * @param address
 * @return T
 *
 */
template <typename T>
bool GTLibc::WritePointer(DWORD address, DWORD offset, const T &value)
{
    AddLog("WritePointer", "Trying to write to pointer at base address: " + to_hex_str(address) + " with offset: " + to_hex_str(offset) + " with value: " + std::to_string(value));
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
 * @brief Read pointer
 * @param address
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadPointer(DWORD address)
{
    const DWORD offset = 0;
    AddLog("ReadPointer", "Trying to read pointer at base address: " + to_hex_str(address) + " with offset: " + std::to_string(offset));
    try
    {
        DWORD pointerAddress = ReadAddress<DWORD>(address);
        T returnValue = ReadPointerOffset<T>(pointerAddress, offset);
        AddLog("ReadPointer", "Return value: " + to_hex_str(returnValue));
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
 * @param address and offset
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadPointerOffset(DWORD address, const DWORD offset)
{
    AddLog("ReadPointerOffset", "Trying to read pointer at base address: " + to_hex_str(address) + " with offset: " + to_hex_str(offset));
    try
    {
        DWORD pointerAddress = address + offset;
        pointerAddress = ReadAddress<DWORD>(pointerAddress);
        AddLog("ReadPointerOffset", "Return value: " + to_hex_str(pointerAddress));
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
 * @param address and offsets
 * @return T
 *
 */

template <typename T>
T GTLibc::ReadPointerOffsets(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ReadPointerOffsets", "Trying to read pointer at base address: " + to_hex_str(address) + " with offsets size: " + std::to_string(offsets.size()));
    // Add log the values of offsets iterate all items in offsetsList
    std::ostringstream oss;
    for (auto d : offsets)
        oss << std::hex << to_hex_str(d) << ",";

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
        AddLog("ReadPointerOffsets", "Return value: " + to_hex_str(pointerAddress));
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
 * @param address and offset
 * @return bool
 *
 */
template <typename T>
bool GTLibc::WritePointerOffset(DWORD address, const std::vector<DWORD> &offsets, const T &value)
{
    AddLog("WritePointerOffset", "Trying to write to pointer at base address: " + to_hex_str(address) + " with offsets: " + std::to_string(offsets.size()) + " with value: " + std::to_string(value));
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
 * @param address and offsets
 * @return bool
 *
 */
template <typename T>
bool GTLibc::WritePointerOffsets(DWORD address, const std::vector<DWORD> &offsetsList, const T &value)
{
    AddLog("WritePointerOffsets", "Trying to write to pointer at base address: " + to_hex_str(address) + " with offsets: " + std::to_string(offsetsList.size()) + " with value: " + std::to_string(value));
    try
    {
        DWORD pointerAddress = ReadPointerOffsets<DWORD>(address, offsetsList);
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
 * @brief Read string
 * @param address and length
 * @return string
 *
 */
std::string GTLibc::ReadString(DWORD address, size_t length)
{
    AddLog("ReadString", "Trying to read string at address: " + std::to_string(address) + " with length: " + std::to_string(length));
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
 * @brief Write string
 * @param address and value
 * @return bool
 *
 */
bool GTLibc::WriteString(DWORD address, const std::string &value)
{
    AddLog("WriteString", "Trying to write string to address: " + std::to_string(address));
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
 * @brief Get the game name
 * @param None
 * @return string
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
 * @brief Get process ID of the game
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
            AddLog("GetGameBaseAddress", "Return value: " + std::to_string(this->gameBaseAddress));
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
 * @brief Checks if Hotkey is down
 * @param None
 * @return bool
 *
 */

bool GTLibc::HotKeysDown(int virtualKeyCode)
{
    AddLog("HotKeysDown", "Checking if hotkey with virtual key code: " + std::to_string(virtualKeyCode) + " is down");
    try
    {
        return (GetAsyncKeyState(virtualKeyCode) & 0x8000) != 0;
    }
    catch (const std::runtime_error &e)
    {
        AddLog("HotKeysDown", "Error: " + std::string(e.what()));
        ShowError(e.what());
        return false;
    }
    return false;
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

DWORD GTLibc::ReadPointerOffsetsUntilLast(DWORD address, const std::vector<DWORD> &offsetsList)
{
    DWORD staticAddress = address - gameBaseAddress;
    DWORD result = ReadPointerOffset<DWORD>(gameBaseAddress, staticAddress);

    if (offsetsList.size() > 1)
    {
        for (size_t i = 0; i < offsetsList.size() - 1; ++i)
        {
            result = ReadPointerOffset<DWORD>(result, offsetsList[i]);
        }
    }

    // Add the last offset to the result
    result += offsetsList.back();
    return result;
}

DataType GTLibc::ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList)
{
    static const std::unordered_map<std::string, std::function<DataType(DWORD, const std::vector<DWORD> &)>> typeMap =
        {
            {"Byte", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<BYTE>(addr) : ReadAddress<BYTE>(ReadPointerOffsetsUntilLast(addr, offs)); }},
            {"2 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int16_t>(addr) : ReadAddress<int16_t>(ReadPointerOffsetsUntilLast(addr, offs)); }},
            {"4 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int32_t>(addr) : ReadAddress<int32_t>(ReadPointerOffsetsUntilLast(addr, offs)); }},
            {"8 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int64_t>(addr) : ReadAddress<int64_t>(ReadPointerOffsetsUntilLast(addr, offs)); }},
            {"Float", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<float>(addr) : ReadAddress<float>(ReadPointerOffsetsUntilLast(addr, offs)); }},
            {"Double", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<double>(addr) : ReadAddress<double>(ReadPointerOffsetsUntilLast(addr, offs)); }},
            {"String", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? std::string(ReadString(addr, 0xFF)) : std::string(ReadString(ReadPointerOffsetsUntilLast(addr, offs), 0xFF)); }},
        };

    const auto it = typeMap.find(dataType);
    if (it == typeMap.end())
    {
        throw std::runtime_error("Invalid data type specified");
    }
    return it->second(address, offsetsList);
}

void GTLibc::PrintValue(const DataType &value)
{
    std::visit([](const auto &item)
               { std::cout << "Value: " << item << std::endl; },
               value);
}

CheatEntries GTLibc::ReadCheatTable(const std::string &filename)
{
    std::ifstream ifs(filename);
    std::string cheatTableData((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    if (cheatTableData.empty())
    {
        AddLog("ReadCheatTable", "Error: Could not read file: " + filename);
        ShowError("Could not read file: " + filename);
    }

    else if (IsValidCheatTable(cheatTableData))
    {
        AddLog("ReadCheatTable", "Successfully read file: " + filename);
        // Show Error if gameBaseAddress is not set and is trying to read cheat table.
        if (gameBaseAddress == 0)
        {
            AddLog("ReadCheatTable", "Error: GameBaseAddress is invalid, Try finding the game using FindGameProcess() first.");
            ShowError("GameBaseAddress is invalid, Try finding the game using FindGameProcess() first.");
        }
        else
        {
            CheatEntries cheatEntries(this->gameBaseAddress);
            return cheatEntries.ParseCheatTable(cheatTableData);
        }
    }

    return CheatEntries();
}

void GTLibc::PrintCheatTable(CheatEntries &cheatEntries)
{
    for (auto &entry : cheatEntries.entries)
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
        std::cout << "Hotkeys: ";
        for (auto &hotkey : entry->Hotkeys)
        {
            std::cout << "[";
            for (auto &key : hotkey)
            {
                std::cout << key << " ";
            }
            std::cout << "] ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
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

/*
    Read the generic table and print the results.
*/
void GTLibc::ReadCheatTableEntries(CheatEntries &cheatEntries)
{
    for (auto &entry : cheatEntries.entries)
    {
        const DWORD address = entry->Address;
        const vector<DWORD> offsets = entry->Offsets;

        vector<DWORD> offsetsSorted = offsets;
        std::reverse(offsetsSorted.begin(), offsetsSorted.end());

        if (offsets.size() >= 1)
        {
            std::cout << "Description: " << entry->Description;
            std::cout << " Address: " << to_hex_str(address);
            std::cout << " Offsets: ";
            for (auto &offset : offsetsSorted)
            {
                std::cout << to_hex_str(offset) << ",";
            }

            auto result = ReadAddressGeneric(entry->VariableType, address, offsetsSorted);
            PrintValue(result);
        }

        if (offsets.size() == 0 && address != 0)
        {
            std::cout << "Description: " << entry->Description << " ";
            DataType result = ReadAddressGeneric(entry->VariableType, address);
            PrintValue(result);
        }
    }
}

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