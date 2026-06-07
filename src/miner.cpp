#include "miner.h"

std::string getHex(const Hash& hash) {
    std::string hex;
    hex.resize(Hash{}.size() * 2 + 1);
    sodium_bin2hex(hex.data(), hex.size(), hash.data(), hash.size());
    hex.pop_back();
    return hex;
}

Miner::Miner(const Hash& ph, const std::vector<Hash> txs) : previousHash(ph), nonce(0), transactions(std::move(txs)), difficulty(1) {
    merkleRoot.fill(0);
    timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    buildMerkleRoot();
    buildTarget();
}

void Miner::mine() {
    std::size_t nonceOffset{};
    std::array<unsigned char, rawBytesSize> bytes;

    std::copy(previousHash.begin(), previousHash.end(), bytes.begin());
    nonceOffset += previousHash.size();
    std::copy(merkleRoot.begin(), merkleRoot.end(), bytes.begin() + nonceOffset);
    nonceOffset += merkleRoot.size();
    std::memcpy(bytes.data() + nonceOffset, &timestamp, sizeof(timestamp));
    nonceOffset += sizeof(timestamp);
    std::memcpy(bytes.data() + nonceOffset, &nonce, sizeof(nonce));
    Hash hash{};
    for(;;) {
        hash = hashBytes(hash);
        if(hash <= target) {
            break;
        }
        ++nonce;
        std::memcpy(bytes.data() + nonceOffset, &nonce, sizeof(nonce));
    }
    std::cout << "Found a valid hash at :\n\t\tNonce: " << nonce << "\n\t\tHash: " << getHex(hash) << "\n\t\tPrevious hash: " << getHex(previousHash) << "\n\t\tMerkle root: " << getHex(merkleRoot) << "\n\t\tTimestamp: " << timestamp << "\n";
}

void Miner::buildMerkleRoot() {
    std::vector<Hash> currentLevel = transactions;
    while(currentLevel.size() > 1) {
        if(currentLevel.size() % 2 != 0) currentLevel.push_back(currentLevel.back());
        std::vector<Hash> nextLevel;
        for(size_t i = 0; i < currentLevel.size(); i += 2) {
            std::array<unsigned char, Hash{}.size() * 2> combinedHash;
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
