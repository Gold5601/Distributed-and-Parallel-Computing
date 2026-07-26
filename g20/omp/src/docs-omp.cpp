#include <iostream>
#include <fstream>
#include <string>
#include <bits/stdc++.h>
#include <omp.h>

using namespace std;

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

    // get the firt line
    if (getline(file, line)){
        stringstream first_line(line);

        // first 3 elements
        first_line >> Cabinets >> Documents >> Subjects;

        // resize to create Documents Spaces
        subject_scores.assign(Documents * Subjects, 0.0);
        associated_cabins.resize(Documents);
    };

    // get the remaining lines
    while (file >> id) {
        int offset = id * Subjects;
        for (sub = 0; sub < Subjects; sub++) {
            file >> subject_scores[offset + sub];
        }
    }

    // close file
    file.close();
    return 0;
}

int associate_cabin() {
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
        for (int i = 0; i < documents; i++) {
            int cabin = i % cabinets;
            associated_cabins[i] = cabin;
            local[cabin] += 1;
        }
    }
    #pragma omp parallel for schedule(static)
    for (int cabin = 0; cabin < cabinets; cabin++) {
        int sum = 0;
        for (int t = 0; t < num_threads; t++) {
            sum += size_deltas[(t * cabinets) + cabin];
        }
        cabins_size[cabin] = sum;
    }

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
        #pragma omp parallel for schedule(static) reduction(+:raw_cabins_values[:cab_sub_count])
        for (int id = 0; id < documents; id++) {
            int cabin = associated_cabins[id];
            int doc_offset = id * subjects;
            int cab_offset = cabin * subjects;
            const double* doc_ptr = scores + doc_offset;
            double* cab_ptr = raw_cabins_values + cab_offset;
            // for each subject inside the document scores
            #pragma omp simd
            for (int subject = 0; subject < subjects; subject++) {
                cab_ptr[subject] += doc_ptr[subject];
            }
        }

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
            for (int id = 0; id < documents; id++) {
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
        #pragma omp parallel for schedule(static)
        for (int cabin = 0; cabin < cabinets; cabin++) {
            int sum = 0;
            for (int t = 0; t < num_threads; t++) {
                sum += size_deltas[(t * cabinets) + cabin];
            }
            cabins_size[cabin] += sum;
        }
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
    string file_name;
    vector<int> results;
    double exec_time;

    // args from the input
    if (argc > 2)
        cerr << "Provide only one argument!";

    // read the input file
    if (read_input(argv[1])) {
        cerr << "Error parsing the file";
        return 1;
    }

    exec_time = -omp_get_wtime();
    if (associate_cabin()) {
        cerr << "Error parsing the file";
        return 1;
    }
    exec_time += omp_get_wtime();

    fprintf(stderr, "%.8fs\n", exec_time);
    if (print_results(associated_cabins)) {
        cerr << "Something went terrible wrong";
        return 1;
    }

    return 0;
}
