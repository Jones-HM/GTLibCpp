#pragma once
#ifndef CE_PARSER_H
#define CE_PARSER_H

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
#include <algorithm>
#include <ranges>
#include <tuple>

using namespace std;
typedef vector<tuple<string, vector<DWORD>, string, int>> HOTKEY;

namespace GTLIBC
{
    // CEParser.h

    // Define HotkeyAction struct
    struct CheatAction
    {
        string SetValue;
        string IncreaseValue;
        string DecreaseValue;
        string Freeze;
        string Unfreeze;

        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };

    // Define CheatDataType struct
    struct CheatType
    {
        string Byte;
        string Short;
        string Integer;
        string Long;
        string Float;
        string Double;
        string String;
    };
    CheatAction CheatActions = {"Set Value", "Increase Value", "Decrease Value", "Freeze", "Unfreeze"};
    CheatType CheatTypes = {"Byte", "2 Bytes", "4 Bytes", "8 Bytes", "Float", "Double", "String"};

    class CheatEntry
    {
    public:
        string Description;
        int Id;
        string VariableType;
        DWORD Address;
        vector<DWORD> Offsets;
        HOTKEY Hotkeys;
        vector<shared_ptr<CheatEntry>> NestedEntries;

        CheatEntry(const string &description, int id, const string &variableType, DWORD address,
                   const vector<DWORD> &offsets, const HOTKEY &hotkeys)
            : Description(description), Id(id), VariableType(variableType), Address(address),
              Offsets(offsets), Hotkeys(hotkeys) {}
    };

    class CheatTable
    {
    public:
        // Create default constructor and parameterized constructor with setting game base address.
        CheatTable() = default;
        CheatTable(DWORD gameBaseAddress) : gameBaseAddress(gameBaseAddress) {}

        vector<shared_ptr<CheatEntry>> cheatEntries;

        void SetGameBaseAddress(DWORD gameBaseAddress)
        {
            this->gameBaseAddress = gameBaseAddress;
        }

        CheatTable ParseCheatTable(const string &cheatTablePath,int entries);
        void AddCheatEntry(shared_ptr<CheatEntry> entry);
        void AddCheatEntry(const string &description, int id, const string &dataType, const DWORD address,
                           const vector<DWORD> &offsets, const HOTKEY &hotkeys);
    private:
        // Create variable and method to get base address of the game.
        DWORD gameBaseAddress = 0x00400000;
        DWORD getGameBaseAddress()
        {
            return gameBaseAddress;
        }

        DWORD ParseAddress(const string &address);
        vector<DWORD> ParseOffsets(const string &offsets);
        vector<tuple<string, vector<DWORD>, string, int>> ParseHotkeys(const string &hotkeys);
        void ParseNestedCheatEntries(const string &parentNode, shared_ptr<CheatEntry> &parentEntry);
    };

    void CheatTable::AddCheatEntry(shared_ptr<CheatEntry> entry)
    {
        cheatEntries.push_back(entry);
    }

    void CheatTable::AddCheatEntry(const string &description, int id, const string &dataType, const DWORD address,
                                   const vector<DWORD> &offsets, const HOTKEY &hotkeys)
    {
        // Create a cheat entry object and pass the parameters to it.
        auto entry = make_shared<CheatEntry>(description, id, dataType, address, offsets, hotkeys);

        // Add the cheat entry to the cheat table.
        AddCheatEntry(entry);
    }

    DWORD CheatTable::ParseAddress(const string &address)
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
            return absoluteAddress;
        }

        DWORD offset = offsetStr.empty() ? 0 : stoul(offsetStr, nullptr, 16);
        DWORD baseAddress = moduleName.empty() ? 0 : gameBaseAddress;
        return baseAddress + offset;
    }

    vector<DWORD> CheatTable::ParseOffsets(const string &offsets)
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

    vector<tuple<string, vector<DWORD>, string, int>> CheatTable::ParseHotkeys(const string &hotkeys)
    {
        vector<tuple<string, vector<DWORD>, string, int>> result;
        smatch matches;
        regex hotkeyRegex("<Hotkey>([\\s\\S]*?)<Action>([\\s\\S]*?)</Action>([\\s\\S]*?)<Keys>([\\s\\S]*?)</Keys>([\\s\\S]*?)<Value>([\\s\\S]*?)</Value>([\\s\\S]*?)<ID>([\\s\\S]*?)</ID>([\\s\\S]*?)</Hotkey>");

        auto hotkeysBegin = sregex_iterator(hotkeys.begin(), hotkeys.end(), hotkeyRegex);
        auto hotkeysEnd = sregex_iterator();

        for (auto i = hotkeysBegin; i != hotkeysEnd; ++i)
        {
            string action = (*i)[2].str();
            vector<DWORD> keys;
            string keysStr = (*i)[4].str();
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

            string value = (*i)[6].str();
            int id = stoi((*i)[8].str());
            result.push_back(make_tuple(action, keys, value, id));
        }
        return result;
    }

    void CheatTable::ParseNestedCheatEntries(const string &parentNode, shared_ptr<CheatEntry> &parentEntry)
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
            DWORD address = variableType == "Auto Assembler Script" ? 0 : ParseAddress(matches[1].str());

            vector<DWORD> offsets = ParseOffsets(entryStr);
            auto hotkeys = ParseHotkeys(entryStr);

            shared_ptr<CheatEntry> entry = make_shared<CheatEntry>(description, id, variableType, address, offsets, hotkeys);
            parentEntry->NestedEntries.push_back(entry);

            ParseNestedCheatEntries(entryStr, entry);
        }
    }

    CheatTable CheatTable::ParseCheatTable(const string &cheatTablePath,int entries=-1)
    {
        CheatTable cheatTable;
        smatch entryMatches;
        regex entryRegex("<CheatEntry>([\\s\\S]*?)</CheatEntry>");
        auto entriesBegin = sregex_iterator(cheatTablePath.begin(), cheatTablePath.end(), entryRegex);
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
            DWORD address = variableType == "Auto Assembler Script" ? 0 : ParseAddress(matches[1].str());

            vector<DWORD> offsets = ParseOffsets(entryStr);
            auto hotkeys = ParseHotkeys(entryStr);
            shared_ptr<CheatEntry> entry = make_shared<CheatEntry>(description, id, variableType, address, offsets, hotkeys);

            cheatTable.AddCheatEntry(entry);
            ParseNestedCheatEntries(entryStr, entry);

            if (entries > 0 && cheatTable.cheatEntries.size() >= entries)
            {
                break;
            }
        }

        return cheatTable;
    }
} // namespace GTLibc

#endif