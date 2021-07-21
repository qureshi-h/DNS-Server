# Sample Makefile
# CC - compiler
# OBJ - compiled source files that should be linked
# COPT - compiler flags
CC=clang
OBJ=helper1.o
COPT=-Wall -Wpedantic -g
BIN_PHASE1=phase1
BIN_PHASE2=dns_svr

# Running "make" with no argument will make the first target in the file
all: $(BIN_PHASE1) $(BIN_PHASE2)

$(BIN_PHASE2): main.c $(OBJ)
	$(CC) -o $(BIN_PHASE2) main.c $(OBJ) $(COPT)

$(BIN_PHASE1): phase1.c $(OBJ)
	$(CC) -o $(BIN_PHASE1) phase1.c $(COPT)

%.o: %.c %.h
	$(CC) -c $< $(COPT) -g

format:
	clang-format -i *.c *.h

clean:
	rm -f $(BIN_PHASE1) $(BIN_PHASE2) *.o dns_svr.log
