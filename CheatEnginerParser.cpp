#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <memory>
#include <cstdint>
#include <variant>
#include <stdexcept>
#include <unordered_map>
#include "GTLibc.cpp"


GTLibc gtlibc(true);

using namespace std;

class CheatEntry
{
public:
    string Description;
    int Id;
    string VariableType;
    DWORD Address;
    vector<DWORD> Offsets;
    vector<vector<DWORD>> Hotkeys;
    vector<shared_ptr<CheatEntry>> NestedEntries;

    CheatEntry(const string &description, int id, const string &variableType, DWORD address,
               const vector<DWORD> &offsets, const vector<vector<DWORD>> &hotkeys)
        : Description(description), Id(id), VariableType(variableType), Address(address),
          Offsets(offsets), Hotkeys(hotkeys) {}
};

class CheatEntries
{
public:
    vector<shared_ptr<CheatEntry>> entries;

    void addEntry(shared_ptr<CheatEntry> entry)
    {
        entries.push_back(entry);
    }
};

DWORD getBaseAddress(const string &moduleName)
{
    return gtlibc.GetGameBaseAddress();
}

DWORD parseAddress(const string &address)
{
    smatch matches;
    regex_search(address, matches, regex("(\"([^\"]+)\")?([^+]+)?\\s*(\\+)?\\s*(0x)?([0-9A-Fa-f]+)?"));
    string moduleName = matches[2].str().empty() ? matches[3].str() : matches[2].str();
    string offsetStr = matches[6].str();

    bool isModuleNameHex = std::all_of(moduleName.begin(), moduleName.end(), [](char c)
                                       { return std::isxdigit(c); });

    if (isModuleNameHex)
    {
        DWORD absoluteAddress = stoul(moduleName, nullptr, 16);
        //std::cout << "Address: " << address << " Absolute address: " << to_hex_str(absoluteAddress) << std::endl;
        return absoluteAddress;
    }

    DWORD offset = offsetStr.empty() ? 0 : stoul(offsetStr, nullptr, 16);
    DWORD baseAddress = moduleName.empty() ? 0 : getBaseAddress(moduleName);
    //std::cout << "Address: " << address << " Base address: " << to_hex_str(baseAddress) << " Module name: " << moduleName << " offset: " << offsetStr << std::endl;
    return baseAddress + offset;
}

vector<DWORD> parseOffsets(const string &offsets)
{
    vector<DWORD> result;
    smatch matches;
    regex offsetRegex("<Offset>([0-9a-fA-F]+)</Offset>");

    auto offsetsBegin = sregex_iterator(offsets.begin(), offsets.end(), offsetRegex);
    auto offsetsEnd = sregex_iterator();

    for (auto i = offsetsBegin; i != offsetsEnd; ++i)
    {
        result.push_back(stoul((*i)[1].str(), nullptr, 16));
    }

    return result;
}

vector<vector<DWORD>> parseHotkeys(const string &hotkeys)
{
    vector<vector<DWORD>> result;
    smatch matches;
    regex hotkeyRegex("<Hotkey>([\\s\\S]*?)<Keys>([\\s\\S]*?)</Keys>([\\s\\S]*?)</Hotkey>");

    auto hotkeysBegin = sregex_iterator(hotkeys.begin(), hotkeys.end(), hotkeyRegex);
    auto hotkeysEnd = sregex_iterator();

    for (auto i = hotkeysBegin; i != hotkeysEnd; ++i)
    {
        vector<DWORD> keys;
        string keysStr = (*i)[2].str();
        // trim the keysStr.
        keysStr = keysStr.substr(1, keysStr.size() - 2);

        smatch keyMatches;
        regex keyRegex("<Key>([0-9]+)</Key>");

        auto keysBegin = sregex_iterator(keysStr.begin(), keysStr.end(), keyRegex);
        auto keysEnd = sregex_iterator();

        for (auto j = keysBegin; j != keysEnd; ++j)
        {
            keys.push_back(stoul((*j)[1].str()));
        }
        result.push_back(keys);
    }
    return result;
}

void parseNestedCheatEntries(const string &parentNode, shared_ptr<CheatEntry> &parentEntry)
{
    smatch entryMatches;
    regex entryRegex("<CheatEntry>.*?</CheatEntry>");
    auto entriesBegin = sregex_iterator(parentNode.begin(), parentNode.end(), entryRegex);
    auto entriesEnd = sregex_iterator();

    for (auto i = entriesBegin; i != entriesEnd; ++i)
    {
        string entryStr = (*i).str();
        smatch matches;

        regex_search(entryStr, matches, regex("<Description>(.*?)</Description>"));
        string description = matches[1].str();

        regex_search(entryStr, matches, regex("<ID>(\\d+)</ID>"));
        int id = stoi(matches[1].str());

        regex_search(entryStr, matches, regex("<VariableType>(.*?)</VariableType>"));
        string variableType = matches[1].str();

        regex_search(entryStr, matches, regex("<Address>(.*?)</Address>"));
        DWORD address = variableType == "Auto Assembler Script" ? 0 :parseAddress(matches[1].str());

        vector<DWORD> offsets = parseOffsets(entryStr);
        vector<vector<DWORD>> hotkeys = parseHotkeys(entryStr);

        shared_ptr<CheatEntry> entry = make_shared<CheatEntry>(description, id, variableType, address, offsets, hotkeys);
        parentEntry->NestedEntries.push_back(entry);

        parseNestedCheatEntries(entryStr, entry);
    }
}

CheatEntries parseCheatEntries(const string &xmlData)
{
    CheatEntries result;
    smatch entryMatches;
    regex entryRegex("<CheatEntry>([\\s\\S]*?)</CheatEntry>");
    auto entriesBegin = sregex_iterator(xmlData.begin(), xmlData.end(), entryRegex);
    auto entriesEnd = sregex_iterator();

    for (auto i = entriesBegin; i != entriesEnd; ++i)
    {
        string entryStr = (*i).str();
        smatch matches;

        regex_search(entryStr, matches, regex("<Description>(.*?)</Description>"));
        string description = matches[1].str();

        regex_search(entryStr, matches, regex("<ID>(\\d+)</ID>"));
        int id = stoi(matches[1].str());

        regex_search(entryStr, matches, regex("<VariableType>(.*?)</VariableType>"));
        string variableType = matches[1].str();

        regex_search(entryStr, matches, regex("<Address>(.*?)</Address>"));
        DWORD address = variableType == "Auto Assembler Script" ? 0 :parseAddress(matches[1].str());

        vector<DWORD> offsets = parseOffsets(entryStr);
        vector<vector<DWORD>> hotkeys = parseHotkeys(entryStr);
        shared_ptr<CheatEntry> entry = make_shared<CheatEntry>(description, id, variableType, address, offsets, hotkeys);

        result.addEntry(entry);

        parseNestedCheatEntries(entryStr, entry);
    }

    return result;
}

using DataType = std::variant<BYTE, int16_t, int32_t, int64_t, float, double, std::string>;

template <typename T>
T ReadMemoryAddress(DWORD address)
{
    return gtlibc.ReadAddress<T>(address);
}

DataType ReadMemoryAddress(const std::string& dataType, DWORD address)
{
    static const std::unordered_map<std::string, std::function<DataType(DWORD)>> typeMap =
    {
        { "Byte",   &ReadMemoryAddress<BYTE> },
        { "2 Bytes",&ReadMemoryAddress<int16_t> },
        { "4 Bytes",&ReadMemoryAddress<int32_t> },
        { "8 Bytes",&ReadMemoryAddress<int64_t> },
        { "Float",  &ReadMemoryAddress<float> },
        { "Double", &ReadMemoryAddress<double> },
        { "String", [](DWORD addr) { return std::string(gtlibc.ReadString(addr, 0xFF)); } }
    };

    const auto it = typeMap.find(dataType);
    if (it == typeMap.end())
    {
        throw std::runtime_error("Invalid data type specified");
    }
    return it->second(address);
}

std::string readFile(const std::string &filename)
{
    std::ifstream ifs(filename);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    return content;
}

void PrintValue(const DataType &value)
{
    std::visit([](const auto &item)
               { std::cout << item << std::endl; },
               value);
}

int main()
{

    // Read the XML data from a file or any other source
    string xmlData = readFile("IGI.ct");
    gtlibc.FindGameProcess("igi");

    // Parse the XML data and populate the cheat entries
    CheatEntries cheatEntries = parseCheatEntries(xmlData);

    // Print the cheat entries using std::cout

    for (auto &entry : cheatEntries.entries)
    {

        const DWORD address = entry->Address;
        const vector<DWORD> offsets = entry->Offsets;

        // if (offsets.size() > 1)
        // {
        //     // Copy offsets vector and remove the last element
        //     vector<DWORD> offsetsCopy = offsets;
        //     offsetsCopy.pop_back();

        //     auto data = gtLibc.ReadPointerOffsets<DWORD>(address, offsetsCopy) + offsets[offsets.size() - 1]];
        //     std::cout << "Data: " << data << std::endl;
        // }
        // else if (offsets.size() == 1)
        // {
        //     auto data = gtLibc.ReadPointerOffset<DWORD>(address, offsets[0]);
        //     std::cout << "Data: " << data << std::endl;
        // }
        if (offsets.size() == 0 && address != 0)
        {
            std::cout << "Description: " << entry->Description << std::endl;
            DataType result = ReadMemoryAddress(entry->VariableType, address);
            PrintValue(result);
        }
    }

    return 0;
}
