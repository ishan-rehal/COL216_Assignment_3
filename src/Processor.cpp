#include "Processor.hpp"
#include <iostream>
#include <fstream>

Processor::Processor(int id, const std::string &traceFile, Cache *cache)
    : processorId(id), l1Cache(cache), currentInstructionIndex(0),
      stallCounter(0), totalCycles(0), idleCycles(0)
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
    int cyclesForOperation = 0;
    bool hit = false;

    if (instr.op == OperationType::READ)
    {
        hit = l1Cache->read(instr.address, cyclesForOperation);
    }
    else if (instr.op == OperationType::WRITE)
    {
        hit = l1Cache->write(instr.address, cyclesForOperation);
    }

    if (hit)
    {
        totalCycles++;
    }
    else
    {
        stallCounter = cyclesForOperation - 1;
        totalCycles++;
    }

    currentInstructionIndex++;
}

bool Processor::isFinished() const
{
    return (currentInstructionIndex >= instructions.size() && stallCounter == 0);
}

int Processor::getTotalCycles() const
{
    return totalCycles;
}

int Processor::getIdleCycles() const
{
    return idleCycles;
}
