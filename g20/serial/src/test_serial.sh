#!/bin/bash

# Nome do teu executável
EXEC="./docs"

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

# Iterar sobre cada caso de teste
for TEST in "${TESTS[@]}"; do
    INPUT="${TEST}.in"
    EXPECTED="${TEST}.txt"
    MY_OUTPUT="${TEST}.myout"

    # Verifica se o input existe antes de correr
    if [ ! -f "$INPUT" ]; then
        echo -e "\e[33mSKIPPED\e[0m: $INPUT não encontrado."
        continue
    fi

    echo -n "A correr $INPUT... "

    # Executa o programa e guarda o stdout no ficheiro temporário
    $EXEC "$INPUT" > "$MY_OUTPUT"

    # Compara o teu output com o expected output
    if diff -q "$MY_OUTPUT" "$EXPECTED" > /dev/null; then
        echo -e "\e[32mPASS\e[0m"
        rm "$MY_OUTPUT" # Limpa o ficheiro temporário se estiver tudo correto
    else
        echo -e "\e[31mFAIL\e[0m"
        echo "   -> Usa 'diff $MY_OUTPUT $EXPECTED' para veres as diferenças exatas."
    fi
done

echo "----------------------"
echo "Testes concluídos."