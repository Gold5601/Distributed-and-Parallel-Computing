# CPD Project

Parallel and Distributed Computing Project

## Overview

This repository contains an implementation of a document classification algorithm into cabinets/clusters, developed using different parallelization approaches to study the impact of sequential computation, OpenMP, MPI, and a hybrid version combining MPI + OpenMP + SIMD.

The project was organized to compare performance across:

- a sequential version as a baseline;
- a parallel version using OpenMP for shared-memory execution;
- distributed versions using MPI;
- a hybrid version combining MPI, OpenMP, and SIMD vectorization.

## Project objective

The goal is to analyze how the same clustering algorithm behaves when exposed to different levels of parallelism, evaluating:

- execution time;
- scalability;
- communication overhead;
- the impact of optimization techniques such as SIMD.

## Repository structure

- [g20/serial/src](g20/serial/src): sequential implementation and build files.
- [g20/omp/src](g20/omp/src): parallel versions using OpenMP and OpenMP + SIMD.
- [g20/mpi/src](g20/mpi/src): MPI versions, as well as test and benchmark scripts.
- [g20/mpi/Report.tex](g20/mpi/Report.tex): technical report describing the work and analyzing the results.
- [tests](tests): example input files and their expected outputs.

## How it works

The program reads an input file describing a set of documents and tries to assign them to cabinets/clusters based on Euclidean distances. The algorithm repeats iterations until the document assignments stabilize.

The execution produces:

- the final assignment results in stdout;
- the execution time in stderr.

## Requirements

To compile and run the project, the following are required:

- a C++ compiler with C++17 support;
- OpenMP;
- MPI (for example, mpicxx and mpirun);
- make.

## Build

### Sequential version

```bash
cd g20/serial/src
make
```

### OpenMP version

```bash
cd g20/omp/src
make
```

### MPI versions

```bash
cd g20/mpi/src
make
```

The MPI Makefile builds the following versions:

- docs: MPI + OpenMP + SIMD version;
- docs-mpi-omp: hybrid MPI + OpenMP version;
- docs-mpi-noomp: MPI-only version.

## Execution

### Sequential execution

```bash
cd g20/serial/src
./docs ../../tests/ex5-1d.in
```

### OpenMP execution

```bash
cd g20/omp/src
OMP_NUM_THREADS=4 ./docs ../../tests/ex5-1d.in
```

### MPI execution

```bash
cd g20/mpi/src
mpirun -np 4 ./docs-mpi-noomp ../../../tests/ex5-1d.in
```

### Hybrid MPI + OpenMP execution

```bash
cd g20/mpi/src
OMP_NUM_THREADS=4 mpirun -np 4 ./docs ../../../tests/ex5-1d.in
```

## Validation and testing

The project includes scripts to validate the implementations:

- [g20/mpi/src/test_mpi.sh](g20/mpi/src/test_mpi.sh): compiles the MPI versions and runs tests on several input files, comparing the output with the expected results in [tests](tests).
- [g20/mpi/src/benchmark.py](g20/mpi/src/benchmark.py): runs benchmarks to measure execution time and calculate speedups between the sequential and MPI versions.

## Important notes

- The MPI versions use a block distribution of documents across processes.
- The most optimized version combines three levels of parallelism: distributed (MPI), multithreading (OpenMP), and vectorized instructions (SIMD).
- The detailed project report is available in [g20/mpi/Report.tex](g20/mpi/Report.tex).

## Summary

This project provides a practical demonstration of how an algorithmic solution can evolve from a sequential implementation to a parallel and distributed approach, with a focus on performance, scalability, and communication efficiency.
