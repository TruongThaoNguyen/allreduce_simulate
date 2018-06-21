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

SIMGRID="/home/nguyen/ai/SimGrid-3.19/build/bin/smpirun"
SIZE=1024
PLATFORM="./platforms/Tsubame3_128.xml"
HOSTFILE="./platforms/Tsubame3_128.lr.txt"
APP="./allreduce2M"
NODESIZE="8"
LOG_DIR="./logs"
for i in 0 #1 2 3 4 5 6 7 8 9 
do
	for j in 0 1 2 3 4 5 6 7 8 9
	do
		#for ALGO in "mvapich2" "ompi" "mpich"
		#for ALGO in "ntt_smp_binominal" "ntt_binominal_lr" "ntt_lr_rab" "ntt_lr_lr" "ntt_lr_rdb" #"mvapich2"  "mpich"
		for ALGO in "lr" "rdb" "rab" "mvapich2" "ompi" "mpich"
		do
			CONFIG="--cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO}  --log=smpi_coll.:critical"
			LOG_FILE="${LOG_DIR}/${APP}_${ALGO}_${SIZE}_${i}_${j}.log"
			${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP} >> ${LOG_FILE} 2>&1 &
			sleep 1m		
		done
	done
done

