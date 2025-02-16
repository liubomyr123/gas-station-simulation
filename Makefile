
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

# Validation output executable
VALIDATION_TARGET = validation

# cjson `.c` source files
CJSON_SRCS = cjson/cJSON.c

# All `.c` source files in the current directory for simulation
SRCS = main.c utils.c util_read_data_parser.c $(CJSON_SRCS)

# All `.c` files for validation script
VALIDATION_SRCS = validate_json_file.c util_read_data_parser.c $(CJSON_SRCS)

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

# Run validation script for data.json file
validate: ${VALIDATION_SRCS}
	$(CC) ${CFLAGS} -o ${VALIDATION_TARGET} ${VALIDATION_SRCS}

# Rule for compiling with DEBUG_ and running the program (debug and run)
start_debug: debug
	./$(TARGET)

start_validate: validate
	@echo -n "\n📄 Enter the file name (or leave empty for none): "; \
	read -r ARGS; \
	if [ -z "$$ARGS" ]; then \
		echo "\n🧐 No arguments provided. Program is going to use default 'data.json' file.\n"; \
	else \
		echo "\n✅ You entered: $$ARGS.\n"; \
	fi; \
	eval ./$(VALIDATION_TARGET) $$ARGS

# Clean up generated files
clean:
	rm -f $(TARGET) ${VALIDATION_TARGET} $(OBJS) *.d

# Define targets that are not actual files
.PHONY: all clean run start
