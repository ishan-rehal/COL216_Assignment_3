#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "Processor.hpp"
#include "Cache.hpp"
#include "Bus.hpp"
#ifdef DEBUG
#include "Debug.hpp"
#endif

// Structure for simulation configuration.
struct SimulationConfig
{
    std::string tracePrefix;
    int s; // Number of set index bits.
    int E; // Associativity.
    int b; // Block bits (block size in bytes = 2^b).
    std::string outputFilename;
};

// Simple command-line parser.
SimulationConfig parseArguments(int argc, char *argv[])
{
    SimulationConfig config;
    // Default values.
    config.s = 4; // e.g., 16 sets.
    config.E = 2; // 2-way set associative.
    config.b = 5; // e.g., block size = 2^5 = 32 bytes.
    config.outputFilename = "output.log";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        {
            config.tracePrefix = argv[++i];
        }
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
        {
            config.s = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-E") == 0 && i + 1 < argc)
        {
            config.E = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc)
        {
            config.b = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            config.outputFilename = argv[++i];
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            std::cout << "Usage: " << argv[0]
                      << " -t <tracePrefix> -s <s> -E <E> -b <b> -o <outputFilename>\n";
            exit(0);
        }
    }

    return config;
}

int main(int argc, char *argv[])
{
    // Parse command-line arguments.
    SimulationConfig config = parseArguments(argc, argv);

    const int numCores = 4; // Quad-core simulation.
    std::vector<Processor *> processors;
    std::vector<Cache *> caches;

    // Create a Bus instance.
    Bus bus;

    // Create a separate cache and processor for each core.
    // IMPORTANT: When constructing caches, pass the processor's id.
    for (int i = 0; i < numCores; ++i)
    {
        // Construct trace file name (e.g., "app1_proc0.trace").
        std::string traceFile = config.tracePrefix + "_proc" + std::to_string(i) + ".trace";
        Cache *cache = new Cache(config.s, config.E, config.b, i); // Pass processor id to cache
        caches.push_back(cache);
        Processor *proc = new Processor(i, traceFile, cache, &bus);
        processors.push_back(proc);
    }

    // Global clock simulation loop.
    int globalClock = 0;
    bool allFinished = false;
    while (!allFinished)
    {
        allFinished = true;
        // std::cout << "Global Clock: " << globalClock << "\n";
#ifdef DEBUG
        debug_print_caches(caches, globalClock);
#endif

        // Each processor executes its cycle.
        for (int i = 0; i < numCores; ++i)
        {
            if (!processors[i]->isFinished())
            {
                processors[i]->executeCycle();
                allFinished = false;
            }
        }
        // Resolve bus transactions at the end of the cycle.
        bus.resolveTransactions(caches);

        globalClock++;
    }

    // Output simulation statistics.
    std::cout << "Global Clock: " << globalClock << " cycles\n";
    for (int i = 0; i < numCores; ++i)
    {
        std::cout << "Processor " << i
                  << " - Total Cycles: " << processors[i]->getTotalCycles()
                  << ", Idle Cycles: " << processors[i]->getIdleCycles() << "\n";
    }

    // Clean up.
    for (auto proc : processors)
    {
        delete proc;
    }
    for (auto cache : caches)
    {
        delete cache;
    }

    return 0;
}
// End of main.cpp