#pragma once

#include "c_common.h"

/*
 * Sparse AllReduce following a recoursive doubling like algorithm
 */
template<class IdxType, class ValType> int c_allreduce_ring_ring(const struct stream *sendbuf, struct stream *recvbuf, unsigned dim, MPI_Comm comm, int intraSize) {
	int tag = 1000000;
	int size, rank, intra_count, inter_count, i,j;
	int intra_size = intraSize; //number of GPU per node
	MPI_Status status;
	MPI_Comm_size(comm, &size);; // number of node
	if (size % intra_size != 0){
		printf("FIX ME! allreduce_lr_lr algorithm can't be used if #process is not divisible by #process_per_group ! ");
	}
	MPI_Comm_rank(comm, &rank);
	double t_mpi1, t_mpi, computeTime, intraTime, interTime, initTime; 
	computeTime = 0;
	intraTime=0;
	interTime=0;
	initTime = 0;
	t_mpi = -MPI_Wtime();
	//int extent = (sizeof(unsigned) + maxsegsize * sizeof(ValType));
	int inter_size = (size) / intra_size;
	int intra_rank = rank % intra_size; 
	int inter_rank = rank / intra_size; // nodeIdx
	
	int rcount = dim;
/* 	int remainder, remainder_flag, remainder_offset;
	if (rcount % size != 0) {
		remainder = rcount % size;
		remainder_flag = 1;
		remainder_offset = (rcount / size) * size * extent;
	} else {
		remainder = remainder_flag = remainder_offset = 0;
	}   */
	
	int segsize, *segsizes; /* segment sizes and offsets per segment*/
	segsizes = (int*)malloc(sizeof(int)*size);
	int *intra_segsizes;
	intra_segsizes = (int*)malloc(sizeof(int)*intra_size);
	segsize = rcount/size; // size of the segments 
	int maxsegsize = rcount - ((size-1)*segsize);
	int k;
	for (i =0; i <intra_size; i++){
		intra_segsizes[i] = 0;
		for (j = 0; j < inter_size; j++){
			k = i * inter_size + j;
			segsizes[k] = segsize;
			intra_segsizes[i] += segsizes[k];
		}
	}
	segsizes[size-1] = maxsegsize;
	intra_segsizes[intra_size -1] = intra_segsizes[intra_size -1] - segsize + maxsegsize;
	
	//Prepare for split stream
	char *intra_buf = (char *) malloc(intra_size * (sizeof(unsigned) + (inter_size * maxsegsize * sizeof(ValType))));
	struct stream* intra_splits[intra_size];
	struct stream* intra_recvsplit = (struct stream*)malloc(sizeof(unsigned) + (inter_size * maxsegsize * sizeof(ValType)));
	struct stream* intra_tmpbuf = (struct stream*)malloc(sizeof(unsigned) + (inter_size * maxsegsize * sizeof(ValType)));
	struct stream* ptrForDelete1 = intra_recvsplit;
	struct stream* ptrForDelete2 = intra_tmpbuf;
	
	char *inter_buf = (char *) malloc(inter_size * (sizeof(unsigned) + maxsegsize * sizeof(ValType)));
	struct stream* inter_splits[inter_size];
	struct stream* inter_recvsplit = (struct stream*)malloc(sizeof(unsigned) + maxsegsize * sizeof(ValType));
	struct stream* inter_tmpbuf = (struct stream*)malloc(sizeof(unsigned) + maxsegsize * sizeof(ValType));
	struct stream* ptrForDelete3 = inter_recvsplit;
	struct stream* ptrForDelete4 = inter_tmpbuf;
	
	for(i = 0; i < intra_size; ++i) {
		intra_splits[i] = (struct stream *)(intra_buf + i * (sizeof(unsigned) + (inter_size * maxsegsize * sizeof(ValType))));
	}
	split_stream<IdxType, ValType>(sendbuf, intra_splits, rcount, intra_size);
		
	for(i = 0; i < inter_size; ++i) {
		inter_splits[i] = (struct stream *)(inter_buf + i * (sizeof(unsigned) + maxsegsize * sizeof(ValType)));
	}
		
	// size of each point-to-point communication is equal to the size of the whole message divided by number of processes
	inter_count = rcount / size;
	intra_count = inter_count * intra_size;  //rcount / intra_size;
	//printf("[NNNN] [%d] rcount=%d intra_rank = %d, inter_rank = %d, intra_size=%d, inter_size=%d, intra_count=%d, inter_count=%d, nofitems=%d\n", rank,rcount, intra_rank, inter_rank,intra_size,inter_size,intra_count,inter_count, sendbuf->nofitems);

	t_mpi += MPI_Wtime();
	initTime += t_mpi;
	t_mpi = -MPI_Wtime();
	/*1. reduce-scatter inside each group (local-ring)*/
	/**************************************************/
	//1.1. copy (partial of)send_buf to recv_buf
	int send_element, recv_element, send_segIdx, recv_segIdx;
	int src, dst;
	struct stream* tmpptr = NULL;
	
 	//1.2. reduce-scatter
	src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
	dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
	//printf("[%d-%d] src=%d, dst=%d\n",rank, intra_rank,src,dst);
	for (i = 0; i < (intra_size - 1); i++) {
		send_element = (intra_rank - 1 - i + 2 * intra_size) % intra_size; // * extent;
		recv_element = (intra_rank - 2 - i + 2 * intra_size) % intra_size; // * extent;
		//printf("[NNNN] [%d] src=%d dst=%d, step=%d-%d, send_element=%d, recv_element=%d\n",rank,src,dst,i,j,send_element,recv_element);
		MPI_Sendrecv(intra_splits[send_element], countBytes<IdxType, ValType>(intra_splits[send_element], intra_segsizes[send_element]), MPI_BYTE, dst, tag + i, 
			intra_recvsplit, sizeof(unsigned) + (inter_size * maxsegsize * sizeof(ValType)), MPI_BYTE,src, tag + i, comm, &status);
			
		// compute result to intra_recvsplit[recv_element]
		t_mpi1 = -MPI_Wtime();
		tmpptr = sum_into_stream<IdxType, ValType>(intra_splits[recv_element], intra_recvsplit, intra_tmpbuf, intra_segsizes[recv_element], false);
		if(tmpptr == intra_recvsplit) {
			intra_recvsplit = intra_splits[recv_element];
			intra_splits[recv_element] = tmpptr;
		} 
		else if(tmpptr == intra_tmpbuf) {
			intra_tmpbuf = intra_splits[recv_element];
			intra_splits[recv_element] = tmpptr;
		}
		t_mpi1 += MPI_Wtime();
		computeTime += t_mpi1;
	} 
	t_mpi += MPI_Wtime();
	intraTime += t_mpi;
	t_mpi1 = -MPI_Wtime();
	
	//SPLIT stream for inter communication
	//printf("recv_element: %d",recv_element);
	split_stream<IdxType, ValType>(intra_splits[recv_element], inter_splits, intra_segsizes[recv_element], inter_size);
		
	t_mpi1 += MPI_Wtime();
	computeTime += t_mpi1;	
	t_mpi = -MPI_Wtime();
	/*2. reduce-scatter -inter between groups: the same local_rank nodes*/
	/**************************************************/
 	//if (rank ==0){printf("[NNNN] [%d] inter lr reduce-scatter",rank);}
	//2.2. reduce-scatter
	src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
	dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
	for (i = 0; i < (inter_size - 1); i++) {
		send_element = (inter_rank - 1 - i + 2 * inter_size)%inter_size;// * extent;
		recv_element = (inter_rank - 2 - i + 2 * inter_size)%inter_size;// * extent;
		send_segIdx = (intra_rank * (size/intra_size) + send_element);
		recv_segIdx = (intra_rank * (size/intra_size) + recv_element);
		
		//printf("[NNNN] [%d] src=%d dst=%d, step=%d, send_element=%d, recv_element=%d,send_segIdx=%d,recv_segIdx=%d\n",rank,src,dst,i,send_element,recv_element,send_segIdx,recv_segIdx);
		MPI_Sendrecv(inter_splits[send_element], countBytes<IdxType, ValType>(inter_splits[send_element], segsizes[send_segIdx]), MPI_BYTE, dst, tag + i, 
				inter_recvsplit, sizeof(ValType) * maxsegsize + sizeof(unsigned), MPI_BYTE,src, tag + i, comm, &status);
		
		// compute result to rbuf+recv_offset
		t_mpi1 = -MPI_Wtime();
		tmpptr = sum_into_stream<IdxType, ValType>(inter_splits[recv_element], inter_recvsplit, inter_tmpbuf, segsizes[recv_segIdx], false);
		if(tmpptr == inter_recvsplit) {
			inter_recvsplit = inter_splits[recv_element];
			inter_splits[recv_element] = tmpptr;
		} 
		else if(tmpptr == inter_tmpbuf) {
			inter_tmpbuf = inter_splits[recv_element];
			inter_splits[recv_element] = tmpptr;
		}
		t_mpi1 += MPI_Wtime();
		computeTime += t_mpi1;
	}
	t_mpi += MPI_Wtime();
	interTime += t_mpi;
	t_mpi = -MPI_Wtime();
	
	/*3+4. Allgather with dense format.*/
	// Add into received buffer
	t_mpi1 = -MPI_Wtime();
	unsigned overall = 0;
	
	
  for(i = 0; i < size; ++i) {
    overall += splits[i]->nofitems;
  }

  if (overall * (sizeof(IdxType) + sizeof(ValType)) >= dim * sizeof(ValType)) {
    recvbuf->nofitems = dim;
    ValType * result = (ValType *)recvbuf->items;
#pragma omp simd 
    for(size_t i = 0; i < dim; ++i) {
      result[i] = 0.0;
    }
    unsigned offset = 0;
    for(i = 0; i < size; ++i) {
      if((int)splits[i]->nofitems == segsizes[i]) {
        // Dense
        for(int j = 0; j < segsizes[i]; ++j) {
          result[offset + j] = ((ValType *)splits[i]->items)[j];
        }
      } else {
        // Sparse
        const struct s_item<IdxType, ValType> *values = (const struct s_item<IdxType, ValType> *)splits[i]->items;
        for(unsigned j = 0; j < splits[i]->nofitems; ++j) {
          result[offset + values[j].idx] = values[j].val;
        }
      }
      offset += segsize;
    }
  } else {
    recvbuf->nofitems = overall;
    struct s_item<IdxType, ValType> *result = (struct s_item<IdxType, ValType> *)recvbuf->items;
    int idx = 0;
    unsigned offset = 0;
    for(i = 0; i < size; ++i) {
      if((int)splits[i]->nofitems == segsizes[i]) {
        // Dense
        for(j = 0; j < segsizes[i]; ++j) {
          result[idx].idx = j + offset;
          result[idx].val = ((ValType *)splits[i]->items)[j];
          idx++;
        }
      } else {
        // Sparse
        const struct s_item<IdxType, ValType> *values = (const struct s_item<IdxType, ValType> *)splits[i]->items;
        for(unsigned j = 0; j < splits[i]->nofitems; ++j) {
          result[idx].idx = values[j].idx + offset;
          result[idx].val = values[j].val;
          idx++;
        }
      }
      offset += segsize;
    }
  }
 	t_mpi1 += MPI_Wtime();
	computeTime += t_mpi1;
	
	
	
	/*3. allgather - inter between root of each SMP node*/
	/**************************************************/
/*  	//if (rank ==0){printf("[NNNN] [%d] inter lr allgather",rank);}
	src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
	dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
	for (i = 0; i < (inter_size - 1); i++) {
		send_element = (inter_rank - i + 2 * inter_size)%inter_size;  // * extent;
		recv_element = (inter_rank - 1 - i + 2 * inter_size)%inter_size; // * extent;
		send_segIdx = (intra_rank * (size/intra_size) + send_element);
		recv_segIdx = (intra_rank * (size/intra_size) + recv_element);
		//printf("[NNNN] [%d] src=%d dst=%d, step=%d, send_element=%d, recv_element=%d\n",rank,src,dst,i,send_element,recv_element);
		
		MPI_Sendrecv(inter_splits[send_element], countBytes<IdxType, ValType>(inter_splits[send_element], segsizes[send_segIdx]), MPI_BYTE, dst, tag + i, 
			inter_splits[recv_element], sizeof(ValType) * maxsegsize + sizeof(unsigned), MPI_BYTE,src, tag + i, comm, &status);	   	
	} 
	t_mpi += MPI_Wtime();
	interTime += t_mpi;
	t_mpi = -MPI_Wtime(); */
	
	/*4. allgather - inside each group */
	/**************************************************/
/*  	//if (rank ==0){printf("[NNNN] [%d] intra lr allgather",rank);}
	src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
	dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
	
	for (i = 0; i < (intra_size - 1); i++) {
		for (j=0; j < inter_size; j++){
			send_element = ((intra_rank - i + 2 * intra_size) % intra_size) * (size/intra_size) + j ;// * extent;
			recv_element = ((intra_rank - 1 - i + 2 * intra_size) % intra_size) * (size/intra_size) + j;// * extent;		
			//printf("[NNNN] [%d] src=%d dst=%d, step=%d-%d, send_element=%d, recv_element=%d\n",rank,src,dst,i,j,send_element,recv_element);
		
			MPI_Sendrecv(splits[send_element], countBytes<IdxType, ValType>(splits[send_element], segsizes[send_element]), MPI_BYTE, dst, tag + i, 
				splits[recv_element], sizeof(ValType) * maxsegsize + sizeof(unsigned), MPI_BYTE,src, tag + i, comm, &status);	   	
		}
	} 
	t_mpi += MPI_Wtime();
	intraTime += t_mpi; */
	
 /* // Add into received buffer
	t_mpi1 = -MPI_Wtime();
  unsigned overall = 0;
  for(i = 0; i < size; ++i) {
    overall += splits[i]->nofitems;
  }

  if (overall * (sizeof(IdxType) + sizeof(ValType)) >= dim * sizeof(ValType)) {
    recvbuf->nofitems = dim;
    ValType * result = (ValType *)recvbuf->items;
#pragma omp simd 
    for(size_t i = 0; i < dim; ++i) {
      result[i] = 0.0;
    }
    unsigned offset = 0;
    for(i = 0; i < size; ++i) {
      if((int)splits[i]->nofitems == segsizes[i]) {
        // Dense
        for(int j = 0; j < segsizes[i]; ++j) {
          result[offset + j] = ((ValType *)splits[i]->items)[j];
        }
      } else {
        // Sparse
        const struct s_item<IdxType, ValType> *values = (const struct s_item<IdxType, ValType> *)splits[i]->items;
        for(unsigned j = 0; j < splits[i]->nofitems; ++j) {
          result[offset + values[j].idx] = values[j].val;
        }
      }
      offset += segsize;
    }
  } else {
    recvbuf->nofitems = overall;
    struct s_item<IdxType, ValType> *result = (struct s_item<IdxType, ValType> *)recvbuf->items;
    int idx = 0;
    unsigned offset = 0;
    for(i = 0; i < size; ++i) {
      if((int)splits[i]->nofitems == segsizes[i]) {
        // Dense
        for(j = 0; j < segsizes[i]; ++j) {
          result[idx].idx = j + offset;
          result[idx].val = ((ValType *)splits[i]->items)[j];
          idx++;
        }
      } else {
        // Sparse
        const struct s_item<IdxType, ValType> *values = (const struct s_item<IdxType, ValType> *)splits[i]->items;
        for(unsigned j = 0; j < splits[i]->nofitems; ++j) {
          result[idx].idx = values[j].idx + offset;
          result[idx].val = values[j].val;
          idx++;
        }
      }
      offset += segsize;
    }
  }
 	t_mpi1 += MPI_Wtime();
	computeTime += t_mpi1;
*/	
	
	free(inter_buf);
	free(intra_buf);
	free(ptrForDelete1);
	free(ptrForDelete2);
	free(ptrForDelete3);
	free(ptrForDelete4);
	free(intra_segsizes);
	free(segsizes);
    if(rank == 0) {
		printf("\t\tSparse Ring Ring Compute time: \t%f\tsecs\n", computeTime);
		printf("\t\tSparse Ring Ring Intra time: \t%f\tsecs\n", intraTime);
		printf("\t\tSparse Ring Ring Inter time: \t%f\tsecs\n", interTime);
		printf("\t\tSparse Ring Ring Init time: \t%f\tsecs\n", initTime);
	}
	return MPI_SUCCESS;
}
