#include <iostream>
#include <fstream>
#include <string>
#include <bits/stdc++.h>
#include <cstdlib>
#include <omp.h>
#include <mpi.h>

using namespace std;

#define SEED 1234
#define RAND_RANGE 10.0
#define UNIF01 ((double) rand() / RAND_MAX)

int Cabinets;
int Documents;
int Subjects;
vector<double> subject_scores; // flatten [[score,score],[score,score],...]
vector<int> associated_cabins; // [(id)cabin, (id+1)cabin,...]

// read input based on the file_path
int read_input(string file_name) {

    int id, sub;

    string line;
    ifstream file(file_name);

    // try to open file
    if (!file.is_open()) {
        cerr << "Unable to open file!" << endl;
        return 1;
    }

    // get the first line
    if (getline(file, line)){
        stringstream first_line(line);

        // first 3 elements
        first_line >> Cabinets >> Documents >> Subjects;

        // resize to create Documents Spaces
        subject_scores.assign(Documents * Subjects, 0.0);
        associated_cabins.resize(Documents);
    };

    // close file - we don't read the rest
    file.close();

    // Generate random values for all documents
    srand(SEED);
    for(id = 0; id < Documents; id++) {
        int offset = id * Subjects;
        for (sub = 0; sub < Subjects; sub++) {
            subject_scores[offset + sub] = UNIF01 * RAND_RANGE;
        }
    }

    return 0;
}

int associate_cabin(int init, int end, int rank, int size) {
    vector<double> cabins_values(Cabinets * Subjects, 0.0);
    vector<int> cabins_size(Cabinets, 0);
    
    // CORREÇÃO 1: Separar os vetores para o C++ não se confundir nos tipos!
    vector<int> local_cabins_size(Cabinets, 0);
    vector<double> local_cabins_values(Cabinets * Subjects, 0.0);

    // round-robin
    for (int i = init; i < end ; i++) {
        associated_cabins[i] = i % Cabinets ;
        local_cabins_size[i%Cabinets] += 1;
    }

    MPI_Allreduce(
        local_cabins_size.data(), 
        cabins_size.data(),       
        Cabinets,                 
        MPI_INT,                  
        MPI_SUM,                  
        MPI_COMM_WORLD            
    );

    // CORREÇÃO 2: Usar inteiros para o changed para o MPI não falhar em nenhum cluster!
    int changed = 1;
    int local_changed = 1;
    
    while (changed) {
        changed = 0;
        local_changed = 0;

        local_cabins_values.assign(Cabinets*Subjects, 0.0);
        cabins_values.assign(Cabinets*Subjects, 0.0);
        
        vector<int> local_cabins_size_deltas(Cabinets, 0); 

        for (int id = init; id < end; id++) {
            int cabin = associated_cabins[id]; 

            for (int subject = 0; subject < Subjects; subject++) {
                local_cabins_values[(cabin * Subjects) + subject] += subject_scores[(id * Subjects) + subject];
            }
        }
        
        // Agora sim, é um Double a sério!
        MPI_Allreduce(
            local_cabins_values.data(), 
            cabins_values.data(),    
            Cabinets*Subjects,     
            MPI_DOUBLE,                  
            MPI_SUM,                  
            MPI_COMM_WORLD           
        );


        for (int cabin= 0; cabin < Cabinets; cabin++) {
            for (int subject=0; subject < Subjects; subject++) {
                if (cabins_size[cabin] == 0) {
                    cabins_values[cabin*Subjects + subject] = 0.0; 
                }
                else {
                    cabins_values[cabin*Subjects + subject] /= cabins_size[cabin];
                }
            }
        }

        // compute distances
        int current_associated, new_associated;
        double minimum;
        for (int id = init; id < end; id++) {
            current_associated = associated_cabins[id];
            new_associated = current_associated;
            minimum = DBL_MAX;

            for (int id_cabin = 0; id_cabin < Cabinets; id_cabin++) {
                double current_dist = 0.0;
                int doc_offset = id * Subjects;
                int cab_offset = id_cabin * Subjects;

                for (int s = 0; s < Subjects; s++) {
                    double dif = subject_scores[doc_offset + s] - cabins_values[cab_offset + s];
                    current_dist += dif * dif;
                }

                if (current_dist < minimum) {
                    minimum = current_dist;
                    new_associated = id_cabin;
                }
            }

            if (new_associated != current_associated) {
                local_cabins_size_deltas[new_associated]++;
                local_cabins_size_deltas[current_associated]--;
                associated_cabins[id] = new_associated;
                local_changed = 1; // Usamos 1 em vez de true
            }
        }
        
        vector<int> global_deltas(Cabinets, 0);
        MPI_Allreduce(local_cabins_size_deltas.data(), global_deltas.data(), Cabinets, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        for(int i = 0; i < Cabinets; i++) {
            cabins_size[i] += global_deltas[i];
        }

        // Allreduce do changed usando MPI_INT
        MPI_Allreduce(&local_changed, &changed, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        
    }

    // --- GRANDE RECOLHA FINAL ---
    vector<int> counts(size, 0);
    vector<int> displacements(size, 0);

    for (int i = 0; i < size; i++) {
        int c_chunk = Documents / size;
        int c_init = c_chunk * i;
        int c_end = c_init + c_chunk;
        
        if (i == size - 1) {
            c_end = Documents;
        }
        
        counts[i] = c_end - c_init; 
        displacements[i] = c_init;  
    }

    int my_count = end - init;

    // O Chefe cria um vetor em branco para receber tudo
    vector<int> final_cabins;
    if (rank == 0) {
        final_cabins.resize(Documents);
    }

    MPI_Gatherv(
        &associated_cabins[init], 
        my_count,                 
        MPI_INT,                  
        final_cabins.data(), 
        counts.data(),            
        displacements.data(),     
        MPI_INT,                  
        0,                        
        MPI_COMM_WORLD            
    );

    // O Chefe fica com a versão final
    if (rank == 0) {
        associated_cabins = final_cabins;
    }
    
    return 0;
}

int print_results(vector<int> results) {
    int size = results.size();

    cout << results[0];
    for (int cabin = 1; cabin < size; cabin++) {
        cout << "\n" << results[cabin];
    }

    cout << endl;
    return 0;

}
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    string file_name;
    vector<int> results;
    double exec_time;
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // args from the input
    if (argc > 2) {
        if (rank == 0) cerr << "Provide only one argument!";
        MPI_Finalize(); // <--- Dizer adeus antes de sair com erro!
        return 1;
    }

    // read the input file
    if (read_input(argv[1])) {
        if (rank == 0) cerr << "Error parsing the file";
        MPI_Finalize(); // <--- Dizer adeus!
        return 1;
    }

    int chunk = Documents / size;
    int init = chunk * rank;
    int end = init + chunk;

    if (rank == size - 1) {
        end = Documents; 
    }

    exec_time = -MPI_Wtime(); 
    if (associate_cabin(init, end, rank, size)) {
        if (rank == 0) cerr << "Error calculating cabins";
        MPI_Finalize(); // <--- Dizer adeus!
        return 1;
    }
    exec_time += MPI_Wtime(); 

    if (rank == 0) {
        fprintf(stderr, "%.8fs\n", exec_time);
        if (print_results(associated_cabins)) {
            cerr << "Something went terrible wrong";
            MPI_Finalize(); // <--- Dizer adeus!
            return 1;
        }
    }

    MPI_Finalize();
    return 0;
}