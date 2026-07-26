docs: docs-omp.cpp
	g++ -O2 -std=c++17 -Wall -Wextra -fopenmp -fopenmp-simd -DNDEBUG -pedantic docs-omp-nosimd.cpp -o docs-nosimd
	g++ -O2 -std=c++17 -Wall -Wextra -fopenmp -fopenmp-simd -DNDEBUG -pedantic docs-omp.cpp -o docs

clean:
	rm -f docs docs-nosimd