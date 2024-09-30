import ROOT
import numpy as np

#from Hit import Hit
#from Cluster import Cluster
#from Event import Event
#from Track import Track


# Define a class that inherits from ROOT.TObject
class ChannelMap:
	def __init__(self, config, run_config):
		self.config = config # geometry info
		self.run_config = run_config # voltage run info 
       
		self.mac5 = []
		self.xpos = []
		self.top_measure = []
		self.bottom_measure = []
		self.FEB_dir = []
		self.channel_ascending = []
		self.cable_delays = [] # not impleented currently
		self.zpos = []
		self.angle = []
		self.m = [] # slope of the modules in the yz plane
		self.b = [] # y intercept of the module line equation in the YZ plane
		self.ymin = []
		self.ymax = []
		self.xmin = []
		self.xmax = []
		
		self.runs = []
		self.voltages = []
		self.real_voltages = []
		

		# Geometry Constants
		self.module_width_h = 180.0; # cm
		self.module_width_v = 95.9; # cm	
		self.module_length_h = 450.0; # cm
		self.module_length_v = 272.2; # cm
		self.width_offset = 25.0; # cm
		self.strip_w_h = 11.2; # cm

		self.strip_w_v = 5.95; # cm
		self.strip_l_h = 449.2; # cm TODO Note: this is for the Y-modules
		self.strip_l_v = 271.5; # cm
		self.strip_t = 1.0; # cm --> This thickness is the same for X, Y, and Bern Modules
		self.alumin_t = 0.4; # TODO assume symmetric for now

	def calc_bias_volt(self, s):
		return 68.1 - ((255.0-s)/255.0)*4.0
	
	def initialize_config(self):
		# Open the text file in read mode
		with open(self.config, 'r') as file:
			# Loop through each line in the file
			count = 0
			for line in file:
				# Remove newline character at the end of the line
				line = line.strip()
				 # Split the line into elements based on commas
				line = line.split(',')
				# Print the elements as a list
				print(line)
				if count == 0:
					for e in line:
						if e != "":
							self.mac5.append(int(e))
				if count == 1:
					for e in line:
						if e != "":
							self.xpos.append(float(e))
				if count == 2:
					for e in line:
						if e != "":
							self.top_measure.append(float(e))
				if count == 3:
					for e in line:
						if e != "":
							self.bottom_measure.append(float(e))
	
				if count == 4:
					for e in line:
						if e != "":
							self.FEB_dir.append(int(e))
				if count == 5:
					for e in line:
						if e != "":
							self.channel_ascending.append(int(e))


				count += 1



	# use the fact that vertical FEBs always have FEB_dir 2 since the FEB side is at the bottom of the frame

	def is_horiz(self, _mac5):
		index = self.mac5.index(_mac5)
		if self.FEB_dir[index] == 2:
			return 0
		else:
			return 1

	def is_outer(self, _mac5):
		index = self.mac5.index(_mac5)
		if (index == 2 or index == len(self.mac5) - 3):
			return 1
		else:
			return 0


	# Determine if this is one of the outer horizontal Modules (1 in from outermost)

	def is_outer_second(self, _mac5):
		index = self.mac5.index(_mac5)
		if (index == 3 or index == len(self.mac5) - 4):
			return 1
		else:
			return 0

	def is_left(self, _mac5):
		index = self.mac5.index(_mac5)
		if self.zpos[index] < 0:
			return 1
		else:
			return 0


	# Functions to set the values of each of the vectors in this block after Calculate Params has been called --> During initialization
	def set_m(self):
		#Loop over the modules
		for num in range(len(self.mac5)):
			if self.zpos[num] < 0:
				self.m.append(ROOT.TMath.Tan(self.angle[num]))
			else:
				self.m.append(-1*ROOT.TMath.Tan(self.angle[num]))

	def set_b(self):
		for num in range(len(self.mac5)):
			self.b.append(abs(self.zpos[num])*ROOT.TMath.Tan(self.angle[num]))

	def set_ymin(self):
		for num in range(len(self.mac5)):
			self.ymin.append(0)


	def set_ymax(self):
		for num in range(len(self.mac5)):
			if (self.is_horiz(self.mac5[num])):
				ymax_temp = self.module_width_h*ROOT.TMath.Sin(self.angle[num])
				self.ymax.append(ymax_temp)
			else:
				self.ymax.append(self.module_length_v*ROOT.TMath.Sin(self.angle[num]))
	def set_xrange(self):
		for num in range(len(self.mac5)):
			if self.is_horiz(self.mac5[num]):
				self.xmin.append(-1*self.module_length_h/2.0)
				self.xmax.append(self.module_length_h/2.0)

			else:
				if (num == 0 or num == len(self.mac5)-2):
					x1 = -1*(self.module_length_h/2.0 - self.xpos[num])
					self.xmin.append(x1)
					self.xmax.append(x1 + self.module_width_v)
				else:
					x2 = self.module_length_h/2.0 - self.xpos[num]
					self.xmax.append(x2)
					self.xmin.append(x2 - self.module_width_v)


	def calculate_params(self):
		middle_top = self.top_measure[0]
		middle_bottom = self.bottom_measure[0]
		z0_right = middle_bottom/2.0
		z0_left = -1*(middle_bottom)/2.0
		z1_right = middle_top/2.0
		z1_left = -1*(middle_top)/2.0
		
		flag = 0
		for num in range(1, len(self.top_measure)):
			if self.top_measure[num] < 0:
				temp0 = self.bottom_measure[num] + z0_left			
				temp1 = self.top_measure[num] + z1_left			
				self.zpos.append(temp0)
				temp = abs(temp0 - temp1)/self.module_width_h
				alpha = ROOT.TMath.ACos(temp)
				self.angle.append(alpha)

			elif self.top_measure[num] > 0 and flag == 0:

				self.zpos.append(z0_left)
				self.zpos.append(z0_right)
				temp1 = (abs(z0_left - z1_left)/ self.module_width_h)
				temp2 = (abs(z0_right - z1_right)/ self.module_width_h)
				angle1 = ROOT.TMath.ACos(temp1)
				angle2 = ROOT.TMath.ACos(temp2)
				self.angle.append(angle1)
				self.angle.append(angle2)
				flag = 1
				
				temp3 = self.bottom_measure[num] + z0_right
				temp4 = self.top_measure[num] + z1_right	

				self.zpos.append(temp3)
			
				temp = (abs(temp3 - temp4)/ self.module_width_h)
				alpha = ROOT.TMath.ACos(temp)	
			
				self.angle.append(alpha)

			else:
				temp0 = self.bottom_measure[num] + z0_right		
				temp1 = self.top_measure[num] + z1_right		
				self.zpos.append(temp0)
			
				temp = (abs(temp0 - temp1)/ self.module_width_h)
				alpha = ROOT.TMath.ACos(temp)	
			
				self.angle.append(alpha)
		self.set_m();
		self.set_b();
		self.set_ymin();
		self.set_ymax();
		self.set_xrange();

	# End of calculate_params function



	def initialize_run_config(self):
		# Open the text file in read mode
		with open(self.run_config, 'r') as file:
			# Loop through each line in the file
			count = 0
			for line in file:
				# Remove newline character at the end of the line
				line = line.strip()
				 # Split the line into elements based on commas
				line = line.split(',')
				# Print the elements as a list
				print(line)
				if count == 0:
					for e in line:
						if e != "":
							self.runs.append(int(e))
				
				if count == 1:
					for e in line:
						if e != "":
							self.voltages.append(int(e))

				count += 1
	
			
		for v in self.voltages:
			self.real_voltages.append(self.calc_bias_volt(v))


	# End of initialize_run_config


# below involves objects that are not needed for this analysis


"""
	
	# Only Checking the yz trajectory for now and ignoring the x direction
	def was_hit(self, track, feb):
		index = self.mac5.index(feb)	
		zint = (self.b[index] - track.yz_b) /(track.yz_m - self.m[index])  
		yint = track.yz_m*zint + track.yz_b
		if 0 <= yint <= self.ymax[index]:
			return 1
		else:
			return 0
	
	def get_hit(self, track, feb):
		index = self.mac5.index(feb)	
		zint = (self.b[index] - track.yz_b) /(track.yz_m - self.m[index])  
		yint = track.yz_m*zint + track.yz_b
		return [zint, yint]

"""
