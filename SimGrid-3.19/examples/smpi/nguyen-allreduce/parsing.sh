#!/bin/bash
## PARSING CG' LOG FILE
OUTPUT_FILE="./parse.txt"
FILES="./logs/*"
echo -e "file\tALGORITHM\t#CORE_NUMBER\tBUF_SIZE\tITERATION\tSIMULATE_TIME\tDETAIL" >> ${OUTPUT_FILE}
for f in $FILES
do
	#echo "Processing $f"	
	ALGO=$(grep -o "'smpi/allreduce' to '[a-z|0-9|_]*'" $f | head -1)
	CORE_NUMBER=$(grep -o "'smpi/process_of_node' to '[0-9|.]*'" $f | head -1)
	BUF_SIZE=$(grep -o "weight size: [0-9]*" $f | head -1)
	ITER=$(grep -o "iteration: [0-9|.]*" $f | head -1)
	DETAIL=$(grep -o ".*\[NNNN\] \[0\] [a-z,A-Z, ,-]*" $f)
	SIMULATE_TIME=$(grep -o "Simulated time: [0-9|.]*" $f | head -1)
	
	echo -e ${f}"\t"${ALGO}"\t"${CORE_NUMBER}"\t"${BUF_SIZE}"\t"${ITER}"\t"${SIMULATE_TIME}"\t"${DETAIL} >> ${OUTPUT_FILE}
done

sed -i 's/\.\/logs\// /g' ${OUTPUT_FILE}
sed -i 's/\[n0:0:(1)/ /g'  ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] Start function/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] Start algorithm/\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] intra lr reduce-scatter/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] binomial reduce intra communication/\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] inter lr reduce-scatter/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] inter reduce-scatter/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] binomial reduce inter-communication/\t/g' ${OUTPUT_FILE}
#sed -i 's/\]\s\/home.*\[0\] inter reduce-scatter/\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] inter lr allgather/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] inter all-gather/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] binomial broadcast inter-communication/\t/g' ${OUTPUT_FILE}
#sed -i 's/\]\s\/home.*\[0\] inter lr all-gather/\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] inter rdb/\t\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] intra lr allgather/\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] binomial broadcast intra-communication/\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] lr reduce-scatter/\t\t/g' ${OUTPUT_FILE}
sed -i 's/\]\s\/home.*\[0\] lr all-gather/\t\t/g' ${OUTPUT_FILE}

sed -i 's/\]\s\/home.*\[0\] Finish algorithm/ /g' ${OUTPUT_FILE}

sed -i 's/ to / /g' ${OUTPUT_FILE}
sed -i 's/smpi\/allreduce/ /g' ${OUTPUT_FILE}
sed -i 's/weight size:/ /g' ${OUTPUT_FILE}
sed -i 's/iteration:/ /g' ${OUTPUT_FILE}
sed -i 's/smpi\/process_of_node/ /g' ${OUTPUT_FILE}
sed -i 's/Simulated time: / /g' ${OUTPUT_FILE}
sed -i 's/ //g' ${OUTPUT_FILE}
sed -i "s/'//g" ${OUTPUT_FILE}
