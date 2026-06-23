// main.cpp
#include <iostream>
#include <sodium/utils.h>
#include <vector>
#include "miner.h"

int main() {
    if(sodium_init() != 0) {
        std::cout << "Sodium initialization failed!\n";
        return 1;
    }

    std::cout << " KASPA MINER ";
    std::cout << "\n";

    std::string minerAddressHex;
    uint64_t blocksToMine;

    std::cout << "Miner address (hex): ";
    std::cin >> minerAddressHex;

    std::cout << "Number of blocks to mine: ";
    std::cin >> blocksToMine;

    Addr minerAddress = getBytes<20>(minerAddressHex);

    std::cout << "\nStarting miner...\n";
    std::cout << "Address: " << getHex(minerAddress) << "\n";
    std::cout << "Blocks to mine: " << blocksToMine << "\n\n";

    Miner miner(minerAddress, blocksToMine);
    miner.mine();

    return 0;
}
