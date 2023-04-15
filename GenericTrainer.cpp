// include the header files and GTLIbc class
#include "GTLibc.cpp"

//define the base address of the game
DWORD GAME_BASE_ADDRESS = 0x00400000;

//define the main with gtlibc object.
int main()
{
    // Finding the game process
    std::string gameName = "igi";
    GTLibc gtlibc(true);
    gtlibc.FindGameProcess(gameName);
    GAME_BASE_ADDRESS = gtlibc.GetGameBaseAddress();

    // Selecting the cheat table file.
    string cheatTableFile = "igi.ct";

    // Read the cheat table file
    auto cheatTable = gtlibc.ReadCheatTable(cheatTableFile);

    // Parse the XML data and populate the cheat entries
    gtlibc.PrintCheatTable(cheatTable);

    // Print count the cheat entries
    std::cout << "Count: " << cheatTable.entries.size() << std::endl;

    return 0;
}
