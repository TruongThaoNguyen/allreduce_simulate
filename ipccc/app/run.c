#define OUTPUT_RANK 0

#include "mpi.h"

#include "c_allreduce_recdoubling.h"
#include "c_allreduce_big.h"
#include "c_allreduce_small.h"
#include "c_allreduce_ring.h"
#include "c_allreduce_dense_ring.h"
#include "c_allreduce_dense_ring_ring.h"
#include "c_allreduce_ring.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <set>
#include <vector>

typedef unsigned int IdxType;
typedef int ValType;
#define MPI_IDX_TYPE MPI_UNSIGNED
#define MPI_VAL_TYPE MPI_INT

//#define COMPARE_TO_DENSE true
#define TAKES 1 //10
#define EXPERIMENTS 1 //5

typedef struct s_item<IdxType, ValType> my_s_item;

typedef struct stream my_stream;

void set_seed(unsigned int seed) {
  srand(seed);
  srand48(seed);
}

void set_seed_random(int id) {
  set_seed(clock() + (id * 147));
}

// Between 0 (included) and max (excluded)
unsigned int get_random_int(unsigned int max) {
  return rand()%max;
}

// Between 0 (included) and max(excluded)
float get_random_float(unsigned int max) {
  return drand48()*max;
}

ValType get_random_value() {
  //return get_random_float(100) - 50;
  return get_random_int(200) - 100; // Change to int if this changes
}

void create_dense(const unsigned dim, ValType *v) {
  for(size_t i = 0; i < dim; ++i) {
    v[i] = get_random_value();
  }
}

void create_sparse(const unsigned dim, const unsigned count, my_s_item *v) {
  // Create indices from 0 to dim 
  std::vector<unsigned int> indices(dim) ;
  std::iota (indices.begin(), indices.end(), 0);

  // Random suffel indices
  std::random_shuffle ( indices.begin(), indices.end() );
  // Sort first count items
  std::sort( indices.begin(), indices.begin() + count);

  size_t idx = 0;
  for(std::vector<unsigned int>::const_iterator index = indices.begin(); index != indices.end() && index < indices.begin() + count; ++index) {
    ValType val = get_random_value();
    v[idx].idx = *index;
    v[idx].val = val;
    idx++;
  }
}

int main(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  if (argc < 2) {
    printf("You have to specify the dimension as integer and the density (0,1) as a positive float as arguments!\n");
    exit(-1);
  }
  int rank, worldsize;
  MPI_Comm_size(MPI_COMM_WORLD, &worldsize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  set_seed_random(rank);

  unsigned int dim = atoi(argv[1]);
  float density = atof(argv[2]);
  unsigned cnt = dim * density;

  for(int i = 0; i < EXPERIMENTS; ++i) {

    // Create data
    my_stream *sendbuf;
    if(cnt == dim) {
      sendbuf = (my_stream *)malloc(sizeof(unsigned) + cnt * sizeof(ValType));
      sendbuf->nofitems = cnt;
      create_dense(dim, (ValType *)sendbuf->items);
    } else {
      sendbuf = (my_stream *)malloc(sizeof(unsigned) + cnt * (sizeof(IdxType) + sizeof(ValType)));
      sendbuf->nofitems = cnt;
      create_sparse(dim, cnt, (my_s_item *)sendbuf->items);
    }
    if(rank == OUTPUT_RANK) printf("Data created with %d / %d indices (%f percentage)\n", cnt, dim, density*100);

    // Allocate buffers
    // TODO Maybe allocate only cnt*worldsize if smaller than dim
    size_t maxbytes = sizeof(unsigned) + dim * sizeof(ValType);
    my_stream *recvbuf = (my_stream *)malloc(maxbytes);

    // Dense reduce for result comparison
    my_stream *mys = (my_stream *)malloc(sizeof(unsigned) + dim * sizeof(ValType));
    mys->nofitems = dim;
    ValType *my = (ValType*)(mys->items);
    //ValType *my = (ValType*)malloc(sizeof(ValType)*dim);
    unsigned idx = 0;
    for(size_t i = 0; i < dim; ++i) {
      if(cnt == dim) {
          my[i] = ((ValType *)sendbuf->items)[i];
      } else {
        if(idx < cnt && ((my_s_item *)sendbuf->items)[idx].idx == i) {
          my[i] = ((my_s_item *)sendbuf->items)[idx].val;
          idx++;
        } else {
          my[i] = 0.0;
        }
      }
    }
    ValType *res = (ValType*)malloc(sizeof(ValType)*dim);

#ifdef COMPARE_TO_DENSE 
      int neq, all_neq;
      neq = isDifferent<IdxType, ValType>(my, sendbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("DATA Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif

    for(int j = 0; j < TAKES; ++j) {

      double t_mpi, maxT;

      // ===
      // Run Dense AllReduce
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      MPI_Allreduce(my, res, dim, MPI_VAL_TYPE, MPI_SUM, MPI_COMM_WORLD);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Dense blocking finished [0]:\t\t\t\t%f secs\n", maxT);
      // ===

      // ===
      // Run dense ring AllReduce
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      c_allreduce_dense_ring<IdxType, ValType>(mys, recvbuf, dim, MPI_COMM_WORLD);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Dense Ring AllReduce finished [6]:\t\t\t%f secs - Dense: %s\n", maxT, (recvbuf->nofitems == dim) ? "True" : "False");

#ifdef COMPARE_TO_DENSE 
      neq = isDifferent<IdxType, ValType>(res, recvbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif
      // ===

      // ===
      // Run dense ring_ring AllReduce
 	  int gpuPerNode = 4;
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      c_allreduce_dense_ring_ring<IdxType, ValType>(mys, recvbuf, dim, MPI_COMM_WORLD, gpuPerNode);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Dense Ring_Ring AllReduce finished [7]:\t\t%f secs - Dense: %s\n", maxT, (recvbuf->nofitems == dim) ? "True" : "False");

#ifdef COMPARE_TO_DENSE 
      neq = isDifferent<IdxType, ValType>(res, recvbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif 
      // ===
  
      // ===
      // Run custom AllReduce (RecDoubling)
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      c_allreduce_recdoubling<IdxType, ValType>(sendbuf, recvbuf, dim, MPI_COMM_WORLD);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Sparse blocking (RecDoulbing) finished [2]:\t%f secs - Dense: %s\n", maxT, (recvbuf->nofitems == dim) ? "True" : "False");

#ifdef COMPARE_TO_DENSE 
      neq = isDifferent<IdxType, ValType>(res, recvbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif
      // ===

      // ===
      // Run custom AllReduce (Big - DSAR)
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      c_allreduce_big<IdxType, ValType>(sendbuf, recvbuf, dim, MPI_COMM_WORLD);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Sparse blocking (Big) finished [3]:\t\t\t%f secs - Dense: %s\n", maxT, (recvbuf->nofitems == dim) ? "True" : "False");

#ifdef COMPARE_TO_DENSE 
      neq = isDifferent<IdxType, ValType>(res, recvbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif
      // ===
      
      // ===
      // Run custom AllReduce (Small - SSAR_Spli_AllGather)
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      c_allreduce_small<IdxType, ValType>(sendbuf, recvbuf, dim, MPI_COMM_WORLD);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Sparse blocking (Small) finished [4]:\t\t%f secs - Dense: %s\n", maxT, (recvbuf->nofitems == dim) ? "True" : "False");

#ifdef COMPARE_TO_DENSE 
      neq = isDifferent<IdxType, ValType>(res, recvbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif

      // ===
      // Run ring AllReduce
      MPI_Barrier(MPI_COMM_WORLD);
      t_mpi = -MPI_Wtime();
      c_allreduce_ring<IdxType, ValType>(sendbuf, recvbuf, dim, MPI_COMM_WORLD);
      t_mpi += MPI_Wtime();
      MPI_Reduce(&t_mpi, &maxT, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
      if(rank == OUTPUT_RANK) printf("Sparse Ring AllReduce finished [5]:\t\t\t%f secs - Dense: %s\n", maxT, (recvbuf->nofitems == dim) ? "True" : "False");

#ifdef COMPARE_TO_DENSE 
      neq = isDifferent<IdxType, ValType>(res, recvbuf, dim, rank);
      MPI_Allreduce(&neq, &all_neq, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD); 
      if(rank == OUTPUT_RANK) {
        printf("Equal [With AllReduce Dense]: %s\n", (all_neq > 0) ? "False!" : "True");
      }
#endif
      // ===
	  
      if(rank == OUTPUT_RANK) printf("=================================================================\n");
    }

    free(sendbuf);
    free(recvbuf);
    free(res);
    free(mys);

    if(rank == OUTPUT_RANK) printf("=================================================================\n");
  }

  // finish
  MPI_Finalize();

  return 0;
}
