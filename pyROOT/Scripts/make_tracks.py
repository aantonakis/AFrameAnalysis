import ROOT
import numpy as np
import sys
import os
import math


sys.path.append(os.path.abspath('../src/'))
sys.path.append(os.path.abspath('../../Configs/'))

#from ChannelMap import *


bin_width = 300 # ns

infile = ROOT.TFile.Open(sys.argv[1], "READ")
outfile = ROOT.TFile(sys.argv[2], "RECREATE")

frame = sys.argv[3]
config_file = "../../Configs/config_frame"+frame+".txt"


all_febs = 0
with open(config_file, 'r') as file:
    # Read the first line
    all_febs = file.readline().strip().split(",")

all_febs = [int(all_febs[num]) for num in range(len(all_febs)-1)]


print("FEBs in this file")
print(all_febs)

left_verticals = all_febs[:2]
right_verticals = all_febs[-2:]

print("left vert", left_verticals)
print("right vert", right_verticals)

feb_string = "Run:"
for feb in all_febs[2:-2]:
	feb_string += str(feb)
	feb_string += ":"

feb_string = feb_string[:-1]
print("feb string", feb_string)

#z_tree = ROOT.TNtuple("z_tree", "z_tree", feb_string)
#y_tree = ROOT.TNtuple("y_tree", "y_tree", feb_string)
time_tree = ROOT.TNtuple("time_tree", "time_tree", feb_string)
strip_tree = ROOT.TNtuple("strip_tree", "strip_tree", feb_string)
adcA_tree = ROOT.TNtuple("adcA_tree", "adcA_tree", feb_string)
adcB_tree = ROOT.TNtuple("adcB_tree", "adcB_tree", feb_string)


#infile.cd("reco")
#print("file contents:")
#infile.ls()
#print("\n")

event_tree = infile.Get("reco/events")
n_entries = event_tree.GetEntries()

print("This tree has", n_entries, "events ...")

#print("Event Tree Contents:")
#event_tree.Print()
print("")
print("starting event loop", "\n")

count = 0
for event in event_tree:

	if count % 10 == 0:
		print("Processing event", count)
		print("")

	evID = getattr(event, "fEvent")
	run = getattr(event, "fRun")

	mac5 = list(getattr(event, "mac5"))
	if len(mac5) == 0:
		print("Skipping empty event")
		continue	

	flags = list(getattr(event, "flags"))

	# filter non physics
	indices_to_keep = [i for i, val in enumerate(flags) if val == 3]	

	timestamp = list(getattr(event, "timestamp"))
	min_time = min(timestamp)

	timestamp = np.array(timestamp) - min_time
	max_time = max(timestamp)

	nbins = math.ceil(max_time / float(bin_width))
	#print("Number of possible clusters in this event", nbins)
	
	time_hist = ROOT.TH1D("time_hist"+str(count), "", nbins, 0, nbins*bin_width)

	adc = [getattr(event, "adc"+str(num)) for num in range(32)]

	cluster_febs =   {i: [] for i in range(nbins)}
	cluster_strips = {i: [] for i in range(nbins)}
	cluster_adcA =   {i: [] for i in range(nbins)}
	cluster_adcB =   {i: [] for i in range(nbins)}
	cluster_runs =   {i: run for i in range(nbins)}
	cluster_times =  {i: [] for i in range(nbins)}
	

	#time_bins = np.arange(min_time, max_time + bin_width, bin_width)

	for num in range(len(flags)):
		if flags[num] != 3:
			continue
		
		cluster_id = time_hist.FindBin(timestamp[num]) - 1
		cluster_febs[cluster_id].append(mac5[num])
		cluster_times[cluster_id].append(timestamp[num])
		
		maxADC = 0
		max_id = -1

		for i in range(32):
			adc_val = adc[i][num]
			if adc_val > maxADC:
				maxADC = adc_val
				max_id = i

		cluster_strips[cluster_id].append(int(max_id/2))


		if max_id % 2 == 0:
			cluster_adcA[cluster_id].append(adc[max_id][num])			
			cluster_adcB[cluster_id].append(adc[max_id+1][num])			
		else:
			cluster_adcA[cluster_id].append(adc[max_id-1][num])			
			cluster_adcB[cluster_id].append(adc[max_id][num])			

	# We have formed all potential clusters


	if count % 10 == 0:
		print("We are looping over the clusters formed ...")

	cluster_count = 0
	# Now, Let's loop over the clusters that we've formed
	for num in range(nbins):
		febs = cluster_febs[num]
		# skip anything that doesn't have at least 2 verticals + 3 hits for efficiency
		
		if len(febs) < 5:
			continue
		
		lv0 = febs.count(left_verticals[0])
		lv1 = febs.count(left_verticals[1])
		rv0 = febs.count(right_verticals[0])
		rv1 = febs.count(right_verticals[1])
		
		# veto any double vertical activity on either side of the frame
		if lv0 > 0 and lv1 > 0:
			continue
		if rv0 > 0 and rv1 > 0:
			continue

		nl = lv0 + lv1
		nr = rv0 + rv1
		
		# must have one vertical on each side
		if nl != 1 or nr != 1:
			continue

		run = cluster_runs[num]
		#print("Potential Cluster, nl", nl, "nr", nr)
		strips = cluster_strips[num]
		times = cluster_times[num]
		adcA = cluster_adcA[num]
		adcB = cluster_adcB[num]

		strip_list = [run]
		time_list =  [run]
		adcA_list =  [run]
		adcB_list =  [run]

		for f in all_febs[2:-2]:
			if f in febs:
				i = febs.index(f)
				strip_list.append(strips[i])
				time_list.append(times[i])
				adcA_list.append(adcA[i])
				adcB_list.append(adcB[i])

			else:
				strip_list.append(-1)
				time_list.append(-1)
				adcA_list.append(-1)
				adcB_list.append(-1)


		strip_tree.Fill(*strip_list)
		time_tree.Fill(*time_list)
		adcA_tree.Fill(*adcA_list)
		adcB_tree.Fill(*adcB_list)
		cluster_count += 1	
			
	# Debugging
	if count % 10 == 0:
		print("processed event", count)
		print("Number of clusters:", cluster_count)
		print("")
		#print("Min Stamp after Subtraction", min(timestamp))
		
	count += 1


print("finished event loop")


# Write results to a file
strip_tree.SetDirectory(0)
time_tree.SetDirectory(0)
adcA_tree.SetDirectory(0)
adcB_tree.SetDirectory(0)


outfile.cd()
strip_tree.Write()
time_tree.Write()
adcA_tree.Write()
adcB_tree.Write()
outfile.Close()

print("Finished making clusters")






