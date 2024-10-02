#include <TFile.h>
#include <TNtuple.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <string>


void make_sipm_corr(const char* input_file, const char* output_file) {
    // Open the input and output ROOT files
    TFile* infile = TFile::Open(input_file, "READ");
    TFile* outfile = new TFile(output_file, "RECREATE");

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
    // Set the ADC branches
    for (int i =0; i < 32; ++i) {
      adc[i] = 0;
      //badc[i] = 0;
      std::string branchName = "adc" + std::to_string(i);
      //tree1->SetBranchAddress(branchName.c_str(), &adc[i], &badc[i]);
      event_tree->SetBranchAddress(branchName.c_str(), &adc[i]);
    }

        
    std::map<std::tuple<int, int>, std::array<int, 16>> oddHits;
    std::map<std::tuple<int, int>, std::array<int, 16>> evenHits;


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

	// loop over the flags (hits) in this event ...
	for (int f = 0; f < flags->size(); ++f) {
	
	  // skip anything that is not physics
	  if (flags->at(f) != 3) {
	    continue;
	  }
	  
	  int maxADC = 0;
	  int max_id = -1;

	  for (int num = 0; num < 32; ++num) {
            int adc_val = adc[num]->at(f);
	    if (adc_val > maxADC) {
	      maxADC = adc_val;
	      max_id = num;
            }
	  } // end of loop over 32 channels
          
	  // check if ADC == 0 --> Weird stuff ???
	  if (max_id == -1) {
            continue;
          }

	  // We have a hit at this point !!!
	  std::tuple<int, int> curr_key = std::make_tuple(Run, mac5->at(f));
	  
          int strip = static_cast<int>(max_id/2);	  
	  

	  if (max_id % 2 == 0) {

	     evenHits[curr_key][strip] += 1;
	  }
	  else {
	     oddHits[curr_key][strip] += 1;
	
	  }

	} // end of flag loop
	
        count++;
    }

    std::cout << "Finished event loop" << std::endl;
    
    outfile->cd();

    for (const auto & [key, value] : oddHits) {
       
      int r = std::get<0>(key);
      int f = std::get<1>(key);

      std::string odd_label = std::to_string(r)+"_"+std::to_string(f)+"_odd";
      std::string even_label = std::to_string(r)+"_"+std::to_string(f)+"_even";
      TH1D* new_hist_odd = new TH1D(odd_label.c_str(), "", 16, 0, 16);	
      TH1D* new_hist_even = new TH1D(even_label.c_str(), "", 16, 0, 16);	
     
      for (int num = 0; num < 16; ++num) {
        new_hist_odd->SetBinContent(num+1, oddHits[key][num]);
        new_hist_even->SetBinContent(num+1, evenHits[key][num]);
        
      }
      new_hist_even->SetDirectory(0);
      new_hist_odd->SetDirectory(0);
      new_hist_odd->Write();
      new_hist_even->Write();
    }

    outfile->Close();
    infile->Close();
    
    std::cout << "Finished making sipm corrections" << std::endl;
}
// end of macro







