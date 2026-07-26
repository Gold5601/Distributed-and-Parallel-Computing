#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <omp.h>

struct Document {
    int id;
    int cabinet_id;
    std::vector<double> scores;
};

struct Cabinet {
    std::vector<Document> documents;
    std::vector<double> subject_averages;
};

int n_cabinets;
int n_documents;
int n_subjets;

std::vector<Document> documents;

int extract_input(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return -1;
    }

    // Parse the first line to extract n_cabinets, n_documents, and n_subjets
    if (fscanf(file, "%d %d %d", &n_cabinets, &n_documents, &n_subjets) != 3) {
        std::cerr << "Error reading first line of file: " << filename << std::endl;
        fclose(file);
        return -1;
    }

    // resize the documents vector to hold n_documents
    documents.resize(n_documents);

    // loop to populate the documents vector with the scores from each document
    for (int i = 0; i < n_documents; i++) {
        Document doc;
        doc.scores.resize(n_subjets);

        // document id
        if (fscanf(file, "%d", &doc.id) != 1) {
            std::cerr << "Error reading document id for document " << i << std::endl;
            fclose(file);
            return -1;
        }

        // scores for each document
        for (int j = 0; j < n_subjets; j++) {
            if (fscanf(file, "%lf", &doc.scores[j]) != 1) {
                std::cerr << "Error reading score for document " << i << ", subject " << j << std::endl;
                fclose(file);
                return -1;
            }
        }

        documents[i] = doc;
    }

    fclose(file);
    return 0;
}

int initial_assignment(std::vector<Cabinet>& cabinets) {
    for (size_t doc_id = 0; doc_id < documents.size(); doc_id++) {
        int cabinet_destination = doc_id % n_cabinets;
        cabinets[cabinet_destination].documents.push_back(documents[doc_id]);
        documents[doc_id].cabinet_id = cabinet_destination;
    }

    return 0;
}

int cabinet_subject_average(int cabinet_index, std::vector<Cabinet>& cabinets) {
    // compute the average score for each subject in the cabinet and store it in the cabinet's subject_averages vector
    Cabinet& cabinet = cabinets[cabinet_index];
    cabinet.subject_averages.resize(n_subjets, 0.0);

    for (int i = 0; i < n_subjets; i++) {
        double sum = 0.0;
        for (Document& doc : cabinet.documents) {
            sum += doc.scores[i];
        }
        cabinet.subject_averages[i] = sum / cabinet.documents.size();
    }

    return 0;
}

double compute_distance(const Document& doc, const Cabinet& cabinet) {
    double distance = 0.0;

    for (size_t i = 0; i < doc.scores.size(); i++) {
        distance += (doc.scores[i] - cabinet.subject_averages[i]) * (doc.scores[i] - cabinet.subject_averages[i]);
    }

    return sqrt(distance);
}

int rearange_document(int doc_id, std::vector<Cabinet>& cabinets) {
    int changes = 0;
    Document& doc = documents[doc_id];

    // compute best cabinet for a document
    int best_cabinet_id;
    double best_distance;
    for (size_t i = 0; i < cabinets.size(); i++) {
        double distance = compute_distance(doc, cabinets[i]);
        if (i == 0 || distance < best_distance) {
            best_distance = distance;
            best_cabinet_id = i;
        }
    }

    if (best_cabinet_id != doc.cabinet_id) {
        // remove the document from its current cabinet
        Cabinet& current_cabinet = cabinets[doc.cabinet_id];
        current_cabinet.documents.erase(std::remove_if(current_cabinet.documents.begin(), current_cabinet.documents.end(),
            [&doc](const Document& d) { return d.id == doc.id; }), current_cabinet.documents.end());

        // add the document to the new cabinet
        Cabinet& new_cabinet = cabinets[best_cabinet_id];
        new_cabinet.documents.push_back(doc);

        // update the document's cabinet_id
        doc.cabinet_id = best_cabinet_id;
        changes++;
    }

    return changes;
}


int main(int argc, char** argv) {
    double exec_time;
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    char *filename = argv[1];

    if (extract_input(filename) != 0) {
        std::cerr << "Error extracting input from file: " << filename << std::endl;
        return 1;
    }

    exec_time = -omp_get_wtime();
    // n_cabinets vectors of documents: each vector will hold the documents that belong to that cabinet
    std::vector<Cabinet> cabinets(n_cabinets);

    if (initial_assignment(cabinets) != 0) {
        std::cerr << "Error during initial assignment of documents to cabinets.\n";
        return 1;
    }

    // main loop
    bool flag_changes = true;
    while(flag_changes) {
        flag_changes = false;

        // compute the subject averages for each cabinet
        for (int i = 0; i < n_cabinets; i++) {
            if (cabinet_subject_average(i, cabinets) != 0) {
                std::cerr << "Error calculating subject averages for cabinet " << i << ".\n";
                return 1;
            }
        }

        // rearange documents based on the new subject averages
        for (int i = 0; i < n_documents; i++ ) {
            if (rearange_document(i, cabinets) > 0) {
                flag_changes = true;
            }
        }
    }

    exec_time += omp_get_wtime();
    fprintf(stderr, "%.8fs\n", exec_time);

    // OUTPUT PRINT
    for (const auto& doc : documents) {
        std::cout << doc.cabinet_id << std::endl;
    }

    return 0;
}





