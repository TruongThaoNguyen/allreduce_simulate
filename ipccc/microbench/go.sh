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
SIZE=8
PLATFORM="../../platforms/Tsubame3_64.xml"
HOSTFILE="../../platforms/Tsubame3_64.txt"
#APP="../app/run_all"
APP_DIR="/home/nguyen_truong/allreduce_simulate/ipccc/app/"
APP="run.out"
NODESIZE="4"
LOG_DIR="./logs"
densities=(0.0625 0.03125 0.015625 0.0078125 0.00390625 0.001953125)
dimensions=(16777216 33554432 67108864)


for SIZE in 32 #16 #32 #64 128 256 512 1024
do
	for N in "${dimensions[@]}"; do
		for D in "${densities[@]}"; do
			APP1="${APP_DIR}${APP} ${N} ${D}"
			CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE}" # --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical --cfg=plugin:Link_Energy"
			LOG_FILE="${LOG_DIR}/${SIZE}_${N}_${D}.log"
			${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP1} >> ${LOG_FILE} 2>&1 &
		done
		sleep(2m)
	done
done

