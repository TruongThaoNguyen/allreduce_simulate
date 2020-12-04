CC=/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicxx
#CC=/home/nguyen/ai/allreduce_simulate/SimGrid-3.19/build/bin/smpicxx
CC_DAINT=CC
#CCFLAGS=-O3 -Wall -std=c++11 -fopenmp
CCFLAGS=-O3 -Wall -std=c++11
CCLNFLAGS=

run: c_common.h c_allreduce_recdoubling.h 
	${CC} ${CCFLAGS} -c run.c 
	${CC} -o $@.out $@.o ${CCLNFLAGS}

# run_daint: c_common.h c_allreduce_recdoubling.h
	# ${CC_DAINT} ${CCFLAGS} -c run.c 
	# ${CC_DAINT} -o run.out run.o ${CCLNFLAGS}

clean:
	rm -rf *.o *.out
	
ntt: c_allreduce_dense_ring_ring.h
	${CC} ${CCFLAGS} -c c_allreduce_dense_ring_ring.h
	${CC} -o $@.out ${CCLNFLAGS}