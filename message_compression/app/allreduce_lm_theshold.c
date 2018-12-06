#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <smpi/smpi.h>

//#define N (1024 * 1024 * 1)
//#define N (1024 * 1024 / 4)  //Weight size
#define M 1000	//Number of iteration
#define bufSize 29999 //maximum length of each line
#define MPI_REDUCE_TAG 1
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

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
	struct timeval smpi_start, smpi_end;
	float smpi_current_time;
	//1. Check parameter
	if (argc < 2)
	{
		fprintf(stderr,"Usage: %s <soure-folder>\n", argv[0]);
		return;
	}

	//2. MPI init
	smpi_gettimeofday(&smpi_start,NULL);
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Get_processor_name(hostname,&hostname_len);
	//printf("%s: %d,%s,%s\n",hostname, argc, argv[0],argv[1]);

	//2. Get data filename
	sprintf(srank, "%d", rank);
	//printf("rank:%s\n",srank);
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
	//printf("%s", buf);
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
	printf("rank%d: N=%d\n", rank, N);
	smpi_gettimeofday(&smpi_end,NULL);
	smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
	printf("[@%f] Rank %d (running on '%s') Finished get data %d\n",smpi_current_time,rank,hostname);
	
	// 4. Quantize the data using threshold: only send a fixed propotion value of data
	// val+ = data > T+;
	float threshold_positive = 0.01;
	float threshold_negative = -0.01;
	int* data_numbers = malloc(sizeof(int) * 1); 
	int* comm_data_numbers = malloc(sizeof(int) * size); //Buffer for number of communication data...
	int i=0;
	
	// 4.1. Distribute number of communicate
	data_numbers[0] = 0;
	for (i = 0; i < N; i++){
		if (local_data[i] > threshold_positive){
			data_numbers[0]++;
		}
		else if (local_data[i] < threshold_negative){
			data_numbers[0]++;
		}
	}		
	for (i = 0; i < size; i++){ 
		comm_data_numbers[i] = 0;
	}
	comm_data_numbers[rank] = data_numbers[0];
	MPI_Allgather(data_numbers,1,MPI_INT,comm_data_numbers,1,MPI_INT,MPI_COMM_WORLD);
	//printf("rank%d: comm_data_numbers=%d, data_numbers=%d\n", rank, comm_data_numbers[rank],data_numbers[0]);
	
	//4.2. Malloc data for compressed data
	int max_reiv_buffer_size = data_numbers[0];
	for (i = 0; i < size; i++){
		if (rank == 0){
			printf("[%d]Rank %d send %d values over %d (%f)\n",rank,i,comm_data_numbers[i],N,((float)comm_data_numbers[i])/N);
		}
		if(max_reiv_buffer_size < comm_data_numbers[i]){
			max_reiv_buffer_size = comm_data_numbers[i];
		}			
	}
	int * local_buffer = malloc(sizeof(unsigned int) * data_numbers[0]);
	int * global_buffer = malloc(sizeof(unsigned int) * max_reiv_buffer_size);
	
	// 4.3. Compress the data
	//TODO: What happends if number of values N > 2^31 
	int next_compress_value = 0;
	for (i = 0; i < N; i++){
		if (rank == 0){
			if (local_data[i] > threshold_positive){
				global_sum[i] = threshold_positive;
			}
			else if(local_data[i] < threshold_negative){
				global_sum[i] = threshold_negative;
			}
			else{
				global_sum[i] = 0;
			}
		}
		else{
			if (local_data[i] > threshold_positive || local_data[i] < threshold_negative){
				local_buffer[next_compress_value] = (unsigned int) i;
				if (local_data[i] > threshold_positive){local_buffer[next_compress_value] &= ~(1 << 31);}
				else {local_buffer[next_compress_value] |= (1 << 31);}
				next_compress_value = next_compress_value + 1;
			}
		}
	}	
	
	smpi_gettimeofday(&smpi_end,NULL);
	smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
	printf("[@%f] Rank %d (running on '%s') Finished Compression %d\n",smpi_current_time,rank,hostname);
	
	//5. Communication Reduce (pair-wise) + Bcast (pair-wise)
	// 5.1 Copy the root value first
	// 5.2 Reduce (pair-wise)
	//TYPE: MPI_DOUBLE_PRECISION, MPI_REAL, MPI_INT, MPI_BYTE

	if (rank == 0){
		for (i = 1; i < size; i++){
			//i = 1;
			//Root
			MPI_Recv(global_buffer,comm_data_numbers[i],MPI_INT,i,MPI_REDUCE_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			// printf("Recv from %d",1);
			
			// Unquantized and compute the value
				int j =0;
				int position = 0;
				for(j = 0; j < comm_data_numbers[i]; j++){
					position = global_buffer[j];
					position &= ~(1 << 31);
					if (CHECK_BIT(global_buffer[j],31)){
						global_sum[position] += threshold_positive;
					}
					else
					{
						global_sum[position] += threshold_negative;
					}
				}
				// printf("Finish compute from %d\n",i);
		}
	}
	else{ // if (rank == 1){
		//Worker
		MPI_Send(local_buffer,comm_data_numbers[rank],MPI_INT,0,MPI_REDUCE_TAG,MPI_COMM_WORLD);
	}
	smpi_gettimeofday(&smpi_end,NULL);
	smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
	printf("[@%f] Rank %d (running on '%s') Finished Reduce %d\n",smpi_current_time,rank,hostname);
	
	//5.3 Bcast
	MPI_Bcast(global_sum, N, MPI_FLOAT, 0, MPI_COMM_WORLD);
	//printf("Rank:%d, global_sum[3]=%f\n",rank, global_sum[3]);
	smpi_gettimeofday(&smpi_end,NULL);
	smpi_current_time = (smpi_end.tv_sec*1000000.0 + smpi_end.tv_usec - smpi_start.tv_sec*1000000.0 - smpi_start.tv_usec) / 1000000.0;
	printf("[@%f] Rank %d (running on '%s') Finished Reduce %d\n",smpi_current_time,rank,hostname);
	MPI_Finalize();
	return 0;
}