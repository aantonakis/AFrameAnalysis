#include <TFile.h>
#include <TNtuple.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>


// Put the cluster window size in ns
const int bin_width = 300;

void run_diagnostics(const char* input_file) {
    // Open the input and output ROOT files
    TFile* infile = TFile::Open(input_file, "READ");

    // Access the event tree
    TTree* event_tree = (TTree*)infile->Get("reco/events");
    Long64_t n_entries = event_tree->GetEntries();
    std::cout << "This tree has " << n_entries << " events ..." << std::endl;

    // Set up branches to access event data (adjust types as needed)
    int Run = 0;
    std::vector<int>* mac5 = 0;
    std::vector<int>* flags = 0;
    std::vector<uint64_t>* timestamp = 0;
    std::vector<int> *adc[32];

    event_tree->SetBranchAddress("fRun", &Run);
    event_tree->SetBranchAddress("mac5", &mac5);
    event_tree->SetBranchAddress("flags", &flags);
    event_tree->SetBranchAddress("timestamp", &timestamp);
    // Set the ADC branches
    for (int i =0; i < 32; ++i) {
      adc[i] = 0;
      //badc[i] = 0;
      std::string branchName = "adc" + std::to_string(i);
      //tree1->SetBranchAddress(branchName.c_str(), &adc[i], &badc[i]);
      event_tree->SetBranchAddress(branchName.c_str(), &adc[i]);
    }

    // Event loop
    int count = 0;
    for (Long64_t i = 0; i < n_entries; ++i) {
        event_tree->GetEntry(i);

        if (count % 1000 == 0) {
            std::cout << "Processing event " << count << " Run " << Run << std::endl;
        }

        // Filter empty events
        if (mac5->empty()) {
            std::cout << "Skipping empty event" << std::endl;
            continue;
        }


        auto min_time = std::min_element(timestamp->begin(), timestamp->end());
        auto max_time = std::max_element(timestamp->begin(), timestamp->end());
	//std::cout << "min time before " << *min_time << std::endl;
	//std::cout << "max time before " << *max_time << std::endl;
	std::vector<uint64_t> new_times;
	for (uint64_t& element : *timestamp) {
	  new_times.push_back(element - *min_time);
	}
	
        auto new_max_time = std::max_element(new_times.begin(), new_times.end());
	//std::cout << "max time after " << *new_max_time << std::endl;
	// calculate the total possible number of time bins for this event
	uint64_t nbins = static_cast<uint64_t>(std::ceil(static_cast<double>(*new_max_time) / bin_width));
	//std::cout << "Possible number of time bins " << nbins << std::endl;
	
        if (count % 1000 == 0) {
            std::cout << "Number of time bins " << nbins << std::endl;
        }

	
	
        count++;
    }

    std::cout << "Finished event loop" << std::endl;

    infile->Close();
    
    std::cout << "Finished making diagnostics" << std::endl;
}
// end of macro

