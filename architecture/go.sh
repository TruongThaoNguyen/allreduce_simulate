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
PLATFORM="../platforms/Dragonfly_4_64.xml"
HOSTFILE="../platforms/Dragonlfy_4_64.txt"
APP="../app/"
NODESIZE="4"
LOG_DIR="./logs"
#for ALGO in "mvapich2" "ompi" "mpich"
#for ALGO in "lr" "rdb" "rab_rdb"
# for ALGO in "lr" "ntt_smp_binominal" "ntt_binominal_lr" #"rdb" 
# do	
for ALGO in "ntt_lr_rdb" "lr" #"ntt_lr_rab" "ntt_lr_lr" #"ntt_lr_rdb" "lr" #"ntt_lr_binominal"
do
	APP1="allreduce32M"
	HOSTFILE="../platforms/Dragonfly_64.lr.txt"
	CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO} --log=smpi_coll.:critical --cfg=plugin:Link_Energy"
	LOG_FILE="${LOG_DIR}/${APP1}_${ALGO}_Dragonfly_4_64.log"
	${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP}${APP1} >> ${LOG_FILE} 2>&1 &

	# if [ "$SIZE" = "32" ]
	# then
		# sleep 3100s
	# elif [ "$SIZE" = "64" ]
	# then
		# sleep 6200s
	# elif [ "$SIZE" = "128" ]
	# then
		# sleep 12400s
	# elif [ "$SIZE" = "256" ]
	# then
		# sleep 24800s
	# else
		# sleep 10m
	# fi
done

