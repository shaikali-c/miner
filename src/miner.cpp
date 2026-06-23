// miner.cpp
#include "miner.h"
#include "sodium/crypto_generichash.h"
#include <chrono>
#include <sodium.h>

using namespace cpr;

void printLines() {
    for(int i = 0; i < 16; i++) std::cout << "=";
}

Miner::Miner(const Addr& addr, uint64_t blocks) :
    address(addr),
    nonce(0),
    blocksToMine(blocks),
    difficulty(1) {
    merkleRoot.fill(0);
    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

    // Fetch chain info and pool transactions
    fetchChainInfo();
    fetchPoolTransactions();

    // Insert coinbase transaction at the beginning
    transactions.insert(transactions.begin(), createCoinbaseTransaction());
    buildMerkleRoot();
    buildTarget();
}

void Miner::fetchChainInfo() {
    try {
        Response response = Get(Url{"http://127.0.0.1:18080/chain"});
        if (response.status_code == 200) {
            auto json = nlohmann::json::parse(response.text);
            difficulty = json["difficulty"].get<uint32_t>();
            previousHash = getBytes<crypto_generichash_BYTES>(json["tip"].get<std::string>());
            std::cout << "Fetched chain info:\n";
            std::cout << "\tDifficulty: " << difficulty << "\n";
            std::cout << "\tHeight: " << json["height"] << "\n";
            std::cout << "\tTip: " << json["tip"] << "\n";
            std::cout << "\tUTXOs: " << json["utxosCount"] << "\n\n";
        } else {
            std::cerr << "Failed to fetch chain info. Status: " << response.status_code << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching chain info: " << e.what() << "\n";
    }
}

void Miner::fetchPoolTransactions() {
    try {
        Response response = Get(Url{"http://127.0.0.1:18080/pool"});
        if (response.status_code == 200) {
            auto pjson = nlohmann::json::parse(response.text);
            auto json = pjson["pool"];
            if (json.is_array()) {
                for (const auto& txHash : json) {
                    transactions.push_back(getBytes<crypto_generichash_BYTES>(txHash["transactionHash"].get<std::string>()));
                }
                std::cout << "Fetched " << transactions.size() << " pool transactions\n\n";
            }
        } else {
            std::cerr << "Failed to fetch pool transactions. Status: " << response.status_code << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error fetching pool transactions: " << e.what() << "\n";
    }
}

void Miner::mine() {
    std::cout << "Starting mining for " << blocksToMine << " blocks...\n";
    std::cout << "Mining address: " << getHex(address) << "\n\n";

    for (uint64_t blockNum = 0; blockNum < blocksToMine; blockNum++) {
        // Reset nonce for each block
        nonce = 0;

        // Update timestamp for each block
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();


        // Rebuild coinbase and merkle root for each block
        transactions.clear();
        fetchPoolTransactions();
        transactions.insert(transactions.begin(), createCoinbaseTransaction());
        buildMerkleRoot();
        buildTarget();

        std::size_t nonceOffset{};
        std::array<unsigned char, rawBytesSize> bytes;

        std::copy(previousHash.begin(), previousHash.end(), bytes.begin());
        nonceOffset += previousHash.size();
        std::copy(merkleRoot.begin(), merkleRoot.end(), bytes.begin() + nonceOffset);
        nonceOffset += merkleRoot.size();
        std::memcpy(bytes.data() + nonceOffset, &timestamp, sizeof(timestamp));
        nonceOffset += sizeof(timestamp);
        std::memcpy(bytes.data() + nonceOffset, &nonce, sizeof(nonce));

        uint64_t attempts = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        for(;;) {
            // Hash the block header
            hash = hashBytes(bytes);
            if(hash <= target) {
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

                std::cout << "\n======= Block " << blockNum + 1 << " of " << blocksToMine << " =======\n";
                std::cout << "Found a valid hash!\n";
                std::cout << "\tNonce: " << nonce << "\n";
                std::cout << "\tHash: " << getHex(hash) << "\n";
                std::cout << "\tPrevious hash: " << getHex(previousHash) << "\n";
                std::cout << "\tMerkle root: " << getHex(merkleRoot) << "\n";
                std::cout << "\tTimestamp: " << timestamp << "\n";
                std::cout << "\tDifficulty: " << difficulty << "\n";
                std::cout << "\tAttempts: " << attempts << "\n";
                std::cout << "\tTime: " << duration.count() << " seconds\n";
                std::cout << "\tTransactions: " << transactions.size() << "\n\n";

                // Send the block to be validated
                setupJson();
                sendRequest(setupJson());

                // Update previous hash for next block
                previousHash = hash;
                break;
            }
            ++nonce;
            ++attempts;
            std::memcpy(bytes.data() + nonceOffset, &nonce, sizeof(nonce));
        }

        // Fetch updated chain info for next block
        if (blockNum < blocksToMine - 1) {
            fetchChainInfo();
        }
    }

    std::cout << "\n======= Mining Complete =======\n";
    std::cout << "Mined " << blocksToMine << " blocks to address: " << getHex(address) << "\n";
}

void Miner::buildMerkleRoot() {
    if (transactions.empty()) {
        merkleRoot = Hash{};
        return;
    }
    std::vector<Hash> currentLevel = transactions;
    if(currentLevel.size() % 2 != 0) currentLevel.push_back(currentLevel.back());
    while(currentLevel.size() > 1) {
        if(currentLevel.size() % 2 != 0) currentLevel.push_back(currentLevel.back());
        std::vector<Hash> nextLevel;
        for(size_t i = 0; i < currentLevel.size(); i += 2) {
            std::array<unsigned char, crypto_generichash_BYTES * 2> combinedHash;
            std::copy(currentLevel[i].begin(), currentLevel[i].end(), combinedHash.begin());
            std::copy(currentLevel[i+1].begin(), currentLevel[i+1].end(), combinedHash.begin() + currentLevel[i].size());
            nextLevel.push_back(hashBytes(combinedHash));
        }
        currentLevel = std::move(nextLevel);
    }
    merkleRoot = currentLevel.front();
}

void Miner::buildTarget() {
    target.fill(0xFF);
    for(int i = 0; i < difficulty; i++) {
        target[i] = 0x00;
    }
}

nlohmann::json Miner::setupJson() {
    if(hash == Hash{}) return {};
    nlohmann::json json;
    json["minerAddress"] = getHex(address);
    json["previousHash"] = getHex(previousHash);
    json["merkleRoot"] = getHex(merkleRoot);
    json["nonce"] = nonce;
    json["hash"] = getHex(hash);
    json["timestamp"] = timestamp;
    nlohmann::json transactionsArray = nlohmann::json::array();
    for(const auto& t : transactions) {
        transactionsArray.push_back(getHex(t));
    }
    json["transactions"] = transactionsArray;
    return json;
}

void Miner::sendRequest(const nlohmann::json& json) {
    try {
        Response post_r = Post(Url{"http://127.0.0.1:18080/validateBlock"},
                              Body{json.dump()},
                              Header{{"Content-Type", "application/json"}});
        std::cout << "Block validation response:\n";
        std::cout << "\tStatus: " << post_r.status_code << "\n";
        std::cout << "\tResponse: " << post_r.text << "\n\n";
    } catch (const std::exception& e) {
        std::cerr << "Error sending block: " << e.what() << "\n";
    }
}

Hash Miner::createCoinbaseTransaction() {
    auto reward = MINER_REWARD;

    std::array<unsigned char, Addr{}.size() + (sizeof(uint64_t) * 2)> bytes;

    auto* rawBytesReward = reinterpret_cast<unsigned char*>(&reward);
    auto* rawBytesTimestamp = reinterpret_cast<unsigned char*>(&timestamp);

    std::memcpy(bytes.data(), address.data(), address.size());
    std::memcpy(bytes.data() + address.size(), rawBytesReward, sizeof(uint64_t));
    std::memcpy(bytes.data() + address.size() + sizeof(reward), rawBytesTimestamp, sizeof(uint64_t));

    return hashBytes(bytes);
}
