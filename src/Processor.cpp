#include "Processor.hpp"
#include <iostream>
#include <fstream>

Processor::Processor(int id,
                     const std::string &traceFile,
                     Cache *cache,
                     Bus *busPtr)
    : processorId(id),
      l1Cache(cache),
      bus(busPtr), // ← store pointer
      currentInstructionIndex(0),
      stallCounter(0),
      totalCycles(0),
      idleCycles(0)
{
    loadTrace(traceFile);
}

void Processor::loadTrace(const std::string &traceFile)
{
    instructions = TraceParser::parseTraceFile(traceFile);
}

void Processor::executeCycle()
{
    // 0) If _this_ core is the one doing a 100-cycle write-back, stall:
    if (bus->getPendingBusWr() &&
    bus->getPendingBusWrSource() == processorId)
    {
        idleCycles++;
        totalCycles++;
        return;
    }

    // If the cache has a pending transaction, decrement its pending delay.
    if (l1Cache->isTransactionPending())
    {
        l1Cache->decrementPendingCycle();
        idleCycles++;
        totalCycles++;
        return;
    }

    // If no more instructions, just increment cycles
    if (currentInstructionIndex >= instructions.size())
    {
        totalCycles++;
        return;
    }

    // Get the current instruction
    Instruction &instr = instructions[currentInstructionIndex];
    int dummy = 0;
    bool hit = false;

    if (instr.op == OperationType::READ)
    {
        hit = l1Cache->read(instr.address, dummy, bus);
        if (hit || !l1Cache->isTransactionPending()) {
            // Instruction completed or no transaction started
            totalReadInstructions++;
            currentInstructionIndex++; // Only advance if instruction completed
        }
    }
    else if (instr.op == OperationType::WRITE)
    {
        hit = l1Cache->write(instr.address, dummy, bus);
        if (hit || !l1Cache->isTransactionPending()) {
            // Instruction completed or no transaction started
            totalWriteInstructions++;
            currentInstructionIndex++; // Only advance if instruction completed
        }
    }

    totalCycles++; // one core cycle always elapses
}

bool Processor::isFinished() const
{
    return (currentInstructionIndex >= instructions.size() &&
            !l1Cache->isTransactionPending());
}

int Processor::getTotalCycles() const
{
    return totalCycles;

}

int Processor::getIdleCycles() const
{
    return idleCycles;
}
