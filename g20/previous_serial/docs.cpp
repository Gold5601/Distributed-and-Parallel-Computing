#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>   
#include <limits>  
#include <omp.h>

using namespace std;

struct Document{
    int id;
    vector<double> scores;
    int cabinet_id;
};

struct Cabinet{
    vector<double> centroid;
    int doc_count;
    vector<double> sum_scores;
};


int store_input(string file_name, int& C, int& D, int& S, vector<Document>& documents, vector<Cabinet>& cabinets) {
    ifstream inputFile(file_name);

    if (!inputFile.is_open()) {
        cerr << "Erro ao abrir o ficheiro: " << file_name << std::endl;
        return 1;
    }

    if (inputFile >> C >> D >> S) {

        cabinets.resize(C);

        for(int i = 0; i < C; i++) {
            cabinets[i].centroid.assign(S, 0.0);    
            cabinets[i].sum_scores.assign(S, 0.0);  
            cabinets[i].doc_count = 0; 
        }

        for (int i = 0; i < D; ++i) {
            int doc_id;
            inputFile >> doc_id; 
            
            Document new_document;

            new_document.id = doc_id;
            new_document.cabinet_id = i % C;

            vector<double> new_scores(S);
            
            for (int j = 0; j < S; ++j) {
                inputFile >> new_scores[j];
            }

            new_document.scores = new_scores;

            documents.push_back(new_document);
        }
    }

    return 0;
}

int update_centroid(vector<Document>& documents, vector<Cabinet>& cabinets, int S, int C, int D){

    //Reset Cabinets
    for(int i = 0; i < C; ++i){ 
        cabinets[i].doc_count = 0;
        cabinets[i].sum_scores.assign(S, 0.0);
    }

    //Go through all documents and add their score values to their respective cabinets
    for(int i = 0; i < D; ++i){
        int cab_id = documents[i].cabinet_id;
        cabinets[cab_id].doc_count++;
        for(int j = 0; j < S; ++j){
            cabinets[cab_id].sum_scores[j] += documents[i].scores[j];
        }
    }
    //Calculate the mean of those scores
    for(int i = 0; i < C; ++i){
        if(cabinets[i].doc_count > 0){
            int cab_doc_count = cabinets[i].doc_count;
            for(int j = 0; j < S; ++j){
                cabinets[i].centroid[j] = cabinets[i].sum_scores[j] / cab_doc_count;
            }
        } else{
            for(int j = 0; j < S; ++j){
                cabinets[i].centroid[j] = 0.0;
            }
        }
    }
    return 0;
}

bool reassign_documents(vector<Document>& documents, vector<Cabinet>& cabinets, int S, int C, int D) {
    bool reassignment = false;

    for(int i = 0; i < D; ++i) {   
        double min_dist = numeric_limits<double>::max(); 
        int best_cab = documents[i].cabinet_id;

        for(int j = 0; j < C; ++j) {
            double sum_sq = 0.0;
            
            for(int k = 0; k < S; ++k) {
                double diff = documents[i].scores[k] - cabinets[j].centroid[k];
                
                sum_sq += diff * diff; 
            }
            
            double dist = sqrt(sum_sq);

            if(dist < min_dist) {
                min_dist = dist;
                best_cab = j;
            }
        }

        if(documents[i].cabinet_id != best_cab) {
            documents[i].cabinet_id = best_cab; 
            reassignment = true; 
        }
    }
    
    return reassignment; 
}
int print_documents(vector<Document>& documents, int D, int S){

    for(int i = 0; i < D; ++i){
        printf("Document: %i Cabinet: %i", documents[i].id, documents[i].cabinet_id);
        printf(" Scores: ");
        for(int j = 0; j < S; ++j){
            printf("%f ", documents[i].scores[j]);
        }
        printf("\n");
    }
    return 0;
}

int main(int argc, char *argv[]) {

    if(argc != 2){
        cerr << "Expected: ./docs text.in" << endl;
        return 1;
    }

    int C = 0, D = 0, S = 0;
    vector<Document> documents;
    vector<Cabinet> cabinets;

    if (store_input(argv[1], C, D, S, documents, cabinets) != 0) {
        return 1;
    }    

    double exec_time = -omp_get_wtime();
    
    bool reassignment;
    do {
        update_centroid(documents, cabinets, S, C, D);
        
        reassignment = reassign_documents(documents, cabinets, S, C, D);
        
    } while (reassignment);

    exec_time += omp_get_wtime();

    fprintf(stderr, "%.8fs\n", exec_time);

    for (int i = 0; i < D; ++i) {
        printf("%d\n", documents[i].cabinet_id);
    }

    return 0;
}
