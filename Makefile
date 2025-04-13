# Compiler and flags.
CXX = g++
CXXFLAGS = -Wall -O2 -Iheader

# Directories.
SRCDIR = src
BINDIR = .

# Source and object files.
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/Bus.cpp $(SRCDIR)/Cache.cpp $(SRCDIR)/Processor.cpp $(SRCDIR)/TraceParser.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable.
TARGET = L1simulate

# Default target.
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Rule for compiling .cpp files to .o files.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
