#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <vector>
#include "Cache.hpp"
#include "TraceParser.hpp"
#include "Bus.hpp"  // Added for bus transactions support

class Processor {
public:
    Processor(int id, const std::string &traceFile, Cache* cache, Bus* bus);
    
    // Simulate one cycle for this processor.
    void executeCycle();
    // New overload: simulate one cycle for this processor with a Bus pointer.
    void executeCycle(Bus *bus);
    // Returns the total number of instructions in this core's trace.
    int getTotalInstructions() const { return instructions.size(); }

    // Returns the total number of read instructions executed.
    int getTotalReads() const { return totalReadInstructions; }

    // Returns the total number of write instructions executed.
    int getTotalWrites() const { return totalWriteInstructions; }
    // Check if the processor has finished processing its trace.
    bool isFinished() const;
    // Get total and idle cycle counts for statistics.
    int getTotalCycles() const;
    int getIdleCycles() const;
    int getInstructionsExecuted() const { return currentInstructionIndex; }
    
private:
    int processorId;
    Cache* l1Cache;
    Bus* bus;  // Pointer to the bus for bus transactions.
    std::vector<Instruction> instructions;
    int currentInstructionIndex;
    // Stall counter for memory delays.
    int stallCounter;
    // Statistics counters.
    int totalCycles;
    int idleCycles;
    int totalReadInstructions = 0;
    int totalWriteInstructions = 0;
    // Helper to load instructions from the trace file.
    void loadTrace(const std::string &traceFile);
};

#endif // PROCESSOR_HPP
