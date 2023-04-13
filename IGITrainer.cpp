#include "GTLibc.cpp"
#include <iostream>
#include <vector>

GTLibc gtlibc(true);
DWORD game_base_address = 0x00400000;

DWORD GetClipsAddress()
{
    const DWORD clips_static_pointer = (DWORD)0x00671890;
    const std::vector<DWORD> clips_addr_off = {0x0, 0x4C4};

    DWORD clips_base_pointer = gtlibc.ReadPointerOffset<DWORD>(game_base_address, clips_static_pointer);
    DWORD clips_address = gtlibc.ReadPointerOffsets<DWORD>(clips_base_pointer, clips_addr_off) + 0x144;
    return clips_address;
}

DWORD GetHumanBaseAddress()
{
    const DWORD human_static_pointer = (DWORD)0x0016E210;
    const std::vector<DWORD> human_address_offsets = {0x8, 0x7CC, 0x14};

    DWORD human_base_pointer = gtlibc.ReadPointerOffset<DWORD>(game_base_address, human_static_pointer);
    DWORD human_base_address =  gtlibc.ReadPointerOffsets<DWORD>(human_base_pointer, human_address_offsets) + 0x348;

    return human_base_address;
}

void SetAmmo()
{
    int unlimited_ammo = 0x7FFFFFFF;
    DWORD weapon_base_address = GetHumanBaseAddress();

    std::vector<DWORD> weapons_offsets = {0x0, 0xC, 0x18, 0x24, 0x30, 0x3C, 0x48, 0x54, 0x60, 0x6C};
    if (gtlibc.WriteAddressOffsets(weapon_base_address, weapons_offsets, unlimited_ammo))
    {
        std::cout << "[+] Unlimited ammo enabled" << std::endl;
    }
}

void SetClips()
{
    int unlimited_clips = 0xFFFFFF;
    DWORD clips_address = GetClipsAddress();

    if (gtlibc.WriteAddress(clips_address, unlimited_clips))
    {
        std::cout << "[+] Unlimited clips enabled" << std::endl;
    }
}

void SetHealth()
{
    int unlimited_health = 0xFFFFFFFF;
    DWORD health_address = GetHumanBaseAddress() - 0xF4;
    std::cout << "Health address: " << to_hex_str(health_address) << std::endl;
    if (gtlibc.WriteAddress(health_address, unlimited_health))
    {
        std::cout << "[+] Unlimited health enabled" << std::endl;
    }
}

int main()
{
    std::cout << "IGI Trainer +3." << std::endl;
    std::cout << "F1 - Unlimited health" << std::endl;
    std::cout << "F2 - Unlimited ammo" << std::endl;
    std::cout << "F3 - Unlimited clips" << std::endl;
    std::cout << "F5 - Exit" << std::endl;

    std::string gameName = "igi";
    gtlibc.FindGameProcess(gameName);
    


    while (true)
    {
        if (gtlibc.IsKeyToggled(VK_F1))
        {
            SetHealth();
        }
        else if (gtlibc.IsKeyToggled(VK_F2))
        {
            SetAmmo();
        }
        else if (gtlibc.IsKeyToggled(VK_F3))
        {
            SetClips();
        }
        else if (gtlibc.IsKeyToggled(VK_F5))
        {
            break;
        }

        Sleep(100);
    }

    std::cout << "Trainer made using GTLibc :)" << std::endl;
    return 0;
}
