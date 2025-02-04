
# Define the compiler to be used
CC = gcc

# Compiler flags:
# -Wall     -> Enable all common warnings
# -Wextra   -> Enable extra warnings
# -pthread  -> Add support for multithreading
CFLAGS = -Wall -Wextra -pthread

# Define debug flags (list of flags)
DEBUG_FLAGS = DEBUG_

# Add -D prefix to each debug flag
DEBUG_DEFINE_FLAGS = $(addprefix -D, $(DEBUG_FLAGS))

# Name of the output executable
TARGET = gas_station

# Find all `.c` source files in the current directory
SRCS = $(wildcard *.c)

# Default target: compile the program
all: $(TARGET)

# Rule to build the executable from source files
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Run the compiled program
run: $(TARGET)
	./$(TARGET)

# Build and then run the program in one step
start: all run

# Rule for debugging
debug: $(SRCS)
	$(CC) $(CFLAGS) $(DEBUG_DEFINE_FLAGS) -o $(TARGET) $(SRCS)

# Rule for compiling with DEBUG_ and running the program (debug and run)
start_debug: debug
	./$(TARGET)

# Clean up generated files
clean:
	rm -f $(TARGET) $(OBJS) *.d

# Define targets that are not actual files
.PHONY: all clean run start
