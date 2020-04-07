make alexnet_para
ALGO="lr" #"ntt_lr_lr" #"lr"
DATA_SIZE=1280000 #1280000	# Dataset size (number of samples)
BATCH=32 # Minibatch size per node.
EPOCH=1 #Number of epoch;
NODE=3
NODESIZE=4
APP_NAME="ALEXNET"
#APP_NAME="allreduce"
LOG_DIR="logs/" #${APP_NAME}"
mkdir ${LOG_DIR}
for NODE in 256 #128 64 #2 4 8 16 32 #128 64 256 #1024 # 512 # 64 128 256 #2 4 8 16 32 
do
	for i in 0 #1 2 3 4 5 6 7 8 9 
	do
		for j in 0 #1 2 3 4 5 6 7 8 9 #0 #
		do
			for k in 0 #1 2 3 4 5 6 7 8 9
			do
				APP="alexnet_para"
				LOG_FILE="${LOG_DIR}/${APP}_${NODE}_${DATA_SIZE}_${BATCH}_${EPOCH}_${i}${j}${k}.log"
				PARAM="--cfg=smpi/host-speed:100000000000 --cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO}"
				PLATFORM="-hostfile ./platforms/ABCI_340.txt -platform ./platforms/ABCI_340.xml "
				APP="${APP}.run" # ${DATA_SIZE} ${BATCH} ${EPOCH}"
				#APP="./allreduce.run" # ${DATA_SIZE} ${BATCH} ${EPOCH}"
				/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpirun -np ${NODE} ${PLATFORM} ${PARAM} ${APP} >> ${LOG_FILE} 2>&1 &
				
				sleep 1s
			done
		done
	done
done

