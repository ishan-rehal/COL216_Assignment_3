#pragma once
#include "Cache.hpp"
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>  // Add this for file output

inline void debug_print_caches(const std::vector<Cache*>& caches, int cycle)
{
    static std::ofstream outFile("output.txt", std::ios::app);  // Open file in append mode
    
    outFile << "\n===== DEBUG  cycle " << cycle << " =====\n";
    for (auto c : caches)
    {
        outFile << "CPU" << c->getProcessorId()
                << (c->isTransactionPending() ? "  PENDING" : "  READY  ")
                << "  addr=0x" << std::hex << std::setw(8) << std::setfill('0')
                << c->getPendingAddress() << std::dec
                << "  delay=" << c->getPendingCycleCount() << '\n';
    }
    outFile.flush();  // Ensure output is written immediately
}
