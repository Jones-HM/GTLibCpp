// including the header files and GTLIbc class

// Define this macro to use the CE parser.
#define GT_USE_CE_PARSER

#include "GTLibc.hpp"

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
        gtlibc.DisplayCheatTable();
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
        // Trainer with default values.
        gtlibc.ExecuteCheatTable();

        // Exit the trainer with the key 'X' and Index, Description, Hotkeys set to true.
        //gtlibc.ExecuteCheatTable(true,'X',true,true,false,true);

        // Exit the trainer with F5 and Description and Hotkeys set to true.
        // gtlibc.ExecuteCheatTable(true, VK_F5, false, true, false, true);

        // Exit the trainer with Default Key and Description,Action and Hotkeys set to true.
        // gtlibc.ExecuteCheatTable(false, EXIT_TRAINER_KEY, false, true, true, true);
    }
}

int main()
{
    // Finding the game process
    gtlibc.EnableLogs(true);
    std::string gameName = "ac_client";
    auto gameHandle = gtlibc.FindGameProcess(gameName);

     std::cout << "Game found with name and id : " << gameName << " " << gtlibc.GetProcessId() << std::endl;


    // Selecting the cheat table file.
    std::string cheatTableFile = "CheatTable/assaultcube.ct";

    // Generate the cheat table demo with read and execute.
    GenerateGenericTrainer(cheatTableFile, false, false, true, false, {});

    return 0;
}
