## Compile: /home/nguyen/ai/SimGrid-3.19/build/bin/smpicc allreduce.c -o allreduce
###
# mpich: 
	# + rab for long message (> 2048)
	# + rdb for short message
# mvapich2: ~~> 1 leader (reduce /bcast, ..?)
# openmpi: message size (10000)
	# < 10000: rdb
	# if (opperation is not define), use lr or ring_segmented
	# else use   redbcast.

# cd ../
# rm ./smpi-*
# rm ./allreduce	
# /home/nguyen/ai/SimGrid-3.19/build/bin/smpicc allreduce.c -o allreduce
# cd pipeline
	
#SIMGRID="/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpirun"
SIMGRID=" /home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpirun"
SIZE=32
PLATFORM="../../platforms/Tsubame3_64.xml"
#PLATFORM="../../platforms/NVCluster_4_64.xml"
HOSTFILE="../../platforms/Tsubame3_64.lr.txt"
#HOSTFILE="../../platforms/NVCluster_4_64.lr.txt"
APP="./allreduce32"
NODESIZE="4"
LOG_DIR="./logs"
for i in 0 #1 2 3 4 5 6 7 8 9 
do
	for j in 0 #1 2 #3 4 5 6 7 8 9
	do
		#for ALGO in "mvapich2" "ompi" "mpich"
		#for ALGO in "lr" "rdb" "rab" "mvapich2" "ompi" "mpich"
		#for ALGO in "ntt_smp_binominal" "ntt_binominal_lr" "lr"
		#for ALGO in "ntt_lr_rab" "ntt_lr_lr" "ntt_lr_rdb" 
		for ALGO in "ntt_lr_lr_pipeline" 
		do
			for APP in "allreduce32" #"allreduce32_10" #"allreduce" "allreduce8" "allreduce16" "allreduce32"
			do
				SEGMENT=2
				CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/pipeline_segment_number:${SEGMENT} --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical"
				LOG_FILE="${LOG_DIR}/${APP}_${ALGO}_${SIZE}_${SEGMENT}.log"
				APP1="../${APP}"
				${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP1} > ${LOG_FILE} 2>&1 &
			done
		done
	done
done

