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
    //gtlibc.FindGameProcess(gameName);
    HWND hwnd = gtlibc.FindGameWindow("IGI");
    DWORD pid = gtlibc.GetProcessID4mHWND(hwnd);
    HANDLE hProcess = gtlibc.GetGameHandle4mHWND(hwnd);
    std::cout << "PID: " << to_hex_string(pid) << " hProcess: " << to_hex_string(hProcess) << std::endl;

    //GAME_BASE_ADDRESS = gtlibc.GetGameBaseAddress();

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
