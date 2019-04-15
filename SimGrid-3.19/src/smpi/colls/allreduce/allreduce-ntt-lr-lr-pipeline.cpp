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
int allreduce_ntt_lr_lr_pipeline_segment_number = 2;

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
	
int Coll_allreduce_ntt_lr_lr_pipeline::allreduce(void *sbuf, void *rbuf, int rcount,
                                           MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)
{
    if (comm->rank() ==0){ XBT_WARN("[NNNN] [%d] Start function",comm->rank());}
	int tag = COLL_TAG_ALLREDUCE;
	MPI_Status status, status2;
	int size, rank, intra_count, inter_count, i;
	
 	int intra_size=1;
	intra_size = xbt_cfg_get_int("smpi/process_of_node");
	if ( intra_size <= 0){
		THROWF(arg_error,0, "allreduce ntt_lr_lr_pipeline algorithm can't be used with %d processes per a group", intra_size);  
	}

	size = comm->size(); 
	// if((size&(size-1))){
		// THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_lr_pipeline algorithm can't be used with non power of two number of processes ! ");
	// }
	
	if (size % intra_size != 0){
		THROWF(arg_error,0, "FIX ME! allreduce ntt_lr_lr_pipeline algorithm can't be used if #process is not divisible by #process_per_group ! ");
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
		XBT_WARN("FIX ME! MPI_allreduce ntt_lr_lr_pipeline can't support the abbitrary data size. The data size should be devisible by #process");
	}
 */

	int phase;
	int number_segments = xbt_cfg_get_int("smpi/pipeline_segment_number"); //Number of segments (or phases)
	int segment_size = (size*number_segments); // number of items with number_segments per a rank
	/* when communication size is smaller than segment_size, can not use pipeline*/	
	if (rcount < segment_size) {
		XBT_WARN("MPI_allreduce ntt_lr_lr_pipeline use MPI_allreduce ntt_lr_lr.");
 		Coll_allreduce_ntt_lr_lr::allreduce(sbuf, rbuf, rcount, dtype, op, comm);
		return MPI_SUCCESS;
	}	
	
	int remainder, remainder_flag, remainder_offset;
	if (rcount % segment_size != 0) {
		remainder = rcount % segment_size;
		remainder_flag = 1;
		remainder_offset = (rcount / segment_size) * segment_size * extent;
	} else {
		remainder = remainder_flag = remainder_offset = 0;
	}  
	int segment_count = (rcount/ segment_size) * size;  // number of items per segment
	XBT_DEBUG("[NNNN] [%d] segment_count %d, segment_size %d, ",rank, segment_count,segment_size);	
	int intra_segment_count = segment_count / intra_size;
	int inter_segment_count = intra_segment_count / inter_size;
	intra_count = intra_segment_count * number_segments;
	inter_count = inter_segment_count * number_segments;

	int step;	
	MPI_Request *phase3_rrequest_array, *phase3_srequest_array, *phase4_rrequest_array, *phase4_srequest_array;
	phase4_rrequest_array = (MPI_Request *) xbt_malloc((intra_size-1) * sizeof(MPI_Request));
	phase4_srequest_array = (MPI_Request *) xbt_malloc((intra_size-1) * sizeof(MPI_Request));
	phase3_rrequest_array = (MPI_Request *) xbt_malloc((inter_size-1) * sizeof(MPI_Request));
	phase3_srequest_array = (MPI_Request *) xbt_malloc((inter_size-1) * sizeof(MPI_Request));
	
	MPI_Status *phase3_rstatus_array, *phase3_sstatus_array,*phase4_rstatus_array, *phase4_sstatus_array;
	phase3_rstatus_array = (MPI_Status *) xbt_malloc((inter_size-1) * sizeof(MPI_Status));
	phase3_sstatus_array = (MPI_Status *) xbt_malloc((inter_size-1) * sizeof(MPI_Status));
	phase4_rstatus_array = (MPI_Status *) xbt_malloc((intra_size-1) * sizeof(MPI_Status));
	phase4_sstatus_array = (MPI_Status *) xbt_malloc((intra_size-1) * sizeof(MPI_Status));
	
	/* int ring_next_node_4[] = {2,3,1,0}; // ring: 0-2-1-3-0
	int ring_prev_node_4[] = {3,2,0,1}; // ring: 0-2-1-3-0
	// standard ring: 0-1-2-7-4-5-6-3-0
	// node_id to rank: 0-0, 1-1, 2-2, 7-3,4-4, 5-5, 6-6, 3-7
	// new ring in node_id: 0-2-3-1-4-6-7-5-0. ==> new ring in rank: 0-2-7-1-4-6-3-5-0
	int ring_next_node_8[] = {2,4,7,1,6,0,3,5}; 
	int ring_prev_node_8[] = {5,7,0,6,1,3,4,2};  */
	
	XBT_DEBUG("[NNNN] [%d] intra_segment_count %d, inter_segment_count %d,intra_count %d, inter_count %d",rank, intra_segment_count, inter_segment_count,intra_count,inter_count);
	//XBT_WARN("[NNNN] [%d] Start algorithm",rank);
	/* pipelining over number_segments (+1 is because we have 2 big_phases*/
	XBT_DEBUG("[NNNN] [%d] number_segments %d, rcount %d",rank, number_segments, rcount);
	for (step = 0; step < number_segments + 1; step++) {
		int send_offset, recv_offset;
		int src, dst;
		if (rank ==0){XBT_WARN("[NNNN] [%d] Step %d",rank, step);}
		if (step < number_segments) {
			//char alert[1000];
			//XBT_WARN("[NNNN] [%d] sbuf=[%s]",rank, print_buffer(sbuf,rcount,alert));	
			/*1. reduce-scatter inside each group (local-ring)*/
			/**************************************************/
			if (step > 0){
				if (rank ==0){XBT_DEBUG("[NNNN] [%d] inter lr allgather for step %d",rank,step -1);}
				// phase 3 of step-1
				for (i = 0; i < (inter_size - 1); i++) {	
					recv_offset = ((intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) + (step-1) * inter_segment_count) * extent;
					src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
					send_offset = ((intra_rank * intra_count  + ((inter_rank - i + 2 * inter_size)%inter_size) * inter_count) + (step-1) * inter_segment_count) * extent;
					dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
					
					phase3_rrequest_array[i] = Request::irecv((char *) rbuf + recv_offset, inter_segment_count, dtype,src, 30000 + tag + i * inter_size + step - 1, comm);
					//XBT_WARN("[NNNN] [%d] phase 3 - irecv for step %d",rank, step-1);
					phase3_srequest_array[i] = Request::isend((char *) rbuf + send_offset, inter_segment_count, dtype, dst, 30000 + tag + i * inter_size + step - 1, comm);
					//XBT_WARN("[NNNN] [%d] phase 3 - isend for step %d",rank, step-1);
				}
			}
			
			//1.1. copy (partial of)send_buf to recv_buf
			//XBT_WARN("[NNNN] [%d] intra lr reduce-scatter",rank);
			send_offset = (((intra_rank - 1 + intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
			recv_offset = (((intra_rank - 1 + intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
			Request::sendrecv((char *) sbuf + send_offset, intra_segment_count, dtype, rank, tag - 1,
					   (char *) rbuf + recv_offset, intra_segment_count, dtype, rank, tag - 1, comm, &status);
			//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   

			//1.2. reduce-scatter
			if (rank ==0){XBT_DEBUG("[NNNN] [%d] intra lr reduce-scatter for step %d",rank, step);}
			for (i = 0; i < (intra_size - 1); i++) {
				send_offset = (((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
				recv_offset = (((intra_rank - 2 - i + 2 * intra_size) % intra_size) * intra_count + step * intra_segment_count)* extent;
				src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
				dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
				Request::sendrecv((char *) rbuf + send_offset, intra_segment_count, dtype, dst, tag + i * intra_size + step , 
				(char *) rbuf + recv_offset, intra_segment_count, dtype,src, tag + i* intra_size + step, comm, &status);

				// compute result to rbuf+recv_offset
				if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
							   &intra_segment_count, dtype);
			}
			//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
			
			if (step > 0){
				// wait for phase-3 here
				if (rank ==0){XBT_DEBUG("[NNNN] [%d] Wait phase 3 for step %d",rank, step-1);}
				Request::waitall(inter_size - 1, phase3_rrequest_array, phase3_rstatus_array);
				Request::waitall(inter_size - 1, phase3_srequest_array, phase3_sstatus_array);			   
			}
			/*2. reduce-scatter -inter between groups: the same local_rank nodes*/
			/**************************************************/
			if (rank ==0){XBT_WARN("[NNNN] [%d] inter lr reduce-scatter",rank);}
			//2.1. copy (partial of)recv_buf to send_buf
			send_offset = (((intra_rank) % intra_size) * intra_count + (step) * intra_segment_count) * extent;
			recv_offset = (((intra_rank) % intra_size) * intra_count + (step) * intra_segment_count) * extent;
			Request::sendrecv((char *) rbuf + send_offset, intra_segment_count, dtype, rank, tag - 1,
					   (char *) sbuf + recv_offset, intra_segment_count, dtype, rank, tag - 1, comm,
					   &status);
			////XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));
			//2.1. reduce-scatter
			// phase 4 of step-1
			if (step > 0){
				if (rank ==0){XBT_DEBUG("[NNNN] [%d] intra lr allgather for step",rank, step -1);}
				for (i = 0; i < (intra_size - 1); i++) {
					recv_offset = (((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count + (step -1) * intra_segment_count)  * extent;
					src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
					send_offset = (((intra_rank - i + 2 * intra_size) % intra_size) * intra_count + (step - 1) * intra_segment_count) * extent;
					dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
					
					phase4_rrequest_array[i] = Request::irecv((char *) rbuf + recv_offset, intra_segment_count, dtype,src, 40000 + tag + i * intra_size + step - 1, comm);
					//XBT_WARN("[NNNN] [%d] phase 4 - irecv for step %d",rank, step-1);
					phase4_srequest_array[i] = Request::isend((char *) rbuf + send_offset, intra_segment_count, dtype, dst, 40000 + tag + i * intra_size + step - 1, comm);
					//XBT_WARN("[NNNN] [%d] phase 4 - isend for step %d",rank, step-1);
				}
			}
			
			for (i = 0; i < (inter_size - 1); i++) {
				if (rank ==0){XBT_DEBUG("[NNNN] [%d] Phase 2-Communication: %d, %d",rank,step, i);}
				send_offset = ((intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) + (step) * inter_segment_count) * extent;
				recv_offset = ((intra_rank * intra_count  + ((inter_rank - 2 - i + 2 * inter_size)%inter_size) * inter_count) + (step) * inter_segment_count) * extent;
				src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
				dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
				Request::sendrecv((char *) rbuf + send_offset, inter_segment_count, dtype, dst, tag + i * inter_size + step, 
				(char *) rbuf + recv_offset, inter_segment_count, dtype,src, tag + i * inter_size + step, comm, &status);
				if (rank ==0){XBT_DEBUG("[NNNN] [%d] Phase 2-Computation: %d, %d",rank,step, i);}
				// compute result to rbuf+recv_offset
				if(op!=MPI_OP_NULL) op->apply( (char *) sbuf + recv_offset, (char *) rbuf + recv_offset,
							   &inter_segment_count, dtype);
				//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   			   
			}

			if (step > 0){
				// wait for phase-4 here
				if (rank ==0){XBT_DEBUG("[NNNN] [%d] Wait  phase 4 for step %d",rank, step-1);}
				Request::waitall(intra_size - 1, phase4_rrequest_array, phase4_rstatus_array);
				Request::waitall(intra_size - 1, phase4_srequest_array, phase4_sstatus_array);			   
			}
			/*3. allgather - inter between root of each SMP node*/
			/**************************************************/
			/*4. allgather - inside each group */
			/**************************************************/
		} else {
			if (rank ==0){XBT_WARN("[NNNN] [%d] Last inter lr allgather at step %d",rank, step);}
			for (i = 0; i < (inter_size - 1); i++) {
				send_offset = ((intra_rank * intra_count  + ((inter_rank - i + 2 * inter_size)%inter_size) * inter_count) + (step-1) * inter_segment_count) * extent;
				recv_offset = ((intra_rank * intra_count  + ((inter_rank - 1 - i + 2 * inter_size)%inter_size) * inter_count) + (step-1) * inter_segment_count) * extent;
				src = intra_rank + ((inter_rank + inter_size - 1)% inter_size)* intra_size;
				dst = intra_rank + ((inter_rank + 1)% inter_size)* intra_size;
				Request::sendrecv((char *) rbuf + send_offset, inter_segment_count, dtype, dst, 30000 + tag + i * inter_size + step - 1, 
				(char *) rbuf + recv_offset, inter_segment_count, dtype,src,  30000 + tag + i * inter_size + step - 1, comm, &status);
				//XBT_WARN("[NNNN] [%d] rbuf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   	
			}
			
			if (rank ==0){XBT_WARN("[NNNN] [%d] Last intra lr allgather at step %d",rank, step);}
			for (i = 0; i < (intra_size - 1); i++) {
				send_offset = (((intra_rank - i + 2 * intra_size) % intra_size) * intra_count + (step -1) * intra_segment_count)* extent;
				recv_offset = (((intra_rank - 1 - i + 2 * intra_size) % intra_size) * intra_count + (step -1) * intra_segment_count)* extent;
				src = ((intra_rank + intra_size - 1) % intra_size) + inter_rank * intra_size;
				dst = ((intra_rank + 1) % intra_size) + inter_rank * intra_size;
				Request::sendrecv((char *) rbuf + send_offset, intra_segment_count, dtype, dst, 40000 + tag + i* intra_size + step-1, 
				(char *) rbuf + recv_offset, intra_segment_count, dtype,src, 40000 + tag + i* intra_size + step-1, comm, &status);
				//XBT_WARN("[NNNN] [%d] Last sendrecv for step %d",rank, step-1);
				
			}
		}
	}
	
	/* when communication size is not divisible by number of process:
	 call the native implementation for the remain chunk at the end of the operation */
	if (remainder_flag) {
		//XBT_WARN("[NNNN] [%d] remainder path",rank);
		XBT_WARN("For MPI_allreduce ntt_lr_lr_pipeline when communication data count is not divisible by number of process, call the native implementation for the remain chunk at the end of the operation");
		return Colls::allreduce((char *) sbuf + remainder_offset,(char *) rbuf + remainder_offset, remainder, dtype, op,comm);
		//XBT_WARN("[NNNN] [%d] buf=[%s]",rank, print_buffer(rbuf,rcount,alert));		   
	}
	free(phase3_rrequest_array);
	free(phase3_srequest_array);
	free(phase3_rstatus_array);
	free(phase3_sstatus_array);	
	
	free(phase4_rrequest_array);
	free(phase4_srequest_array);
	free(phase4_rstatus_array);
	free(phase4_sstatus_array);
	
    if (rank ==0){XBT_WARN("[NNNN] [%d] Finish algorithm",rank);}	
	return MPI_SUCCESS;
}
}
}