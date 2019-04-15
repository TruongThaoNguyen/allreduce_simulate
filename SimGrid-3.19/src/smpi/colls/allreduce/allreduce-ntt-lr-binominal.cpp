/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * implemented by NguyenTT, 12/06/2018
 * a part logic of code copy from allreduce-lr, allreduce-rdb
 */
#include "../colls_private.hpp"
#include "xbt/config.h"
/*
This fucntion performs all-reduce operation as follow.
Device network into groups (or nodes). Each group has K ranks.
1) reduce-scatter inside each group (local-ring)
2) recusive doubling
3) allgather - inside each group
*/
namespace simgrid{
namespace smpi{
	
int Coll_allreduce_ntt_lr_binominal::allreduce(void *sbuf, void *rbuf, int rcount,
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
		THROWF(arg_error,0, "allreduce ntt_lr_binominal algorithm can't be used with %d processes per a group", intra_size);  
	}

	size = comm->size(); 
	// if((size&(size-1))){
		// THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_binominal algorithm can't be used with non power of two number of processes ! ");
	// }
	
	if (size % intra_size != 0){
		THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_binominal algorithm can't be used if #process is not divisible by #process_per_group ! ");
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
		XBT_WARN("MPI_allreduce ntt_lr_binominal use default MPI_allreduce.");
		Coll_allreduce_default::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	} 

	/* When communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
/* 	if (rcount % size != 0) {
		XBT_WARN("FIX ME! MPI_allreduce ntt_lr_binominal can't support the abbitrary data size. The data size should be devisible by #process");
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

	/*2. binominal reduce -inter between groups: the same local_rank nodes*/
	/**************************************************/
	/* start binomial reduce inter-communication between each SMP nodes:
	 more than one process that can communicate to other nodes */
	if (rank ==0){XBT_WARN("[NNNN] [%d] binomial reduce inter-communication",rank);}
	int mask;
	mask = 1;
	send_offset = (intra_rank * intra_count) * extent;
	recv_offset = send_offset;
		   
	void *tmp_buf = NULL;
	tmp_buf = (void *) smpi_get_tmp_sendbuffer(intra_count * extent);
	
	while (mask < inter_size) {
	if ((mask & inter_rank) == 0) {
		src = (((inter_rank | mask)+ 1 + inter_size)%inter_size) * intra_size + intra_rank;
		if (src < size) {
			Request::recv(tmp_buf, intra_count, dtype, src, tag, comm, &status);
			if(op!=MPI_OP_NULL) op->apply( tmp_buf, (char *) rbuf + recv_offset, &intra_count, dtype);
		}
	} else {
		dst = (((inter_rank & (~mask))+ 1 + inter_size)%inter_size) * intra_size + intra_rank;
		Request::send((char *) rbuf + send_offset, intra_count, dtype, dst, tag, comm);
		break;
	}
	mask <<= 1;
	////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,count,alert));	
	}
	
	/*3. binominal bcast -inter between groups: the same local_rank nodes*/
	/**************************************************/
	/* start binomial broadcast inter-communication between each SMP nodes:
	 each node only have one process that can communicate to other nodes */
	if (rank ==0){XBT_WARN("[NNNN] [%d] binomial broadcast inter-communication",rank);}
	mask = 1;
	while (mask < inter_size) {
		if (inter_rank & mask) {
			src = (((inter_rank - mask) + 1 + inter_size)%inter_size) * intra_size + intra_rank;
			Request::recv(rbuf, intra_count, dtype, src, tag, comm, &status);
			break;
		}
		mask <<= 1;
		////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,count,alert));	
	}
	mask >>= 1;

	while (mask > 0) {
		if (inter_rank < inter_size) {
			dst = (((inter_rank + mask) + 1 + inter_size)%inter_size) * intra_size + intra_rank;
			if (dst < size) {
				Request::send(rbuf, intra_count, dtype, dst, tag, comm);
			}
		}
		mask >>= 1;
		////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,count,alert));	
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
		XBT_WARN("For MPI_allreduce ntt_lr_binominal when communication data count is not divisible by number of process, call the native implementation for the remain chunk at the end of the operation");
		return Colls::allreduce((char *) sbuf + remainder_offset,(char *) rbuf + remainder_offset, remainder, dtype, op,comm);
		//XBT_WARN("[NNNN] [%d] buf=[%s]",rank, print_buffer(rbuf,rcount,alert));
	}
    if (rank ==0){XBT_WARN("[NNNN] [%d] Finish algorithm",rank);}
	return MPI_SUCCESS;
}
}
}
