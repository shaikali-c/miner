#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <sodium/utils.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <sodium/crypto_generichash.h>

using Hash = std::array<unsigned char, crypto_generichash_BYTES>;

template<std::size_t N>
Hash hashBytes(const std::array<unsigned char, N> bytes) {
    Hash hash;
    crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
    return hash;
}

std::string getHex(const Hash& hash);

struct Miner {
    Hash previousHash;
    Hash merkleRoot;
    uint64_t nonce;
    uint64_t timestamp;
    uint32_t difficulty;
    Hash target;
    std::vector<Hash> transactions;
    static constexpr size_t rawBytesSize = (Hash{}.size() * 2) + (sizeof(uint64_t) * 2);
    Miner(const Hash& ph, const std::vector<Hash> txs);
    void mine();
    void buildMerkleRoot();
    void buildTarget();
};
