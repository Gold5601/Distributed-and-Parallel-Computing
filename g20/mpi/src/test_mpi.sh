#!/bin/bash

TESTS=(
    "../../../tests/ex5-1d"
    "../../../tests/ex5-100-3"
    "../../../tests/ex10-2d"
    "../../../tests/ex40-100000-20"
    "../../../tests/ex1000-50d"
    "../../../tests/ex1000-50000-5"
    "../../../tests/ex1000-50000-200"
    "../../../tests/ex10000-1000000-100"
)

# Variações de threads OpenMP para testar
OMP_THREADS_CONFIGS=(1 2 4 6)

# Função para executar testes
run_tests() {
    local exec_name=$1
    local test_mode=$2
    local omp_threads=$3
    
    echo ""
    echo "A iniciar os testes ($test_mode)..."
    echo "------------------------------"
    
    local total=0
    local passed=0
    local failed=0

    for TEST in "${TESTS[@]}"; do
        INPUT="${TEST}.in"
        EXPECTED="${TEST}.out"

        if [ ! -f "$INPUT" ]; then
            echo -e "\e[33mSKIPPED\e[0m: $INPUT não encontrado."
            continue
        fi

        for NPROCS in 1 4 16; do
            MY_OUTPUT="${TEST}.myout.${NPROCS}p"
            
            echo -n "A correr $INPUT: MPI_PROCS=$NPROCS, OMP_THREADS=$omp_threads... "
            
            # Exportar número de threads OpenMP
            export OMP_NUM_THREADS=$omp_threads
            
            mpirun --bind-to none -np $NPROCS $exec_name "$INPUT" > "$MY_OUTPUT"

            ((total++))
            if diff -q "$MY_OUTPUT" "$EXPECTED" > /dev/null; then
                echo -e "\e[32mPASS\e[0m"
                ((passed++))
                rm "$MY_OUTPUT"
            else
                echo -e "\e[31mFAIL\e[0m"
                ((failed++))
                echo "   -> Erro na execução com P=$NPROCS"
            fi
        done
    done

    echo "------------------------------"
    echo "Resultados para $test_mode:"
    echo "  Total: $total | Passou: $passed | Falhou: $failed"
}

# Compilar todas as versões
echo "Compilando versões..."
make clean
make docs-mpi-noomp
make docs
echo ""

# Testes com MPI apenas
run_tests "./docs-mpi-noomp" "MPI apenas" 1

# Testes com MPI + OpenMP + SIMD com diferentes números de threads
for threads in "${OMP_THREADS_CONFIGS[@]}"; do
    run_tests "./docs" "MPI + OpenMP (OMP_NUM_THREADS=$threads) + SIMD" $threads
done

echo ""
echo "=========================================="
echo "Todos os testes concluídos!"
echo "=========================================="