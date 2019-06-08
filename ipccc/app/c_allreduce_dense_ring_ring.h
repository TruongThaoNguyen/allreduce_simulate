#pragma once

#include "c_common.h"

/*
 * Sparse AllReduce following a recoursive doubling like algorithm
 */
template<class IdxType, class ValType> int c_allreduce_dense_ring_ring(const struct stream *sendbuf, struct stream *recvbuf, unsigned dim, MPI_Comm comm, int intraSize) {
	int tag = 1000;
	int size, rank, intra_count, inter_count, i,j;
	int intra_size = intraSize; //number of GPU per node
	
	MPI_Comm_size(comm, &size);; // number of node
	if (size % intra_size != 0){
		printf("FIX ME! allreduce_lr_lr algorithm can't be used if #process is not divisible by #process_per_group ! ");
	}
	MPI_Comm_rank(comm, &rank);
	int extent = sizeof(ValType);
	int inter_size = (size + intra_size - 1) / intra_size;
	
	int intra_rank = rank % intra_size; 
	int inter_rank = rank / intra_size; // nodeIdx
	//printf("[NNNN] node %d intra_rank = %d, inter_rank = %d\n", rank, intra_rank, inter_rank);
	
	int rcount = dim;
	int remainder, remainder_flag, remainder_offset;
	if (rcount % size != 0) {
		remainder = rcount % size;
		remainder_flag = 1;
		remainder_offset = (rcount / size) * size * extent;
	} else {
		remainder = remainder_flag = remainder_offset = 0;
	}  
	// size of each point-to-point communication is equal to the size of the whole message divided by number of processes
	intra_count = rcount / intra_size;
	inter_count = intra_count / inter_size;
	
	ValType *snd = (ValType*)sendbuf->items;
	ValType *rcv = (ValType*)recvbuf->items;
	/*1. reduce-scatter inside each group (local-ring)*/
	/**************************************************/
	//1.1. copy (partial of)send_buf to recv_buf
	//if (rank ==0){printf("[NNNN] [%d] intra lr reduce-scatter",rank);}
	int send_offset, recv_offset;
	int src, dst;
	send_offset = ((intra_rank - 1 + intra_size) % intra_size) * intra_count * extent;
	recv_offset = ((intra_rank - 1 + intra_size) % intra_size) * intra_count * extent;
	MPI_Sendrecv(snd + send_offset, intra_count* extent, MPI_BYTE, rank, tag - 1,
               rcv + recv_offset, intra_count* extent, MPI_BYTE, rank, tag - 1, comm,
               MPI_STATUS_IGNORE);
			   		   
	//1.2. reduce-scatter
	for (i = 0; i < (intra_size - 1); i++) {
		send_offset = ((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count * extent;
		recv_offset = ((intra_rank - 2 - i + 2 * intra_size) % intra_size) * intra_count * extent;
		src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
		dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
		MPI_Sendrecv(rcv + send_offset, intra_count* extent, MPI_BYTE, dst, tag + i, rcv+ recv_offset, intra_count, MPI_BYTE,src, tag + i, comm, MPI_STATUS_IGNORE);
		
		// compute result to rbuf+recv_offset
		for(j = 0; j < intra_count; ++j) {
		  (rcv+recv_offset)[j] = (rcv+recv_offset)[j] + (snd+recv_offset)[j];
		}	   
	}	
	
	/*2. reduce-scatter -inter between groups: the same local_rank nodes*/
	/**************************************************/
	//if (rank ==0){printf("[NNNN] [%d] inter lr reduce-scatter",rank);}
	//2.1. copy (partial of)recv_buf to send_buf
	//printf("[NNNN] [%d] intra lr reduce-scatter",rank);
	send_offset = ((intra_rank) % intra_size) * intra_count * extent;
	recv_offset = ((intra_rank) % intra_size) * intra_count * extent;
	MPI_Sendrecv(rcv + send_offset, intra_count* extent, MPI_BYTE, rank, tag - 1,
               snd + recv_offset, intra_count* extent, MPI_BYTE, rank, tag - 1, comm,
               MPI_STATUS_IGNORE);

	//2.1. reduce-scatter
	for (i = 0; i < (inter_size - 1); i++) {
		send_offset = (intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		recv_offset = (intra_rank * intra_count  + ((inter_rank - 2 - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
		dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
		MPI_Sendrecv(rcv + send_offset, inter_count*extent, MPI_BYTE, dst, tag + i, rcv + recv_offset, inter_count*extent, MPI_BYTE,src, tag + i, comm, MPI_STATUS_IGNORE);

		// compute result to rbuf+recv_offset
		for(j = 0; j < inter_count; ++j) {
		  (rcv+recv_offset)[j] = (rcv+recv_offset)[j] + (snd+recv_offset)[j];
		}   			   
	}
	
	/*3. allgather - inter between root of each SMP node*/
	/**************************************************/
	//if (rank ==0){printf("[NNNN] [%d] inter lr allgather",rank);}
	for (i = 0; i < (inter_size - 1); i++) {
		send_offset = (intra_rank * intra_count  + ((inter_rank - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		recv_offset = (intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
		dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
		MPI_Sendrecv(rcv + send_offset, inter_count*extent, MPI_BYTE, dst, tag + i, rcv + recv_offset, inter_count*extent, MPI_BYTE,src, tag + i, comm, MPI_STATUS_IGNORE);
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
	}

	/*4. allgather - inside each group */
	/**************************************************/
	//if (rank ==0){printf("[NNNN] [%d] intra lr allgather",rank);}
	for (i = 0; i < (intra_size - 1); i++) {
		send_offset = ((intra_rank - i + 2 * intra_size) % intra_size) * intra_count * extent;
		recv_offset = ((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count * extent;
		src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
		dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
		MPI_Sendrecv(rcv + send_offset, intra_count*extent, MPI_BYTE, dst, tag + i, rcv + recv_offset, intra_count*extent, MPI_BYTE,src, tag + i, comm, MPI_STATUS_IGNORE);	   	
	}

  return MPI_SUCCESS;
}
