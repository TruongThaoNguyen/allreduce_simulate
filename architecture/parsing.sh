#!/bin/bash
## PARSING CG' LOG FILE
SIZE=""
OUTPUT_FILE="./parse${SIZE}.txt"
ENERGY_OUPUT_FILE="./energy_parse${SIZE}.txt"
TEMP_FILE="./temp${SIZE}.txt"
FILES="./logs${SIZE}/*"
echo -e "file\tALGORITHM\t#CORE_NUMBER\tBUF_SIZE\tITERATION\tSIMULATE_TIME\tDETAIL" >> ${OUTPUT_FILE}
echo -e "file\tALGORITHM\t#CORE_NUMBER\tBUF_SIZE\tITERATION\tCONSUME_ENERGY\tIDLE_ENERGY\tDETAIL" >> ${ENERGY_OUPUT_FILE}

grep -rl '\/home.*allreduce\/' $FILES | xargs sed -i 's/\/home.*allreduce\//\t/g'

for f in $FILES
do	
	#echo "Processing $f"	
	ALGO=$(grep -o "'smpi/allreduce' to '[a-z|0-9|_]*'" $f | head -1)
	CORE_NUMBER=$(grep -o "'smpi/process_of_node' to '[0-9|.]*'" $f | head -1)
	BUF_SIZE=$(grep -o "weight size: [0-9]*" $f | head -1)
	ITER=$(grep -o "iteration: [0-9|.]*" $f | head -1)
	DETAIL=$(grep -o ".*\[NNNN\] \[0\] [a-z,A-Z, ,-]*" $f)
	SIMULATE_TIME=$(grep -o "Simulated time: [0-9|.]*" $f | head -1)
	echo -e ${f}"\t"${ALGO}"\t"${CORE_NUMBER}"\t"${BUF_SIZE}"\t"${ITER}"\t"${SIMULATE_TIME} >> ${OUTPUT_FILE}
	echo -e ${f}"\t"${DETAIL} > ${TEMP_FILE}
	
	grep -rl 'Finish algorithm' ${TEMP_FILE}| xargs sed -i "s|Finish algorithm|Finish algorithm\n${f}\t|g"

	sed -i "s|\[n0:0:(1)||g"  ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] Start function/\tBegin\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] Start algorithm/\tStart\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] intra lr reduce-scatter/\tIntra-1\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] binomial reduce intra communication/\tIntra-1\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] inter lr reduce-scatter/\tInter-1\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] inter reduce-scatter/\tInter-1\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] binomial reduce inter-communication/\tInter-1\t/g' ${TEMP_FILE}
	# sed -i 's/\]\s*allreduce-.*\[0\] inter reduce-scatter/\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] inter lr allgather/\tInter-2\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] inter all-gather/\tInter-2\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] binomial broadcast inter-communication/\tInter-2\t/g' ${TEMP_FILE}
	# sed -i 's/\]\s*allreduce-.*\[0\] inter lr all-gather/\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] inter rdb/\tInter\t\t\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] intra lr allgather/\tIntra-2\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] binomial broadcast intra-communication/\tIntra-2\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] lr reduce-scatter/\tInter-1\t\t\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] lr all-gather/\tInter-2\t\t\t/g' ${TEMP_FILE}
	sed -i 's/\]\s*allreduce-.*\[0\] rdb rdb/\tInter\t\t\t/g' ${TEMP_FILE}

	sed -i 's/\]\s*allreduce-.*\[0\] Finish algorithm/\tFinish/g' ${TEMP_FILE}

	cat "${TEMP_FILE}" >> ${OUTPUT_FILE}
	
	CONSUME_ENERGY=$(grep -o "Total energy over all links: [0-9|.]*" $f | head -1)
	IDLE_ENERGY=$(grep -o "Total idle energy over all links: [0-9|.]*" $f | head -1)
	echo -e ${f}"\t"${ALGO}"\t"${CORE_NUMBER}"\t"${BUF_SIZE}"\t"${ITER}"\t"${CONSUME_ENERGY}"\t"${IDLE_ENERGY} >> ${ENERGY_OUPUT_FILE}
	
	ENERGY_DETAIL=$(grep -o "\[link_energy\/INFO\] .* Joules" $f)
	echo -e ${f}"\t"${ENERGY_DETAIL} > ${TEMP_FILE}
	sed -i 's/\[link_energy\/INFO\]//g' ${TEMP_FILE}
	grep -rl 'Joules  Energy consumption of link' ${TEMP_FILE}| xargs sed -i "s|Joules  Energy consumption of link|\n${f}\t|g"
	sed -i 's/Energy consumption of link //g' ${TEMP_FILE}
	sed -i 's/Joules  Idle.*:/\t/g' ${TEMP_FILE}
	sed -i 's/:/\t/g' ${TEMP_FILE}
	cat "${TEMP_FILE}" >> ${ENERGY_OUPUT_FILE}
done
rm ${TEMP_FILE}
sed -i 's/\.\/logs\// /g' ${OUTPUT_FILE}
sed -i 's/ to / /g' ${OUTPUT_FILE}
sed -i 's/smpi\/allreduce/ /g' ${OUTPUT_FILE}
sed -i 's/weight size:/ /g' ${OUTPUT_FILE}
sed -i 's/iteration:/ /g' ${OUTPUT_FILE}
sed -i 's/smpi\/process_of_node/ /g' ${OUTPUT_FILE}
sed -i 's/Simulated time: / /g' ${OUTPUT_FILE}
sed -i 's/ //g' ${OUTPUT_FILE}
sed -i "s/'//g" ${OUTPUT_FILE}

sed -i 's/\.\/logs\// /g' ${ENERGY_OUPUT_FILE}
sed -i 's/ to / /g' ${ENERGY_OUPUT_FILE}
sed -i 's/smpi\/allreduce/ /g' ${ENERGY_OUPUT_FILE}
sed -i 's/weight size:/ /g' ${ENERGY_OUPUT_FILE}
sed -i 's/iteration:/ /g' ${ENERGY_OUPUT_FILE}
sed -i 's/smpi\/process_of_node/ /g' ${ENERGY_OUPUT_FILE}
sed -i 's/Total energy over all links: / /g' ${ENERGY_OUPUT_FILE}
sed -i 's/Total idle energy over all links: / /g' ${ENERGY_OUPUT_FILE}
sed -i 's/ //g' ${ENERGY_OUPUT_FILE}
sed -i "s/'//g" ${ENERGY_OUPUT_FILE}
