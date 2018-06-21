roundtrip: roundtrip.c
	~/ai/SimGrid-3.19/build/bin/smpicc roundtrip.c -o roundtrip

SEED=`date +%N` 
####518707082
run: roundtrip
	~/ai/SimGrid-3.19/build/bin/smpirun -np 2 -hostfile ./hostfile -platform ../../platforms/energy_platform.xml --cfg=smpi/display-timing:1 ./roundtrip --cfg=plugin:Energy --cfg=plugin:Link_Energy

clean:
	/bin/rm -f roundtrip
