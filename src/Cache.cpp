#include "../header/Cache.hpp"
#include "../header/DataArray.hpp"
#include "../header/TagArray.hpp"
#include "../header/TraceParser.hpp"
#include "../header/Bus.hpp"
#include <cmath>
#include <iostream>

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
    for (int set = 0; set < numSets; ++set)
    {
        for (int way = 0; way < E; ++way)
        {
            meta[set][way].valid = false;
            meta[set][way].dirty = false;
            meta[set][way].state = MESIState::Invalid;
            meta[set][way].lruCounter = 0;
        }
    }
}

//------------------------------------------------------------------
// Extraction Functions.
uint32_t Cache::extractTag(uint32_t address) const
{
    // (Original implementation assumed word addressing; adjust as needed.)
    return address >> (s + (b));
}

int Cache::extractSetIndex(uint32_t address) const
{
    return (address >> (b)) & ((1 << s) - 1);
}

int Cache::extractBlockOffset(uint32_t address) const
{
    return ((address / 4)) % (blockSizeBytes / 4);
}

//------------------------------------------------------------------
// LRU Update Function.
void Cache::updateLRU(int setIndex, int way)
{
    meta[setIndex][way].lruCounter = 0;
    for (int i = 0; i < E; ++i)
    {
        if (i != way && meta[setIndex][i].valid)
        {
            meta[setIndex][i].lruCounter++;
        }
    }
}

//------------------------------------------------------------------
// Basic read (non-bus-aware).
bool Cache::read(uint32_t address, int &cycles)
{
    if (pendingTransaction)
        return false;
    return read(address, cycles, nullptr);
}

// Bus-aware read.
bool Cache::read(uint32_t address, int &cycles, Bus *bus)
{
    if (pendingTransaction)
        return false;
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);

    for (int way = 0; way < E; ++way)
    {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag)
        {
            updateLRU(setIndex, way);
            cycles = 1;
            return true;
        }
    }
    // Miss: issue a BusRd transaction.
    if (bus)
    {
        BusTransaction tx;
        tx.type = BusTransactionType::BusRd;
        tx.address = address;
        tx.sourceProcessorId = this->processorId;
        bus->addTransaction(tx);
    }
    pendingTransaction = true;
    pendingAddress = address;
    pendingType = BusTransactionType::BusRd;
    pendingCycleCount = -1;
    return false;
}

//------------------------------------------------------------------
// Basic write (non-bus-aware).
bool Cache::write(uint32_t address, int &cycles)
{
    if (pendingTransaction)
        return false;
    return write(address, cycles, nullptr);
}

// Bus-aware write.
bool Cache::write(uint32_t address, int &cycles, Bus *bus)
{
    if (pendingTransaction)
        return false;
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);

    for (int way = 0; way < E; ++way)
    {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag)
        {
            // If the block is in Shared state, issue a BusUpgr transaction.
            if (meta[setIndex][way].state == MESIState::Shared && bus)
            {
                BusTransaction tx;
                tx.type = BusTransactionType::BusUpgr;
                tx.address = address;
                tx.sourceProcessorId = this->processorId;
                bus->addTransaction(tx);
            }
            meta[setIndex][way].dirty = true;
            meta[setIndex][way].state = MESIState::Modified;
            updateLRU(setIndex, way);
            cycles = 1;
            return true;
        }
    }
    // Write miss: issue a BusRdWITWr transaction.
    if (bus)
    {
        BusTransaction tx;
        tx.type = BusTransactionType::BusRdWITWr;
        tx.address = address;
        tx.sourceProcessorId = this->processorId;
        bus->addTransaction(tx);
    }
    pendingTransaction = true;
    pendingAddress = address;
    pendingType = BusTransactionType::BusRdWITWr;
    pendingCycleCount = -1;
    return false;
}

//------------------------------------------------------------------
// resolvePendingTransaction: Called by the Bus to set the delay and install the block.
void Cache::resolvePendingTransaction(BusTransactionType type, uint32_t address, int delay) {
    if (!pendingTransaction || pendingAddress != address)
        return;
    if (pendingCycleCount == -1) {
        pendingCycleCount = delay; // Set the computed delay, but do not clear the pending flag yet.
        // Optionally, if your design installs the block immediately, do that here.
        int setIndex = extractSetIndex(address);
        uint32_t tag = extractTag(address);
        int victim = 0;
        for (int way = 0; way < E; ++way) {
            if (!meta[setIndex][way].valid) {
                victim = way;
                break;
            }
            if (meta[setIndex][way].lruCounter > meta[setIndex][victim].lruCounter) {
                victim = way;
            }
        }
        if (type == BusTransactionType::BusRd) {
            tagArray.tags[setIndex][victim] = tag;
            meta[setIndex][victim].valid = true;
            meta[setIndex][victim].dirty = false;
            meta[setIndex][victim].state = (delay == 100) ? MESIState::Exclusive : MESIState::Shared;
        } else {
            tagArray.tags[setIndex][victim] = tag;
            meta[setIndex][victim].valid = true;
            meta[setIndex][victim].dirty = true;
            meta[setIndex][victim].state = MESIState::Modified;
        }
        updateLRU(setIndex, victim);
    }
    // Do NOT clear pendingTransaction immediately here.
    // Instead, once decrementPendingCycle() is called enough times (until delay becomes 0),
    // then pendingTransaction will be cleared.
}


//------------------------------------------------------------------
// Returns the current pending cycle count.
int Cache::getPendingCycleCount() const
{
    return pendingCycleCount;
}

//------------------------------------------------------------------
// Indicates whether a transaction is pending.
bool Cache::isTransactionPending() const
{
    return pendingTransaction;
}

//------------------------------------------------------------------
// Called each cycle to decrement the pending transaction's delay.
void Cache::decrementPendingCycle() {
    if (pendingTransaction && pendingCycleCount > 0) {
        pendingCycleCount--;
        if (pendingCycleCount == 0) {
            // Now that the delay is over, clear the pending flag.
            pendingTransaction = false;
        }
    }
}


//------------------------------------------------------------------
// Utility: Checks if this cache holds the block for a given address.
// Returns true only if the block is in Shared or Exclusive state.
bool Cache::hasBlock(uint32_t address) const
{
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);
    for (int way = 0; way < E; ++way)
    {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag)
        {
            if (meta[setIndex][way].state == MESIState::Shared ||
                meta[setIndex][way].state == MESIState::Exclusive)
                return true;
        }
    }
    return false;
}

//------------------------------------------------------------------
// Bus transaction snooping: Updates MESI state for local copies.
void Cache::handleBusTransaction(const BusTransaction &tx)
{
    // Ignore transactions originating from this cache.
    if (this->processorId == tx.sourceProcessorId)
        return;
    int setIndex = extractSetIndex(tx.address);
    uint32_t tag = extractTag(tx.address);
    for (int way = 0; way < E; ++way)
    {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag)
        {
            switch (tx.type)
            {
            case BusTransactionType::BusRd:
                // Downgrade to Shared if in Modified, Exclusive, or Shared.
                if (meta[setIndex][way].state == MESIState::Modified ||
                    meta[setIndex][way].state == MESIState::Exclusive ||
                    meta[setIndex][way].state == MESIState::Shared)
                {
                    meta[setIndex][way].state = MESIState::Shared;
                    // Clear the dirty flag since a write-back is assumed.
                    meta[setIndex][way].dirty = false;
                }
                break;
            case BusTransactionType::BusRdX:
            case BusTransactionType::BusRdWITWr:
                meta[setIndex][way].state = MESIState::Invalid;
                meta[setIndex][way].valid = false;
                meta[setIndex][way].dirty = false;
                break;
            case BusTransactionType::BusUpgr:
                // BusUpgr is handled by processUpgrade in the Bus.
                break;
            }
        }
    }
}
//------------------------------------------------------------------
// New function: Invalidate the block if it is in the Shared state.
// This is called by the Bus when processing a BusUpgr transaction.
void Cache::invalidateShared(uint32_t address)
{
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);
    for (int way = 0; way < E; ++way)
    {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag)
        {
            if (meta[setIndex][way].state == MESIState::Shared)
            {
                meta[setIndex][way].state = MESIState::Invalid;
                meta[setIndex][way].valid = false;
                meta[setIndex][way].dirty = false;
            }
        }
    }
}