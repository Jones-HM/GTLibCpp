// including the header files and GTLIbc class

// Define this macro to use the CE parser.
#define GT_USE_CE_PARSER

#include "GTLibc.h"

using namespace GTLIBC;
GTLibc gtlibc;

// define the base address of the game
int GAME_BASE_ADDRESS = 0x00400000;

// Generates a generic trainer using Cheat Table.
void GenerateGenericTrainer(std::string &cheatTableFile, bool printCheat = false, bool executeCheat = false)
{
    // Read the cheat table file
    gtlibc.ReadCheatTable(cheatTableFile);

    // Print the cheat table
    if (printCheat)
    {
        gtlibc.PrintCheatTable();
    }

    // Activate the cheat entries.
    std::vector<int> cheatIds = {0, 1, 2, 7, 8, 9};
    gtlibc.ActivateCheatEntries(cheatIds);

    // Execute the cheat Table
    if (executeCheat)
    {
        gtlibc.ExecuteCheatTable();
    }
}

int main()
{
    // Finding the game process
    std::string gameName = "igi";
    gtlibc.EnableLogs(true);
    gtlibc.FindGameProcess(gameName);
    GAME_BASE_ADDRESS = gtlibc.GetGameBaseAddress();

    std::cout << "Game found with Id " << gtlibc.GetProcessID() << " Name: " << gtlibc.GetGameName() << " Base Address: " << GAME_BASE_ADDRESS << " Handle: " << gtlibc.GetGameHandle() << std::endl;

    // Selecting the cheat table file.
    std::string cheatTableFile = "CheatTable/igi.ct";

    // Generate the cheat table demo.
    GenerateGenericTrainer(cheatTableFile, false, true);

    return 0;
}
