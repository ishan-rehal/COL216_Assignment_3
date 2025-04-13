#include "Bus.hpp"
#include "Cache.hpp" // For accessing Cache methods like handleBusTransaction and resolvePendingTransaction
#include <iostream>

Bus::Bus()
{
    // Nothing special to do.
}

void Bus::addTransaction(const BusTransaction &transaction)
{
    // If the transaction is an upgrade, enqueue it in the upgrade queue.
    if (transaction.type == BusTransactionType::BusUpgr)
    {
        upgradeQueue.push_back(transaction);
    }
    else
    {
        // Otherwise, enqueue it in the normal transactions queue.
        transactions.push_back(transaction);
    }
}

void Bus::processUpgrade(const BusTransaction &tx, const std::vector<class Cache *> &caches)
{
    // Process the BusUpgr transaction immediately:
    // For every cache except the issuing one, invalidate the block if it is Shared.
    for (auto cache : caches)
    {
        if (cache->getProcessorId() == tx.sourceProcessorId)
            continue;
        cache->invalidateShared(tx.address);
    }
    // Optionally, we could also notify the issuing cache that its upgrade is resolved.
}

void Bus::resolveTransactions(const std::vector<class Cache *> &caches)
{
    // First, process all BusUpgr transactions from the upgrade queue.
    if (!upgradeQueue.empty())
    {
        for (const auto &tx : upgradeQueue)
        {
            processUpgrade(tx, caches);
            // Optionally, you could call resolvePendingTransaction on the issuing cache here if needed.
            // For now, we assume the issuing cache will update its own state immediately upon receiving the upgrade.
        }
        upgradeQueue.clear();
    }

    // Now process the normal FIFO transaction queue.
    if (transactions.empty())
        return;

    // Process only the transaction at the front of the FIFO queue.
    BusTransaction tx = transactions.front();
    Cache *issuingCache = nullptr;
    for (auto cache : caches)
    {
        if (cache->getProcessorId() == tx.sourceProcessorId)
        {
            issuingCache = cache;
            break;
        }
    }
    if (!issuingCache)
        return;

    // If the issuing cache's pending transaction is still in progress...
    if (issuingCache->isTransactionPending())
    {
        if (issuingCache->getPendingCycleCount() == -1)
        {
            int delay = 100; // Default memory fetch delay.
            if (tx.type == BusTransactionType::BusRd)
            {
                int n = caches[0]->getBlockSizeBytes() / 4; // number of words per block
                bool copyAvailable = false;
                for (auto cache : caches)
                {
                    if (cache->getProcessorId() == tx.sourceProcessorId)
                        continue;
                    if (cache->hasBlock(tx.address))
                        copyAvailable = true;
                }
                if (copyAvailable)
                    delay = 2 * n;
            }
            issuingCache->resolvePendingTransaction(tx.type, tx.address, delay);
        }
    }
    else
    {
        // If resolved, remove the transaction.
        transactions.erase(transactions.begin());
    }
}

void Bus::clearTransactions()
{
    transactions.clear();
    upgradeQueue.clear();
}
