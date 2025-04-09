#ifndef TRACE_PARSER_HPP
#define TRACE_PARSER_HPP

#include <string>
#include <vector>
#include <cstdint>

enum class OperationType { READ, WRITE };

struct Instruction {
    OperationType op;
    uint32_t address;
};

class TraceParser {
public:
    // Parse the given trace file into a vector of Instructions.
    static std::vector<Instruction> parseTraceFile(const std::string &filename);
};

#endif // TRACE_PARSER_HPP
