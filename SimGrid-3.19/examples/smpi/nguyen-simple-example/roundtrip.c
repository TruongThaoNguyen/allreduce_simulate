#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <smpi/smpi.h>

//#define N (1024 * 1024 * 1)
#define N (1024*1024*10)
#define M 1
int main(int argc, char *argv[])
{
  int size, rank;
  struct timeval start, end;
  char hostname[256];
  int hostname_len;

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
  if (rank == 0) {
	gettimeofday(&start,NULL);
	int i=0;
	for (i = 0; i <M; i++){
		printf("[#%d] Rank %d (running on '%s'): sending the message rank %d\n",i,rank,hostname,1);
		MPI_Send(buffer, N, MPI_DOUBLE_PRECISION, 1, 1, MPI_COMM_WORLD);
		MPI_Recv(buffer, N, MPI_DOUBLE_PRECISION, size-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("[#%d] Rank %d (running on '%s'): received the message from rank %d\n",i,rank,hostname,size-1);
	}
	gettimeofday(&end,NULL);
	printf("%f\n",(end.tv_sec*1000000.0 + end.tv_usec -
			start.tv_sec*1000000.0 - start.tv_usec) / 1000000.0);

  } else {
	int i = 0;
	for (i = 0; i < M; i++){  
		//gettimeofday(&start,NULL);
		MPI_Recv(buffer, N, MPI_DOUBLE_PRECISION, rank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		//gettimeofday(&end,NULL);
		//printf("Slave Received at %f\n",(end.tv_sec*1000000.0 + end.tv_usec -
		//	start.tv_sec*1000000.0 - start.tv_usec) / 1000000.0);
		printf("[#%d] Rank %d (running on '%s'): receive the message and sending it to rank %d\n",i,rank,hostname,(rank+1)%size);
		int j=0;
		for (j =0; j < N; j++){
			//printf("Received double value %d: %.10e \n",j, buffer[j]);
		}
		/* if (rank == 1) {
			SMPI_SAMPLE_FLOPS(1e9) {
			// imagine here some code running for 1e9 flops...
			}
		} */
		MPI_Send(buffer, N, MPI_DOUBLE_PRECISION, (rank+1)%size, 1, MPI_COMM_WORLD);
	}
  }

  MPI_Finalize();
  return 0;
}
