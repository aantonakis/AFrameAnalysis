#include <TFile.h>
#include <TNtuple.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>




void make_clusters(const char* input_file, const char* output_file, const char* frame) {
    // Open the input and output ROOT files
    TFile* infile = TFile::Open(input_file, "READ");
    TFile* outfile = new TFile(output_file, "RECREATE");

    // Prepare configuration file
    std::string config_file = std::string("../Configs/config_frame") + frame + ".txt";

    // Read FEB configuration
    std::vector<int> all_febs;
    std::ifstream file(config_file);
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string feb;
            while (std::getline(ss, feb, ',')) {
                all_febs.push_back(std::stoi(feb));
            }
        }
    } else {
        std::cerr << "Error opening config file: " << config_file << std::endl;
        return;
    }

    // Print FEBs
    std::cout << "FEBs in this file:" << std::endl;
    for (int feb : all_febs) {
        std::cout << feb << " ";
    }
    std::cout << std::endl;

    std::vector<int> left_verticals = { all_febs[0], all_febs[1] };
    std::vector<int> right_verticals = { all_febs[all_febs.size() - 2], all_febs[all_febs.size() - 1] };

    std::cout << "Left verticals: " << left_verticals[0] << ", " << left_verticals[1] << std::endl;
    std::cout << "Right verticals: " << right_verticals[0] << ", " << right_verticals[1] << std::endl;

    // Create TNtuples
    std::string feb_string = "Run:";
    for (size_t i = 2; i < all_febs.size() - 2; ++i) {
        feb_string += std::to_string(all_febs[i]);
        if (i < all_febs.size() - 3) feb_string += ":";
    }

    std::cout << "FEB String " << feb_string << std::endl;

    // Initialize output TNtuples
    TNtuple* time_tree = new TNtuple("time_tree", "time_tree", feb_string.c_str());
    TNtuple* strip_tree = new TNtuple("strip_tree", "strip_tree", feb_string.c_str());
    TNtuple* adcA_tree = new TNtuple("adcA_tree", "adcA_tree", feb_string.c_str());
    TNtuple* adcB_tree = new TNtuple("adcB_tree", "adcB_tree", feb_string.c_str());

    // Access the event tree
    TTree* event_tree = (TTree*)infile->Get("reco/events");
    Long64_t n_entries = event_tree->GetEntries();
    std::cout << "This tree has " << n_entries << " events ..." << std::endl;

    // Set up branches to access event data (adjust types as needed)
    std::vector<int>* mac5 = 0;
    std::vector<int>* flags = 0;
    std::vector<int> *adc[32];
    //TBranch *badc[32];
    
    //std::vector<int>* strips = 0;
    //std::vector<double>* times = 0;
    //std::vector<int>* adcA = 0;
    //std::vector<int>* adcB = 0;

    event_tree->SetBranchAddress("mac5", &mac5);
    event_tree->SetBranchAddress("flags", &flags);
    // Set the ADC branches
    for (int i =0; i < 32; ++i) {
      adc[i] = 0;
      badc[i] = 0;
      std::string branchName = "adc" + std::to_string(i);
      //tree1->SetBranchAddress(branchName.c_str(), &adc[i], &badc[i]);
      tree1->SetBranchAddress(branchName.c_str(), &adc[i]);
    }
    //event_tree->SetBranchAddress("strips", &strips);
    //event_tree->SetBranchAddress("times", &times);
    //event_tree->SetBranchAddress("adcA", &adcA);
    //event_tree->SetBranchAddress("adcB", &adcB);

    // Event loop
    int count = 0;
    for (Long64_t i = 0; i < n_entries; ++i) {
        event_tree->GetEntry(i);

        if (count % 1000 == 0) {
            std::cout << "Processing event " << count << std::endl;
        }

        // Filter empty events
        if (mac5->empty()) {
            std::cout << "Skipping empty event" << std::endl;
            continue;
        }

        // Example processing, similar to Python code
        // Do the filtering and clustering logic here
        // ...

        count++;
    }

    std::cout << "Finished event loop" << std::endl;

    strip_tree->SetDirectory(0);
    time_tree->SetDirectory(0);
    adcA_tree->SetDirectory(0);
    adcB_tree->SetDirectory(0);

    outfile->cd();

    // Write trees to the output file
    strip_tree->Write();
    time_tree->Write();
    adcA_tree->Write();
    adcB_tree->Write();

    outfile->Close();
    infile->Close();
    
    std::cout << "Finished making clusters" << std::endl;
}
// end of macro

