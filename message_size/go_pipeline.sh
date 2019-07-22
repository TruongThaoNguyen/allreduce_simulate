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

SIMGRID="/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpirun"
SIZE=256
PLATFORM="../platforms/Tsubame3_64.xml"
HOSTFILE="../platforms/Tsubame3_64.txt"
APP="../app/"
NODESIZE="4"
LOG_DIR="./logs"
ALGO="ntt_lr_lr_pipeline"
for SEGMENT in 2 #4 8 16 #32 64 128
do
	HOSTFILE="../platforms/Tsubame3_64.lr.txt"
	for MESSAGE_SIZE in "16M" "32M" "64M" "128M" #"256M" # "1M" "2M" "4M" "8M" #
	do
		APP1="allreduce${MESSAGE_SIZE}"
		CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical --cfg=plugin:Link_Energy  --cfg=smpi/pipeline_segment_number:${SEGMENT}"
		LOG_FILE="${LOG_DIR}${MESSAGE_SIZE}/${APP1}_${ALGO}_${SIZE}_${SEGMENT}.log"
		${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP}${APP1} >> ${LOG_FILE} 2>&1 &
	done
done

