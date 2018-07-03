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
#for ALGO in "mvapich2" "ompi" "mpich"
#for ALGO in "lr" "rdb" "rab_rdb"
for ALGO in "lr" "rdb" "ntt_smp_binominal" "ntt_binominal_lr"
do
# for ALGO in "ntt_lr_rab" "ntt_lr_lr" "ntt_lr_rdb" #"ntt_lr_binominal"
# do
	# HOSTFILE="../platforms/Tsubame3_64.lr.txt"
	for APP1 in "allreduce8M" #"allreduce4M" #"allreduce8M" "allreduce16M" "allreduce32M" "allreduce64M" "allreduce128M" "allreduce256M"
	do
		CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical --cfg=plugin:Link_Energy"
		LOG_FILE="${LOG_DIR}/${APP1}_${ALGO}_${SIZE}.log"
		${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP}${APP1} >> ${LOG_FILE} 2>&1 &
	
		# if [ "$APP" = "./allreduce4M" ]
		# then
			# sleep 30m
		# elif [ "$APP" = "./allreduce8M" ]
		# then
			# sleep 1h
		# elif [ "$APP" = "./allreduce16M" ]
		# then
			# sleep 2h
		# elif [ "$APP" = "./allreduce32M" ]
		# then
			# sleep 4h
		# elif [ "$APP" = "./allreduce64M" ]
		# then
			# sleep 8h
		# elif [ "$APP" = "./allreduce128M" ]
		# then
			# sleep 16h
		# elif [ "$APP" = "./allreduce256M" ]
		# then
			# sleep 32h
		# else
			# sleep 10m
		# fi
	done
done

