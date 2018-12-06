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
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
/*
This fucntion performs all-reduce operation as follow.
1) Threshold sparification
2) reduce
3) Unquantized and sum
4) Bcast
*/
namespace simgrid{
namespace smpi{
	
int Coll_allreduce_ntt_thres_redbcast::allreduce(void *sbuf, void *rbuf, int rcount,
                                           MPI_Datatype dtype, MPI_Op op,
                                           MPI_Comm comm)
{
	XBT_WARN("[NNNN] [%d] Start function",comm->rank());
	int tag = COLL_TAG_ALLREDUCE;
	MPI_Status status;
	int size, rank, i;

	size = comm->size(); 
	rank = comm->rank();
	/* make it compatible with all data type */
	MPI_Aint extent;
	extent = dtype->get_extent();
	
	if ( dtype != MPI_FLOAT){
		THROWF(arg_error,0, "allreduce ntt_thres_redbcast algorithm can't be used with datatype rather than MPI_FLOAT");  
	}

	// 1. Quantize the data using threshold: only send a fixed propotion value of data
	// val+ = data > T+;
	float threshold_positive = 0.01;
	float threshold_negative = -0.01;
	int* data_numbers = malloc(sizeof(int) * 1); 
	int* comm_data_numbers = malloc(sizeof(int) * size); //Buffer for number of communication data...
	
	// 1.1. Distribute number of communicate
	XBT_WARN("[NNNN] [%d] Start encode",rank);
	data_numbers[0] = 0;
	int item_offset;
	for (i = 0; i < rcount; i++){
		item_offset = extent * i;
		void * item_void_pointer= (void *) ((char *) sbuf + item_offset);
		float item_i = *(float *)&item_void_pointer;
		if ( item_i > threshold_positive){
			data_numbers[0]++;
		}
		else if (item_i < threshold_negative){
			data_numbers[0]++;
		}
	}		
	// for (i = 0; i < size; i++){ 
		// comm_data_numbers[i] = 0;
	// }
	
	// comm_data_numbers[rank] = data_numbers[0];
	// Coll_allgather_default::allgather(data_numbers,1,MPI_INT,comm_data_numbers,1,MPI_INT,MPI_COMM_WORLD);
	// //printf("rank%d: comm_data_numbers=%d, data_numbers=%d\n", rank, comm_data_numbers[rank],data_numbers[0]);
	// //4.2. Malloc data for compressed data
	// int max_reiv_buffer_size = data_numbers[0];
	// for (i = 0; i < size; i++){
		// if (rank == 0){
			// printf("[%d]Rank %d send %d values over %d (%f)\n",rank,i,comm_data_numbers[i],N,((float)comm_data_numbers[i])/N);
		// }
		// if(max_reiv_buffer_size < comm_data_numbers[i]){
			// max_reiv_buffer_size = comm_data_numbers[i];
		// }			
	// }
	// int * local_buffer = malloc(sizeof(unsigned int) * data_numbers[0]);
	// int * global_buffer = malloc(sizeof(unsigned int) * max_reiv_buffer_size);
	
	// // 4.3. Compress the data
	// //TODO: What happends if number of values N > 2^31 
	// int next_compress_value = 0;
	// for (i = 0; i < rcount; i++){
		// if (rank == 0){
			// if (sbuf[i] > threshold_positive){
				// global_sum[i] = threshold_positive;
			// }
			// else if(sbuf[i] < threshold_negative){
				// global_sum[i] = threshold_negative;
			// }
			// else{
				// global_sum[i] = 0;
			// }
		// }
		// else{
			// if (sbuf[i] > threshold_positive || sbuf[i] < threshold_negative){
				// local_buffer[next_compress_value] = (unsigned int) i;
				// if (sbuf[i] > threshold_positive){local_buffer[next_compress_value] &= ~(1 << 31);}
				// else {local_buffer[next_compress_value] |= (1 << 31);}
				// next_compress_value = next_compress_value + 1;
			// }
		// }
	// }	
	// // MPI_Barrier( MPI_COMM_WORLD);
	// // printf("Rank:%d  Start communication\n",rank);
	// //5. Communication Reduce (pair-wise) + Bcast (pair-wise)
	// // 5.1 Copy the root value first
	// // 5.2 Reduce (pair-wise)
	// //TYPE: MPI_DOUBLE_PRECISION, MPI_REAL, MPI_INT, MPI_BYTE

	// if (rank == 0){
		// for (i = 1; i < size; i++){
			// //i = 1;
			// //Root
			// MPI_Recv(global_buffer,comm_data_numbers[i],MPI_INT,i,MPI_REDUCE_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			// // printf("Recv from %d",1);
			
			// // Unquantized and compute the value
				// int j =0;
				// int position = 0;
				// for(j = 0; j < comm_data_numbers[i]; j++){
					// position = global_buffer[j];
					// position &= ~(1 << 31);
					// if (CHECK_BIT(global_buffer[j],31)){
						// global_sum[position] += threshold_positive;
					// }
					// else
					// {
						// global_sum[position] += threshold_negative;
					// }
				// }
				// // printf("Finish compute from %d\n",i);
		// }
	// }
	// else{ // if (rank == 1){
		// //Worker
		// MPI_Send(local_buffer,comm_data_numbers[rank],MPI_INT,0,MPI_REDUCE_TAG,MPI_COMM_WORLD);
	// }
	
	// MPI_Bcast(global_sum, rcount, MPI_FLOAT, 0, MPI_COMM_WORLD);
	
	
	return MPI_SUCCESS;
}
}
}
