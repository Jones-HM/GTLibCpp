/*
Brief: This shows how to use the GTLibc to generate a generic trainer using Cheat Table.
Using GTLibc and CEParser together, you can generate a generic trainer and execute it.
Author: HeavenHM
Date: 24/04/2023
*/

// Define this to use CEParser. - Cheat Engine Parser.
#define GT_USE_CE_PARSER

// Include the GTLibc header file.
#include "GTLibc.hpp"

using namespace GTLIBC;
GTLibc gtlibc;

// Generates a generic trainer using Cheat Table.
void GenerateGenericTrainer(std::string &cheatTableFile, bool printCheat = false, bool readCheat = false, bool executeCheat = false, bool activateCheat = false, std::vector<int> cheatIds = {})
{
    // Read the cheat table file
    CheatTable cheatTable = gtlibc.ReadCheatTable(cheatTableFile);

    // Check if the cheat table is empty.
    if (cheatTable.IsEmpty())
        return;

    // Add the cheat entry for Armor - Manual Entry.
    gtlibc.AddCheatTableEntry("Armor", CheatTypes.Integer, 0x07290BC8, {}, {VK_CONTROL, 'M'}, CheatActions.SetValue, "150");

    // Add the cheat entry for Ammo with offsets and hotkey Control + F1 with Action freeze. - Manual Entry.
    // gtlibc.AddCheatTableEntry("Ammo", CheatTypes.Float, 0x07290BC, {0x4F,0x23,0x400}, {VK_CONTROL, VK_F1}, CheatActions.FreezeValue, "1000.0");

    // Print the cheat table
    if (printCheat)
    {
        // Display cheat table with default values.
        gtlibc.DisplayCheatTable();

        // Display cheat table with Index, Description, Hotkeys set to true.
        // gtlibc.DisplayCheatTable(true, true, false, true, false);

        // Display cheat table with Index, Description, Action and Hotkeys set to true.
        // gtlibc.DisplayCheatTable(true, true, true, true, false);
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

        // Exit the trainer with the key 'X' and Print Index, Description, Hotkeys set to true.
        // gtlibc.ExecuteCheatTable(true,'X',true,true,false,true);

        // Exit the trainer with F5 and Description and Print Hotkeys set to true.
        // gtlibc.ExecuteCheatTable(true, VK_F5, false, true, false, true);

        // Exit the trainer with Default Key and Print Description,Action and Hotkeys set to true.
        // gtlibc.ExecuteCheatTable(false, EXIT_TRAINER_KEY, false, true, true, true);
    }
}

int main()
{
    // Instantiate the GTLibc and Enable the logs.
    gtlibc.EnableLogs(true);

    std::string gameName = "ac_client";
    // Finding the game process
    auto gameHandle = gtlibc.FindGameProcess(gameName);

    //std::cout << "Game found: " << gameName << " PId: " << gtlibc.GetProcessId() << std::endl;

    // Selecting the cheat table file.
    std::string cheatTableFile = "CheatTable/assaultcube.ct";

    // Generate the cheat table demo with read and execute.
    GenerateGenericTrainer(cheatTableFile, false, false, true, false, {});

    return 0;
}
