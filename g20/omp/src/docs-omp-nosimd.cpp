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
    vector<double> cabins_values(Cabinets * Subjects, 0.0);
    vector<int> cabins_size(Cabinets, 0);

    int* raw_cabins_size = cabins_size.data();

    // round-robin
    #pragma omp parallel for reduction(+:raw_cabins_size[:Cabinets])
    for (int i=0; i < Documents ; i++) {
        associated_cabins[i] = i % Cabinets ;
        raw_cabins_size[i%Cabinets] += 1;
    }

    bool changed = true;
    while (changed) {
        changed = false;

        // clear previus values
        cabins_values.assign(Cabinets*Subjects, 0.0);

        double* raw_cabins_values = cabins_values.data();

        // compute average for each cabin
        #pragma omp parallel for reduction(+:raw_cabins_values[:Cabinets * Subjects])
        for (int id = 0; id < Documents; ++id) {
            int cabin = associated_cabins[id];
            // for each subject inside the document scores
            for (int subject = 0; subject < Subjects; subject++) {
                raw_cabins_values[(cabin * Subjects) + subject] += subject_scores[(id * Subjects) + subject];
            }
        }
        #pragma omp parallel for collapse(2)
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

        int* raw_cabins_size = cabins_size.data();

        // compute distances
        #pragma omp parallel for reduction(+:raw_cabins_size[:Cabinets]) reduction(||:changed)
        for (int id = 0; id < Documents; id++) {
            int current_associated = associated_cabins[id];
            int new_associated = current_associated;
            double minimum = DBL_MAX;

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
                raw_cabins_size[new_associated]++;
                raw_cabins_size[current_associated]--;
                associated_cabins[id] = new_associated;
                changed = true;
            }
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
    

    fprintf(stderr, "%.8fs\n", exec_time);

    return 0;
}