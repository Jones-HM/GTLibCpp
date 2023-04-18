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
    gtlibc.ReadCheatTable(cheatTableFile);

    // Add cheat entries Health.
    // gtlibc.AddCheatEntry<float>("Health", CheatTypes.Float, 0x5693968, vector<DWORD>{596, 20, 1996, 8}, vector<DWORD>{VK_CONTROL,VK_F1}, CheatActions.SetValue, 0.5f);
    // gtlibc.AddCheatEntry<float>("Armor", CheatTypes.Float, 0x5693968, vector<DWORD>{596, 20, 1996, 8}, vector<DWORD>{VK_CONTROL,VK_F2}, CheatActions.SetValue, 1.0f);
    // gtlibc.AddCheatEntry<int>("Level", CheatTypes.Integer, 0x5693968, vector<DWORD>{596, 20, 1996, 8}, vector<DWORD>{VK_CONTROL,VK_F3}, CheatActions.Freeze, 1);

    // Activate the cheat entries.
    std::vector<int> cheatIds = {0, 1, 2, 7, 8, 9};
    gtlibc.ActivateCheatEntries(cheatIds);

    // Execute the cheat Table
    gtlibc.ExecuteCheatTable();

    // Print the cheat table
    // gtlibc.PrintCheatTable();

    // Read the cheat table entries
    // gtlibc.ReadCheatTableEntries();

    // Print count the cheat entries
    std::cout << "Count: " << g_CheatTable.cheatEntries.size() << std::endl;

    return 0;
}
