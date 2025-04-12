#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include <cstdint>

// Define the bus transaction types.
// Note: We've included a third type for "read with intent to write".
enum class BusTransactionType {
    BusRd,       // Read miss transaction.
    BusRdX,      // Write hit transaction that requires invalidation.
    BusRdWITWr   // Read with intent to write (used for write misses).
};

// Structure representing a bus transaction.
struct BusTransaction {
    BusTransactionType type;  // Type of bus transaction.
    uint32_t address;         // Memory address involved in the transaction.
    int sourceProcessorId;    // ID of the processor that initiated the transaction.
};

// The Bus class declaration.
class Bus {
public:
    // Constructor.
    Bus();

    // Adds a new bus transaction to the internal queue.
    void addTransaction(const BusTransaction &transaction);

    // Resolves all queued transactions.
    // Parameter: a vector of pointers to Cache objects that should snoop and update their state.
    void resolveTransactions(const std::vector<class Cache*>& caches);

    // Clears the transaction queue.
    void clearTransactions();
    
private:
    // Container holding the bus transactions for the current cycle.
    std::vector<BusTransaction> transactions;
};

#endif // BUS_HPP
