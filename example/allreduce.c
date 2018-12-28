#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <smpi/smpi.h>

//#define N (1024 * 1024 * 1)
#define N (1024 * 1024 * 1)  //Weight size
#define K 8 	//Batch size
#define M 100	//Number of iteration
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

  // Allocate a 1MiB buffer
	float *local_sum = malloc(sizeof(float) * N);
	float *global_sum = malloc(sizeof(float) * N);
	int i=0;
	if (rank == 0)
	{
		fprintf(stderr,"%s: %d/%d,weight size: %d, iteration: %d\n", hostname, rank, size, N * sizeof(float), M);
	}
	int j=0;
	for (j=0; j < N; j++){
		//local_sum[j] = (N-j) * (rank +1);
		//local_sum[j] = rank * N + j;
		local_sum[j] = j;
	}
	
	for (i = 0; i < M; i++){
		//For each interation
		//printf("Local sum for rank %d - %.10e, iteration: %d\n", rank, local_sum[0],M);

		//Allreduce the local_sum to the global_sum
		MPI_Allreduce(local_sum, global_sum, N, MPI_FLOAT, MPI_SUM,MPI_COMM_WORLD);
		
		// Print the result
		//printf("Global sum in rank %d - %.10e\n", rank, global_sum[N-1]);
	}
	
	if (rank == 0)
	{
		fprintf(stderr,"%s: %d/%d,weight size: %d, iteration: %d\n", hostname, rank, size, N * sizeof(float), M);
		for (j=0; j < 100; j++){
			printf("Global sum in rank %d - %.10e\n", rank, global_sum[j]);
		}
	}
	MPI_Finalize();
	return 0;
}
