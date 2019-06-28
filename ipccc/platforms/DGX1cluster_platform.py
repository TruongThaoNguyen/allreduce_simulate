########################################
# Create by NguyenTT
# This tool help to
# 1. Generate platform file
# Input: generate_platform.py [n] [topo] 
# Ouput: .xml file, .txt file
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
	if len(sys.argv) >= 3:
		layoutFileName = str(sys.argv[2])		
		
	# 3. Parameter for NV-Cluster with GDX-1 as 1 node. (Houston Deep Learning production and Scale)
	HOST_SPEED = 7.8E12 # double precision flop for NVIDIA Tesla V100 for NVLINK (https://www.nvidia.com/en-us/data-center/tesla-v100/)
	HOST_PER_NODE = 8
	# ENERGY_WATT_OFF = 10		#Watts
	# ENERGY_WATT_SLEEP = 93		#Watts
	# ENERGY_WATT_PEAK = 170		#Watts
	
	INTRA_LINK_BW = 50E9 # Bype per second #NVLINK
	# End to end latency of NVLINK 1 9us per hop and 10us per 2 hop, 3.5us for on node communication
	# https://arxiv.org/abs/1903.04611
	# https://www.microway.com/hpc-tech-tips/nvidia-tesla-p100-nvlink-16gb-gpu-accelerator-pascal-gp100-sxm2-close/
	INTRA_SWITCH_LAT = 0 # second. It should be > 0 but very small
	#INTRA_CALBE_LAT = 0# 9E-6# second ~ use 1 hop end-2-end latency...
	INTRA_CALBE_LAT = 9E-6# second ~ use 1 hop end-2-end latency...
	
	PLX_LAT = 110E-9 #second. PLX Switch latency
	# https://arxiv.org/abs/1903.04611
	PLX_CABLE_LAT = 15E-6/2 # 15us GPU-to-GPU ~~> GPU-to-PLX become 0.5x
	#PLX_CABLE_LAT = 110E-9 #15E-6/2 # 15us GPU-to-GPU ~~> GPU-to-PLX become 0.5x
	PLX_BW = 16E9 #Byte per second
	
	#Energy parameter for NVLINK energy of links + switches = total energy by switch in the catalogs
	# Peak energy of 1 links = energy of switch / # number of ports.
	# Ideal energy = peak energy * 80% (due to the survey of master thesis...)
	IDLE_PERCENT = 0.85
	GPU_ENERGY = 300 #W  #https://www.nvidia.com/en-us/data-center/tesla-v100/
	GPU_ENERGY_LINK_NVLINK_PEAK = 0.7*16  #11.2W 0.7W per lanes; https://en.wikipedia.org/wiki/NVLink: 96lanes/6link
	GPU_ENERGY_LINK_NVLINK_IDLE = GPU_ENERGY_LINK_NVLINK_PEAK * IDLE_PERCENT  #9.52W
	
	PCIe_ENERGY = 4 #W (4 links)
	ENERGY_LINK_PCIe_PEAK = 4/4.0 #1W/link
	ENERGY_LINK_PCIe_IDLE = ENERGY_LINK_PCIe_PEAK * IDLE_PERCENT
	
	# Internode parameter:
	# 2 level
	
	INTER_LINK_BW = 12.5E9 # Bype per second  # InfiniBand EDR (12.5 GB/s)
	INTER_SWITCH_LAT_LEAF = 90E-9 #second # 90ns for InfiniBand Switch using cut through switching (http://www.mellanox.com/related-docs/prod_ib_switch_systems/pb_sb7800.pdf)
	INTER_SWITCH_LAT_SPINE = 400E-9 #second # 400ns 
	# (648 ports:http://www.mellanox.com/related-docs/prod_ib_switch_systems/pb_cs7500.pdf)
	#  324 ports: http://www.mellanox.com/related-docs/prod_ib_switch_systems/PB_CS7510.pdf
	# 216 ports:http://www.mellanox.com/related-docs/prod_ib_switch_systems/pb_cs7520.pdf
	# only 90ns if 36 ports.
	CABLE_LENGTH = 5.0 # meter
	CALBE_LAT = CABLE_LENGTH * 5.2E-9 #Speed of light in glass= 190000km/sec ==> 1/19.000.000.000 = 5.2E-9 s/m
	
	IB_ENERGY_LEAF_PEAK = 122 #W(up 36 port / optical - SB7890 model - Max Power)
	IB_ENERGY_LEAF_IDLE = 122 #W(up 36 port / optical - Typical Power)
	ENERGY_LINK_IB_PEAK = IB_ENERGY_LEAF_PEAK / 36.0 #3.389 W/link
	ENERGY_LINK_IB_IDLE = IB_ENERGY_LEAF_IDLE / 36.0 #3.389 W/link
	
	IB_SPINE_ENERGY = 13531 #(up to 648 ports in 28U)  #only use 384 port...
	IB_SPINE_ENERGY_IDLE = 6367.1 #(up to 648 ports in 28U)
	ENERGY_LINK_IB_SPINE_PEAK = IB_SPINE_ENERGY/648 #20.881 W/port 
	ENERGY_LINK_IB_SPINE_IDLE = IB_SPINE_ENERGY_IDLE/648 #9.8 W/port 
	
	#4. Generte xml file 
	ARCHITECTURE = "DGX1Cluster"
	pathFileName = ARCHITECTURE + "_" + str(totalNode) + '.xml'
	print 'Write paths into ' + pathFileName
	fo = open(pathFileName, "w")

	#4.1. File header
	header = '''<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM 'http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd'>
<platform version='4.1'>
<config>
	<prop id='maxmin/precision' value='1e-4'/> 
	<prop id='network/model' value='SMPI'/>
	<!--  Negative values enable auto-select... -->
	<prop id='contexts/nthreads' value='1'/>
	<!--  Power of the executing computer in Flop per seconds. Used for extrapolating tasks execution time by SMPI [default is 20000]-->
	<prop id='smpi/host-speed' value='50000000000.0'/>
	<!--  Display simulated timing at the end of simulation -->
	<prop id='smpi/display-timing' value='1'/>
	<prop id='cpu/optim' value='Lazy'/>
	<prop id='network/optim' value='Lazy'/>
	<!--<prop id='smpi/coll-selector' value='mvapich2'/>-->
	<prop id='smpi/cpu-threshold' value='0.00000000001'/>
</config>
<zone id='AS0' routing='Floyd'>
'''
	fo.writelines(header)
	fo.writelines("<!--  Generate node -->\r\n") 
	for nodeIdx in range(0,totalNode):
		# 4.2 Node generate
		#fo.writelines("\r\n") 
		#fo.writelines("<!--  Generate node "+ str(nodeIdx) + " -->\r\n") 
		for hostIdx in range(0,HOST_PER_NODE):
			#4.2.1. Host generate (GPU/CPU)
			line = '''	<host id="n''' + str(nodeIdx*HOST_PER_NODE + hostIdx) +'''"'''
			line += '''speed="''' + str(HOST_SPEED) + '''f"'''
			line += "core=\"1\" />\r\n"
			# line += "core=\"1\" >\r\n"
			# line += '''		<prop id="watt_per_state" value="''' + str(ENERGY_WATT_SLEEP) + ":" + str(ENERGY_WATT_PEAK) + '''" />'''
			# line += '''
		# <prop id="watt_off" value="''' + str(ENERGY_WATT_OFF) + '''" />
	# </host>\r\n'''
	
			fo.writelines(line)
		
		# 4.2.2. PLX Router generate
		#fo.writelines("\r\n")
		for hostIdx in range(0,HOST_PER_NODE):
			if hostIdx % 2 == 0:			
				line = '''	<router id="plx'''  + str(nodeIdx*HOST_PER_NODE + hostIdx) + '''"/>\r\n'''
				fo.writelines(line)
		
		
	# 4.2.3. IB Switch generate
	#TODO: Should load inter-node network from file ??
	# In this example, generate the topology like NV_Cluster.
	#	+ 1:1 oversubscription, LEAF-SPINE (12 links each LEAF (6nodes), 72 links each pods)
	# 	+ 2:1 oversubscription, 36-ports (24:12)
	
	L1_SWITCH_UP = 12
	L1_SWITCH_DOWN = 24
	print L1_SWITCH_DOWN, HOST_PER_NODE, (HOST_PER_NODE/2)
	nodePerL1Switch = L1_SWITCH_DOWN/(HOST_PER_NODE/2) #4 Plx switch per nodes; 6 nodes per L1 switch	
	print totalNode,nodePerL1Switch
	totalL1Switch = int(math.ceil(1.0*totalNode/nodePerL1Switch))
	#print totalL1Switch,totalNode
	SPINE_BW = 130E12/8 #130 Tbps
	LEAF_BW = 7.2E12/8 #7.2 Tbps
	#L2 switch
	#fo.writelines("\r\n")
	line = '''	<router id="sroot"/>\r\n'''
	fo.writelines(line)
	#L2 switch latency
	line = '''	<link id="ls_root"''' 
	line += '''bandwidth="''' + str(SPINE_BW) + '''Bps" '''
	line += '''latency="''' +  str(INTER_SWITCH_LAT_SPINE) +'''s"/>\r\n''' 
	fo.writelines(line)
	
	#L1-switch and latency
	for switchIdx in range(0,totalL1Switch):
		line = '''	<router id="s''' + str(switchIdx) + '''"/>\r\n'''
		fo.writelines(line)
		#switch latency
		line = '''	<link id="ls''' + str(switchIdx) + '''" ''' 
		line += '''bandwidth="''' + str(LEAF_BW) + '''Bps" '''
		line += '''latency="''' +  str(INTER_SWITCH_LAT_LEAF) +'''s"/>\r\n''' 
		fo.writelines(line)	
		
	#4.3. Link generate 
	#4.3.1. Intra-link generate (NVLINK)
	# This is only for 8 nodes.
	fo.writelines("<!--  Generate intra-links -->\r\n")
	linkList = [(0,1,1),(0,2,1),(0,3,2),(1,2,2),(1,3,1),(2,3,2),(0,4,2),(1,5,2),(2,6,1),(3,7,1),(4,5,1),(4,6,1),(4,7,2),(5,6,2),(5,7,1),(6,7,2)]
		
	for nodeIdx in range(0,totalNode):
		for linkIdx in range (0,len(linkList)):
			src = str(get_host_Id(nodeIdx,linkList[linkIdx][0],HOST_PER_NODE))
			dst = str(get_host_Id(nodeIdx,linkList[linkIdx][1],HOST_PER_NODE))
			line = '''	<link id="ln''' + src + '''_n''' + dst + '''" ''' 
			line += '''bandwidth="''' + str(INTRA_LINK_BW * linkList[linkIdx][2]) + '''Bps" '''
			line += '''latency="''' +  str(INTRA_CALBE_LAT) +'''s">'''
			line += '''<prop id="watt_range" value="''' + str(GPU_ENERGY_LINK_NVLINK_IDLE*2 * linkList[linkIdx][2])
			line += ''':''' + str(GPU_ENERGY_LINK_NVLINK_PEAK*2 * linkList[linkIdx][2]) +'''" />'''
			line += '''</link>\r\n'''
			fo.writelines(line)
	
		# 4.3.2. Node-PLX Router links
		# fo.writelines("\r\n")
		for hostIdx in range(0,HOST_PER_NODE):
			line = ""
			src = str(get_host_Id(nodeIdx,hostIdx,HOST_PER_NODE))
			dst = src	
			if hostIdx % 2 == 1:
				dst = str(get_host_Id(nodeIdx,hostIdx-1,HOST_PER_NODE))
			line += '''	<link id="ln''' + src + '''_plx''' + dst + '''" ''' 
			line += '''bandwidth="''' + str(PLX_BW) + '''Bps" '''
			#It should be no latency for this link but here state the PLX switch latency
			line += '''latency="''' +  str(PLX_CABLE_LAT) +'''s">'''
			line += '''<prop id="watt_range" value="''' + str(GPU_ENERGY_LINK_NVLINK_IDLE + ENERGY_LINK_PCIe_IDLE)
			line += ''':''' + str(GPU_ENERGY_LINK_NVLINK_PEAK + ENERGY_LINK_PCIe_PEAK) +'''" />'''
			line += '''</link>\r\n'''
			fo.writelines(line)

	fo.writelines("<!--  Generate inter-links -->\r\n")
	#4.3.3. LEAF to SPINE
	for switchIdx in range(0,totalL1Switch):
		#up link
		line = '''	<link id="ls''' + str(switchIdx) + '''_root" ''' 
		line += '''bandwidth="''' + str(INTER_LINK_BW*L1_SWITCH_UP) + '''Bps" '''
		line += '''latency="''' +  str(CALBE_LAT) +'''s">'''
		line += '''<prop id="watt_range" value="''' + str((ENERGY_LINK_IB_IDLE + ENERGY_LINK_IB_SPINE_IDLE)*L1_SWITCH_UP)
		line += ''':''' + str((ENERGY_LINK_IB_PEAK + ENERGY_LINK_IB_SPINE_PEAK)*L1_SWITCH_UP) +'''" />'''
		line += '''</link>\r\n'''
		fo.writelines(line)

		for plxId in range(0,nodePerL1Switch*HOST_PER_NODE,2):
			src = switchIdx*nodePerL1Switch*HOST_PER_NODE + plxId
			if src < totalNode * HOST_PER_NODE:
				src = str(src)
				#down link
				line = '''	<link id="lplx''' + src + '''_s''' + str(switchIdx) +'''" ''' 
				line += '''bandwidth="''' + str(INTER_LINK_BW) + '''Bps" '''
				line += '''latency="''' +  str(CALBE_LAT) +'''s">'''
				line += '''<prop id="watt_range" value="''' + str(ENERGY_LINK_IB_IDLE + ENERGY_LINK_PCIe_IDLE)
				line += ''':''' + str(ENERGY_LINK_IB_PEAK + ENERGY_LINK_PCIe_PEAK) +'''" />'''
				line += '''</link>\r\n'''
				fo.writelines(line)

	# 4.4 Generate route
	#4.4.1. Intra-route (via NVLINK)
	fo.writelines("<!--  Generate intra-route -->\r\n") 
	for nodeIdx in range(0,totalNode):		
		for linkIdx in range (0,len(linkList)):
			src = str(get_host_Id(nodeIdx,linkList[linkIdx][0],HOST_PER_NODE))
			dst = str(get_host_Id(nodeIdx,linkList[linkIdx][1],HOST_PER_NODE))
			line = '''	<route src="n''' + src + '''" dst="n''' + dst + '''">
		<link_ctn id="ln'''+ src + '''_n''' + dst +'''" />
	</route>\r\n'''
			fo.writelines(line) 
		
		#4.4.2. Intra-route via PCIex3
		for hostIdx in range(0,HOST_PER_NODE):
			src = str(get_host_Id(nodeIdx,hostIdx,HOST_PER_NODE))
			dst = src	
			if hostIdx % 2 == 1:
				dst = str(get_host_Id(nodeIdx,hostIdx-1,HOST_PER_NODE))
			line = '''	<route src="n''' + src + '''" dst="plx''' + dst + '''">
		<link_ctn id="ln'''+ src + '''_plx''' + dst +'''" />
	</route>\r\n'''
			fo.writelines(line) 
			
	fo.writelines("<!--  Generate inter-route -->\r\n")
	for switchIdx in range(0,totalL1Switch):	
		#4.4.3. Route to L2
		line = '''	<route src="s''' + str(switchIdx) + '''" dst="sroot">
		<link_ctn id="ls''' + str(switchIdx) + '''_root"/>
		<link_ctn id="ls_root"/>
	</route>\r\n'''
		fo.writelines(line) 
	
		for plxId in range(0,nodePerL1Switch*HOST_PER_NODE,2):
			src = switchIdx*nodePerL1Switch*HOST_PER_NODE + plxId
			if src < totalNode * HOST_PER_NODE:
				src = str(src)
				#4.4.4. Route to L1
				line = '''	<route src="plx''' + src + '''" dst="s''' + str(switchIdx) + '''">
		<link_ctn id="lplx''' + src + '''_s''' + str(switchIdx) +'''"/>
		<link_ctn id="ls''' + str(switchIdx) + '''"/>
	</route>\r\n'''
				fo.writelines(line) 	

	# 4.5 File Footer
	footer = '''</zone>
</platform>
	'''
	fo.writelines(footer)
	fo.close()

	#5. Generate deploy file 	
	pathFileName = ARCHITECTURE + "_" + str(totalNode) + '.deploy.xml'
	print 'Write paths into ' + pathFileName
	fo = open(pathFileName, "w")
	header = '''<?xml version='1.0'?> 
<!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid.dtd">
<platform version='4'>
'''
	fo.writelines(header)
	
	for nodeIdx in range(0,totalNode):
		for hostIdx in range(0,HOST_PER_NODE):
			idx = nodeIdx*HOST_PER_NODE + hostIdx
			if idx == 0:
				line = "	<process host='n" + str(idx) + "' function='master'/>\r\n"
			else:
				line = "	<process host='n" + str(idx) + "' function='worker'/>\r\n"
			fo.writelines(line)
	footer = '''</platform>'''
	fo.writelines(footer)
	fo.close()
	
	#6. Generate txt file 	
	pathFileName = ARCHITECTURE + "_" + str(totalNode) + '.txt'
	print 'Write paths into ' + pathFileName
	fo = open(pathFileName, "w")
	for nodeIdx in range(0,totalNode):
		for hostIdx in range(0,HOST_PER_NODE):
			idx = nodeIdx*HOST_PER_NODE + hostIdx
			line = "n" + str(idx) + ":1\r\n"
			fo.writelines(line)
	fo.close()
	
def get_host_Id(nodeIdx,hostIdx,hostPerNode):
	return nodeIdx*hostPerNode + hostIdx
	
main()

