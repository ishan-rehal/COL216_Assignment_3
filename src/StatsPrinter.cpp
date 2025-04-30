#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "Processor.hpp"
#include "Cache.hpp"
#include "Bus.hpp"
#include "TraceParser.hpp"

// Assuming that we have a SimulationConfig structure defined as in main.cpp:
struct SimulationConfig {
    std::string tracePrefix;
    int s; // Number of set index bits.
    int E; // Associativity.
    int b; // Block bits (block size in bytes = 2^b).
    std::string outputFilename;
};

// Function to print simulation parameters
void printSimulationParameters(const SimulationConfig &config, int numSets, int cacheSizeKB) {
    std::cout << "Simulation Parameters:\n";
    std::cout << "Trace Prefix: " << config.tracePrefix << "\n";
    std::cout << "Set Index Bits: " << config.s << "\n";
    std::cout << "Associativity: " << config.E << "\n";
    std::cout << "Block Bits: " << config.b << "\n";
    int blockSize = (1 << config.b);
    std::cout << "Block Size (Bytes): " << blockSize << "\n";
    std::cout << "Number of Sets: " << numSets << "\n";
    std::cout << "Cache Size (KB per core): " << cacheSizeKB << "\n";
    std::cout << "MESI Protocol: Enabled\n";
    std::cout << "Write Policy: Write-back, Write-allocate\n";
    std::cout << "Replacement Policy: LRU\n";
    std::cout << "Bus: Central snooping bus\n";
    std::cout << std::endl;
}

// Function to print per-core statistics.
// For this example, we assume that the Processor class provides the following getters:
//   getTotalInstructions(), getTotalReads(), getTotalWrites()
//   getTotalCycles(), getIdleCycles()
// And the Cache class provides:
//   getCacheMisses(), getEvictions(), getWritebacks()
// And there is an accessor for Bus invalidations and data traffic.
// (These must be implemented in your classes; here we simulate with dummy returns if needed.)
void printCoreStats(const std::vector<Processor*> &processors, const std::vector<Cache*> &caches) {
    for (size_t i = 0; i < processors.size(); ++i) {
        // These functions should be implemented in your Processor and Cache classes.
        int totalInstr = processors[i]->getTotalInstructions();
        int totalReads = processors[i]->getTotalReads();
        int totalWrites = processors[i]->getTotalWrites();
        int totalCycles = processors[i]->getTotalCycles();
        int idleCycles = processors[i]->getIdleCycles();
        int misses = caches[i]->getCacheMisses();
        int accesses = totalReads + totalWrites;
        double missRate = (accesses > 0) ? (100.0 * misses / accesses) : 0.0;
        int evictions = caches[i]->getEvictions();
        int writebacks = caches[i]->getWritebacks();
        int busInvalidations = caches[i]->getBusInvalidations();
        int dataTraffic = caches[i]->getDataTrafficBytes();
        
        std::cout << "Core " << i << " Statistics:\n";
        std::cout << "Total Instructions: " << totalInstr << "\n";
        std::cout << "Total Reads: " << totalReads << "\n";
        std::cout << "Total Writes: " << totalWrites << "\n";
        std::cout << "Total Execution Cycles: " << totalCycles - idleCycles << "\n";
        std::cout << "Idle Cycles: " << idleCycles << "\n";
        std::cout << "Cache Misses: " << misses << "\n";
        std::cout << "Cache Miss Rate: " << std::fixed << std::setprecision(2) << missRate << "%\n";
        std::cout << "Cache Evictions: " << evictions << "\n";
        std::cout << "Writebacks: " << writebacks << "\n";
        std::cout << "Bus Invalidations: " << busInvalidations << "\n";
        std::cout << "Data Traffic (Bytes): " << dataTraffic << "\n\n";
    }
}

// Function to print overall bus summary.
// We assume the Bus class provides getTotalBusTransactions() and getBusTrafficBytes().
void printBusSummary(Bus *bus) {
    int totalBusTx = bus->getTotalBusTransactions();
    int totalTraffic = bus->getBusTrafficBytes();
    
    std::cout << "Overall Bus Summary:\n";
    std::cout << "Total Bus Transactions: " << totalBusTx << "\n";
    std::cout << "Total Bus Traffic (Bytes): " << totalTraffic << "\n";
}

int main(int argc, char* argv[]) {
    // For a test run, assume configuration is passed on the command line.
    // You can reuse your existing parseArguments.
    SimulationConfig config;
    config.tracePrefix = "app1";  // Example value
    config.s = 5;
    config.E = 2;
    config.b = 5;
    config.outputFilename = "output.txt";
    
    // Derived parameters.
    int numSets = (1 << config.s);
    // For a cache: 2^s sets * (2^E lines per set) * (blockSize in bytes)
    int blockSize = (1 << config.b);
    int numWays = config.E; // (Assuming E is already the number of ways; adjust if E is exponent)
    // For 4KB per core (2^12 bytes), we can approximate as:
    int cacheSizeKB = (numSets * numWays * blockSize) / 1024;
    
    // For demonstration, assume we have 4 cores:
    const int numCores = 4;
    std::vector<Processor*> processors;
    std::vector<Cache*> caches;
    
    // Create a Bus instance.
    Bus bus;
    
    // Create caches and processors.
    for (int i = 0; i < numCores; ++i) {
        // Construct trace file name (e.g., "app1_proc0.trace").
        std::string traceFile = config.tracePrefix + "_proc" + std::to_string(i) + ".trace";
        Cache* cache = new Cache(config.s, config.E, config.b, i, &bus);
        caches.push_back(cache);
        Processor* proc = new Processor(i, traceFile, cache, &bus);
        processors.push_back(proc);
    }
    
    // Run the simulation loop (assuming your simulation is complete).
    // For demonstration purposes, we assume simulation has run and then output stats.
    // (Your main.cpp simulation loop should already complete.)
    
    // Print Simulation Parameters.
    printSimulationParameters(config, numSets, cacheSizeKB);
    
    // Print per-core statistics.
    printCoreStats(processors, caches);
    
    // Print Bus summary.
    printBusSummary(&bus);
    
    // Write output to file if desired.
    // Here we simply print to stdout.
    
    // Clean up.
    for (auto proc : processors) {
        delete proc;
    }
    for (auto cache : caches) {
        delete cache;
    }
    
    return 0;
}
