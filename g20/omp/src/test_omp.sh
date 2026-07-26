#!/bin/bash

# Nome do teu executável
EXEC="./docs"
# EXEC="./docs-nosimd"

# Lista dos prefixos dos testes
TESTS=(
    "../../../tests/ex5-1d"
    "../../../tests/ex10-2d"
    "../../../tests/ex1000-50d"
    "../../../tests/ex1000-50000-200"
    "../../../tests/ex40-100000-20"
    "../../../tests/ex1000-50000-5"
)

echo "A iniciar os testes..."
echo "----------------------"


# Para cada teste, corre com 1 2 4 6 threads
for TEST in "${TESTS[@]}"; do
    INPUT="${TEST}.in"
    EXPECTED="${TEST}.txt"

    # Verifica se o input existe antes de correr
    if [ ! -f "$INPUT" ]; then
        echo -e "\e[33mSKIPPED\e[0m: $INPUT não encontrado."
        continue
    fi

    for NTHREADS in 1 2 4 6 ; do
        MY_OUTPUT="${TEST}.myout.${NTHREADS}t"
        echo -n "A correr $INPUT com OMP_NUM_THREADS=$NTHREADS... "
        OMP_NUM_THREADS=$NTHREADS $EXEC "$INPUT" > "$MY_OUTPUT"

        if diff -q "$MY_OUTPUT" "$EXPECTED" > /dev/null; then
            echo -e "\e[32mPASS\e[0m"
            rm "$MY_OUTPUT"
        else
            echo -e "\e[31mFAIL\e[0m"
            echo "   -> Usa 'diff $MY_OUTPUT $EXPECTED' para veres as diferenças exatas."
        fi
    done
done

echo "----------------------"
echo "Testes concluídos."