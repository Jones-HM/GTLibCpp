// including the header files and GTLIbc class
#include "GTLibc.cpp"

//define the base address of the game
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
    // Make it work for both Windows and Linux and check which OS is being used.
    #ifdef _WIN32
        std::replace(cheatTableFile.begin(), cheatTableFile.end(), '/', '\\');
    #elif __linux__ || __APPLE__
        std::replace(cheatTableFile.begin(), cheatTableFile.end(), '\\', '/');
    #endif

    // Read the cheat table file
    CheatTable cheatTable = gtlibc.ReadCheatTable(cheatTableFile);

    // Print the cheat table
    gtlibc.PrintCheatTable(cheatTable);

    // Read the cheat table entries
    //gtlibc.ReadCheatTableEntries(cheatTable);
    
    // Print count the cheat entries
    std::cout << "Count: " << cheatTable.cheatEntries.size() << std::endl;

    return 0;
}
