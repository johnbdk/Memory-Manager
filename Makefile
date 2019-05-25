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

SOURCES = $(SRC)/lock.c $(SRC)/queue.c $(SRC)/streamflow.c
OBJECTS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SOURCES))
EXEC = $()

all: main checkUnusedPageblocks

main: $(OBJECTS) $(OBJ)/main.o
	$(CC) $(CFLAGS) $(SOURCES) $(SRC)/main.c -o $@ $(LFLAGS) -lpthread

checkUnusedPageblocks: $(OBJECTS) $(OBJ)/checkUnusedPageblocks.o
	$(CC) $(CFLAGS) $(SOURCES) $(SRC)/checkUnusedPageblocks.c -o $@ $(LFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@ $(LFLAGS)

#main.o: $(SRC)/main.c
#	$(CC) -c $(CFLAGS) $< -o %(OBJ)/$@

#checkUnusedPageblocks.o: $(SRC)/checkUnusedPageblocks.c
#	$(CC) -c $(CFLAGS) $< -o %(OBJ)/$@

.PHONY: clean
# Cleans the executable and the object files
clean:
	$(RM) -r obj main checkUnusedPageblocks
	mkdir obj
