#include "TraceParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<Instruction> TraceParser::parseTraceFile(const std::string &filename) {
    std::vector<Instruction> instructions;
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening trace file: " << filename << std::endl;
        return instructions;
    }
    
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;
        
        std::istringstream iss(line);
        char opChar;
        std::string addrStr;
        if (!(iss >> opChar >> addrStr))
            continue;
        
        Instruction inst;
        if (opChar == 'R' || opChar == 'r') {
            inst.op = OperationType::READ;
        } else if (opChar == 'W' || opChar == 'w') {
            inst.op = OperationType::WRITE;
        }
        
        uint32_t address;
        std::istringstream(addrStr) >> std::hex >> address;
        inst.address = address;
        
        instructions.push_back(inst);
    }
    
    return instructions;
}
