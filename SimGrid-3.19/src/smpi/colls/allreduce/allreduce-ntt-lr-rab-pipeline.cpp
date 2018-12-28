/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * implemented by NguyenTT, 06/12/2018
 * a apart of code clone from allreduce-smp-rsag-rab.cpp
 */
#include "../colls_private.hpp"
#include "xbt/config.h"

/* this is a default segment size for pipelining,
   but it is typically passed as a command line argument */
int allreduce_ntt_lr_rab_pipeline_segment_size = 1024;


/*
This fucntion performs all-reduce operation as follow.
Device network into groups (or nodes). Each group has K ranks.
1) reduce-scatter inside each group (local-ring)
2) reduce-scatter -inter between groups: the same local_rank nodes (using doubling)
3) allgather - inter between root of each SMP node (using halving)
4) allgather - inside each group
*/
namespace simgrid{
namespace smpi{

int Coll_allreduce_ntt_lr_rab_pipeline::allreduce(void *sbuf, void *rbuf, int rcount,
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
		THROWF(arg_error,0, "allreduce ntt_lr_rab algorithm can't be used with %d processes per a group", intra_size);  
	}

	size = comm->size(); 
	// if((size&(size-1))){
		// THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_rab algorithm can't be used with non power of two number of processes ! ");
	// }
	
	if (size % intra_size != 0){
		THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_rab algorithm can't be used if #process is not divisible by #process_per_group ! ");
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
		XBT_WARN("MPI_allreduce ntt_lr_rab use default MPI_allreduce.");
		Coll_allreduce_default::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	} 

	/* When communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
/* 	if (rcount % size != 0) {
		XBT_WARN("FIX ME! MPI_allreduce ntt_lr_rab can't support the abbitrary data size. The data size should be devisible by #process");
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
	XBT_WARN("[NNNN] [%d] Start algorithm",rank);
	char alert[1000];
	//XBT_WARN("[NNNN] [%d] sbuf=[%s]",rank, print_buffer(sbuf,rcount,alert));	
	/*1. reduce-scatter inside each group (local-ring)*/
	/**************************************************/
	//1.1. copy (partial of)send_buf to recv_buf
	XBT_WARN("[NNNN] [%d] intra lr reduce-scatter",rank);
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
	XBT_WARN("[NNNN] [%d] inter reduce-scatter",rank);
	
	// find nearest power-of-two less than or equal to comm_size
	void *tmp_buf = NULL;
	tmp_buf = (void *) smpi_get_tmp_sendbuffer(intra_count * extent);
	
	int pof2, inter_remainder, mask, new_inter_rank, new_dst, inter_dst;
	pof2 = 1;
	while (pof2 <= inter_size)
		pof2 <<= 1;
	pof2 >>= 1;
	inter_remainder = inter_size - pof2;
	
	// In the non-power-of-two case, all even-numbered
	// processes of rank < 2*rem send their data to
	// (rank+1). These even-numbered processes no longer
	// participate in the algorithm until the very end. The
	// remaining processes form a nice power-of-two.
	send_offset = (intra_rank * intra_count) * extent;
	recv_offset = send_offset;
	if (inter_rank < 2 * inter_remainder) {
		// even
		if (inter_rank % 2 == 0) {
			dst = intra_rank + ((inter_rank + 1 + inter_size)%inter_size)*intra_size;
			Request::send((char *) rbuf + send_offset, intra_count, dtype, dst, tag, comm);

			// temporarily set the rank to -1 so that this
			// process does not pariticipate in recursive
			// doubling
			new_inter_rank = -1;
		} 
		else { // odd
			src = intra_rank + ((inter_rank - 1 + inter_size)%inter_size) *intra_size;
			Request::recv(tmp_buf, intra_count, dtype, src, tag, comm, &status);
			// do the reduction on received data. since the
			// ordering is right, it doesn't matter whether
			// the operation is commutative or not.
			if(op!=MPI_OP_NULL) op->apply( tmp_buf, (char *) rbuf + recv_offset, &intra_count, dtype);

			// change the rank
			new_inter_rank = inter_rank / 2;
		}
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
	}
	else {                         // inter_rank >= 2 * inter_remainder
		new_inter_rank = inter_rank - inter_remainder;	
	}
	
	// If op is user-defined or count is less than pof2, use
	// recursive doubling algorithm. Otherwise do a reduce-scatter
	// followed by allgather. (If op is user-defined,
	// derived datatypes are allowed and the user could pass basic
	// datatypes on one process and derived on another as long as
	// the type maps are the same. Breaking up derived
	// datatypes to do the reduce-scatter is tricky, therefore
	// using recursive doubling in that case.)
	int *cnts, *disps;
	int send_idx, recv_idx, send_cnt, recv_cnt, last_idx;
	if (new_inter_rank != -1) {
		// do a reduce-scatter followed by allgather. for the
		// reduce-scatter, calculate the count that each process receives
		// and the displacement within the buffer
		cnts = (int *) xbt_malloc(pof2 * sizeof(int));
		disps = (int *) xbt_malloc(pof2 * sizeof(int));
		
		// #of data at each step
		for (i = 0; i < (pof2 - 1); i++)
			cnts[i] = intra_count / pof2;
		cnts[pof2 - 1] = intra_count - (intra_count / pof2) * (pof2 - 1);
		
		disps[0] = 0;
		for (i = 1; i < pof2; i++)
			disps[i] = disps[i - 1] + cnts[i - 1];
		
		mask = 0x1;
		send_idx = recv_idx = 0;
		last_idx = pof2;
		
		while (mask < pof2) {
			new_dst = new_inter_rank ^ mask;
			// find real rank of dest
			inter_dst = (new_dst < inter_remainder) ? new_dst * 2 + 1 : new_dst + inter_remainder;
			dst = intra_rank + ((inter_dst + inter_size)%inter_size)*intra_size;
			
			send_cnt = recv_cnt = 0;
			if (new_inter_rank < new_dst) {
				send_idx = recv_idx + pof2 / (mask * 2);
				for (i = send_idx; i < last_idx; i++)
					send_cnt += cnts[i];
				for (i = recv_idx; i < send_idx; i++)
					recv_cnt += cnts[i];
			} else {
				recv_idx = send_idx + pof2 / (mask * 2);
				for (i = send_idx; i < recv_idx; i++)
					send_cnt += cnts[i];
				for (i = recv_idx; i < last_idx; i++)
					recv_cnt += cnts[i];
			}
			// Send data from recvbuf. Recv into tmp_buf
			Request::sendrecv((char *) rbuf + send_offset + disps[send_idx] * extent, send_cnt, dtype, dst, tag,
					   (char *) tmp_buf + disps[recv_idx] * extent, recv_cnt, dtype, dst, tag, comm, &status);

			// tmp_buf contains data received in this step.
			// recvbuf contains data accumulated so far

			// This algorithm is used only for predefined ops
			// and predefined ops are always commutative.
			if(op!=MPI_OP_NULL) op->apply( (char *) tmp_buf + disps[recv_idx] * extent, (char *) rbuf + send_offset + disps[recv_idx] * extent, &recv_cnt, dtype);
			
			// update send_idx for next iteration
			send_idx = recv_idx;
			mask <<= 1;
			
			// update last_idx, but not in last iteration because the value
			// is needed in the allgather step below.
			if (mask < pof2){
				last_idx = recv_idx + pof2 / mask;
			}
			//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
		}
	
		// now do the allgather
		/*3. allgather - inter between root of each SMP node*/
		/**************************************************/
		XBT_WARN("[NNNN] [%d] inter all-gather",rank);
		mask >>= 1;
		while (mask > 0) {
			new_dst = new_inter_rank ^ mask;
			// find real rank of dest
			inter_dst = (new_dst < inter_remainder) ? new_dst * 2 + 1 : new_dst + inter_remainder;
			dst = intra_rank + ((inter_dst + inter_size)%inter_size)*intra_size;
			
			send_cnt = recv_cnt = 0;
			if (new_inter_rank < new_dst) {
				// update last_idx except on first iteration
				if (mask != pof2 / 2)
				  last_idx = last_idx + pof2 / (mask * 2);

				recv_idx = send_idx + pof2 / (mask * 2);
				for (i = send_idx; i < recv_idx; i++)
					send_cnt += cnts[i];
				for (i = recv_idx; i < last_idx; i++)
					recv_cnt += cnts[i];
			} else {
				recv_idx = send_idx - pof2 / (mask * 2);
				for (i = send_idx; i < last_idx; i++)
					send_cnt += cnts[i];
				for (i = recv_idx; i < send_idx; i++)
					recv_cnt += cnts[i];
			}
			
			Request::sendrecv((char *) rbuf + send_offset + disps[send_idx] * extent, send_cnt, dtype, dst, tag,
                   (char *) rbuf + send_offset + disps[recv_idx] * extent, recv_cnt, dtype, dst, tag, comm, &status);

		if (new_inter_rank > new_dst)
			send_idx = recv_idx;

			mask >>= 1;
			//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
		}
		
		free(cnts);
		free(disps);
	}	
	// In the non-power-of-two case, all odd-numbered processes of
	// rank < 2 * rem send the result to (rank-1), the ranks who didn't
	// participate above.

	if (inter_rank < 2 * inter_remainder) {
		if (inter_rank % 2){               // odd
			dst = intra_rank + ((inter_rank - 1 + inter_size)%inter_size)*intra_size;
			Request::send((char *) rbuf + send_offset, intra_count, dtype, dst, tag, comm);
		}
		else{                        // even
			src = intra_rank + ((inter_rank + 1 + inter_size)%inter_size)*intra_size;
			Request::recv((char *) rbuf + recv_offset, intra_count, dtype, src, tag, comm, &status);
		}
		//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
	}

	smpi_free_tmp_buffer(tmp_buf);
	
	/*4. allgather - inside each group */
	/**************************************************/
	XBT_WARN("[NNNN] [%d] intra lr allgather",rank);
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
		XBT_WARN("For MPI_allreduce ntt_lr_rab when communication data count is not divisible by number of process, call the native implementation for the remain chunk at the end of the operation");
		return Colls::allreduce((char *) sbuf + remainder_offset,
						 (char *) rbuf + remainder_offset, remainder, dtype, op,
						 comm);
		//XBT_WARN("[NNNN] [%d] buf=[%s]",rank, print_buffer(rbuf,rcount,alert));				 
	}
	XBT_WARN("[NNNN] [%d] Finish algorithm",rank);	   
	return MPI_SUCCESS;
}
}
}
