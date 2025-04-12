#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <vector>
#include "Cache.hpp"
#include "TraceParser.hpp"
#include "Bus.hpp"  // Added for bus transactions support

class Processor {
public:
    Processor(int id, const std::string &traceFile, Cache* cache);
    
    // Simulate one cycle for this processor.
    void executeCycle();
    // New overload: simulate one cycle for this processor with a Bus pointer.
    void executeCycle(Bus *bus);
    
    // Check if the processor has finished processing its trace.
    bool isFinished() const;
    // Get total and idle cycle counts for statistics.
    int getTotalCycles() const;
    int getIdleCycles() const;
    
private:
    int processorId;
    Cache* l1Cache;
    std::vector<Instruction> instructions;
    int currentInstructionIndex;
    // Stall counter for memory delays.
    int stallCounter;
    // Statistics counters.
    int totalCycles;
    int idleCycles;
    
    // Helper to load instructions from the trace file.
    void loadTrace(const std::string &traceFile);
};

#endif // PROCESSOR_HPP
