#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <iomanip>
#include "Processor.hpp"
#include "Cache.hpp"
#include "Bus.hpp"

#ifdef DEBUG
#include "Debug.hpp"
#endif

// Structure for simulation configuration.
struct SimulationConfig {
    std::string tracePrefix;
    int s; // Number of set index bits.
    int E; // Associativity.
    int b; // Block bits (block size in bytes = 2^b).
    std::string outputFilename;
};

// Simple command-line parser.
SimulationConfig parseArguments(int argc, char *argv[]) {
    SimulationConfig config;
    // Default values.
    config.s = 4; // e.g., 16 sets.
    config.E = 2; // 2-way set associative.
    config.b = 5; // e.g., block size = 2^5 = 32 bytes.
    config.outputFilename = "output.log";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            config.tracePrefix = argv[++i];
        }
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            config.s = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-E") == 0 && i + 1 < argc) {
            config.E = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            config.b = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config.outputFilename = argv[++i];
        }
        else if (strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: " << argv[0]
                      << " -t <tracePrefix> -s <s> -E <E> -b <b> -o <outputFilename>\n";
            exit(0);
        }
    }
    return config;
}

// Function to print simulation parameters.
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
    std::cout << "Bus: Central snooping bus\n\n";
}

// Function to print per-core statistics.
// (This example assumes that your Processor and Cache classes
// provide getters for all required counters. You may need to add them if not yet implemented.)
void printCoreStatistics(const std::vector<Processor*>& processors, const std::vector<Cache*>& caches) {
    for (size_t i = 0; i < processors.size(); ++i) {
        // These functions should be implemented in your classes.
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
        std::cout << "Total Execution Cycles: " << totalCycles << "\n";
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
void printBusSummary(Bus &bus, const std::vector<Cache*>& caches) {
    int totalTraffic = bus.updateBusTrafficBytes(caches); // Updates and returns total bus traffic.
    std::cout << "Overall Bus Summary:\n";
    std::cout << "Total Bus Transactions: " << bus.getTotalBusTransactions() << "\n";
    std::cout << "Total Bus Traffic (Bytes): " << totalTraffic << "\n";
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments.
    SimulationConfig config = parseArguments(argc, argv);

    const int numCores = 4; // Quad-core simulation.
    std::vector<Processor*> processors;
    std::vector<Cache*> caches;

    // Create a Bus instance.
    Bus bus;

    // Create a separate cache and processor for each core.
    // IMPORTANT: When constructing caches, pass the processor's id.
    for (int i = 0; i < numCores; ++i) {
        // Construct trace file name (e.g., "app1_proc0.trace").
        std::string traceFile = config.tracePrefix + "_proc" + std::to_string(i) + ".trace";
        Cache* cache = new Cache(config.s, config.E, config.b, i, &bus);
        caches.push_back(cache);
        Processor* proc = new Processor(i, traceFile, cache, &bus);
        processors.push_back(proc);
    }

    // Global clock simulation loop.
    int globalClock = 0;
    bool allFinished = false;
    while (!allFinished) {
#ifdef DEBUG
        debug_print_caches(caches, globalClock);
#endif

        allFinished = true;
        // Let each processor execute one cycle.
        // if(globalClock % 100000000 == 0) {
        //     bus.printBusinfo();
        // }
        // Resolve any bus transactions at the end of the cycle.
        bus.resolveTransactions(caches);
        for (int i = 0; i < numCores; ++i) {
            if (!processors[i]->isFinished() || bus.hasPendingtransaction()) {
                processors[i]->executeCycle();
                allFinished = false;
            }
            // std :: cout << "Pending Bus Wr cycles ->" << bus.getPendingBusWrCycles() << "\n";
            if(globalClock % 100000000 == 0) {
                // std::cout << "Global Clock: " << globalClock << ", Processor " << i << " executed the instruction ->. " << processors[i]->getInstructionsExecuted() << "\n";
                
                
                //print cache info
                // caches[i]->printCacheInfo();
            }
        }
        

        globalClock++;
    }

    // After simulation, update bus traffic bytes.
    int totalBusTraffic = bus.updateBusTrafficBytes(caches);

    // Derived parameters.
    int numSets = (1 << config.s);
    int blockSize = (1 << config.b);
    int numWays = config.E; // (if E is the number of ways)
    int cacheSizeKB = (numSets * numWays * blockSize) / 1024;

    //Print Bus info
    // bus.printBusinfo();

    // Print simulation output.
    std::cout << "\nSimulation Output:\n";
    printSimulationParameters(config, numSets, cacheSizeKB);
    printCoreStatistics(processors, caches);
    printBusSummary(bus, caches);
    // std::cout << "Global Clock: " << globalClock << " cycles\n";

    // Clean up.
    for (auto proc : processors) {
        delete proc;
    }
    for (auto cache : caches) {
        delete cache;
    }
    return 0;
}
