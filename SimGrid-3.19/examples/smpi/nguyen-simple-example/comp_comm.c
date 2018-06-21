#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <smpi/smpi.h>

//#define N (1024 * 1024 * 1)
#define N (1024*100)
#define M 1
int main(int argc, char *argv[])
{
  int size, rank;
  struct timeval start, end, end_c;
  struct timeval smpi_start, smpi_end;
  char hostname[256];
  int hostname_len;
  float current_time, smpi_current_time;
  
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Get_processor_name(hostname,&hostname_len);
  
  //TYPE: MPI_DOUBLE_PRECISION, MPI_REAL, MPI_INT, MPI_BYTE

  // Allocate a 10MiB buffer
  double *buffer = malloc(sizeof(double) * N);
	int j=0;
	for (j=0; j < N; j++){
		buffer[j] = j;
	}
  fprintf(stderr,"%s: %d/%d,buffer: %d, iteration: %d\n", hostname, rank, size, N, M);

  // Communicate along the ring
  gettimeofday(&start,NULL);
  smpi_gettimeofday(&smpi_start,NULL);
  if (rank == 0) {
	int i=0;
	for (i = 0; i <M; i++){
		gettimeofday(&end,NULL);
		smpi_gettimeofday(&smpi_end,NULL);
		current_time = (end.tv_sec*1000000.0 + end.tv_usec - start.tv_sec*1000000.0 - start.tv_usec) / 1000000.0;
		smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
		printf("[#%d][@%f - %f] Rank %d (running on '%s'): send the message to rank %d\n",i,current_time,smpi_current_time,rank,hostname,1);
		MPI_Send(buffer, N, MPI_DOUBLE_PRECISION, 1, 1, MPI_COMM_WORLD);
		
		MPI_Recv(buffer, N, MPI_DOUBLE_PRECISION, size-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		gettimeofday(&end,NULL);
		smpi_gettimeofday(&smpi_end,NULL);
		current_time = (end.tv_sec*1000000.0 + end.tv_usec - start.tv_sec*1000000.0 - start.tv_usec) / 1000000.0;
		smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
		printf("[#%d][@%f -%f] Rank %d (running on '%s'): received the message from rank %d\n",i,current_time,smpi_current_time,rank,hostname,size-1);
	}
  } else {
	int i = 0;
	for (i = 0; i < M; i++){  
		MPI_Recv(buffer, N, MPI_DOUBLE_PRECISION, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		gettimeofday(&end,NULL);
		smpi_gettimeofday(&smpi_end,NULL);
		current_time = (end.tv_sec*1000000.0 + end.tv_usec - start.tv_sec*1000000.0 - start.tv_usec) / 1000000.0;
		smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
		printf("[#%d][@%f - %f] Rank %d (running on '%s'): receive the message from rank %d\n",i,current_time,smpi_current_time,rank,hostname,(rank-1)%size);
		int j=0;
		for (j =0; j < N; j++){
			//printf("Received double value %d: %.10e \n",j, buffer[j]);
		}
		if (rank == 1) {
			SMPI_SAMPLE_FLOPS(1e10) {
			 //imagine here some code running for 1e9 flops...
			}
		}
		gettimeofday(&end_c,NULL);
		current_time = (end_c.tv_sec*1000000.0 + end_c.tv_usec - end.tv_sec*1000000.0 - end.tv_usec) / 1000000.0;
		printf("[#%d] %d (running on '%s'): Computing in %f\n",i,rank,hostname,current_time);
		
		gettimeofday(&end,NULL);
		smpi_gettimeofday(&smpi_end,NULL);
		current_time = (end.tv_sec*1000000.0 + end.tv_usec - start.tv_sec*1000000.0 - start.tv_usec) / 1000000.0;
		smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
		printf("[#%d][@%f - %f] %d (running on '%s'): send message to rank %d\n",i,current_time,smpi_current_time,rank,hostname,(rank+1)%size);
		MPI_Send(buffer, N, MPI_DOUBLE_PRECISION, (rank+1)%size, 1, MPI_COMM_WORLD);
	}
  }

  MPI_Finalize();
  return 0;
}
