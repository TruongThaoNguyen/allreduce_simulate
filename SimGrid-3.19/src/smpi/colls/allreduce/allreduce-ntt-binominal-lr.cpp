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

int Coll_allreduce_ntt_binominal_lr::allreduce(void *sbuf, void *rbuf, int rcount,
                                           MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)
{
	XBT_WARN("[NNNN] [%d] Start function",comm->rank());
	int tag = COLL_TAG_ALLREDUCE;
	MPI_Status status;
	int size, rank, intra_count, inter_count, i;
	
 	int intra_size=1;
	intra_size = xbt_cfg_get_int("smpi/process_of_node");
	if ( intra_size <= 0){
		THROWF(arg_error,0, "allreduce ntt_binominal_lr algorithm can't be used with %d processes per a group", intra_size);  
	}

	size = comm->size(); 
	// if((size&(size-1))){
		// THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_lr algorithm can't be used with non power of two number of processes ! ");
	// }
	
	if (size % intra_size != 0){
		THROWF(arg_error,0, "FIX ME! allreduce ntt_binominal_lr algorithm can't be used if #process is not divisible by #process_per_group ! ");
	}
	
	rank = comm->rank();
	/* make it compatible with all data type */
	MPI_Aint extent;
	extent = dtype->get_extent();
	int inter_size = (size + intra_size - 1) / intra_size;
	int intra_rank = rank % intra_size; 
	int inter_rank = rank / intra_size; // nodeIdx
	XBT_DEBUG("[NNNN] node %d intra_rank = %d, inter_rank = %d\n", rank, intra_rank, inter_rank);

	/* when communication size is smaller than number of process (not support) */
	if (rcount < size) {
		XBT_WARN("MPI_allreduce ntt_lr_lr use default MPI_allreduce.");
		Coll_allreduce_default::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	} 

	XBT_WARN("[NNNN] [%d] Start algorithm",rank);
	char alert[1000];
	//XBT_WARN("[NNNN] [%d] sbuf=[%s]",rank, print_buffer(sbuf,rcount,alert));	
	/*1. reduce binominal inside each group */
	/**************************************************/
	XBT_WARN("[NNNN] [%d] binomial reduce intra communication",rank);	
	//1.1. copy send_buf to rbuf
	Request::sendrecv(sbuf, rcount, dtype, rank, tag,rbuf, rcount, dtype, rank, tag, comm, &status);
	//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
	void *tmp_buf = NULL;
	tmp_buf = (void *) smpi_get_tmp_sendbuffer(rcount * extent);
	int src, dst, mask;
	//1.2. start binomial reduce intra communication inside each SMP node 
	mask = 1;
	while (mask < intra_size) {
		if ((mask & intra_rank) == 0) {
			src = (inter_rank * intra_size) + (intra_rank | mask);
		if (src < size) {
			Request::recv(tmp_buf, rcount, dtype, src, tag, comm, &status);
			if(op!=MPI_OP_NULL) op->apply( tmp_buf, rbuf, &rcount, dtype);
		}
		} else {
			dst = (inter_rank * intra_size) + (intra_rank & (~mask));
			Request::send(rbuf, rcount, dtype, dst, tag, comm);
			break;
		}
		mask <<= 1;
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		
	}
	
	/*2. reduce-scatter -inter between groups: the same local_rank nodes*/
	/**************************************************/
	int send_offset, recv_offset;
	XBT_WARN("[NNNN] [%d] inter lr reduce-scatter",rank);
	if(intra_rank == 0){
		//2.1. copy (partial of)recv_buf to send_buf
		send_offset = ((intra_rank) % intra_size) * rcount * extent;
		recv_offset = ((intra_rank) % intra_size) * rcount * extent;
		Request::sendrecv(rbuf , rcount, dtype, rank, tag - 1,sbuf , rcount, dtype, rank, tag - 1, comm, &status);
		////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
		//2.1. reduce-scatter
		inter_count = rcount / inter_size;
		for (i = 0; i < (inter_size - 1); i++) {
			send_offset = ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count * extent;
			recv_offset = ((inter_rank - 2 - i + 2 * inter_size)%inter_size) * inter_count * extent;
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
		XBT_WARN("[NNNN] [%d] inter lr allgather",rank);
		for (i = 0; i < (inter_size - 1); i++) {
			send_offset = ((inter_rank - i + 2 * inter_size)%inter_size) * inter_count * extent;
			recv_offset = ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count * extent;
			src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
			dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
			Request::sendrecv((char *) rbuf + send_offset, inter_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, inter_count, dtype,src, tag + i, comm, &status);
			//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
		}
	}
	/*4. bcast - inside each group */
	/**************************************************/
	/* start binomial broadcast intra-communication inside each SMP nodes */
	XBT_WARN("[NNNN] [%d] binomial broadcast intra-communication",rank);
	int num_core_in_current_smp = intra_size;
	if (inter_rank == (inter_size - 1)) {
		num_core_in_current_smp = size - (inter_rank * intra_size);
	}
	mask = 1;
	while (mask < num_core_in_current_smp) {
		if (intra_rank & mask) {
			src = (inter_rank * intra_size) + (intra_rank - mask);
			Request::recv(rbuf, rcount, dtype, src, tag, comm, &status);
			//XBT_WARN("[NNNN] [%d] HEHE rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));	
			break;
		}
		mask <<= 1;
	}
	mask >>= 1;
	
	while (mask > 0) {
		dst = inter_rank * intra_size + intra_rank + mask;
		if (dst < size) {
			Request::send(rbuf, rcount, dtype, dst, tag, comm);
		}
		mask >>= 1;
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));	
	}

	smpi_free_tmp_buffer(tmp_buf);
    XBT_WARN("[NNNN] [%d] Finish algorithm",rank);
	return MPI_SUCCESS;
}
}
}
