## Compile: /home/nguyen/ai/SimGrid-3.19/build/bin/smpicc allreduce.c -o allreduce
###
SIMGRID="/home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpirun"
SIZE=32
PLATFORM="../platforms/Tsubame3_64.xml"
HOSTFILE="../platforms/Tsubame3_64.txt"
NODESIZE="4"
LOG_DIR="./log"
for ALGO in "redbcast" #"lr" #"ntt_lr_rdb" #"ntt_lr_binominal"
do
	HOSTFILE="../platforms/Tsubame3_64.lr.txt"
	#for APP1 in "lenet_mnist/" #"allreduce4M" #"allreduce8M" "allreduce16M" "allreduce32M" "allreduce64M" "allreduce128M" "allreduce256M"
	#do
		APP="./app/allreduce_lm_simgrid /home/nguyen/work/10_Approx/caffe_test/logs/lenet_mnist/"
		CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1 --cfg=smpi/process_of_node:${NODESIZE} --cfg=smpi/allreduce:${ALGO}" #--log=smpi_coll.:critical  --cfg=plugin:Link_Energy"
		LOG_FILE="${LOG_DIR}/lenet_mnist_${ALGO}_${SIZE}.log"
		#${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP} >> ${LOG_FILE} 2>&1 &	
		
		
		APP="./app/allreduce_lm_thres_redbcast_simgrid /home/nguyen/work/10_Approx/caffe_test/logs/lenet_mnist/"
		LOG_FILE="${LOG_DIR}/lenet_mnist_thres_${ALGO}_${SIZE}.log"
		CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1" # --cfg=smpi/process_of_node:${NODESIZE} --log=smpi_coll.:critical" # --cfg=smpi/allreduce:${ALGO}  --cfg=plugin:Link_Energy"
		#${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP} >> ${LOG_FILE} 2>&1 &	
	
		ALGO="rscateragather1"
		APP="./app/allreduce_lm_thres_rscateragather1_simgrid /home/nguyen/work/10_Approx/caffe_test/logs/lenet_mnist/"
		LOG_FILE="${LOG_DIR}/lenet_mnist_thres_${ALGO}_${SIZE}.log"
		CONFIG="--cfg=exception/cutpath:1 --cfg=smpi/display-timing:1" # --cfg=smpi/process_of_node:${NODESIZE} --log=smpi_coll.:critical" # --cfg=smpi/allreduce:${ALGO}  --cfg=plugin:Link_Energy"
		${SIMGRID} -np ${SIZE} -map -platform ${PLATFORM} -hostfile ${HOSTFILE} ${CONFIG} ${APP} #>> ${LOG_FILE} 2>&1 &	
	#done
done

