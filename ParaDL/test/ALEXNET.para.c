#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <smpi/smpi.h>

#define  COLL_TAG_ALLREDUCE 10
/* //#define N (1024 * 1024 * 1)
#define N (1024 * 1024 /4)  //Weight size
#define K 8 	//Batch size
#define M 1	//Number of iteration */
int main(int argc, char *argv[])
{
	int size, rank;
	struct timeval start, end, checkpoint;
	char hostname[256];
	int hostname_len;
	int message_size = 0;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Get_processor_name(hostname,&hostname_len);

	/* ALEXNET Deep Neural Network for the sample (image) size 226x226x3 
	Layer Name	|x|	|w|	|y|	Tcomp
	CONV1	153228	34848	279936	5.21E-05
	RELU	0	0	0	7.18E-08
	MPOOL	279936	0	69984	1.55E-05
	CONV2	69984	614400	186624	2.30E-04
	RELU	0	0	0	4.79E-08
	MPOOL	186624	0	43264	2.56E-05
	CONV3	43264	884736	64896	7.68E-05
	RELU	0	0	0	1.66E-08
	CONV4	64896	1327104	64896	1.15E-04
	RELU	0	0	0	1.66E-08
	CONV5	64896	884736	43264	7.68E-05
	RELU	43264	0	0	1.11E-08
	MPOOL	43264	0	9216	5.44E-06
	FC6	9216	37748736	4096	1.45E-05
	RELU	0	0	0	1.05E-09
	DROPOUT	4096	0	4096	0.00E+00
	FC7	4096	16777216	4096	6.45E-06
	RELU	0	0	0	1.05E-09
	DROPOUT	4096	0	4096	0.00E+00
	FC8	4096	4096000	1000	1.58E-06
	Total	974956	62367776	779464	0.000619873
	*/  
	
	int i = 0;
	double input = 244*244*3;
	double nw[166][4] = {
		// |x|	|w|	|y|	Tcomp	LayerName
		{153228,34848,279936,5.21E-05},//CONV1
		{0,0,0,7.18E-08},//RELU
		{279936,0,69984,1.55E-05},//MPOOL
		{69984,614400,186624,2.30E-04},//CONV2
		{0,0,0,4.79E-08},//RELU
		{186624,0,43264,2.56E-05},//MPOOL
		{43264,884736,64896,7.68E-05},//CONV3
		{0,0,0,1.66E-08},//RELU
		{64896,1327104,64896,1.15E-04},//CONV4
		{0,0,0,1.66E-08},//RELU
		{64896,884736,43264,7.68E-05},//CONV5
		{43264,0,0,1.11E-08},//RELU
		{43264,0,9216,5.44E-06},//MPOOL
		{9216,37748736,4096,1.45E-05},//FC6
		{0,0,0,1.05E-09},//RELU
		{4096,0,4096,0.00E+00},//DROPOUT
		{4096,16777216,4096,6.45E-06},//FC7
		{0,0,0,1.05E-09},//RELU
		{4096,0,4096,0.00E+00},//DROPOUT
		{4096,4096000,1000,1.58E-06}//FC8
	 };
	
	int L = 20; // Number of Layer
	double total_comp;

	for (i = 0; i < L; i++){
		message_size = message_size +  nw[i][1];
		total_comp = total_comp + nw[i][3];
	}	
	
	if (rank == 0) {
/* 		for (i = 0; i < L; i++){
			printf("Layer %d [%f,%f,%f,%lf]\n",i,nw[i][0],nw[i][1],nw[i][2],nw[i][3]);
		} */
		printf("Total number of gradient %d, total comp %f\n",message_size, total_comp);
	}
	
	double *sbuf = malloc(sizeof(double) * (message_size));
	double *rbuf = malloc(sizeof(double) * (message_size));
	MPI_Barrier(MPI_COMM_WORLD);
	/****** Training - calculate communication time only ***********/

	//Allreduce 
	gettimeofday(&checkpoint,NULL);
	MPI_Allreduce(sbuf, rbuf, message_size, MPI_DOUBLE_PRECISION, MPI_SUM,MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	gettimeofday(&end,NULL);
	if (rank == 0) {
		double all_reduce_time = (end.tv_sec*1000000.0 + end.tv_usec - checkpoint.tv_sec*1000000.0 - checkpoint.tv_usec) / 1000000.0;
		printf("all_reduce_time:%f \n",all_reduce_time);
	}
  //TYPE: MPI_DOUBLE_PRECISION, MPI_REAL, MPI_INT, MPI_BYTE
	MPI_Finalize();
	return 0;
}