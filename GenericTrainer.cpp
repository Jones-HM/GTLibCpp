// including the header files and GTLIbc class
#include "GTLibc.cpp"

// define the base address of the game
int GAME_BASE_ADDRESS = 0x00400000;

int main()
{
    // Finding the game process
    std::string gameName = "igi";
    GTLibc gtlibc(true);
    gtlibc.FindGameProcess(gameName);
    GAME_BASE_ADDRESS = gtlibc.GetGameBaseAddress();

    std::cout << "Game found with Id " << gtlibc.GetProcessID() << " Name: " << gtlibc.GetGameName() << " Base Address: " << GAME_BASE_ADDRESS << 
    " Handle: " << gtlibc.GetGameHandle() << std::endl;


    // Selecting the cheat table file.
    string cheatTableFile = "CheatTable/igi.ct";

    // Read the cheat table file
    //gtlibc.ReadCheatTable(cheatTableFile);

    // Activate the cheat entries.
    //std::vector<int> cheatIds = {0, 1, 2, 7, 8, 9};
    //gtlibc.ActivateCheatEntries(cheatIds);

    // Execute the cheat Table
    //gtlibc.ExecuteCheatTable();

    return 0;
}
