########################################
# Create by NguyenTT
# This tool help to
# 1. Generate platform file
# Input: generate_txt.py [n] [deploy-type] [host-per-node] [architecture] [topo] 
#	[deploy-type]
#		default ~> 1 to 1
#		lr 		~> mapping for cluster-node
#	[host-per-node]
#		default = 4
# Ouput: .txt file
#####################################

import re
import string
import sys
import math
import pprint
import random
import Queue


##--------MAIN FUNCTION-----------
def main():
	# 1. Checking syntax
	if len(sys.argv) == 1:
		print 'Syntax error. Neeed to add number of switches:'
		return
		
	# 2. Get the argument
	totalNode = int(sys.argv[1])
	topoFileName = None
	if len(sys.argv) >= 6:
		layoutFileName = str(sys.argv[5])		
	
	deployType = "default"
	if len(sys.argv) >= 3:
		deployType = str(sys.argv[2])
	
	hostPerNode = 8
	if len(sys.argv) >= 4:
		hostPerNode = int(sys.argv[3])

	architecture = "Tsubame3"
	if len(sys.argv) >= 5:
		architecture = str(sys.argv[4])
			
	#6. Generate txt file 
	HOST_PER_NODE = hostPerNode
	ARCHITECTURE = architecture
	
	pathFileName = ARCHITECTURE + "_" + str(totalNode) + '.' + deployType + '.txt'
	if (ARCHITECTURE == "NVCluster"):
		pathFileName = ARCHITECTURE + "_" + str(hostPerNode) + "_" + str(totalNode) + '.' + deployType + '.txt'
	print 'Write paths into ' + pathFileName
	fo = open(pathFileName, "w")
	for nodeIdx in range(0,totalNode):
		if (deployType == "default"):
			for hostIdx in range(0,HOST_PER_NODE):
				idx = nodeIdx*HOST_PER_NODE + hostIdx
				line = "n" + str(idx) + ":1\r\n"
				fo.writelines(line)
		if (deployType == "lr"):
			mapping_order = [0,1,2,7,4,5,6,3]
			if ( hostPerNode == 8):
				mapping_order = [0,1,2,3]
			if ( hostPerNode == 8):
				mapping_order = [0,1]
				
			for hostIdx in mapping_order:
				idx = nodeIdx*HOST_PER_NODE + hostIdx
				line = "n" + str(idx) + ":1\r\n"
				fo.writelines(line)
	fo.close()	
	
main()