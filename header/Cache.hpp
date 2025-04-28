#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <vector>
#include "DataArray.hpp"
#include "TagArray.hpp"
#include "Bus.hpp"  // For bus transactions

// Define MESI protocol states.
enum class MESIState { Modified, Exclusive, Shared, Invalid };

enum class HasBlockState { HasBlock, NoBlock, HasBlockBeingWrittenBack };

// Metadata for each cache line.
struct CacheLineMeta {
    bool valid;
    bool dirty;
    MESIState state;
    int lruCounter;  // For LRU replacement policy.
};

class Cache {
public:
    // Constructor parameters:
    // s: number of set index bits (numSets = 2^s)
    // E: associativity (number of ways = 2^E)
    // b: block bits (blockSizeBytes = 2^b)
    // processorId: identifier for the processor owning this cache.
    Cache(int s, int E, int b, int processorId, Bus *busPtr);

    // Basic read/write functions.
    bool read(uint32_t address, int &cycles);
    bool write(uint32_t address, int &cycles);

    // Bus-aware read/write functions.
    bool read(uint32_t address, int &cycles, Bus *bus);
    bool write(uint32_t address, int &cycles, Bus *bus);

    // Called by the Bus to resolve a pending transaction.
    void resolvePendingTransaction(BusTransactionType type, uint32_t address, int delay);

    // Called each cycle to check pending delay.
    int getPendingCycleCount() const;
    bool isTransactionPending() const;
        // --- debugging helpers ---------------------------------
        // bool        isTransactionPending() const { return pendingTransaction; }
        // int         getPendingCycleCount() const { return pendingCycleCount; }
        uint32_t    getPendingAddress()   const { return pendingAddress; }
    void decrementPendingCycle();

    // Returns true if the cache holds the block (only if in Shared or Exclusive state).
    bool hasBlock(uint32_t address) const;

    // Accessors.
    int getBlockSizeBytes() const { return blockSizeBytes; }
    int getProcessorId() const { return processorId; }

    // Bus snooping: updates MESI state for a transaction.
    void handleBusTransaction(const BusTransaction &tx);

    // New: Invalidate the block if it is in Shared state.
    void invalidateShared(uint32_t address);
    // Returns the number of cache misses for this cache.
    int getCacheMisses() const { return cacheMisses; }

    // Returns the number of cache evictions.
    int getEvictions() const { return cacheEvictions; }

    // Returns the number of writebacks performed.
    int getWritebacks() const { return writebacks; }

    // Returns the number of bus invalidations seen by this cache.
    int getBusInvalidations() const { return busInvalidations; }

    // Returns the total data traffic (in bytes) generated on the bus by this cache.
    int getDataTrafficBytes() const { return dataTrafficBytes; }

    void printCacheInfo() const;

    void installPendingBlock();

    void setPendingWritebackCycles(int cycles) { pendingwritebackCycles = cycles; }
    bool is_writing_to_mem = false; // Indicates if the cache is writing to memory.
    bool modified_invalidated = false; // Indicates if the cache is invalidated after a writeback.

private:
    int s;                // Number of set index bits.
    int E;                // Associativity exponent (number of ways = 2^E).
    int b;                // Block bits.
    int numSets;          // Calculated as 2^s.
    int blockSizeBytes;   // Calculated as 2^b.
    int processorId;      // Processor ID.

    DataArray dataArray;  // Data storage array.
    TagArray tagArray;    // Tag storage array.
    std::vector<std::vector<CacheLineMeta>> meta;

    // Helper functions.
    uint32_t extractTag(uint32_t address) const;
    int extractSetIndex(uint32_t address) const;
    int extractBlockOffset(uint32_t address) const;
    void updateLRU(int setIndex, int way);

    // Pending transaction information.
    bool pendingTransaction;
    uint32_t pendingAddress;
    BusTransactionType pendingType;
    int pendingCycleCount;
    int cacheMisses = 0; // Cache misses counter.
    int cacheEvictions = 0;
    int writebacks = 0;
    int busInvalidations = 0;
    int dataTrafficBytes = 0;
    int pendingDelay = 0; // Delay for pending transactions.
    bool is_writeback = false; // Indicates if the pending transaction is a writeback.
    bool is_mem_occupied = false; // Indicates if the memory is occupied.
    int pendingwritebackCycles = 0; // Number of cycles for pending writeback.
    Bus* bus;
    
};

#endif // CACHE_HPP
