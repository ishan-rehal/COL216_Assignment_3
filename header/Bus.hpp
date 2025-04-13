#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include <cstdint>

// Define the bus transaction types.
enum class BusTransactionType
{
    BusRd,      // Read miss transaction.
    BusRdX,     // Write transaction (write miss or write hit when not in Shared).
    BusRdWITWr, // Read with intent to write (for write misses).
    BusUpgr     // Upgrade: write hit on a Shared block that must cause immediate invalidation.
};

// Structure representing a bus transaction.
struct BusTransaction
{
    BusTransactionType type; // Type of bus transaction.
    uint32_t address;        // Memory address involved (assumed block-aligned).
    int sourceProcessorId;   // ID of the processor that initiated the transaction.
};

class Bus
{
public:
    Bus();

    // Adds a new bus transaction to the appropriate queue.
    void addTransaction(const BusTransaction &transaction);

    // Resolves queued transactions.
    // This function is called each simulation cycle.
    // BusUpgr transactions are processed first (from upgradeQueue),
    // then the normal transactions (from transactions vector) are processed.
    void resolveTransactions(const std::vector<class Cache *> &caches);

    // Clears the transaction queues.
    void clearTransactions();

    // Process a BusUpgr transaction immediately.
    void processUpgrade(const BusTransaction &tx, const std::vector<class Cache *> &caches);
    // Returns the total number of bus transactions issued.
    int getTotalBusTransactions() const { return totalBusTransactions; }

    // Returns the total bus traffic (in bytes).
    // You could update a member variable dataTrafficBytes in Bus.cpp each time a transaction is processed.
    int getBusTrafficBytes() const { return busTrafficBytes; }
    int updateBusTrafficBytes(const std::vector<Cache*>& caches);

private:
    // FIFO queue for normal transactions.
    std::vector<BusTransaction> transactions;
    // Separate high-priority queue for upgrade transactions.
    std::vector<BusTransaction> upgradeQueue;
    int busTrafficBytes = 0;
    int totalBusTransactions=0;
};

#endif // BUS_HPP
