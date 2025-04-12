#include "Bus.hpp"
#include "Cache.hpp"  // Needed to call Cache::handleBusTransaction
#include <iostream>

Bus::Bus() { }

void Bus::addTransaction(const BusTransaction &#include "Bus.hpp"
    #include "Cache.hpp"  // Forward declaration to use Cache* in resolveTransactions.
    
    // Constructor.
    Bus::Bus() {
        // Implementation to be added.
    }
    
    // Adds a transaction to the bus queue.
    void Bus::addTransaction(const BusTransaction &transaction) {
        // Implementation to be added.
    }
    
    // Resolves all transactions by notifying each cache.
    void Bus::resolveTransactions(const std::vector<class Cache*>& caches) {
        // Implementation to be added.
    }
    
    // Clears the transaction queue.
    void Bus::clearTransactions() {
        // Implementation to be added.
    }
    transaction) {
    transactions.push_back(transaction);
}

void Bus::resolveTransactions(const std::vector<Cache*>& caches) {
    // Process each bus transaction.
    for (const auto &tx : transactions) {
        // Let every cache (including the source cache) snoop the transaction.
        for (auto cache : caches) {
            cache->handleBusTransaction(tx);
        }
    }
    clearTransactions();
}

void Bus::clearTransactions() {
    transactions.clear();
}
