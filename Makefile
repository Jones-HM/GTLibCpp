CC=g++
CFLAGS=-std=c++17 -w -DGT_USE_CE_PARSER
LIBS=GTLibc.cpp CEParser.cpp

all: GenericTrainer.exe

GenericTrainer.exe: GenericTrainer.cpp $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean run

clean:
	rm -f GenericTrainer.exe

run: GenericTrainer.exe
	./GenericTrainer.exe
