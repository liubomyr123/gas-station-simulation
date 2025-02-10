CC = gcc

SRC = read_data_parser.c cjson/cJSON.c
OBJ = $(SRC:.c=.o)
TARGET = read_data_parser

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)