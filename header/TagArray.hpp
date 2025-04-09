#ifndef TAG_ARRAY_HPP
#define TAG_ARRAY_HPP

#include <vector>

// Struct representing the tag array of the cache.
// The tag array is a 2D vector of dimensions:
//   tags[set][way]
struct TagArray {
    int associativity; // Number of ways per set.
    int numSets;       // Number of sets in the cache.
    std::vector<std::vector<unsigned int>> tags;

    // Constructor: initializes the 2D tag array with default 0 values.
    TagArray(int associativity, int numSets)
        : associativity(associativity),
          numSets(numSets),
          tags(numSets, std::vector<unsigned int>(associativity, 0)) {}
};

#endif // TAG_ARRAY_HPP
