#include <iostream>
#include <sodium/utils.h>
#include <vector>
#include "miner.h"
#include <sodium.h>

void printLines() {
    for(int i = 0; i < 16; i++) std::cout << "=";
}

Hash getBytes(const std::string& hex) {
    Hash bytes;
    sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr);
    return bytes;
}

Miner setupMiner() {
    std::string previousHashHex;
    Hash previousHash{};
    std::vector<Hash> transactionHashes;
    size_t txSize;

    printLines();
    std::cout << " MINING ";
    printLines();

    std::cout << "\nPrevious block hash: ";
    std::cin >> previousHashHex;
    previousHash = getBytes(previousHashHex);
    std::cout << "\nNumber of transactions: ";
    std::cin >> txSize;
    for(int i = 0; i < txSize; i++) {
        std::string txHashHex;
        std::cout << "\t" << i + 1 << " Transaction hash: ";
        std::cin >> txHashHex;
        transactionHashes.emplace_back(getBytes(txHashHex));
    }
    std::cout << "\n";
    Miner miner{previousHash, transactionHashes};
    return miner;
}

int main() {
    if(sodium_init() != 0) {
        std::cout << "Somethings wrong!\n";
    }
    Miner miner = setupMiner();
    miner.mine();
    return 0;
}
