/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * implemented by NguyenTT, 12/06/2018
 * logic of code copy from allreduce-lr.cpp
 */
#include "../colls_private.hpp"
#include "xbt/config.h"
/*
This fucntion performs all-reduce operation as follow.
Device network into groups (or nodes). Each group has K ranks.
1) reduce-scatter inside each group (local-ring)
2) reduce-scatter -inter between groups: the same local_rank nodes
3) allgather - inter between root of each SMP node
4)allgather - inside each group
*/
namespace simgrid{
namespace smpi{
	
int Coll_allreduce_ntt_lr_lr::allreduce(void *sbuf, void *rbuf, int rcount,
                                           MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)
{
	if (comm->rank() ==0){ XBT_WARN("[NNNN] [%d] Start function",comm->rank());}
	int tag = COLL_TAG_ALLREDUCE;
	MPI_Status status;
	int size, rank, intra_count, inter_count, i;
	
 	int intra_size=1;
	intra_size = xbt_cfg_get_int("smpi/process_of_node");
	if ( intra_size <= 0){
		THROWF(arg_error,0, "allreduce ntt_lr_lr algorithm can't be used with %d processes per a group", intra_size);  
	}

	size = comm->size(); 
	// if((size&(size-1))){
		// THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_lr algorithm can't be used with non power of two number of processes ! ");
	// }
	
	if (size % intra_size != 0){
		THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_lr algorithm can't be used if #process is not divisible by #process_per_group ! ");
	}
	
	rank = comm->rank();
	/* make it compatible with all data type */
	MPI_Aint extent;
	extent = dtype->get_extent();
	int inter_size = (size + intra_size - 1) / intra_size;
	//int intra_rank, inter_rank;
	int intra_rank = rank % intra_size; 
	int inter_rank = rank / intra_size; // nodeIdx
	XBT_DEBUG("[NNNN] node %d intra_rank = %d, inter_rank = %d\n", rank, intra_rank, inter_rank);

	/* when communication size is smaller than number of process (not support) */
	if (rcount < size) {
		XBT_WARN("MPI_allreduce ntt_lr_lr use default MPI_allreduce.");
		Coll_allreduce_default::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	} 

	/* When communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
/* 	if (rcount % size != 0) {
		XBT_WARN("FIX ME! MPI_allreduce ntt_lr_lr can't support the abbitrary data size. The data size should be devisible by #process");
	}
 */		
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
	
	if (rank ==0){XBT_WARN("[NNNN] [%d] Start algorithm",rank);}
	char alert[1000];
	//XBT_WARN("[NNNN] [%d] sbuf=[%s]",rank, print_buffer(sbuf,rcount,alert));	
	/*1. reduce-scatter inside each group (local-ring)*/
	/**************************************************/
	//1.1. copy (partial of)send_buf to recv_buf
	if (rank ==0){XBT_WARN("[NNNN] [%d] intra lr reduce-scatter",rank);}
	int send_offset, recv_offset;
	int src, dst;
	send_offset = ((intra_rank - 1 + intra_size) % intra_size) * intra_count * extent;
	recv_offset = ((intra_rank - 1 + intra_size) % intra_size) * intra_count * extent;
	Request::sendrecv((char *) sbuf + send_offset, intra_count, dtype, rank, tag - 1,
               (char *) rbuf + recv_offset, intra_count, dtype, rank, tag - 1, comm,
               &status);
	//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   
	//1.2. reduce-scatter
	for (i = 0; i < (intra_size - 1); i++) {
		send_offset = ((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count * extent;
		recv_offset = ((intra_rank - 2 - i + 2 * intra_size) % intra_size) * intra_count * extent;
		src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
		dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
		Request::sendrecv((char *) rbuf + send_offset, intra_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, intra_count, dtype,src, tag + i, comm, &status);

		// compute result to rbuf+recv_offset
		if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
					   &intra_count, dtype);
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   			   
	}

	/*2. reduce-scatter -inter between groups: the same local_rank nodes*/
	/**************************************************/
	if (rank ==0){XBT_WARN("[NNNN] [%d] inter lr reduce-scatter",rank);}
	//2.1. copy (partial of)recv_buf to send_buf
	//XBT_WARN("[NNNN] [%d] intra lr reduce-scatter",rank);
	send_offset = ((intra_rank) % intra_size) * intra_count * extent;
	recv_offset = ((intra_rank) % intra_size) * intra_count * extent;
	Request::sendrecv((char *) rbuf + send_offset, intra_count, dtype, rank, tag - 1,
               (char *) sbuf + recv_offset, intra_count, dtype, rank, tag - 1, comm,
               &status);
	////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
	//2.1. reduce-scatter
	for (i = 0; i < (inter_size - 1); i++) {
		send_offset = (intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		recv_offset = (intra_rank * intra_count  + ((inter_rank - 2 - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
		dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
		Request::sendrecv((char *) rbuf + send_offset, inter_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, inter_count, dtype,src, tag + i, comm, &status);

		// compute result to rbuf+recv_offset
		if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
					   &inter_count, dtype);
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   			   
	}
	
	/*3. allgather - inter between root of each SMP node*/
	/**************************************************/
	if (rank ==0){XBT_WARN("[NNNN] [%d] inter lr allgather",rank);}
	for (i = 0; i < (inter_size - 1); i++) {
		send_offset = (intra_rank * intra_count  + ((inter_rank - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		recv_offset = (intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) * extent;
		src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
		dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
		Request::sendrecv((char *) rbuf + send_offset, inter_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, inter_count, dtype,src, tag + i, comm, &status);
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
	}

	/*4. allgather - inside each group */
	/**************************************************/
	if (rank ==0){XBT_WARN("[NNNN] [%d] intra lr allgather",rank);}
	for (i = 0; i < (intra_size - 1); i++) {
		send_offset = ((intra_rank - i + 2 * intra_size) % intra_size) * intra_count * extent;
		recv_offset = ((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count * extent;
		src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
		dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
		Request::sendrecv((char *) rbuf + send_offset, intra_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, intra_count, dtype,src, tag + i, comm, &status);
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
	}
	
	/* when communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
	if (remainder_flag) {
		//XBT_WARN("[NNNN] [%d] remainder path",rank);
		XBT_WARN("For MPI_allreduce ntt_lr_lr when communication data count is not divisible by number of process, call the native implementation for the remain chunk at the end of the operation");
		return Colls::allreduce((char *) sbuf + remainder_offset,(char *) rbuf + remainder_offset, remainder, dtype, op,comm);
		//XBT_WARN("[NNNN] [%d] buf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   
	}
    if (rank ==0){XBT_WARN("[NNNN] [%d] Finish algorithm",rank);}	
	return MPI_SUCCESS;
}
}
}
