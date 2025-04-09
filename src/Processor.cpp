#include "Processor.hpp"
#include <iostream>
#include <fstream>

Processor::Processor(int id, const std::string &traceFile, Cache* cache)
    : processorId(id), l1Cache(cache), currentInstructionIndex(0),
      stallCounter(0), totalCycles(0), idleCycles(0)
{
    loadTrace(traceFile);
}

void Processor::loadTrace(const std::string &traceFile) {
    // Use the TraceParser to load instructions.
    instructions = TraceParser::parseTraceFile(traceFile);
}

void Processor::executeCycle() {
    // If the processor is stalled, decrement the stall counter.
    if (stallCounter > 0) {
        stallCounter--;
        idleCycles++;
        totalCycles++;
        return;
    }
    
    // If no instructions remain, count the cycle.
    if (currentInstructionIndex >= instructions.size()) {
        totalCycles++;
        return;
    }
    
    // Process the current instruction.
    Instruction &instr = instructions[currentInstructionIndex];
    int cyclesForOperation = 0;
    bool hit = false;
    
    if (instr.op == OperationType::READ) {
        hit = l1Cache->read(instr.address, cyclesForOperation);
    } else if (instr.op == OperationType::WRITE) {
        hit = l1Cache->write(instr.address, cyclesForOperation);
    }
    
    if (hit) {
        totalCycles++;
    } else {
        // On a miss, set the stall counter (minus the current cycle).
        stallCounter = cyclesForOperation - 1;
        totalCycles++;
    }
    
    currentInstructionIndex++;
}

bool Processor::isFinished() const {
    return (currentInstructionIndex >= instructions.size() && stallCounter == 0);
}

int Processor::getTotalCycles() const {
    return totalCycles;
}

int Processor::getIdleCycles() const {
    return idleCycles;
}
