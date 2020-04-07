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

	/* RESNET50 Deep Neural Network for the sample (image) size 226x226x3 
	Layer Name	|x|	|w|	|y|	Tcomp
	CONV_1	153228	9408	817216	6.16E-05
	BNORM	0	0	0	8.38E-07
	RELU	0	0	0	2.10E-07
	MPOOL	817216	0	200704	2.96E-05
	CONV_2a1	200704	4096	200704	6.59E-06
	BNORM	0	0	0	2.06E-07
	RELU	0	0	0	5.15E-08
	CONV_2a2	200704	36864	200704	5.93E-05
	BNORM	0	0	0	2.06E-07
	RELU	0	0	0	5.15E-08
	CONV_2a3	200704	16384	802816	2.64E-05
	BNORM	0	0	0	8.23E-07
	ADD	802816	0	802816	2.06E-07
	RELU	0	0	0	2.06E-07
	CONV_2b1	802816	16384	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	RELU	0	0	0	5.15E-08
	CONV_2b2	200704	36864	200704	5.93E-05
	BNORM	0	0	0	2.06E-07
	RELU	0	0	0	5.15E-08
	CONV_2b3	200704	16384	802816	2.64E-05
	BNORM	0	0	0	8.23E-07
	ADD	802816	0	802816	2.06E-07
	RELU	0	0	0	2.06E-07
	CONV_2c1	802816	16384	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	RELU	0	0	0	5.15E-08
	CONV_2c2	200704	36864	200704	5.93E-05
	BNORM	0	0	0	2.06E-07
	RELU	0	0	0	5.15E-08
	CONV_2c3	200704	16384	802816	2.64E-05
	BNORM	0	0	0	8.23E-07
	ADD	802816	0	802816	2.06E-07
	RELU	0	0	0	2.06E-07
	CONV_3a1	802816	32768	100352	1.32E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3a2	100352	147456	100352	5.93E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3a3	100352	65536	401408	2.64E-05
	BNORM	0	0	0	4.12E-07
	ADD	401408	0	401408	1.03E-07
	RELU	0	0	0	1.03E-07
	CONV_3b1	401408	65536	100352	2.64E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3b2	100352	147456	100352	5.93E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3b3	100352	65536	401408	2.64E-05
	BNORM	0	0	0	4.12E-07
	ADD	401408	0	401408	1.03E-07
	RELU	0	0	0	1.03E-07
	CONV_3c1	401408	65536	100352	2.64E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3c2	100352	147456	100352	5.93E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3c3	100352	65536	401408	2.64E-05
	BNORM	0	0	0	4.12E-07
	ADD	401408	0	401408	1.03E-07
	RELU	0	0	0	1.03E-07
	CONV_3d1	401408	65536	100352	2.64E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3d2	100352	147456	100352	5.93E-05
	BNORM	0	0	0	1.03E-07
	RELU	0	0	0	2.57E-08
	CONV_3d3	100352	65536	401408	2.64E-05
	BNORM	0	0	0	4.12E-07
	ADD	401408	0	401408	1.03E-07
	RELU	0	0	0	1.03E-07
	CONV_4a1	401408	131072	50176	1.32E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4a2	50176	589824	50176	5.94E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4a3	50176	262144	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	ADD	200704	0	200704	5.15E-08
	RELU	0	0	0	5.15E-08
	CONV_4b1	200704	262144	50176	2.64E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4b2	50176	589824	50176	5.94E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4b3	50176	262144	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	ADD	200704	0	200704	5.15E-08
	RELU	0	0	0	5.15E-08
	CONV_4c1	200704	262144	50176	2.64E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4c2	50176	589824	50176	5.94E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4c3	50176	262144	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	ADD	200704	0	200704	5.15E-08
	RELU	0	0	0	5.15E-08
	CONV_4d1	200704	262144	50176	2.64E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4d2	50176	589824	50176	5.94E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4d3	50176	262144	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	ADD	200704	0	200704	5.15E-08
	RELU	0	0	0	5.15E-08
	CONV_4e1	200704	262144	50176	2.64E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4e2	50176	589824	50176	5.94E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4e3	50176	262144	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	ADD	200704	0	200704	5.15E-08
	RELU	0	0	0	5.15E-08
	CONV_4f1	200704	262144	50176	2.64E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4f2	50176	589824	50176	5.94E-05
	BNORM	0	0	0	5.15E-08
	RELU	0	0	0	1.29E-08
	CONV_4f3	50176	262144	200704	2.64E-05
	BNORM	0	0	0	2.06E-07
	ADD	200704	0	200704	5.15E-08
	RELU	0	0	0	5.15E-08
	CONV_5a1	200704	524288	25088	1.32E-05
	BNORM	0	0	0	2.57E-08
	RELU	0	0	0	6.43E-09
	CONV_5a2	25088	2359296	25088	5.96E-05
	BNORM	0	0	0	2.57E-08
	RELU	0	0	0	6.43E-09
	CONV_5a3	25088	1048576	100352	2.65E-05
	BNORM	0	0	0	1.03E-07
	ADD	100352	0	100352	2.57E-08
	RELU	0	0	0	2.57E-08
	CONV_5b1	100352	1048576	25088	2.65E-05
	BNORM	0	0	0	2.57E-08
	RELU	0	0	0	6.43E-09
	CONV_5b2	25088	2359296	25088	5.96E-05
	BNORM	0	0	0	2.57E-08
	RELU	0	0	0	6.43E-09
	CONV_5b3	25088	1048576	100352	2.65E-05
	BNORM	0	0	0	1.03E-07
	ADD	100352	0	100352	2.57E-08
	RELU	0	0	0	2.57E-08
	CONV_5c1	100352	1048576	25088	2.65E-05
	BNORM	0	0	0	2.57E-08
	RELU	0	0	0	6.43E-09
	CONV_5c2	25088	2359296	25088	5.96E-05
	BNORM	0	0	0	2.57E-08
	RELU	0	0	0	6.43E-09
	CONV_5c3	25088	1048576	100352	2.65E-05
	BNORM	0	0	0	1.03E-07
	ADD	100352	0	100352	2.57E-08
	RELU	0	0	0	2.57E-08
	APOOL	100352	0	2048	5.27E-05
	FC6	2048	2048000	1000	7.88E-07
	Total	14971596	22734016	14819368	0.001892898
	*/  
	
	int i = 0;
	double input = 244*244*3;
	double nw[166][4] = {
		// |x|	|w|	|y|	Tcomp	LayerName
		{153228,9408,817216,6.16E-05},//"CONV_1"
		{0,0,0,8.38E-07},//"BNORM"
		{0,0,0,2.10E-07},//"RELU"
		{817216,0,200704,2.96E-05},//"MPOOL"
		{200704,4096,200704,6.59E-06},//"CONV_2a1"
		{0,0,0,2.06E-07},//"BNORM"
		{0,0,0,5.15E-08},//"RELU"
		{200704,36864,200704,5.93E-05},//"CONV_2a2"
		{0,0,0,2.06E-07},//"BNORM"
		{0,0,0,5.15E-08},//"RELU"
		{200704,16384,802816,2.64E-05},//"CONV_2a3"
		{0,0,0,8.23E-07},//"BNORM"
		{802816,0,802816,2.06E-07},//"ADD"
		{0,0,0,2.06E-07},//"RELU"
		{802816,16384,200704,2.64E-05},//"CONV_2b1"
		{0,0,0,2.06E-07},//"BNORM"
		{0,0,0,5.15E-08},//"RELU"
		{200704,36864,200704,5.93E-05},//"CONV_2b2"
		{0,0,0,2.06E-07},//"BNORM"
		{0,0,0,5.15E-08},//"RELU"
		{200704,16384,802816,2.64E-05},//"CONV_2b3"
		{0,0,0,8.23E-07},//"BNORM"
		{802816,0,802816,2.06E-07},//"ADD"
		{0,0,0,2.06E-07},//"RELU"
		{802816,16384,200704,2.64E-05},//"CONV_2c1"
		{0,0,0,2.06E-07},//"BNORM"
		{0,0,0,5.15E-08},//"RELU"
		{200704,36864,200704,5.93E-05},//"CONV_2c2"
		{0,0,0,2.06E-07},//"BNORM"
		{0,0,0,5.15E-08},//"RELU"
		{200704,16384,802816,2.64E-05},//"CONV_2c3"
		{0,0,0,8.23E-07},//"BNORM"
		{802816,0,802816,2.06E-07},//"ADD"
		{0,0,0,2.06E-07},//"RELU"
		{802816,32768,100352,1.32E-05},//"CONV_3a1"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,147456,100352,5.93E-05},//"CONV_3a2"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,65536,401408,2.64E-05},//"CONV_3a3"
		{0,0,0,4.12E-07},//"BNORM"
		{401408,0,401408,1.03E-07},//"ADD"
		{0,0,0,1.03E-07},//"RELU"
		{401408,65536,100352,2.64E-05},//"CONV_3b1"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,147456,100352,5.93E-05},//"CONV_3b2"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,65536,401408,2.64E-05},//"CONV_3b3"
		{0,0,0,4.12E-07},//"BNORM"
		{401408,0,401408,1.03E-07},//"ADD"
		{0,0,0,1.03E-07},//"RELU"
		{401408,65536,100352,2.64E-05},//"CONV_3c1"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,147456,100352,5.93E-05},//"CONV_3c2"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,65536,401408,2.64E-05},//"CONV_3c3"
		{0,0,0,4.12E-07},//"BNORM"
		{401408,0,401408,1.03E-07},//"ADD"
		{0,0,0,1.03E-07},//"RELU"
		{401408,65536,100352,2.64E-05},//"CONV_3d1"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,147456,100352,5.93E-05},//"CONV_3d2"
		{0,0,0,1.03E-07},//"BNORM"
		{0,0,0,2.57E-08},//"RELU"
		{100352,65536,401408,2.64E-05},//"CONV_3d3"
		{0,0,0,4.12E-07},//"BNORM"
		{401408,0,401408,1.03E-07},//"ADD"
		{0,0,0,1.03E-07},//"RELU"
		{401408,131072,50176,1.32E-05},//"CONV_4a1"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,589824,50176,5.94E-05},//"CONV_4a2"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,262144,200704,2.64E-05},//"CONV_4a3"
		{0,0,0,2.06E-07},//"BNORM"
		{200704,0,200704,5.15E-08},//"ADD"
		{0,0,0,5.15E-08},//"RELU"
		{200704,262144,50176,2.64E-05},//"CONV_4b1"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,589824,50176,5.94E-05},//"CONV_4b2"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,262144,200704,2.64E-05},//"CONV_4b3"
		{0,0,0,2.06E-07},//"BNORM"
		{200704,0,200704,5.15E-08},//"ADD"
		{0,0,0,5.15E-08},//"RELU"
		{200704,262144,50176,2.64E-05},//"CONV_4c1"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,589824,50176,5.94E-05},//"CONV_4c2"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,262144,200704,2.64E-05},//"CONV_4c3"
		{0,0,0,2.06E-07},//"BNORM"
		{200704,0,200704,5.15E-08},//"ADD"
		{0,0,0,5.15E-08},//"RELU"
		{200704,262144,50176,2.64E-05},//"CONV_4d1"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,589824,50176,5.94E-05},//"CONV_4d2"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,262144,200704,2.64E-05},//"CONV_4d3"
		{0,0,0,2.06E-07},//"BNORM"
		{200704,0,200704,5.15E-08},//"ADD"
		{0,0,0,5.15E-08},//"RELU"
		{200704,262144,50176,2.64E-05},//"CONV_4e1"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,589824,50176,5.94E-05},//"CONV_4e2"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,262144,200704,2.64E-05},//"CONV_4e3"
		{0,0,0,2.06E-07},//"BNORM"
		{200704,0,200704,5.15E-08},//"ADD"
		{0,0,0,5.15E-08},//"RELU"
		{200704,262144,50176,2.64E-05},//"CONV_4f1"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,589824,50176,5.94E-05},//"CONV_4f2"
		{0,0,0,5.15E-08},//"BNORM"
		{0,0,0,1.29E-08},//"RELU"
		{50176,262144,200704,2.64E-05},//"CONV_4f3"
		{0,0,0,2.06E-07},//"BNORM"
		{200704,0,200704,5.15E-08},//"ADD"
		{0,0,0,5.15E-08},//"RELU"
		{200704,524288,25088,1.32E-05},//"CONV_5a1"
		{0,0,0,2.57E-08},//"BNORM"
		{0,0,0,6.43E-09},//"RELU"
		{25088,2359296,25088,5.96E-05},//"CONV_5a2"
		{0,0,0,2.57E-08},//"BNORM"
		{0,0,0,6.43E-09},//"RELU"
		{25088,1048576,100352,2.65E-05},//"CONV_5a3"
		{0,0,0,1.03E-07},//"BNORM"
		{100352,0,100352,2.57E-08},//"ADD"
		{0,0,0,2.57E-08},//"RELU"
		{100352,1048576,25088,2.65E-05},//"CONV_5b1"
		{0,0,0,2.57E-08},//"BNORM"
		{0,0,0,6.43E-09},//"RELU"
		{25088,2359296,25088,5.96E-05},//"CONV_5b2"
		{0,0,0,2.57E-08},//"BNORM"
		{0,0,0,6.43E-09},//"RELU"
		{25088,1048576,100352,2.65E-05},//"CONV_5b3"
		{0,0,0,1.03E-07},//"BNORM"
		{100352,0,100352,2.57E-08},//"ADD"
		{0,0,0,2.57E-08},//"RELU"
		{100352,1048576,25088,2.65E-05},//"CONV_5c1"
		{0,0,0,2.57E-08},//"BNORM"
		{0,0,0,6.43E-09},//"RELU"
		{25088,2359296,25088,5.96E-05},//"CONV_5c2"
		{0,0,0,2.57E-08},//"BNORM"
		{0,0,0,6.43E-09},//"RELU"
		{25088,1048576,100352,2.65E-05},//"CONV_5c3"
		{0,0,0,1.03E-07},//"BNORM"
		{100352,0,100352,2.57E-08},//"ADD"
		{0,0,0,2.57E-08},//"RELU"
		{100352,0,2048,5.27E-05},//"APOOL"
		{2048,2048000,1000,7.88E-07}//"FC6"
	 };
	
	int L = 166; // Number of Layer
	int G1_last_layer = 165;
	
	int node_ratio = 2;
	int G1_size = size * node_ratio / (1+node_ratio);
	int G2_size = size - G1_size;
	
	int G1_message_size = 0;
	int G2_message_size = 0;
	float G1_Tcomp = 0;
	float G2_Tcomp = 0;
	int G1toG2_message = 0;

	for (i = 0; i < L; i++){
		if (i < G1_last_layer){
			G1_message_size = G1_message_size + nw[i][1];
			G1_Tcomp = G1_Tcomp + nw[i][3];
			
		} else {
			G2_message_size = G2_message_size + nw[i][1];
			G2_Tcomp = G2_Tcomp + nw[i][3];
		}
		
		if (i == G1_last_layer - 1){
			G1toG2_message = nw[i][2];
		}
	}	
	
	if (rank == 0) {
/* 		for (i = 0; i < L; i++){
			printf("Layer %d [%f,%f,%f,%lf]\n",i,nw[i][0],nw[i][1],nw[i][2],nw[i][3]);
		} */
		printf("Group 1[%d --> %d], w = %d, Tcomp =%f, last_y = %d \n",0,G1_last_layer-1,G1_message_size, G1_Tcomp,G1toG2_message);
		printf("Group 2[%d --> %d], w = %d, Tcomp =%f \n",G1_last_layer,L-1,G2_message_size, G2_Tcomp);
		printf("Total number of gradient %d, total comp %f\n",G1_message_size+ G2_message_size, G1_Tcomp + G2_Tcomp);
		printf("G1_size: %d, G2_size: %d\n",G1_size,G2_size);
	}
	
	double *sbuf = malloc(sizeof(double) * (G1_message_size + G2_message_size));
	double *rbuf = malloc(sizeof(double) * (G1_message_size + G2_message_size));
	MPI_Barrier(MPI_COMM_WORLD);
	/****** Training - calculate communication time only ***********/
 	// Gather
	gettimeofday(&checkpoint,NULL);
	
	int g2_rank = -1;
	int g1_rank = rank;
	if (rank >= G1_size){
		g2_rank = rank - G1_size;
		g1_rank = g2_rank * node_ratio;
	}
	else{
		g2_rank = rank / node_ratio + G1_size;
	}
	
	//printf("rank: %d, g1_rank: %d, g2_rank: %d \n",rank, g1_rank, g2_rank);
	if (rank >= G1_size){
		
		for (i = 0; i < node_ratio; i++){
			MPI_Recv(rbuf, G1toG2_message, MPI_DOUBLE_PRECISION, g1_rank+i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//printf("rank: %d wait for: g2_rank: %d \n",rank, g1_rank+i);
		}
	}
	else{
		MPI_Send(sbuf, G1toG2_message, MPI_DOUBLE_PRECISION, g2_rank, 1, MPI_COMM_WORLD);
		//printf("rank: %d send to rank: %d\n",rank, g2_rank);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	gettimeofday(&end,NULL);
	if (rank == 0) {
		double gather_time = (end.tv_sec*1000000.0 + end.tv_usec - checkpoint.tv_sec*1000000.0 - checkpoint.tv_usec) / 1000000.0;
		printf("gather_time:%f \n",gather_time);
	}
	
	
	//Scatter
	gettimeofday(&checkpoint,NULL);
	if (rank >= G1_size){
		
		for (i = 0; i < node_ratio; i++){
			MPI_Send(sbuf, G1toG2_message, MPI_DOUBLE_PRECISION, g1_rank+i, 2, MPI_COMM_WORLD);
		}
	}
	else{
		MPI_Recv(rbuf, G1toG2_message, MPI_DOUBLE_PRECISION, g2_rank, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	gettimeofday(&end,NULL);
	if (rank == 0) {
		double scatter_time = (end.tv_sec*1000000.0 + end.tv_usec - checkpoint.tv_sec*1000000.0 - checkpoint.tv_usec) / 1000000.0;
		printf("scatter_time:%f \n",scatter_time);
	}
	
	//Allreduce G1
	MPI_Group world_group;
	MPI_Comm_group(MPI_COMM_WORLD, &world_group);
	int g1_ranks[G1_size] = {};
	for (i = 0; i < G1_size; i++){
		g1_ranks[i] = i;
	}
	
	MPI_Group g1_group;
	MPI_Group_incl(world_group, G1_size, g1_ranks, &g1_group);
	MPI_Comm g1_comm;
	MPI_Comm_create_group(MPI_COMM_WORLD, g1_group, 0, &g1_comm);
	
	gettimeofday(&checkpoint,NULL);
	MPI_Allreduce(sbuf, rbuf, G1_message_size, MPI_FLOAT, MPI_SUM,g1_comm);
	MPI_Barrier(MPI_COMM_WORLD);
	gettimeofday(&end,NULL);
	if (rank == 0) {
		double g1_reduce_time = (end.tv_sec*1000000.0 + end.tv_usec - checkpoint.tv_sec*1000000.0 - checkpoint.tv_usec) / 1000000.0;
		printf("g1_reduce_time:%f \n",g1_reduce_time);
	}
  //TYPE: MPI_DOUBLE_PRECISION, MPI_REAL, MPI_INT, MPI_BYTE

  // Allocate a 1MiB buffer
/* 	float *local_sum = malloc(sizeof(float) * N);
	float *global_sum = malloc(sizeof(float) * N);
	int i=0;
	if (rank == 0)
	{
		fprintf(stderr,"%s: %d/%d,weight size: %d, iteration: %d\n", hostname, rank, size, N * sizeof(float), M);
	}
	int j=0;
	for (j=0; j < N; j++){
		//local_sum[j] = (N-j) * (rank +1);
		local_sum[j] = rank * N + j;
	}
	
	for (i = 0; i < M; i++){
		//For each interation
		//printf("Local sum for rank %d - %.10e, iteration: %d\n", rank, local_sum[0],M);

		//Allreduce the local_sum to the global_sum
		MPI_Allreduce(local_sum, global_sum, N, MPI_FLOAT, MPI_SUM,MPI_COMM_WORLD);
		
		// Print the result
		if (rank == 0)
		{
			fprintf(stderr,"%s: %d/%d,weight size: %d, iteration: %d\n", hostname, rank, size, N * sizeof(float), M);
			for (j=0; j < 100; j++){
				printf("Global sum in rank %d - %.10e\n", rank, global_sum[j]);
			}
		}
		
		//printf("Global sum in rank %d - %.10e\n", rank, global_sum[N-1]);
	} */
	MPI_Finalize();
	return 0;
}


/* int c_allreduce_ring(double *sendbuf,double *recvbuf, int rcount, MPI_Comm comm, int start_rank, int end_rank) {
	int size, rank;
	char hostname[256];
	int hostname_len;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);
	MPI_Get_processor_name(hostname,&hostname_len);

	
} */