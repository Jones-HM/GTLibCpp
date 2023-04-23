// including the header files and GTLIbc class

// Define this macro to use the CE parser.
#define GT_USE_CE_PARSER

#include "GTLibc.h"

using namespace GTLIBC;
GTLibc gtlibc;

// Generates a generic trainer using Cheat Table.
void GenerateGenericTrainer(std::string &cheatTableFile, bool printCheat = false, bool readCheat = false, bool executeCheat = false, bool activateCheat = false, std::vector<int> cheatIds = {})
{
    // Read the cheat table file
    gtlibc.ReadCheatTable(cheatTableFile);

    // Add the cheat entry for Armor
    gtlibc.AddCheatTableEntry("Armor", CheatTypes.Integer, 0x07290BC8, {}, {VK_CONTROL,'M'}, CheatActions.SetValue, "150");

    // Print the cheat table
    if (printCheat)
    {
        gtlibc.PrintCheatTable();
    }

    // Read the cheat table entries.
    if (readCheat)
    {
        gtlibc.ReadCheatTableEntries();
    }

    // Activate the cheat entries.
    if (activateCheat)
    {
        gtlibc.ActivateCheatTableEntries(cheatIds);
    }

    // Execute the cheat Table
    if (executeCheat)
    {
        // Read the cheat table entries.
        // gtlibc.ReadCheatTableEntries();
        gtlibc.ExecuteCheatTable();
    }
}

int main()
{
    // Finding the game process
    gtlibc.EnableLogs(true);
    std::string gameName = "ac_client.exe";
    auto gameHandle = gtlibc.FindGameProcess(gameName);

    std::cout << "Game found with Id " << gtlibc.GetProcessID() << " Name: " << gtlibc.GetGameName() << " Base Address: " << gtlibc.GetGameBaseAddress() << " Handle: " << gtlibc.GetGameHandle() << std::endl;

    // Selecting the cheat table file.
    std::string cheatTableFile = "CheatTable/assaultcube.ct";

    // Generate the cheat table demo with read and execute.
    GenerateGenericTrainer(cheatTableFile, false, false, true, false, {});

    return 0;
}
