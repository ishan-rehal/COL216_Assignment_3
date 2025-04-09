#ifndef DATA_ARRAY_HPP
#define DATA_ARRAY_HPP

#include <vector>

// Struct representing the data array of the cache.
// It holds the cache blocks as a 3D vector:
//   data[set][way][word offset]
// where each word is 4 bytes.
struct DataArray {
    int associativity;   // Number of ways per set.
    int blockSizeBytes;  // Block size (in bytes).
    int numSets;         // Number of sets in the cache.
    // 3D vector dimensions: [numSets][associativity][blockSizeBytes / 4]
    std::vector<std::vector<std::vector<unsigned int>>> data;

    // Constructor: allocates the 3D vector with initial 0 values.
    DataArray(int associativity, int blockSizeBytes, int numSets)
        : associativity(associativity),
          blockSizeBytes(blockSizeBytes),
          numSets(numSets),
          data(numSets, std::vector<std::vector<unsigned int>>(
                associativity, std::vector<unsigned int>(blockSizeBytes / 4, 0))) {}
};

#endif // DATA_ARRAY_HPP
