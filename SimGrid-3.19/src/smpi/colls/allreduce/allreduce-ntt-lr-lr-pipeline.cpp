/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * implemented by NguyenTT, 06/12/2018
 * logic of code copy from allreduce-ntt-lr-lr.cpp
 */
#include "../colls_private.hpp"
#include "xbt/config.h"

/* this is a default segment size for pipelining,
   but it is typically passed as a command line argument */
int allreduce_ntt_lr_lr_pipeline_segment_size = 1024;

/*
This fucntion performs all-reduce operation as follow.
Devide network into groups (or nodes). Each group has K ranks.
1) reduce-scatter inside each group (local-ring)
2) reduce-scatter -inter between groups: the same local_rank nodes
3) allgather - inter between root of each SMP node
4)allgather - inside each group

Pipeline approaches: devide data into segments with fixed size.
Each of 4 above steps will send and receive 1 segment (so that can be pipeline?), 

*/
namespace simgrid{
namespace smpi{
	
int Coll_allreduce_ntt_lr_lr_pipeline_nb::allreduce(void *sbuf, void *rbuf, int rcount,
                                           MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)
{
	XBT_WARN("[NNNN] [%d] Start function",comm->rank());
	int tag = COLL_TAG_ALLREDUCE;
	MPI_Status status, status2;
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
		XBT_WARN("MPI_allreduce ntt_lr_lr_pipeline use default MPI_allreduce.");
		Coll_allreduce_default::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	} 

	/* When communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
/* 	if (rcount % size != 0) {
		XBT_WARN("FIX ME! MPI_allreduce ntt_lr_lr can't support the abbitrary data size. The data size should be devisible by #process");
	}
 */

	int phase;
	int segment_size = xbt_cfg_get_int("smpi/pipeline_segment_size");
	if (segment_size < 0) { segment_size = allreduce_ntt_lr_lr_pipeline_nb_segment_size;}
	int inter_segment_count = segment_size / extent; // #items per segment. Assume that segemnt size is devided by extent (e.g, 4bytes)
		
	/* when communication size is smaller than segment_size, can not use pipeline*/	
	if (inter_segment_count > rcount) {
		XBT_WARN("MPI_allreduce ntt_lr_lr_pipeline use MPI_allreduce ntt_lr_lr.");
 		Coll_allreduce_ntt_lr_lr::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	}

	int pcount = (size*inter_segment_count);
	int remainder, remainder_flag, remainder_offset;
	if (rcount % pcount != 0) {
		remainder = rcount % pcount;
		remainder_flag = 1;
		remainder_offset = (rcount / pcount) * pcount * extent;
	} else {
		remainder = remainder_flag = remainder_offset = 0;
	}  
	
	/* compute pipe length */
	int pipelength; // lenth of pipeline or #of phases.
	pipelength = rcount / pcount;
	int step;	

	// size of each point-to-point communication is equal to the size of the whole message divided by number of processes
	inter_count = (rcount / pcount) * inter_segment_count; 
	intra_count = inter_count * inter_size;
	int intra_segment_count = inter_segment_count * inter_size;

	MPI_Request *phase1_rrequest_array, *phase1_srequest_array, *phase4_rrequest_array, *phase4_srequest_array;
	//MPI_Status *intra_status_array, *intra1_status_array;
	phase4_rrequest_array = (MPI_Request *) xbt_malloc(intra_size * sizeof(MPI_Request));
	phase4_srequest_array = (MPI_Request *) xbt_malloc(intra_size * sizeof(MPI_Request));
	//intra_status_array = (MPI_Status *) xbt_malloc(intra_size * sizeof(MPI_Status));
	//intra1_status_array = (MPI_Status *) xbt_malloc(intra_size * sizeof(MPI_Status));
	
	XBT_WARN("[NNNN] [%d] Start algorithm",rank);
	/* pipelining over pipelength (+1 is because we have 2 big_phases*/
	XBT_WARN("[NNNN] [%d] pipelength %d",rank, pipelength);
	for (step = 0; step < pipelength + 1; step++) {
		int send_offset, recv_offset, send_offset1;
		int src, dst;
		//XBT_WARN("[NNNN] [%d] Step %d",rank, step);

			//char alert[1000];
			//XBT_WARN("[NNNN] [%d] sbuf=[%s]",rank, print_buffer(sbuf,rcount,alert));	
			/*1. reduce-scatter inside each group (local-ring)*/
			/**************************************************/
			//1.1. copy (partial of)send_buf to recv_buf
			//XBT_WARN("[NNNN] [%d] intra lr reduce-scatter",rank);
			send_offset = (((intra_rank - 1 + intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
			recv_offset = (((intra_rank - 1 + intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
			Request::sendrecv((char *) sbuf + send_offset, intra_segment_count, dtype, rank, tag - 1,
					   (char *) rbuf + recv_offset, intra_segment_count, dtype, rank, tag - 1, comm,
					   &status);
			//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   
			//1.2. reduce-scatter
			for (i = 0; i < (intra_size - 1); i++) {
				if (step > 0){
					send_offset1 = (((intra_rank - i + 2 * intra_size) % intra_size) * intra_count + (step - 1) * intra_segment_count) * extent;
					dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
					phase4_srequest_array[i] = Request::isend((char *) rbuf + send_offset1, intra_segment_count, dtype, dst, 40000 + tag + i, comm);
				}
				
				if (step < pipelength) {
					send_offset = (((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
					recv_offset = (((intra_rank - 2 - i + 2 * intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
					src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
					dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
					Request::sendrecv((char *) rbuf + send_offset, intra_segment_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, intra_segment_count, dtype,src, tag + i, comm, &status);

					// compute result to rbuf+recv_offset
					if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
								   &intra_segment_count, dtype);
				}
				
				if (step > 0){
					//XBT_WARN("[NNNN] [%d] Wait for step %d",rank, step);
					Request::wait(&phase4_rrequest_array[i], &status);
					Request::wait(&phase4_srequest_array[i], &status2);			   
				}
			   //XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   			   
			}
		if (step < pipelength) {
			/*2. reduce-scatter -inter between groups: the same local_rank nodes*/
			/**************************************************/
			//XBT_WARN("[NNNN] [%d] inter lr reduce-scatter",rank);
			//2.1. copy (partial of)recv_buf to send_buf
			//XBT_WARN("[NNNN] [%d] intra lr reduce-scatter",rank);
			send_offset = (((intra_rank) % intra_size) * intra_count + (step) * inter_segment_count) * extent;
			recv_offset = (((intra_rank) % intra_size) * intra_count + (step) * inter_segment_count) * extent;
			Request::sendrecv((char *) rbuf + send_offset, inter_segment_count, dtype, rank, tag - 1,
					   (char *) sbuf + recv_offset, inter_segment_count, dtype, rank, tag - 1, comm,
					   &status);
			////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
			//2.1. reduce-scatter
			for (i = 0; i < (inter_size - 1); i++) {
				send_offset = ((intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) + (step) * inter_segment_count) * extent;
				recv_offset = ((intra_rank * intra_count  + ((inter_rank - 2 - i + 2 * inter_size)%inter_size) * inter_count) + (step) * inter_segment_count) * extent;
				src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
				dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
				Request::sendrecv((char *) rbuf + send_offset, inter_segment_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, inter_segment_count, dtype,src, tag + i, comm, &status);

				// compute result to rbuf+recv_offset
				if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
							   &inter_segment_count, dtype);
				//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   			   
			}

			/*3. allgather - inter between root of each SMP node*/
			/**************************************************/
			//XBT_WARN("[NNNN] [%d] inter lr allgather",rank);
			for (i = 0; i < (inter_size - 1); i++) {
				send_offset = ((intra_rank * intra_count  + ((inter_rank - i + 2 * inter_size)%inter_size) * inter_count) + (step) * inter_segment_count) * extent;
				recv_offset = ((intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) + (step) * inter_segment_count) * extent;
				src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
				dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
				Request::sendrecv((char *) rbuf + send_offset, inter_segment_count, dtype, dst, tag + i, (char *) rbuf + recv_offset, inter_segment_count, dtype,src, tag + i, comm, &status);
				//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
			}
	
			/*4. allgather - inside each group */
			/**************************************************/
			//XBT_WARN("[NNNN] [%d] intra lr allgather",rank);
			//if (step > 0){
				for (i = 0; i < (intra_size - 1); i++) {
					recv_offset = (((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count + (step) * intra_segment_count)  * extent;
					src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
					phase4_rrequest_array[i] = Request::irecv((char *) rbuf + recv_offset, intra_segment_count, dtype,src, 40000 + tag + i, comm);
				}
			//}
			/* 
			//4.2. Send data
			for (i = 0; i < (intra_size - 1); i++) {
				send_offset = (((intra_rank - i + 2 * intra_size) % intra_size) * intra_count + (step - 1) * intra_segment_count) * extent;
				dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
				phase4_srequest_array[i] = Request::isend((char *) rbuf + send_offset, intra_segment_count, dtype, dst, tag + i, comm);
				//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
			} */
		}/*  else {
			XBT_WARN("[NNNN] [%d] Last wait at step %d",rank, step);
			for (i = 0; i < (intra_size - 1); i++) {
				Request::wait(&phase4_rrequest_array[i], &status);
				Request::wait(&phase4_srequest_array[i], &status2);
			}
		} */
	}
	
	/* when communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
	if (remainder_flag) {
		//XBT_WARN("[NNNN] [%d] remainder path",rank);
		XBT_WARN("For MPI_allreduce ntt_lr_lr when communication data count is not divisible by number of process, call the native implementation for the remain chunk at the end of the operation");
		return Colls::allreduce((char *) sbuf + remainder_offset,(char *) rbuf + remainder_offset, remainder, dtype, op,comm);
		//XBT_WARN("[NNNN] [%d] buf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   
	}
	free(phase4_rrequest_array);
	free(phase4_srequest_array);
	
    XBT_WARN("[NNNN] [%d] Finish algorithm",rank);	
	return MPI_SUCCESS;
}
}
}
