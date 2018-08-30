########################################
# Create by NguyenTT on 30.Aug.2018
# This tool help to generate the parameter (values from -1 to 1) and save to file
# generate_gauss_params.py [seed] [number_of_values] [output_file_pre] [output_idx] 
# File format: 
# + 1st line: "iter0,number_of_values"
# + From 2nd line: value1,value2...
# + Each line has less than 29999 values.
#####################################
import sys
import re
import string
import math
import pprint
import random
import time

##--------MAIN FUNCTION-----------
def main():
	# 1. Checking syntax	
	if len(sys.argv) == 1:
		print 'Syntax error. Lack of argument'
		return
	# 2. Get the argument
	seed = str(sys.argv[1])
	number_of_values = 0
	if len(sys.argv) >= 3:
		number_of_values = int(sys.argv[2])
	out_file_prefix = "param_" + str(number_of_values) + ".txt"
	if len(sys.argv) >= 4:
		out_file_prefix = str(sys.argv[3])
	file_maxidx = 0
	if len(sys.argv) >= 5:
		file_maxidx = int(sys.argv[4])
	print file_maxidx
	
	
	# 3. Generate random values and write to files
	MAX_BUFF_SIZE = 29900
	for fileIdx in range(0,file_maxidx):
		out_file_name = out_file_prefix + str(fileIdx) + ".txt"
		print "Write " + str(number_of_values) +  " values into " + out_file_name
		fo = open(out_file_name, "w")
		# 3.1. 1st line
		line = "iter0," + str(number_of_values) + "\n"
		fo.writelines(line)
		random.seed(seed)
		line = ""
		for i in range (0, number_of_values):
			if line <> "":
				line  += ","
			value = random.gauss(0,0.1)
			line = line + str(value)
			if (len(line) > 29900):
				line += "\n"
				fo.writelines(line)
				line = ""
		fo.close()
	
main()