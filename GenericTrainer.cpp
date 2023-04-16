// including the header files and GTLIbc class
#include "GTLibc.cpp"
#include <windows.h>

// define the base address of the game
DWORD GAME_BASE_ADDRESS = 0x00400000;

int main()
{
    // Finding the game process
    std::string gameName = "igi";
    GTLibc gtlibc(true);
    gtlibc.FindGameProcess(gameName);
    GAME_BASE_ADDRESS = gtlibc.GetGameBaseAddress();

    // Selecting the cheat table file.
    string cheatTableFile = "CheatTable/igi.ct";

    // Read the cheat table file
    CheatTable cheatTable = gtlibc.ReadCheatTable(cheatTableFile);
    //CheatTable cheatTable;

    // Add cheat entries Health.
    //cheatTable.AddCheatEntry<float>("Health", CheatTypes.Float, 0x5693968, vector<DWORD>{596, 20, 1996, 8}, vector<int>{VK_CONTROL,VK_F1}, CheatActions.SetValue, 0.5f);

    // Print the cheat table
    gtlibc.PrintCheatTable(cheatTable);

    // Read the cheat table entries
    // gtlibc.ReadCheatTableEntries(cheatTable);

    // Print count the cheat entries
    std::cout << "Count: " << cheatTable.cheatEntries.size() << std::endl;

    return 0;
}
