// Bus.cpp
#include "Bus.hpp"
#include "Cache.hpp"
#include <iostream>

Bus::Bus()
    : totalBusTransactions(0),
      pendingBusWr(false),
      pendingBusWrCycles(0),
      pendingBusWrSourceId(-1)
{
}

void Bus::addTransaction(const BusTransaction &transaction)
{
    ++totalBusTransactions;
    switch (transaction.type)
    {
        case BusTransactionType::BusUpgr:
            // Invalidate any shared copies immediately
            upgradeQueue.push_back(transaction);
            break;

        case BusTransactionType::BusWr:
            // Queue a write-back to memory
            writebackQueue.push_back(transaction);
            break;

        default:
            // Regular loads/stores
            transactions.push_back(transaction);
    }
}

void Bus::processUpgrade(
    const BusTransaction &tx,
    const std::vector<Cache *> &caches)
{
    for (auto cache : caches)
    {
        if (cache->getProcessorId() == tx.sourceProcessorId) continue;
        cache->invalidateShared(tx.address);
    }
}

void Bus::resolveTransactions(const std::vector<Cache *> &caches)
{
    //
    // 0) Finish any outstanding write-back stall
    //
    if (pendingBusWr)
    {
        if (--pendingBusWrCycles > 0)
            return;                   // still busy writing back

        // write-back just completed
        pendingBusWr = false;
        for (auto cache : caches)
        {
            if (cache->getProcessorId() == pendingBusWrSourceId)
            {
                cache->is_writing_to_mem = false;
            }
        }
        pendingBusWrSourceId = -1;
        
        return;
    }

    //
    // 1) Process all pending BusUpgr invalidations
    //
    if (!upgradeQueue.empty())
    {
        for (auto &tx : upgradeQueue)
            processUpgrade(tx, caches);
        upgradeQueue.clear();
    }

    //
    // 2) If we have a write-back queued, start it now
    //
    if (!writebackQueue.empty())
    {
        auto wb = writebackQueue.front();
        writebackQueue.erase(writebackQueue.begin());
        pendingBusWr        = true;
        pendingBusWrCycles  = 100;
        pendingBusWrSourceId = wb.sourceProcessorId;
        return;
    }

    //
    // 3) No normal transactions?  We’re done.
    //
    if (transactions.empty())
        return;

    //
    // 4) Snooping: inform every other cache of this access
    //
    BusTransaction tx = transactions.front();
    for (auto cache : caches)
    {
        if (cache->getProcessorId() != tx.sourceProcessorId)
            cache->handleBusTransaction(tx);
    }

    //
    // 5) Let the source cache resolve its miss
    //
    Cache *src = nullptr;
    for (auto c : caches)
    {
        if (c->getProcessorId() == tx.sourceProcessorId && 
            c->getPendingAddress() == tx.address)  // ADD ADDRESS CHECK
        {
            src = c;
            break;
        }
    }
    
    // If no matching cache found, dequeue this transaction and return
    if (!src) {
        std::cout << "No matching cache found for transaction: addr=0x" 
                 << std::hex << tx.address 
                 << " src=" << std::dec << tx.sourceProcessorId << "\n";
        transactions.erase(transactions.begin());
        return;
    }

    // MODIFIED: If we've already set the delay but the transaction is still pending,
    // just return and let it complete
    if (src->isTransactionPending() && src->getPendingCycleCount() > 0) {
        return;
    }

    // If the source is still waiting on that block…
    if (src->isTransactionPending()) {
        // We haven't set its delay yet
        if (src->getPendingCycleCount() == -1) {
            int delay = 100;  // default: memory
            int extraDelay = 0;
            // If it’s a load, check for cache-to-cache
            if (tx.type == BusTransactionType::BusRd)
            {
                int n = caches[0]->getBlockSizeBytes() / 4;
                int supplierId = -1;
                for (auto c : caches)
                {
                    if (c->getProcessorId() == tx.sourceProcessorId) continue;
                    if (c->hasBlock(tx.address))
                    {
                        supplierId = c->getProcessorId();
                        if(c->is_writing_to_mem) {
                            extraDelay = 100; // add 100 cycles if the supplier is writing to memory
                            // std::cout << "[Bus] Supplier " << supplierId << " is writing to memory\n";
                        }
                        break;
                    }
                }
                if (supplierId >= 0)
                {
                    // 2·N cycles, plus 100 if that supplier is itself mid-writeback
                    delay = 2 * n + extraDelay;
                }
            }

            src->resolvePendingTransaction(tx.type, tx.address, delay);
        }
        // ADDED: If delay has been set to 0, dequeue the transaction
        else if (src->getPendingCycleCount() == 0) {
            transactions.erase(transactions.begin());
        }
    }
    else
    {
        // Miss fully resolved → dequeue
        transactions.erase(transactions.begin());
    }
}

void Bus::clearTransactions()
{
    transactions.clear();
    upgradeQueue.clear();
    writebackQueue.clear();
}

int Bus::updateBusTrafficBytes(const std::vector<Cache *> &caches)
{
    int total = 0;
    for (auto c : caches)
        total += c->getDataTrafficBytes();
    busTrafficBytes = total;
    return total;
}

void Bus::printBusinfo() const
{
    std::cout << "Bus Information:\n";
    std :: cout << "Pending BusWr: " << (pendingBusWr ? "Yes" : "No") << "\n";
    std::cout << "Pending BusWr Source: " << pendingBusWrSourceId << "\n";
    std::cout << "Pending BusWr Cycles: " << pendingBusWrCycles << "\n";

    std::cout << "Upgrade Queue:\n";
    if (upgradeQueue.empty())
    {
        std::cout << "  <empty>\n";
    }
    else
    {
        for (size_t i = 0; i < upgradeQueue.size(); ++i)
            std::cout << "  [" << i
                      << "] type=" << static_cast<int>(upgradeQueue[i].type)
                      << " addr=0x" << std::hex << upgradeQueue[i].address << std::dec
                      << " src=" << upgradeQueue[i].sourceProcessorId
                      << "\n";
    }

    std::cout << "WriteBack Queue:\n";
    if (writebackQueue.empty())
    {
        std::cout << "  <empty>\n";
    }
    else
    {
        for (size_t i = 0; i < writebackQueue.size(); ++i)
            std::cout << "  [" << i
                      << "] type=" << static_cast<int>(writebackQueue[i].type)
                      << " addr=0x" << std::hex << writebackQueue[i].address << std::dec
                      << " src=" << writebackQueue[i].sourceProcessorId
                      << "\n";
    }

    std::cout << "Transaction Queue:\n";
    if (transactions.empty())
    {
        std::cout << "  <empty>\n";
    }
    else
    {
        for (size_t i = 0; i < transactions.size(); ++i)
            std::cout << "  [" << i
                      << "] type=" << static_cast<int>(transactions[i].type)
                      << " addr=0x" << std::hex << transactions[i].address << std::dec
                      << " src=" << transactions[i].sourceProcessorId
                      << "\n";
    }

    if (pendingBusWr)
        std::cout << "Pending BusWr: " << pendingBusWrCycles << " cycles remaining\n";
}

bool Bus::hasPendingtransaction() const
{
    return pendingBusWr
        || !upgradeQueue.empty()
        || !writebackQueue.empty()
        || !transactions.empty();
}
