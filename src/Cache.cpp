#include "../header/Cache.hpp"
#include "../header/DataArray.hpp"
#include "../header/TagArray.hpp"
#include "../header/TraceParser.hpp"
#include <cmath>
#include <iostream>

Cache::Cache(int s, int E, int b)
    : s(s),
      E(E),
      b(b),
      numSets(1 << s),           // 2^s sets
      blockSizeBytes(1 << b),      // 2^b bytes per block
      dataArray(E, (1 << b), (1 << s)), // Initialize DataArray with: associativity, blockSizeBytes, numSets
      tagArray(E, (1 << s))            // Initialize TagArray with: associativity, numSets
{
    // Initialize metadata for each cache line.
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

uint32_t Cache::extractTag(uint32_t address) const {
    // Calculate the number of offset bits for the block:
    // Since each word is 4 bytes, number of word offset bits = log2(blockSizeBytes/4) = b - 2.
    // Then the tag is the high-order bits: 32 - s - (b - 2)
    return address >> (s + (b - 2));
}

int Cache::extractSetIndex(uint32_t address) const {
    // Extract set index bits located just above the word offset bits.
    return (address >> (b - 2)) & ((1 << s) - 1);
}

int Cache::extractBlockOffset(uint32_t address) const {
    // Extract the word offset within the block:
    // Each block has (blockSizeBytes/4) words.
    return ((address / 4)) % (blockSizeBytes / 4);
}

void Cache::updateLRU(int setIndex, int way) {
    // Reset the LRU counter for the accessed line and increment for others.
    meta[setIndex][way].lruCounter = 0;
    for (int i = 0; i < E; ++i) {
        if (i != way && meta[setIndex][i].valid) {
            meta[setIndex][i].lruCounter++;
        }
    }
}

bool Cache::read(uint32_t address, int &cycles) {
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);
    
    // Check for a hit in the target set.
    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            updateLRU(setIndex, way);
            cycles = 1; // Cache hit cost: 1 cycle.
            return true;
        }
    }
    
    // Cache miss: simulate memory fetch delay (e.g., 100 cycles).
    cycles = 100;
    
    // Select a victim for replacement using LRU policy.
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
    
    // If the victim cache line is dirty, add a write-back delay.
    if (meta[setIndex][victim].valid && meta[setIndex][victim].dirty) {
        cycles += 100;
    }
    
    // Update the victim cache line with the new tag and mark it valid.
    tagArray.tags[setIndex][victim] = tag;
    meta[setIndex][victim].valid = true;
    meta[setIndex][victim].dirty = false;
    meta[setIndex][victim].state = MESIState::Exclusive;
    updateLRU(setIndex, victim);
    
    // Optionally: update dataArray by fetching the block from memory.
    
    return false;
}

bool Cache::write(uint32_t address, int &cycles) {
    int setIndex = extractSetIndex(address);
    uint32_t tag = extractTag(address);
    
    // Look for a cache hit.
    for (int way = 0; way < E; ++way) {
        if (meta[setIndex][way].valid && tagArray.tags[setIndex][way] == tag) {
            meta[setIndex][way].dirty = true;
            meta[setIndex][way].state = MESIState::Modified;
            updateLRU(setIndex, way);
            cycles = 1; // Hit cost.
            return true;
        }
    }
    
    // Write miss: simulate delay (100 cycles for fetching block).
    cycles = 100;
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
    
    if (meta[setIndex][victim].valid && meta[setIndex][victim].dirty) {
        cycles += 100;
    }
    
    // Write-allocate: update the cache with the new tag and mark as dirty.
    tagArray.tags[setIndex][victim] = tag;
    meta[setIndex][victim].valid = true;
    meta[setIndex][victim].dirty = true;
    meta[setIndex][victim].state = MESIState::Modified;
    updateLRU(setIndex, victim);
    
    // Optionally: update dataArray with written data.
    
    return false;
}
