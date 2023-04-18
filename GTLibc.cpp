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
                AddLog("FindGameProcess", "Game process id: " + std::to_string(processId));
                AddLog("FindGameProcess", "Game process handle: " + to_hex_string(gameHandle));
                AddLog("FindGameProcess", "Game window handle: " + to_hex_string(gameWindow));

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
 * @param address
 * @param offsets
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadAddressOffsets(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ReadAddressOffsets", "Trying to read address with offsets: " + to_hex_string(address));

    // Add log all the values of the offsets
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
 * @param address
 * @return T
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
 * @param address
 * @param value
 * @return bool
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
 * @param address
 * @param offset
 * @param value
 * @return bool
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
 * @param address
 * @param offsets
 * @param value
 * @return bool
 *
 */

template <typename T>
bool GTLibc::WriteAddressOffsets(DWORD address, const std::vector<DWORD> &offsets, const T &value)
{
    AddLog("WriteAddressOffsets", "Trying to write to address: " + to_hex_string(address) + " with offsets: " + std::to_string(offsets.size()) + " with value: " + std::to_string(value));
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
 * @brief Read pointer
 * @param address
 * @return T
 *
 */
template <typename T>
T GTLibc::ReadPointer(DWORD address)
{
    const DWORD offset = 0;
    AddLog("ReadPointer", "Trying to read pointer at base address: " + to_hex_string(address) + " with offset: " + std::to_string(offset));
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
 * @param address and offset
 * @return T
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
 * @param address and offsets
 * @return T
 *
 */

template <typename T>
T GTLibc::ReadPointerOffsets(DWORD address, const std::vector<DWORD> &offsets)
{
    AddLog("ReadPointerOffsets", "Trying to read pointer at base address: " + to_hex_string(address) + " with offsets size: " + std::to_string(offsets.size()));
    // Add log the values of offsets iterate all items in offsetsList
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
 * @param address and offset
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
 * @param address and offsets
 * @return bool
 *
 */
template <typename T>
bool GTLibc::WritePointerOffsets(DWORD address, const std::vector<DWORD> &offsetsList, const T &value)
{
    AddLog("WritePointerOffsets", "Trying to write to pointer at base address: " + to_hex_string(address) + " with offsets: " + std::to_string(offsetsList.size()) + " with value: " + std::to_string(value));
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

/**
 * INFO : Hot keys can be in combination of like GT_HotKeysDown(LCTRL,VK_F1) or GT_HotKeysDown(LSHIFT,'F')
 * @description - check for Hot keys combinations pressed or not.
 * @param - Combination of hot keys using virtual key_codes or characters A-Z,a-z.
 * @return - If keys pressed it will return TRUE otherwise returns FALSE.
 * PS : Don't use this method directly instead use 'GT_HotKeysPressed' MACRO.
 */

bool GTLibc::HotKeysDown(INT key, ...)
{
    short result;
    va_list list;

    result = GetAsyncKeyState(key);
    for (va_start(list, key); key; key = va_arg(list, INT))
        result &= GetAsyncKeyState(key);

    va_end(list);

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

DWORD GTLibc::ResolveAddressGeneric(DWORD address, const std::vector<DWORD> &offsetsList)
{
    // std::cout << "Init parameters: " << std::hex << address << " " << to_hex_string(gameBaseAddress) << std::endl;

    if (offsetsList.size() == 0)
    {
        return address;
    }

    DWORD staticAddress = address - gameBaseAddress;
    // std::cout << "Static address: " << std::hex << staticAddress << std::endl;
    DWORD result = ReadPointerOffset<DWORD>(gameBaseAddress, staticAddress);
    // std::cout << "Startred resolving address: " << std::hex << result << std::endl;

    if (offsetsList.size() > 1)
    {
        for (size_t i = 0; i < offsetsList.size() - 1; ++i)
        {
            result = ReadPointerOffset<DWORD>(result, offsetsList[i]);
        }
    }

    // Add the last offset to the result
    result += offsetsList.back();
    // std::cout << "Resolved address: " << std::hex << result << std::endl;
    return result;
}

template <typename T>
struct Visitor
{
    template <typename U>
    std::enable_if_t<std::is_arithmetic_v<U>, T> operator()(const U &arg) const { return static_cast<T>(arg); }

    T operator()(const std::string &arg) const
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return arg;
        }
        else
        {
            throw std::runtime_error("Invalid conversion from std::string to non-string type");
        }
    }
};

DataType GTLibc::ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList)
{
    static const std::unordered_map<std::string, std::function<DataType(DWORD, const std::vector<DWORD> &)>> typeMap =
        {
            {"Byte", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<BYTE>(addr) : ReadAddress<BYTE>(ResolveAddressGeneric(addr, offs)); }},
            {"2 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int16_t>(addr) : ReadAddress<int16_t>(ResolveAddressGeneric(addr, offs)); }},
            {"4 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int32_t>(addr) : ReadAddress<int32_t>(ResolveAddressGeneric(addr, offs)); }},
            {"8 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int64_t>(addr) : ReadAddress<int64_t>(ResolveAddressGeneric(addr, offs)); }},
            {"Float", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<float>(addr) : ReadAddress<float>(ResolveAddressGeneric(addr, offs)); }},
            {"Double", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<double>(addr) : ReadAddress<double>(ResolveAddressGeneric(addr, offs)); }},
            {"String", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? std::string(ReadString(addr, 0xFF)) : std::string(ReadString(ResolveAddressGeneric(addr, offs), 0xFF)); }},
        };

    const auto it = typeMap.find(dataType);
    if (it == typeMap.end())
    {
        throw std::runtime_error("Invalid data type specified");
    }
    return it->second(address, offsetsList);
}

template <typename T>
T GTLibc::ReadAddressGeneric(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList)
{
    static const std::unordered_map<std::string, std::function<DataType(DWORD, const std::vector<DWORD> &)>> typeMap =
        {
            {"Byte", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<BYTE>(addr) : ReadAddress<BYTE>(ResolveAddressGeneric(addr, offs)); }},
            {"2 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int16_t>(addr) : ReadAddress<int16_t>(ResolveAddressGeneric(addr, offs)); }},
            {"4 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int32_t>(addr) : ReadAddress<int32_t>(ResolveAddressGeneric(addr, offs)); }},
            {"8 Bytes", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<int64_t>(addr) : ReadAddress<int64_t>(ResolveAddressGeneric(addr, offs)); }},
            {"Float", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<float>(addr) : ReadAddress<float>(ResolveAddressGeneric(addr, offs)); }},
            {"Double", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? ReadAddress<double>(addr) : ReadAddress<double>(ResolveAddressGeneric(addr, offs)); }},
            {"String", [this](DWORD addr, const std::vector<DWORD> &offs)
             { return offs.empty() ? std::string(ReadString(addr, 0xFF)) : std::string(ReadString(ResolveAddressGeneric(addr, offs), 0xFF)); }},
        };

    const auto it = typeMap.find(dataType);
    if (it == typeMap.end())
    {
        throw std::runtime_error("Invalid data type specified");
    }

    DataType data = it->second(address, offsetsList);
    return std::visit(Visitor<T>{}, data);
}

DataType GTLibc::ReadAddressGenericWrapper(const std::string &dataType, DWORD address, const std::vector<DWORD> &offsetsList)
{
    if (dataType == CheatTypes.Byte)
    {
        return ReadAddressGeneric<BYTE>(dataType, address, offsetsList);
    }
    else if (dataType == CheatTypes.Short)
    {
        return ReadAddressGeneric<int16_t>(dataType, address, offsetsList);
    }
    else if (dataType == CheatTypes.Integer)
    {
        return ReadAddressGeneric<int32_t>(dataType, address, offsetsList);
    }
    else if (dataType == CheatTypes.Long)
    {
        return ReadAddressGeneric<int64_t>(dataType, address, offsetsList);
    }
    else if (dataType == CheatTypes.Float)
    {
        return ReadAddressGeneric<float>(dataType, address, offsetsList);
    }
    else if (dataType == CheatTypes.Double)
    {
        return ReadAddressGeneric<double>(dataType, address, offsetsList);
    }
    // else if (dataType == CheatTypes.String)
    // {
    //     return ReadAddressGeneric<std::string>(dataType, address, offsetsList);
    // }
    else
    {
        throw std::runtime_error("Invalid data type specified");
    }
}

void GTLibc::PrintValue(const DataType &value)
{
    std::visit([](const auto &item)
               { std::cout << "Value: " << item << std::endl; },
               value);
}

// Create a method to convert keycodes to Key names
std::string KeyCodeToName(int keyCode)
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

CheatTable GTLibc::ReadCheatTable(const std::string &cheatTableFile, int entries = -1)
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

template <typename T>
void GTLibc::AddCheatEntry(const string &description, const string &dataType, const DWORD address,
                           const vector<DWORD> &offsets, const std::vector<int> &hotkeys, const std::string &hotkeyAction,
                           T hotkeyValue)
{
    int id = g_CheatTable.cheatEntries.size();
    const HOTKEYS hotkey = {make_tuple(hotkeyAction, hotkeys, to_string(hotkeyValue), 0)};
    g_CheatTable.AddCheatEntry(description, id, dataType, address, offsets, hotkey);
}

void GTLibc::ActivateCheatEntries(const std::vector<int> &cheatEntryIds)
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

template <typename T>
void PrintDataType(T type)
{
    auto name = typeid(type).name();
    string cmd_str = "echo '" + string(name) + "' | c++filt -t";
    system(cmd_str.c_str());
}

template <typename T>
void GTLibc::CheatAction_SetValue(DWORD address, T value)
{
    std::cout << "CheatAction_SetValue: trying to write value: " << value << " at address: " << to_hex_string(address) << std::endl;
    PrintDataType(value);
    WriteAddress<T>(address, value);
}

template <typename T>
void GTLibc::CheatAction_IncreaseValue(DWORD address, T value)
{
    std::cout << "CheatAction_IncreaseValue: trying to write value: " << value << " at address: " << to_hex_string(address) << std::endl;
    PrintDataType(value);
    T currentValue = ReadAddress<T>(address);
    WriteAddress<T>(address, currentValue + value);
}

template <typename T>
void GTLibc::CheatAction_DecreaseValue(DWORD address, T value)
{
    std::cout << "CheatAction_DecreaseValue: trying to write value: " << value << " at address: " << to_hex_string(address) << std::endl;
    PrintDataType(value);
    T currentValue = ReadAddress<T>(address);
    WriteAddress<T>(address, currentValue - value);
}

template <>
void GTLibc::CheatAction_DecreaseValue<std::string>(DWORD address, std::string value)
{
    throw std::runtime_error("Decrease action is not supported for strings");
}

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

// Now create method called ExecuteCheatAction which takes parameter as DataType and CheatAction struct and based on value of this it will call relevant method.

void GTLibc::ExecuteCheatAction(string &cheatAction, const DWORD &address, DataType &value)
{
    std::cout << "ExecuteCheatAction: trying to execute action: " << cheatAction << " at address: " << to_hex_string(address) << std::endl;
    // Check if type of value is string datatype and if it is then throw error. check compile time.
    if (std::holds_alternative<std::string>(value))
    {
        throw std::runtime_error("ExecuteCheatAction is not supported for strings");
    }

    if (cheatAction == CheatActions.SetValue)
    {
        std::visit([this, address](auto &&val)
                   {
                    using T = std::decay_t<decltype(val)>;
                    CheatAction_SetValue<T>(address, val); },
                   value);
    }
    else if (cheatAction == CheatActions.IncreaseValue)
    {
        std::visit([this, address](auto &&val)
                   {
                using T = std::decay_t<decltype(val)>;
                CheatAction_IncreaseValue<T>(address, val); },
                   value);
    }
    else if (cheatAction == CheatActions.DecreaseValue)
    {
        std::visit([this, address](auto &&val)
                   {
                    using T = std::decay_t<decltype(val)>;
                    CheatAction_DecreaseValue<T>(address, val); },
                   value);
    }
}

template <typename T>
void GTLibc::ExecuteCheatAction(string &cheatAction, const DWORD &address, T &value)
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

// define ConvertStringToDataType which will take string and convert it to actual data type based on the content of string.
DataType ConvertStringToDataType(const std::string &str)
{
    std::stringstream ss(str);

    // Check for integer
    int int_val;
    if (ss >> int_val)
    {
        if (ss.eof())
        {
            return int_val;
        }
        // If not at the end of the string, reset the stringstream
        ss.clear();
        ss.seekg(0);
    }

    // Check for double
    double double_val;
    if (ss >> double_val)
    {
        if (ss.eof())
        {
            return double_val;
        }
        // If not at the end of the string, reset the stringstream
        ss.clear();
        ss.seekg(0);
    }

    // If neither int nor double, return as string
    return str;
}

// Define method AreHotKeysPressed with parameter as vector of keys in int format and return bool.
bool AreHotKeysPressed(const vector<int> &keys)
{
    short result;

    result = GetAsyncKeyState(keys[0]);
    for (auto &key : keys)
        result &= GetAsyncKeyState(key);

    return ((result & 0x8000) != 0);
}

// Create method that returns Keysname using KeyCodeToName method and takes parameter as vector of keys in int format.
string GetHotKeysName(const vector<int> &keys)
{
    string hotkeysName = "";
    for (auto &key : keys)
    {
        hotkeysName += KeyCodeToName(key) + " ";
    }
    return hotkeysName;
}

// Execture the CheatTable.
void GTLibc::ExecuteCheatTable()
{
    // 1. Print Cheat table menu.
    PrintCheatTableMenu();

    // 2. Resolve the address and values.
    for (auto &entry : g_CheatTable.cheatEntries)
    {
        vector<DWORD> offsetsSorted = entry->Offsets;
        std::reverse(offsetsSorted.begin(), offsetsSorted.end());
        DWORD address = ResolveAddressGeneric(entry->Address, offsetsSorted);

        string cheatActionValue = std::get<2>(entry->Hotkeys[0]);
        string cheatAction = std::get<0>(entry->Hotkeys[0]);
        DataType convertedValue = ConvertStringToDataType(cheatActionValue);

        // Resolve the Hotkeys Ids.
        entry->HotkeyIds = std::get<1>(entry->Hotkeys[0]);

        // Update the value in the cheat table.
        entry->Address = address;
        entry->Value = convertedValue;
        entry->CheatActionStr = cheatAction;
    }

    // 3. Execute the cheat table.
    while (true)
    {
        for (auto &entry : g_CheatTable.cheatEntries)
        {
            //std::cout << "Hotkeys:" << GetHotKeysName(entry->HotkeyIds) << std::endl;

            if (AreHotKeysPressed(entry->HotkeyIds))
            {
                ExecuteCheatAction(entry->CheatActionStr, entry->Address, entry->Value);
            }
            // sleep for 100 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // sleep for 100 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

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
void GTLibc::ReadCheatTableEntries()
{
    for (auto &entry : g_CheatTable.cheatEntries)
    {
        const DWORD address = entry->Address;
        const vector<DWORD> offsets = entry->Offsets;

        vector<DWORD> offsetsSorted = offsets;
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
            PrintValue(result);
        }

        if (offsets.size() == 0 && address != 0)
        {
            std::cout << "Description: " << entry->Description << " ";
            auto result = ReadAddressGeneric(entry->VariableType, address);
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