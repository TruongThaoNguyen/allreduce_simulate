#!/bin/bash
## PARSING CG' LOG FILE
OUTPUT_FILE="./parse.txt"
TEMP_FILE="./temp.txt"
FILES="./logs/*"
echo -e "file\tALGORITHM\t#CORE_NUMBER\tBUF_SIZE\tITERATION\tSIMULATE_TIME\tDETAIL" >> ${OUTPUT_FILE}

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

	sed -i 's/\]\s*allreduce-.*\[0\] Finish algorithm/\tFinish/g' ${TEMP_FILE}

	cat "${TEMP_FILE}" >> ${OUTPUT_FILE}
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
