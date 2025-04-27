#include "../header/Cache.hpp"
#include "../header/DataArray.hpp"
#include "../header/TagArray.hpp"
#include "../header/TraceParser.hpp"
#include "../header/Bus.hpp"
#include <cmath>
#include <iostream>

// Helper to convert MESIState to string for debugging
static const char* mesiStateToString(MESIState state) {
    switch (state) {
        case MESIState::Invalid:   return "Invalid";
        case MESIState::Shared:    return "Shared";
        case MESIState::Exclusive: return "Exclusive";
        case MESIState::Modified:  return "Modified";
        default:                   return "Unknown";
    }
}

//------------------------------------------------------------------
// Constructor: Initialize the cache.
Cache::Cache(int s, int E, int b, int processorId)
    : s(s),
      E(E),
      b(b),
      numSets(1 << s),
      blockSizeBytes(1 << b),
      dataArray(E, (1 << b), (1 << s)),
      tagArray(E, (1 << s)),
      processorId(processorId),
      pendingTransaction(false),
      pendingAddress(0),
      pendingCycleCount(0)
{
    meta.resize(numSets, std::vector<CacheLineMeta>(E));
    for (int set = 0; set < numSets; ++set) {
        for (int way = 0; way < E; ++way) {
            meta[set][way].valid = false;
            meta[set][way].dirty = false;
            meta[set][way].state = MESIState::Invalid;
            meta[set][way].lruCounter = 0;
        }
    }
}

//------------------------------------------------------------------
// Extraction Functions.
uint32_t Cache::extractTag(uint32_t address) const {
    return address >> (s + b);
}

int Cache::extractSetIndex(uint32_t address) const {
    return (address >> b) & ((1 << s) - 1);
}

int Cache::extractBlockOffset(uint32_t address) const {
    return ((address / 4)) % (blockSizeBytes / 4);
}

//------------------------------------------------------------------
// LRU Update Function.
void Cache::updateLRU(int setIndex, int way) {
    meta[setIndex][way].lruCounter = 0;
    for (int i = 0; i < E; ++i) {
        if (i != way && meta[setIndex][i].valid) {
            meta[setIndex][i].lruCounter++;
        }
    }
}

//------------------------------------------------------------------
// Basic read (non-bus-aware).
bool Cache::read(uint32_t address, int &cycles) {
    if (pendingTransaction)
        return false;
    return read(address, cycles, nullptr);
}

// Bus-aware read.
bool Cache::read(uint32_t address, int &cycles, Bus *bus) {
    if (pendingTransaction)
        return false;
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);

    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            updateLRU(setIndex, way);
            cycles = 1;
            return true;
        }
    }
    // Miss: issue a BusRd transaction.
    if (bus) {
        BusTransaction tx;
        tx.type = BusTransactionType::BusRd;
        tx.address = address;
        tx.sourceProcessorId = processorId;
        bus->addTransaction(tx);
    }
    pendingTransaction = true;
    pendingAddress = address;
    pendingType = BusTransactionType::BusRd;
    pendingCycleCount = -1;
    cacheMisses++;
    return false;
}

//------------------------------------------------------------------
// Basic write (non-bus-aware).
bool Cache::write(uint32_t address, int &cycles) {
    if (pendingTransaction)
        return false;
    return write(address, cycles, nullptr);
}

// Bus-aware write.
bool Cache::write(uint32_t address, int &cycles, Bus *bus) {
    if (pendingTransaction)
        return false;
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);

    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            // Shared->Modified upgrade
            if (meta[setIndex][way].state == MESIState::Shared && bus) {
                BusTransaction tx;
                tx.type = BusTransactionType::BusUpgr;
                tx.address = address;
                tx.sourceProcessorId = processorId;
                bus->addTransaction(tx);
            }
            meta[setIndex][way].dirty = true;
            MESIState oldState = meta[setIndex][way].state;
            meta[setIndex][way].state = MESIState::Modified;
            std::cout << "[Cache " << processorId << "] WRITE hit at set " << setIndex
                      << ", way " << way << ", tag 0x" << std::hex << tag << std::dec
                      << ", " << mesiStateToString(oldState)
                      << " -> " << mesiStateToString(meta[setIndex][way].state)
                      << std::endl;
            updateLRU(setIndex, way);
            cycles = 1;
            return true;
        }
    }
    // Write miss: issue a BusRdWITWr transaction.
    if (bus) {
        BusTransaction tx;
        tx.type = BusTransactionType::BusRdWITWr;
        tx.address = address;
        tx.sourceProcessorId = processorId;
        bus->addTransaction(tx);
    }
    pendingTransaction = true;
    pendingAddress = address;
    pendingType = BusTransactionType::BusRdWITWr;
    pendingCycleCount = -1;
    cacheMisses++;
    return false;
}

//------------------------------------------------------------------
// resolvePendingTransaction: Called by the Bus to set the delay and install the block.
void Cache::resolvePendingTransaction(BusTransactionType type, uint32_t address, int delay) {
    if (!pendingTransaction || pendingAddress != address)
        return;
    if (pendingCycleCount == -1) {
        pendingCycleCount = delay; // set delay

        // update traffic and eviction counters
        if (delay != 100) {
            dataTrafficBytes += blockSizeBytes;
        } else {
            dataTrafficBytes += blockSizeBytes;
            cacheEvictions++;
        }

        int setIndex = extractSetIndex(address);
        uint32_t tag = extractTag(address);
        int victim = 0;
        // find victim via LRU or empty
        for (int way = 0; way < E; ++way) {
            if (!meta[setIndex][way].valid) {
                victim = way;
                break;
            }
            if (meta[setIndex][way].lruCounter > meta[setIndex][victim].lruCounter) {
                victim = way;
            }
        }
        // evict if needed
        if (meta[setIndex][victim].valid && meta[setIndex][victim].dirty) {
            writebacks++;
        }
        MESIState oldState = meta[setIndex][victim].state;

        // install new block
        tagArray.tags[setIndex][victim] = tag;
        meta[setIndex][victim].valid = true;
        if (type == BusTransactionType::BusRd) {
            meta[setIndex][victim].dirty = false;
            meta[setIndex][victim].state = (delay == 100)
                ? MESIState::Exclusive : MESIState::Shared;
        } else {
            meta[setIndex][victim].dirty = true;
            meta[setIndex][victim].state = MESIState::Modified;
        }
        updateLRU(setIndex, victim);

        std::cout << "[Cache " << processorId << "] Installed block at set " << setIndex
              << ", way " << victim << ", tag 0x" << std::hex << tag << std::dec
              << ", " << mesiStateToString(oldState)
              << " -> " << mesiStateToString(meta[setIndex][victim].state)
              << "\n";
    }
}

//------------------------------------------------------------------
int Cache::getPendingCycleCount() const { return pendingCycleCount; }
bool Cache::isTransactionPending() const { return pendingTransaction; }

void Cache::decrementPendingCycle() {
    if (pendingTransaction && pendingCycleCount > 0) {
        pendingCycleCount--;
        if (pendingCycleCount == 0) {
            pendingTransaction = false;
        }
    }
}

//------------------------------------------------------------------
// Utility: Checks if this cache holds the block for a given address.
bool Cache::hasBlock(uint32_t address) const {
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);
    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            if (meta[setIndex][way].state == MESIState::Shared ||
                meta[setIndex][way].state == MESIState::Exclusive) {
                return true;
            }
        }
    }
    return false;
}

//------------------------------------------------------------------
// Bus transaction snooping: Updates MESI state for local copies.
void Cache::handleBusTransaction(const BusTransaction &tx) {
    if (processorId == tx.sourceProcessorId)
        return;
    int setIndex = extractSetIndex(tx.address);
    uint32_t tag = extractTag(tx.address);
    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            MESIState oldState = meta[setIndex][way].state;
            switch (tx.type) {
              case BusTransactionType::BusRd:
                // Only downgrade M→S or E→S
                if (oldState==MESIState::Modified || oldState==MESIState::Exclusive) {
                  if (oldState==MESIState::Modified) {
                    writebacks++;
                    dataTrafficBytes+=blockSizeBytes;
                  }
                  meta[setIndex][way].state = MESIState::Shared;
                  meta[setIndex][way].dirty = false;
                  std::cout << "[Cache " << processorId << "] Snooped BusRd at set " << setIndex
                            << ", way " << way << ", tag 0x" << std::hex<<tag<<std::dec
                            << ", " << mesiStateToString(oldState)
                            << " -> Shared\n";
                }
                break;
                case BusTransactionType::BusRdX:
                case BusTransactionType::BusRdWITWr:
                    meta[setIndex][way].state = MESIState::Invalid;
                    meta[setIndex][way].valid = false;
                    meta[setIndex][way].dirty = false;
                    std::cout << "[Cache " << processorId << "] Snooped BusRdX/WITWr at set "
                    << setIndex << ", way " << way << ", tag 0x" << std::hex<<tag<<std::dec
                    << ", " << mesiStateToString(oldState)
                    << " -> Invalid\n";
                    busInvalidations++;
                    break;
                case BusTransactionType::BusUpgr:
                    busInvalidations++;
                    meta[setIndex][way].state = MESIState::Invalid;
                    meta[setIndex][way].valid = false;
                    meta[setIndex][way].dirty = false;
                    std::cout << "[Cache " << processorId << "] Snooped BusUpgr at set " << setIndex
                    << ", way " << way << ", tag 0x" << std::hex<<tag<<std::dec
                    << ", " << mesiStateToString(oldState)
                    << " -> Invalid\n";
                    break;
            }
        }
    }
}

//------------------------------------------------------------------
// New function: Invalidate the block if it is in the Shared state.
void Cache::invalidateShared(uint32_t address) {
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);
    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            if (meta[setIndex][way].state == MESIState::Shared) {
                MESIState oldState = meta[setIndex][way].state;
                meta[setIndex][way].state = MESIState::Invalid;
                meta[setIndex][way].valid = false;
                meta[setIndex][way].dirty = false;
                //busInvalidations++;
                std::cout << "[Cache " << processorId << "] invalidateShared at set " << setIndex
                          << ", way " << way << ", tag 0x" << std::hex << tag << std::dec
                          << ", " << mesiStateToString(oldState)
                          << " -> Invalid" << std::endl;
            }
        }
    }
}