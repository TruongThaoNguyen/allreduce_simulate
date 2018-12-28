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
SIZE=32
PLATFORM="../platforms/NVCluster_2_128.xml"
HOSTFILE="../platforms/NVCluster_2_128.lr.txt"
APP="./allreduce"
NODESIZE="4"
LOG_DIR="./logs"
for i in 0 #1 2 3 4 5 6 7 8 9 
do
	for j in 0 #1 2 #3 4 5 6 7 8 9
	do
		#for ALGO in "mvapich2" "ompi" "mpich"
		#for ALGO in "lr" "rdb" "rab" "mvapich2" "ompi" "mpich"
		#for ALGO in "ntt_smp_binominal" "ntt_binominal_lr" "ntt_lr_rab" "ntt_lr_lr" "ntt_lr_rdb" "lr"
		for ALGO in "lr"
		do
			for APP in "./allreduce1M" #"./allreduce2M" "./allreduce4M" "./allreduce8M" "./allreduce16M" "./allreduce32M"
			do
				CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical --cfg=plugin:Link_Energy"
				LOG_FILE="${LOG_DIR}/${APP}_${ALGO}_${SIZE}_${i}_${j}.log"
				${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP} >> ${LOG_FILE} 2>&1 &
			
				# if [ "$APP" = "./allreduce1M" ]
				# then
					# sleep 1m
				# elif [ "$APP" = "./allreduce2M" ]
				# then
					# sleep 1m
				# elif [ "$APP" = "./allreduce4M" ]
				# then
					# sleep 1m
				# elif [ "$APP" = "./allreduce8M" ]
				# then
					# sleep 2m
				# elif [ "$APP" = "./allreduce16M" ]
				# then
					# sleep 5m
				# else
					# sleep 10m
				# fi
			done
		done
	done
done

