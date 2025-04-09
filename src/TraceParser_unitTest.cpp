// TraceParser_unitTest.cpp
#include <iostream>
#include <fstream>
#include <vector>
#include "../header/TraceParser.hpp"

int main() {
    // Create a temporary trace file with 5 instructions.
    std::string testFilename = "test_trace.txt";
    std::ofstream outfile(testFilename);
    if (!outfile) {
        std::cerr << "Error creating test file." << std::endl;
        return 1;
    }
    // Write test instructions to the file.
    outfile << "R 0x7e1afe78\n";
    outfile << "W 0x7e1ac04c\n";
    outfile << "R 0x7e1afe80\n";
    outfile << "W 0x7e1afe90\n";
    outfile << "R 0x7e1afeA0\n";
    outfile.close();

    // Parse the trace file.
    std::vector<Instruction> instructions = TraceParser::parseTraceFile(testFilename);

    // Report the number of instructions parsed.
    std::cout << "Parsed " << instructions.size() << " instructions." << std::endl;
    
    // Display each instruction's details.
    for (size_t i = 0; i < instructions.size(); ++i) {
        std::cout << "Instruction " << i + 1 << ": " 
                  << (instructions[i].op == OperationType::READ ? "READ" : "WRITE")
                  << " " << std::hex << instructions[i].address << std::dec << std::endl;
    }

    // Simple assertions to check correctness.
    if (instructions.size() != 5) {
        std::cerr << "Test failed: Expected 5 instructions, got " << instructions.size() << std::endl;
        return 1;
    }
    // (Optional) Further tests can check specific instruction values if desired.
    
    std::cout << "TraceParser test passed successfully." << std::endl;
    return 0;
}
