CC = mpicxx
CFLAGS = -O2 -std=c++17 -Wall -Wextra -DNDEBUG -pedantic -fopenmp -Wno-cast-function-type

all: docs docs-mpi-omp docs-mpi-noomp

docs: docs-mpi.cpp
	$(CC) $(CFLAGS) $< -o $@

docs-mpi-omp: docs-mpi+omp.cpp
	$(CC) $(CFLAGS) $< -o $@

docs-mpi-noomp: docs-mpi-noomp.cpp
	$(CC) $(filter-out -fopenmp,$(CFLAGS)) $< -o $@

clean:
	rm -f docs docs-mpi-omp docs-mpi-noomp

.PHONY: all clean