# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Wall

# Pthread library flag
LFLAGS = -lm

# Compile with -O3 optimization
OPTFLAGS = -O3

# Rename my_malloc / my_free to malloc / free
DFLAGS = -Dmalloc=my_malloc -Dfree=my_free

# Comment this to remove code for metrics
DMETRICSFLAGS = #-DMETRICS

# Comment this to remove code for debug prints
DBGPRINTS = #-DDBUG

# Directories
SRC = src
OBJ = obj
TST = tests
ROOT = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))

SOURCES = $(wildcard $(SRC)/*.c)
OBJECTS = $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SOURCES))
TESTS = $(wildcard $(TST)/*.c)
TEST_OBJ = $(patsubst $(TST)/%.c,$(OBJ)/%.o,$(TESTS))
EXEC = $(patsubst $(TST)/%.c, %, $(TESTS))


all: $(OBJECTS) $(EXEC)

$(EXEC): % : $(OBJECTS) $(TEST_OBJ) 
	$(CC) $(CFLAGS) $(SOURCES) $(TST)/$@.c -o $@ $(LFLAGS) $(DFLAGS) $(DMETRICSFLAGS) $(DBGPRINTS) -lpthread

$(TEST_OBJ): $(OBJ)/%.o : $(TST)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@ $(LFLAGS) $(DMETRICSFLAGS) $(DBGPRINTS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@ $(LFLAGS) $(DMETRICSFLAGS) $(DBGPRINTS)

.PHONY: clean
# Cleans the executable and the object files
clean:
	$(RM) -r obj $(EXEC)
	mkdir obj
