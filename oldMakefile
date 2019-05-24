# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall

# Pthread library flag
LFLAGS = -lm

# Compile with -O3 optimization
OPTFLAGS = -O3

# Directories
SRC = src
OBJ = obj

SOURCES = $(wildcard $(SRC)/*.c)
OBJECTS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SOURCES))
main: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@ $(LFLAGS)

.PHONY: clean
# Cleans the executable and the object files
clean:
	$(RM) main $(OBJECTS)
