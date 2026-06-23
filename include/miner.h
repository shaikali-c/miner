// miner.h
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <sodium.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <iostream>
#include <chrono>

using Hash = std::array<unsigned char, crypto_generichash_BYTES>;
using Addr = std::array<unsigned char, 20>;

struct UTXO {
    Addr owner;
    uint64_t coins;
    UTXO(const Addr& addr, uint64_t c) : owner(addr), coins(c) {}
    std::string serialize() const;
};

struct Input {
    Hash transaction_hash{};
    uint32_t output_index{};
    Input(const Hash tx_hash, uint32_t oi) : transaction_hash(tx_hash), output_index(oi) {}
    Input(const std::string&);
    Input() = default;
    std::string getUTXOKey() const;
    std::string serialize() const;
};

struct BytesWriter {
    std::vector<unsigned char> buffer;
    size_t offset = 0;
    template<typename T>
    void writeBytes(const T& bytes){
        T value{};
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }
    template<typename T>
    void writeValue(T value) {
        auto* bytes = reinterpret_cast<unsigned char*>(&value);
        for (size_t i = 0; i < sizeof(value); i++) buffer.push_back(bytes[i]);
    }
};

template<std::size_t N>
Hash hashBytes(const std::array<unsigned char, N> bytes) {
    Hash hash;
    crypto_generichash(hash.data(), hash.size(), bytes.data(), bytes.size(), nullptr, 0);
    return hash;
}

template<size_t S>
std::string getHex(const std::array<unsigned char, S>& hash)
{
    std::string hex(hash.size() * 2, '\0');
    sodium_bin2hex(
        hex.data(),
        hex.size() + 1,
        hash.data(),
        hash.size()
    );
    return hex;
}

template<size_t S>
std::array<unsigned char, S> getBytes(const std::string& hex) {
    std::array<unsigned char, S> bytes{};
    sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr);
    return bytes;
}

struct Miner {
    Hash previousHash{};
    Hash merkleRoot{};
    Hash target{};
    Hash hash;
    Addr address;
    uint64_t nonce;
    uint64_t timestamp;
    uint32_t difficulty;
    std::vector<Hash> transactions;
    uint64_t blocksToMine;

    static constexpr size_t rawBytesSize = (Hash{}.size() * 2) + (sizeof(uint64_t) * 2);
    static constexpr uint64_t MINER_REWARD = 1;

    Miner(const Addr& addr, uint64_t blocks);
    void mine();
    void buildMerkleRoot();
    void buildTarget();
    void sendRequest(const nlohmann::json& json);
    Hash createCoinbaseTransaction();
    nlohmann::json setupJson();
    void fetchChainInfo();
    void fetchPoolTransactions();
};
