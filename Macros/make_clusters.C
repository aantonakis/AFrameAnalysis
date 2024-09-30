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

    std::vector<int> horiz_febs;
    for (int num = 2; num < all_febs.size()-2; ++ num) {
      horiz_febs.push_back(all_febs.at(num));
    }
    std::cout << "Horizontal FEBs in this file:" << std::endl;
    for (int feb : horiz_febs) {
        std::cout << feb << " ";
    }
    std::cout << std::endl;

    // Create TNtuples
    std::string feb_string = "Run:";
    for (size_t i = 2; i < all_febs.size() - 2; ++i) {
        feb_string += std::to_string(all_febs[i]);
        if (i < all_febs.size() - 3) feb_string += ":";
    }

    std::cout << "FEB String " << feb_string << std::endl;

    const int N = horiz_febs.size() + 1; // add 1 for the Run

    TTree* cluster_tree = new TTree("cluster_tree", "cluster_tree");
    int strip_list[N];
    int time_list[N];
    int adcA_list[N];
    int adcB_list[N];
    
    cluster_tree->Branch("strips", &strip_list, Form("strips[%d]/I", N));
    cluster_tree->Branch("times", &time_list, Form("times[%d]/I", N));
    cluster_tree->Branch("adcA", &adcA_list, Form("adcA[%d]/I", N));
    cluster_tree->Branch("adcB", &adcB_list, Form("adcB[%d]/I", N));
    //cluster_tree->Branch("strips", &strip_list);
    //cluster_tree->Branch("times", &time_list);
    //cluster_tree->Branch("adcA", &adcA_list);
    //cluster_tree->Branch("adcB", &adcB_list);

    // Initialize output TNtuples
    //TNtuple* time_tree = new TNtuple("time_tree", "time_tree", feb_string.c_str());
    //TNtuple* strip_tree = new TNtuple("strip_tree", "strip_tree", feb_string.c_str());
    //TNtuple* adcA_tree = new TNtuple("adcA_tree", "adcA_tree", feb_string.c_str());
    //TNtuple* adcB_tree = new TNtuple("adcB_tree", "adcB_tree", feb_string.c_str());

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
    //TBranch *badc[32];
    
    //std::vector<int>* strips = 0;
    //std::vector<double>* times = 0;
    //std::vector<int>* adcA = 0;
    //std::vector<int>* adcB = 0;

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
	  std::cout << "Possible number of time bins " << nbins << std::endl;
	}
	
	TH1D* time_hist = new TH1D(std::to_string(count).c_str(), "", nbins, 0, nbins*bin_width);	

	// initialize cluster maps	
	std::map<uint64_t, std::vector<int>> cluster_febs;
	std::map<uint64_t, std::vector<int>> cluster_strips;
	std::map<uint64_t, std::vector<int>> cluster_adcA;
	std::map<uint64_t, std::vector<int>> cluster_adcB;
	std::map<uint64_t, std::vector<int>> cluster_times;

	// loop over the flags in this event
	for (int f = 0; f < flags->size(); ++f) {
	
	  // skip anything that is not physics
	  if (flags->at(f) != 3) {
	    continue;
	  }

          uint64_t cluster_id = time_hist->FindBin(new_times.at(f)) - 1;
	  cluster_febs[cluster_id].push_back(mac5->at(f));
	  cluster_times[cluster_id].push_back(new_times.at(f));
	  
	  int maxADC = 0;
	  int max_id = -1;

	  for (int num = 0; num < 32; ++num) {
            int adc_val = adc[num]->at(f);
	    if (adc_val > maxADC) {
	      maxADC = adc_val;
	      max_id = num;
            }
	  } // end of loop over 32 channels
	  
	  //int curr_strip = static_cast<int>(max_id/2);
	  //std::cout << "current strip " << curr_strip << std::endl;

	  cluster_strips[cluster_id].push_back(static_cast<int>(max_id/2));
	  if (max_id % 2 == 0) {
	    cluster_adcA[cluster_id].push_back(adc[max_id]->at(f));
	    cluster_adcB[cluster_id].push_back(adc[max_id+1]->at(f));
	  }
	  else {
	    cluster_adcA[cluster_id].push_back(adc[max_id-1]->at(f));
	    cluster_adcB[cluster_id].push_back(adc[max_id]->at(f));
	
	  }

	} // end of flag loop

        delete time_hist;
	// Process formed clusters
	
	for (const auto & [key, value] : cluster_febs) {

	  // A cluster should have at least two verticals and 3 horizontals to be efficiency worthy
	  if (cluster_febs[key].size() < 5)
	    continue;
	
	  int lv0 = std::count(cluster_febs[key].begin(), cluster_febs[key].end(), left_verticals.at(0));
	  int lv1 = std::count(cluster_febs[key].begin(), cluster_febs[key].end(), left_verticals.at(1));
	  int rv0 = std::count(cluster_febs[key].begin(), cluster_febs[key].end(), right_verticals.at(0));
	  int rv1 = std::count(cluster_febs[key].begin(), cluster_febs[key].end(), right_verticals.at(1));

	  int nl = lv0 + lv1;
	  int nr = rv0 + rv1;

	  //int strip_list[horiz_febs.size()];
	  //uint64_t time_list[horiz_febs.size()];
	  //int adcA_list[horiz_febs.size()];
	  //int adcB_list[horiz_febs.size()];
	
	  //strip_list.clear();
	  //time_list.clear();
	  //adcA_list.clear();
	  //adcB_list.clear();


	  strip_list[0] = Run;
	  time_list[0] = Run;
	  adcA_list[0] = Run;
	  adcB_list[0] = Run;

	  // must have exactly 1 vertical on each side
	  if ((nl == 1) && (nr == 1)) {
	    //std::cout << "Found a cluster !!!" << std::endl;
	    // loop over all the horizontal febs in this file
	    for (int f = 0; f < horiz_febs.size(); ++f) {
	      auto it = std::find(cluster_febs[key].begin(), cluster_febs[key].end(), horiz_febs[f]);
	      if (it != cluster_febs[key].end()) {
                int idx = std::distance(cluster_febs[key].begin(), it); 	  	
	        strip_list[f+1] = cluster_strips[key].at(idx);	
	        time_list[f+1] = cluster_times[key].at(idx);	
	        adcA_list[f+1] = cluster_adcA[key].at(idx);	
	        adcB_list[f+1] = cluster_adcB[key].at(idx);
	      }
	      else {
	        strip_list[f+1] = -1;
	        time_list[f+1]  = -1;	
	        adcA_list[f+1]  = -1;	
	        adcB_list[f+1]  = -1;
	      }	
	    } // end loop over horizontal modules
	    // Now we can fill our output trees

	    /*
	    for (int s = 0; s < N; ++s) {
	      std::cout << strip_list[s] << " ";
	    }
	    std::cout << std::endl;
	    */

	    cluster_tree->Fill();
	    //strip_tree->Fill(strip_list);
	    //time_tree->Fill(time_list);
	    //adcA_tree->Fill(adcA_list);
	    //adcB_tree->Fill(adcB_list);
	  }
	  else {
	    continue;
	  }

	} // end loop over possible clusters
	cluster_febs.clear();
	cluster_strips.clear();
	cluster_times.clear();
	cluster_adcA.clear();
	cluster_adcB.clear();

        count++;
    }

    std::cout << "Finished event loop" << std::endl;
    cluster_tree->SetDirectory(0);

    /*
    strip_tree->SetDirectory(0);
    time_tree->SetDirectory(0);
    adcA_tree->SetDirectory(0);
    adcB_tree->SetDirectory(0);
    */

    outfile->cd();

    // Write trees to the output file
    cluster_tree->Write();

    /*
    strip_tree->Write();
    time_tree->Write();
    adcA_tree->Write();
    adcB_tree->Write();
    */

    outfile->Close();
    infile->Close();
    
    std::cout << "Finished making clusters" << std::endl;
}
// end of macro

