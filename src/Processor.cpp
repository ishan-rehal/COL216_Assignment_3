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
    // If the cache has a pending transaction, decrement its pending delay.
    if (l1Cache->isTransactionPending())
    {
        l1Cache->decrementPendingCycle();
        idleCycles++;
        totalCycles++;
        return;
    }

    if (currentInstructionIndex >= instructions.size())
    {
        totalCycles++;
        return;
    }

    Instruction &instr = instructions[currentInstructionIndex];
    int  dummy = 0;
    bool hit   = false;

    if (instr.op == OperationType::READ)
        hit = l1Cache->read(instr.address, dummy, bus);   // ← pass bus
    else
        hit = l1Cache->write(instr.address, dummy, bus);  // ← pass bus

    totalCycles++;          // one core cycle always elapses

    currentInstructionIndex++;
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
