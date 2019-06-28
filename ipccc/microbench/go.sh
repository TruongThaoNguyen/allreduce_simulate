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

ARCHITECTURE="ABCI"
PLATFORM="../platforms/ABCI_68.xml"
HOSTFILE="../platforms/ABCI_68.txt"
#APP="../app/run_all"
APP_DIR="/home/nguyen_truong/allreduce_simulate/ipccc/app/"
APP="run.out"
NODESIZE="4"
LOG_DIR="./logs/"
densities=(0.0078125) #(1 0.0625 0.03125 0.015625 0.0078125 0.00390625 0.001953125)
dimensions=(33554432) #4194304 8388608 16777216 33554432 67108864 134217728 268435456)

for i in 0
do
	for j in 1 2 3 4 5 6 7 8 9 #1 2 3 4 5 6 7 8 9 0 
	do
		for ARCHITECTURE in "ABCI" "DGX1" "DGX2"
		do
			if [ "$ARCHITECTURE" = "ABCI" ]
			then
				PLATFORM="../platforms/ABCI_68.xml"
				HOSTFILE="../platforms/ABCI_68.txt"
			elif [ "$ARCHITECTURE" = "DGX1" ]
			then
				PLATFORM="../platforms/DGX1Cluster_36.xml"
				HOSTFILE="../platforms/DGX1Cluster_36.lr.txt"
			elif [ "$ARCHITECTURE" = "DGX2" ]
			then
				PLATFORM="../platforms/DGX2Cluster_18.xml"
				HOSTFILE="../platforms/DGX2Cluster_18.txt"
			else
				PLATFORM="../platforms/ABCI_68.xml"
				HOSTFILE="../platforms/ABCI_68.txt"
			fi

			for SIZE in 4
			do
				for N in "${dimensions[@]}"; do
					for D in "${densities[@]}"; do
						APP1="${APP_DIR}${APP} ${N} ${D}"
						CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE}" # --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical --cfg=plugin:Link_Energy"
						LOG_FILE="${LOG_DIR}/${ARCHITECTURE}_${SIZE}_${N}_${D}_${i}_${j}.log"
						${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP1} >> ${LOG_FILE} 2>&1 &
					done
				done
			done
		done
		sleep 1m
	done
done
