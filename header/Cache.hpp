#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <vector>
#include "DataArray.hpp"
#include "TagArray.hpp"

// Define MESI protocol states.
enum class MESIState { Modified, Exclusive, Shared, Invalid };

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
    // E: associativity (number of ways per set)
    // b: block bits (blockSizeBytes = 2^b)
    Cache(int s, int E, int b);
    
    // Simulate a cache read.
    // 'cycles' returns the additional delay cycles (if a miss, or write-back delay).
    // Returns true on hit.
    bool read(uint32_t address, int &cycles);
    
    // Simulate a cache write.
    // Returns true on hit.
    bool write(uint32_t address, int &cycles);
    
private:
    int s;                // Number of set index bits.
    int E;                // Associativity (number of ways per set).
    int b;                // Block bits.
    int numSets;          // Calculated as 2^s.
    int blockSizeBytes;   // Calculated as 2^b.
    
    DataArray dataArray;  // The data storage array.
    TagArray tagArray;    // The tag storage array.
    
    // Metadata for each cache line: [set][way].
    std::vector<std::vector<CacheLineMeta>> meta;
    
    // Helper functions.
    // extractTag extracts the tag from the address.
    uint32_t extractTag(uint32_t address) const;
    // extractSetIndex extracts the set index from the address.
    int extractSetIndex(uint32_t address) const;
    // extractBlockOffset extracts the word offset (index within the block)
    // Given each word is 4 bytes, offset bits = log₂(blockSizeBytes/4).
    int extractBlockOffset(uint32_t address) const;
    // Update LRU counters after accessing a particular cache line.
    void updateLRU(int setIndex, int way);
};

#endif // CACHE_HPP
