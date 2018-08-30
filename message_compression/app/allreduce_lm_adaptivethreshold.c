#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#inlcude <math.h>
#include <mpi.h>
//#include <smpi/smpi.h>

//#define N (1024 * 1024 * 1)
//#define N (1024 * 1024 / 4)  //Weight size
#define M 1000	//Number of iteration
#define bufSize 29999 //maximum length of each line
int main(int argc, char *argv[])
{
	FILE* fp;
	char buf[bufSize];
	char srank[10];
	char data_filename[256];
	int size, rank;
	struct timeval start, end;
	char hostname[256];
	int hostname_len;

	//1. Check parameter
	if (argc < 2)
	{
		fprintf(stderr,"Usage: %s <soure-folder>\n", argv[0]);
		return;
	}

	//2. MPI init
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Get_processor_name(hostname,&hostname_len);
	printf("%s: %d,%s,%s\n",hostname, argc, argv[0],argv[1]);

	//2. Get data filename
	sprintf(srank, "%d", rank);
	printf("rank:%s\n",srank);
	strcpy(data_filename,argv[1]);
	strcat(data_filename,"param");
	strcat(data_filename,srank);
	strcat(data_filename,".txt");
	printf("data_filename:%s\n",data_filename);

	//3. Read local_data from file to local_data.
	//3.1. Open data file
	if ((fp = fopen(data_filename, "r")) == NULL)
	{ /* Open source file.*/ 
		perror("fopen source-file");
		return;
	}

	// 3.2. Get data size
	fgets(buf, sizeof(buf), fp);
	printf("%s", buf);
	const char delim[2] = ",";
	char *token;
	/* get the 2nd token that declares the number of weight*/
	token = strtok(buf, delim);
	token = strtok(NULL, delim);
	//printf("%s\n", token);
	int N = atoi(token); //Weight size
	float *local_data = malloc(sizeof(float) * N);
	float *global_sum = malloc(sizeof(float) * N);

	// 3.3. Allocate the local_data
	int value_idx = 0;
	float value;
	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		//buf[strlen(buf) - 1] = '\0'; // eat the newline fgets() stores
		// printf("%s\n", buf);
		token = strtok(buf, delim);
		//printf("first value of line:%s\n", token);
		if (token != NULL &&  token[0] != '\0') {
			value = atof(token);
			//printf("first value of line(f) %1.10f\n", value);
			if(value_idx < N){
				local_data[value_idx] = value;
				value_idx++;
			}
		}
		/* walk through other tokens */
		while( token != NULL ) {
		  token = strtok(NULL, delim);
			if (token != NULL && token[0] != '\0') {
				//printf("next value of line:%s\n", token);
				value = atof(token);
				if(value_idx < N){
					local_data[value_idx] = value;
					value_idx ++;
				}
			}
		}
	}
	fclose(fp);
	
	N = value_idx;
	printf("rank%d: N=%d, value_idx=%d\n", rank, N,value_idx);
	
	
	// 4. Quantize the data using adaptive threshold: only send a fixed propotion value of data
	// val+ = top_p(data); val- = top_p(-data)
	// mean+ = mean(val+); mean- = mean(val-);
	
	float propotion = 0.01; //0.1% or 1%
	int comm_data_number = (int) cell(propotion*N);
	int * local_buffer = = malloc(sizeof(int) * comm_data_number);
	int * global_buffer = = malloc(sizeof(int) * comm_data_number);
	
	
	//TYPE: MPI_DOUBLE_PRECISION, MPI_REAL, MPI_INT, MPI_BYTE

	// // Allocate a 1MiB buffer
	int i=0;
	// if (rank == 0)
	// {
		// fprintf(stderr,"%s: %d/%d,weight size: %d, iteration: %d\n", hostname, rank, size, N * sizeof(float), M);
	// }
	// int j=0;
	// for (j=0; j < N; j++){
		// //local_data[j] = (N-j) * (rank +1);
		// local_data[j] = rank * N + j;
	// }
	
	// 4 Allreduce operation
	for (i = 0; i < M; i++){
		//For each interation
		//printf("Local sum for rank %d - %.10e, iteration: %d\n", rank, local_data[0],M);

		//Allreduce the local_data to the global_sum
		MPI_Allreduce(local_data, global_sum, N, MPI_FLOAT, MPI_SUM,MPI_COMM_WORLD);
		
		// Print the result
		/*if (rank == 0)
		{
			fprintf(stderr,"%s: %d/%d,weight size: %d, iteration: %d\n", hostname, rank, size, N * sizeof(float), M);
			printf("Global sum in rank %d - %.10e\n", rank, global_sum[0]);
		}*/
		
		//printf("Global sum in rank %d - %.10e\n", rank, global_sum[N-1]);
	}
	MPI_Finalize();
	return 0;
}
