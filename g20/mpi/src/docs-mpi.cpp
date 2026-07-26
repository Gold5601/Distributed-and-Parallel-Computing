#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cfloat>
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
    const int cabinets = Cabinets;
    const int documents = Documents;
    const int subjects = Subjects;
    const int cab_sub_count = cabinets * subjects;
    const double* scores = subject_scores.data();
    const int num_threads = omp_get_max_threads();
    const int delta_size = cabinets * num_threads;

    vector<double> cabins_values(cab_sub_count, 0.0);
    vector<int> cabins_size(cabinets, 0);
    vector<int> size_deltas(delta_size, 0);

    // init round-robin
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int* local = size_deltas.data() + (tid * cabinets);
        #pragma omp for schedule(static)
        for (int i = init; i < end; i++) {
            int cabin = i % cabinets;
            associated_cabins[i] = cabin;
            local[cabin] += 1;
        }
    }

    // aggregate thread deltas
    vector<int> local_cabins_size(cabinets, 0);
    #pragma omp parallel for schedule(static)
    for (int cabin = 0; cabin < cabinets; cabin++) {
        int sum = 0;
        for (int t = 0; t < num_threads; t++) {
            sum += size_deltas[(t * cabinets) + cabin];
        }
        local_cabins_size[cabin] = sum;
    }

    // MPI: combine cabin counts from all ranks
    MPI_Allreduce(local_cabins_size.data(), cabins_size.data(), cabinets, MPI_INT,
                  MPI_SUM, MPI_COMM_WORLD);

    bool changed = true;
    while (changed) {
        changed = false;

        // clear previus values
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < cab_sub_count; i++) {
            cabins_values[i] = 0.0;
        }

        double* raw_cabins_values = cabins_values.data();

        // compute average for each cabin
        vector<double> local_cabins_values(cab_sub_count, 0.0);
        double* raw_local_cabins_values = local_cabins_values.data();

        #pragma omp parallel for schedule(static) reduction(+:raw_local_cabins_values[:cab_sub_count])
        for (int id = init; id < end; id++) {
            int cabin = associated_cabins[id];
            int doc_offset = id * subjects;
            int cab_offset = cabin * subjects;
            const double* doc_ptr = scores + doc_offset;
            double* cab_ptr = raw_local_cabins_values + cab_offset;
            // for each subject inside the document scores
            #pragma omp simd
            for (int subject = 0; subject < subjects; subject++) {
                cab_ptr[subject] += doc_ptr[subject];
            }
        }

        // MPI: sum local cabin values from all ranks
        MPI_Allreduce(local_cabins_values.data(), raw_cabins_values, cab_sub_count,
                      MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        #pragma omp parallel for schedule(static)
        for (int cabin = 0; cabin < cabinets; cabin++) {
            if (cabins_size[cabin] == 0) {
                #pragma omp simd
                for (int subject = 0; subject < subjects; subject++) {
                    cabins_values[cabin * subjects + subject] = 0.0;
                }
            }
            else {
                double inv = 1.0 / cabins_size[cabin];
                #pragma omp simd
                for (int subject = 0; subject < subjects; subject++) {
                    cabins_values[cabin * subjects + subject] *= inv;
                }
            }
        }

        #pragma omp parallel for schedule(static)
        for (int i = 0; i < delta_size; i++) {
            size_deltas[i] = 0;
        }

        // compute distances
        const double* cab_values = cabins_values.data();

        #pragma omp parallel reduction(||:changed)
        {
            int tid = omp_get_thread_num();
            int* local = size_deltas.data() + (tid * cabinets);
            #pragma omp for schedule(static)
            for (int id = init; id < end; id++) {
                int current_associated = associated_cabins[id];
                int new_associated = current_associated;
                double minimum = DBL_MAX;
                int doc_offset = id * subjects;
                const double* doc_ptr = scores + doc_offset;

                for (int id_cabin = 0; id_cabin < cabinets; id_cabin++) {
                    double current_dist = 0.0;
                    int cab_offset = id_cabin * subjects;
                    const double* cab_ptr = cab_values + cab_offset;

                    #pragma omp simd reduction(+:current_dist)
                    for (int s = 0; s < subjects; s++) {
                        double dif = doc_ptr[s] - cab_ptr[s];
                        current_dist += dif * dif;
                    }

                    if (current_dist < minimum) {
                        minimum = current_dist;
                        new_associated = id_cabin;
                    }
                }

                if (new_associated != current_associated) {
                    local[new_associated]++;
                    local[current_associated]--;
                    associated_cabins[id] = new_associated;
                    changed = true;
                }
            }
        }

        fill(local_cabins_size.begin(), local_cabins_size.end(), 0);
        #pragma omp parallel for schedule(static)
        for (int cabin = 0; cabin < cabinets; cabin++) {
            int sum = 0;
            for (int t = 0; t < num_threads; t++) {
                sum += size_deltas[(t * cabinets) + cabin];
            }
            local_cabins_size[cabin] = sum;
        }

        // MPI: sum deltas from all ranks and check if any rank changed
        vector<int> global_deltas(cabinets, 0);
        MPI_Allreduce(local_cabins_size.data(), global_deltas.data(), cabinets,
                      MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        for (int i = 0; i < cabinets; i++) {
            cabins_size[i] += global_deltas[i];
        }

        // MPI: check if any rank has changes
        int local_changed = changed ? 1 : 0;
        int global_changed = 0;
        MPI_Allreduce(&local_changed, &global_changed, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        changed = (global_changed != 0);
    }

    vector<int> counts(size, 0);
    vector<int> displacements(size, 0);

    for (int i = 0; i < size; i++) {
        int c_chunk = documents / size;
        int c_init = c_chunk * i;
        int c_end = c_init + c_chunk;

        if (i == size - 1) {
            c_end = documents;
        }

        counts[i] = c_end - c_init;
        displacements[i] = c_init;
    }

    int my_count = end - init;

    vector<int> final_cabins;
    if (rank == 0) {
        final_cabins.resize(documents);
    }

    MPI_Gatherv(&associated_cabins[init], my_count, MPI_INT, final_cabins.data(),
                counts.data(), displacements.data(), MPI_INT, 0, MPI_COMM_WORLD);

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
        MPI_Finalize();
        return 1;
    }

    // read the input file
    if (read_input(argv[1])) {
        if (rank == 0) cerr << "Error parsing the file";
        MPI_Finalize();
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
        MPI_Finalize();
        return 1;
    }
    exec_time += MPI_Wtime();

    if (rank == 0) {
        fprintf(stderr, "%.8fs\n", exec_time);
        if (print_results(associated_cabins)) {
            cerr << "Something went terrible wrong";
            MPI_Finalize();
            return 1;
        }
    }

    MPI_Finalize();
    return 0;
}
