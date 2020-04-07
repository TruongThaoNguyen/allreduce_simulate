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

	/* VGG19 Deep Neural Network for the sample (image) size 226x226x3 
	Layer Name	|x|	|w|	|y|	Tcomp
	0 CONV1_1	153228	1728	3268864	4.53E-05
	RELU	0	0	0	8.38E-07
	CONV1_2	3268864	36864	3268864	9.66E-04
	RELU	0	0	0	8.38E-07
	MPOOL	3268864	0	817216	5.36E-05
	CONV2_1	817216	73728	1634432	4.83E-04
	RELU	0	0	0	4.19E-07
	CONV2_2	1634432	147456	1634432	9.66E-04
	RELU	0	0	0	4.19E-07
	MPOOL	1634432	0	415872	5.46E-05
	CONV3_1	415872	294912	831744	4.91E-04
	RELU	0	0	0	2.13E-07
	CONV3_2	831744	589824	831744	9.83E-04
	RELU	0	0	0	2.13E-07
	CONV3_3	831744	589824	831744	9.83E-04
	RELU	0	0	0	2.13E-07
	MPOOL	831744	0	215296	5.65E-05
	CONV4_1	215296	1179648	430592	5.09E-04
	RELU	0	0	0	1.10E-07
	CONV4_2	430592	2359296	430592	1.02E-03
	RELU	0	0	0	1.10E-07
	CONV4_3	430592	2359296	430592	1.02E-03
	RELU	0	0	0	1.10E-07
	MPOOL	430592	0	115200	6.05E-05
	CONV5_1	115200	2359296	115200	2.73E-04
	RELU	0	0	0	2.95E-08
	CONV5_2	115200	2359296	115200	2.73E-04
	RELU	0	0	0	2.95E-08
	CONV5_3	115200	2359296	115200	2.73E-04
	RELU	0	0	0	2.95E-08
	MPOOL	115200	0	32768	1.72E-05
	FC6	32768	134217728	4096	5.16E-05
	RELU	0	0	0	1.05E-09
	DROPOUT	4096	0	4096	0.00E+00
	FC7	4096	16777216	4096	6.45E-06
	RELU	0	0	0	1.05E-09
	DROPOUT	4096	0	4096	0.00E+00
	FC8	4096	4096000	1000	1.58E-06
	Total	15705164	169801408	15552936	0.008584103
	*/  
	
	int i = 0;
	double input = 244*244*3;
	double nw[166][4] = {
		// |x|	|w|	|y|	Tcomp	LayerName
		{153228,1728,3268864,4.53E-05},//CONV1_1
		{0,0,0,8.38E-07},//RELU
		{3268864,36864,3268864,9.66E-04},//CONV1_2
		{0,0,0,8.38E-07},//RELU
		{3268864,0,817216,5.36E-05},//MPOOL
		{817216,73728,1634432,4.83E-04},//CONV2_1
		{0,0,0,4.19E-07},//RELU
		{1634432,147456,1634432,9.66E-04},//CONV2_2
		{0,0,0,4.19E-07},//RELU
		{1634432,0,415872,5.46E-05},//MPOOL
		{415872,294912,831744,4.91E-04},//CONV3_1
		{0,0,0,2.13E-07},//RELU
		{831744,589824,831744,9.83E-04},//CONV3_2
		{0,0,0,2.13E-07},//RELU
		{831744,589824,831744,9.83E-04},//CONV3_3
		{0,0,0,2.13E-07},//RELU
		{831744,0,215296,5.65E-05},//MPOOL
		{215296,1179648,430592,5.09E-04},//CONV4_1
		{0,0,0,1.10E-07},//RELU
		{430592,2359296,430592,1.02E-03},//CONV4_2
		{0,0,0,1.10E-07},//RELU
		{430592,2359296,430592,1.02E-03},//CONV4_3
		{0,0,0,1.10E-07},//RELU
		{430592,0,115200,6.05E-05},//MPOOL
		{115200,2359296,115200,2.73E-04},//CONV5_1
		{0,0,0,2.95E-08},//RELU
		{115200,2359296,115200,2.73E-04},//CONV5_2
		{0,0,0,2.95E-08},//RELU
		{115200,2359296,115200,2.73E-04},//CONV5_3
		{0,0,0,2.95E-08},//RELU
		{115200,0,32768,1.72E-05},//MPOOL
		{32768,134217728,4096,5.16E-05},//FC6
		{0,0,0,1.05E-09},//RELU
		{4096,0,4096,0.00E+00},//DROPOUT
		{4096,16777216,4096,6.45E-06},//FC7
		{0,0,0,1.05E-09},//RELU
		{4096,0,4096,0.00E+00},//DROPOUT
		{4096,4096000,1000,1.58E-06}//FC8
	 };
	
	int L = 38; // Number of Layer
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