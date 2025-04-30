# Cache Simulation and Analysis - COL216 Assignment 3

This repository contains a cache simulator implementation that models a quad-core processor with L1 caches using the MESI coherence protocol. The simulator is designed to analyze the impact of different cache parameters on execution time.

## Project Structure

- [`src`](src) - Source code for the cache simulator (`Cache.cpp`, `Bus.cpp`, `Processor.cpp`, `main.cpp`, `StatsPrinter.cpp`, `TraceParser.cpp`)
- [`header`](header) - Header files for the simulator (`Cache.hpp`, `Bus.hpp`, `Processor.hpp`, `DataArray.hpp`, `TagArray.hpp`, `TraceParser.hpp`, `Debug.hpp`)
- [`graph_tc`](graph_tc) - Test case directories containing memory access traces (e.g., `tc_1/1_0.trace`)
- [`generate_and_plot.py`](generate_and_plot.py) - Python script to run simulations with different configurations and plot results
- `L1simulate.exe`/`L1simulate` - Executable simulator for Windows/Linux (needs to be compiled)

## Cache Simulator Features

- **Architecture**: Four-core processor with private L1 caches.
- **Cache Coherence**: MESI protocol (Modified, Exclusive, Shared, Invalid) implemented via a central snooping bus.
- **Write Policy**: Write-back with write-allocate.
- **Replacement Policy**: LRU (Least Recently Used).
- **Configurable Parameters**:
  - Cache size (controlled by the number of set index bits, `s`).
  - Associativity (number of ways, `E`).
  - Block size (controlled by the number of block offset bits, `b`).

## Running the Simulator

The simulator can be compiled by using the make command:

```bash
#MakeFile works for Linux only
make
```
Then you can run the executable as follows:
```bash
# Windows
.\L1simulate.exe -t <trace_prefix> -s <set_bits> -E <associativity> -b <block_bits>

# Linux/Mac
./L1simulate -t <trace_prefix> -s <set_bits> -E <associativity> -b <block_bits>
```

Where:
- `-t <trace_prefix>`: Path prefix for the trace files. The simulator expects files named `<trace_prefix>_proc0.trace`, `<trace_prefix>_proc1.trace`, etc. (e.g., `graph_tc/tc_1/1`).
- `-s <set_bits>`: Number of set index bits (Cache has 2<sup>s</sup> sets).
- `-E <associativity>`: Associativity (number of ways per set).
- `-b <block_bits>`: Number of block offset bits (Block size is 2<sup>b</sup> bytes).

Example:
```bash
./L1simulate -t graph_tc/tc_1/1 -s 6 -E 2 -b 5
```

## Testing Cache Configurations

The [`generate_and_plot.py`](generate_and_plot.py) script automates running tests with different cache configurations specified in the `PARAMS` list.

To run the script:
```bash
python generate_and_plot.py
```

This script performs the following actions:
1. Iterates through each test case directory specified in `TC_LIST` (e.g., `tc_1`, `tc_2`).
2. For each test case, it finds the base trace file prefix (e.g., `1` for `1_0.trace`).
3. It creates temporary links or copies (depending on the OS) named `*_procN.trace` if they don't exist, as the simulator expects this format.
4. Runs the simulator (`L1simulate` or `L1simulate.exe`) for each parameter configuration defined in `PARAMS`:
   - **Default**: s=6, E=2, b=5 (64 sets, 2-way, 32-byte blocks)
   - **sets×2**: s=7, E=2, b=5 (128 sets, 2-way, 32-byte blocks) - Doubles cache size by doubling sets.
   - **block×2**: s=6, E=2, b=6 (64 sets, 2-way, 64-byte blocks) - Doubles block size.
   - **assoc×2**: s=6, E=4, b=5 (64 sets, 4-way, 32-byte blocks) - Doubles associativity.
5. Parses the simulator's standard output to extract the "Global Clock" value (total execution cycles).
6. Records the maximum execution cycles for each configuration into [`max_cycles.csv`](max_cycles.csv).
7. Generates bar charts (`*.png`) for each test case, plotting the maximum execution cycles against the different parameter settings.
8. Cleans up the temporary trace file links/copies.

## Test Cases

The repository includes four test cases within the [`graph_tc`](graph_tc) directory:
- `tc_1`: Contains basic read/write patterns.
- `tc_2`: Designed to exhibit spatial locality.
- `tc_3`: Designed to exhibit temporal locality.
- `tc_4`: Designed to stress cache coherence with potential contention between cores.

Each test case directory contains trace files for four cores, named like `1_0.trace`, `1_1.trace`, `1_2.trace`, `1_3.trace`.

## Output and Analysis

After running `generate_and_plot.py`, you will find:
- [`max_cycles.csv`](max_cycles.csv): A CSV file containing the maximum execution cycles recorded for each test case and parameter configuration.
- `tc_1_maxcycles.png`, `tc_2_maxcycles.png`, etc.: Bar charts visualizing the data from the CSV file, one for each test case.

**Analysis**:
By examining the generated plots, you can analyze how changing cache parameters affects performance (measured in execution cycles) for different memory access patterns:
- **Increasing Sets (Cache Size)**: Generally expected to decrease cycles by reducing capacity misses, especially if the working set fits better in the larger cache.
- **Increasing Block Size**: Can decrease cycles by improving spatial locality (fetching more useful data at once), but can also increase cycles due to increased miss penalty and potential false sharing or cache pollution if locality is poor.
- **Increasing Associativity**: Generally expected to decrease cycles by reducing conflict misses, but may slightly increase hit time (not modeled here) and complexity. The benefit diminishes as associativity increases further.

Observe how these effects vary across the different test cases (`tc_1` to `tc_4`), which represent different workload characteristics.

## Requirements

- **Python**: Version 3.6+
  - `matplotlib`: For generating plots (`pip install matplotlib`)
- **C++ Compiler**: A modern C++ compiler (like g++) that supports C++11 or later to build the simulator (`L1simulate`/`L1simulate.exe`).
- **Operating System**: Windows, Linux, or macOS.

## Implementation Details

- **Simulator Core (`main.cpp`)**: Parses command-line arguments, sets up the processors, caches, and bus, runs the cycle-by-cycle simulation loop, and prints final statistics.
- **Processor (`Processor.cpp`, `Processor.hpp`)**: Represents a single core. It reads instructions from its assigned trace file, issues read/write requests to its L1 cache, and stalls if the cache access is not immediately satisfied.
- **Cache (`Cache.cpp`, `Cache.hpp`)**: Implements the L1 cache logic, including tag/set/offset extraction, LRU replacement, MESI state transitions, handling hits/misses, interacting with the bus for misses and coherence actions (BusRd, BusRdX, BusUpgr, BusWr), and snooping on bus transactions.
- **Bus (`Bus.cpp`, `Bus.hpp`)**: Models the shared system bus. It queues transactions from different caches, resolves them based on priority (Upgrades first), handles delays, and broadcasts transactions for snooping.
- **Trace Parser (`TraceParser.cpp`, `TraceParser.hpp`)**: Reads trace files containing memory operations ('R' or 'W') and addresses.
- **Data Structures (`DataArray.hpp`, `TagArray.hpp`)**: Represent the physical storage for cache data and tags.
- **Statistics (`StatsPrinter.cpp`, `main.cpp`)**: Functions within `main.cpp` (or potentially a separate `StatsPrinter.cpp`) gather and print statistics like execution cycles, cache misses, writebacks, bus invalidations, and bus traffic.
- **Plotting Script (`generate_and_plot.py`)**: Orchestrates the simulation runs and visualizes the results.