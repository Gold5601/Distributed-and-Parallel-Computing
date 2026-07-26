#!/bin/bash
#SBATCH --job-name=benchmark_single
#SBATCH --output=benchmark_out.txt
#SBATCH --error=benchmark__err.txt
#SBATCH --account=f202500009hpcvlabistul2x
#SBATCH --partition=normal-x86
#SBATCH --nodes=16
#SBATCH --ntasks=2048
#SBATCH --ntasks-per-node=64
#SBATCH --cpus-per-task=1
#SBATCH --time=06:00:00

module purge
module load GCC/11.3.0
module load OpenMPI/4.1.4-GCC-11.3.0
module load Python/3.10.8

export OMP_NUM_THREADS=1

python3 benchmark_single.py
