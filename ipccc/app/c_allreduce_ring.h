#pragma once

#include "c_common.h"

/*
 * Sparse AllReduce following a recoursive doubling like algorithm
 */
template<class IdxType, class ValType> int c_allreduce_ring(const struct stream *sendbuf, struct stream *recvbuf, unsigned dim, MPI_Comm comm) {

  int r, p;
  MPI_Comm_size(comm, &p);
  MPI_Comm_rank(comm, &r);
	double t_mpi1, t_mpi, computeTime, interTime, initTime; 
	computeTime = 0;
	interTime=0;
	initTime = 0;
	t_mpi = -MPI_Wtime();

  if(p == 1) {
    memcpy(recvbuf, sendbuf, countBytes<IdxType, ValType>(sendbuf, dim));
    return MPI_SUCCESS;
  }

  int i; /* runner */
  int segsize, *segsizes; /* segment sizes and offsets per segment (number of segments == number of nodes */
  int speer, rpeer; /* send and recvpeer */
  //int mycount; /* temporary */
  segsizes = (int*)malloc(sizeof(int)*p);
  segsize = dim/p; /* size of the segments */
  int maxsegsize = dim - ((p-1)*segsize);
  //if(dim%p != 0) segsize++;
  //mycount = dim;
  for(i = 0; i<p;i++) {
    //mycount -= segsize;
    segsizes[i] = segsize;
    //if(mycount < 0) {
    //  segsizes[i] = segsize+mycount;
    //  mycount = 0;
    //}
    //if(!r) printf("count: %i, (%i) size: %i, offset: %i\n", count, i, segsizes[i], segoffsets[i]);
  }
  segsizes[p-1] = maxsegsize;

  char *buf = (char *) malloc(p * sizeof(unsigned) + (p * maxsegsize * sizeof(ValType)));
  struct stream* splits[p];
  struct stream* recvsplit = (struct stream*)malloc(sizeof(unsigned) + maxsegsize * sizeof(ValType));
  struct stream* tmpbuf = (struct stream*)malloc(sizeof(unsigned) + maxsegsize * sizeof(ValType));
  struct stream* ptrForDelete1 = recvsplit;
  struct stream* ptrForDelete2 = tmpbuf;
  for(i = 0; i < p; ++i) {
    splits[i] = (struct stream *)(buf + i * (sizeof(unsigned) + maxsegsize * sizeof(ValType)));
  }
  split_stream<IdxType, ValType>(sendbuf, splits, dim, p);
	t_mpi += MPI_Wtime();
	initTime += t_mpi;
	t_mpi = -MPI_Wtime();
	
  /* reduce peers */
  speer = (r+1)%p;
  rpeer = (r-1+p)%p;

  struct stream* tmpptr = NULL;

  int round = 0;
  /* first p-1 rounds are reductions */
  do {
    int selement = (r+1-round + 2*p /*2*p avoids negative mod*/)%p; /* the element I am sending */
    int relement = (r-round + 2*p /*2*p avoids negative mod*/)%p; /* the element that I receive from my neighbor */

    /* first message come out of sendbuf */
    MPI_Sendrecv(splits[selement], countBytes<IdxType, ValType>(splits[selement], segsizes[selement]), MPI_BYTE, speer, 1, recvsplit, sizeof(ValType) * maxsegsize + sizeof(unsigned), MPI_BYTE, rpeer, 1, comm, MPI_STATUS_IGNORE);
    //printf("[%i] round %i - sending %i\n", r, round, selement);
    //printf("[%i] round %i - receiving %i\n", r, round, relement);
	
	t_mpi1 = -MPI_Wtime();
    tmpptr = sum_into_stream<IdxType, ValType>(splits[relement], recvsplit, tmpbuf, segsizes[relement], false);
    if(tmpptr == recvsplit) {
      recvsplit = splits[relement];
      splits[relement] = tmpptr;
    } else if(tmpptr == tmpbuf) {
      tmpbuf = splits[relement];
      splits[relement] = tmpptr;
    }
	t_mpi1 += MPI_Wtime();
	computeTime += t_mpi1;
    //printf("[%i] round %i - reducing %i\n", r, round, relement);

    round++;
  } while(round < p-1);

  do {
    int selement = (r+1-round + 2*p /*2*p avoids negative mod*/)%p; /* the element I am sending */
    int relement = (r-round + 2*p /*2*p avoids negative mod*/)%p; /* the element that I receive from my neighbor */

    //printf("[%i] round %i receiving %i sending %i\n", r, round, relement, selement);
    MPI_Sendrecv(splits[selement], countBytes<IdxType, ValType>(splits[selement], segsizes[selement]), MPI_BYTE, speer, 1, splits[relement], sizeof(ValType) * maxsegsize + sizeof(unsigned), MPI_BYTE, rpeer, 1, comm, MPI_STATUS_IGNORE);
    round++;  
  } while (round < 2*p-2);

  // Add into 
 	t_mpi += MPI_Wtime();
	interTime += t_mpi;
	t_mpi1 = -MPI_Wtime();
  unsigned overall = 0;
  for(i = 0; i < p; ++i) {
    overall += splits[i]->nofitems;
  }

  if (overall * (sizeof(IdxType) + sizeof(ValType)) >= dim * sizeof(ValType)) {
    recvbuf->nofitems = dim;
    ValType * result = (ValType *)recvbuf->items;
#pragma omp simd 
    for(size_t i = 0; i < dim; ++i) {
      result[i] = 0.0;
    }
    unsigned offset = 0;
    for(i = 0; i < p; ++i) {
      if((int)splits[i]->nofitems == segsizes[i]) {
        // Dense
        for(int j = 0; j < segsizes[i]; ++j) {
          result[offset + j] = ((ValType *)splits[i]->items)[j];
        }
      } else {
        // Sparse
        const struct s_item<IdxType, ValType> *values = (const struct s_item<IdxType, ValType> *)splits[i]->items;
        for(unsigned j = 0; j < splits[i]->nofitems; ++j) {
          result[offset + values[j].idx] = values[j].val;
        }
      }
      offset += segsize;
    }
  } else {
    recvbuf->nofitems = overall;
    struct s_item<IdxType, ValType> *result = (struct s_item<IdxType, ValType> *)recvbuf->items;
    int idx = 0;
    unsigned offset = 0;
    for(i = 0; i < p; ++i) {
      if((int)splits[i]->nofitems == segsizes[i]) {
        // Dense
        for(int j = 0; j < segsizes[i]; ++j) {
          result[idx].idx = j + offset;
          result[idx].val = ((ValType *)splits[i]->items)[j];
          idx++;
        }
      } else {
        // Sparse
        const struct s_item<IdxType, ValType> *values = (const struct s_item<IdxType, ValType> *)splits[i]->items;
        for(unsigned j = 0; j < splits[i]->nofitems; ++j) {
          result[idx].idx = values[j].idx + offset;
          result[idx].val = values[j].val;
          idx++;
        }
      }
      offset += segsize;
    }
  }
 	t_mpi1 += MPI_Wtime();
	computeTime += t_mpi1;

  free(buf);
  free(ptrForDelete1);
  free(ptrForDelete2);
  free(segsizes);
    if(r == 0) {
		printf("\t\tSparse Ring Compute time: \t%f\tsecs\n", computeTime);
		printf("\t\tSparse Ring Inter time: \t%f\tsecs\n", interTime);
		printf("\t\tSparse Ring Ring Init time: \t%f\tsecs\n", initTime);
	}
  return MPI_SUCCESS;
}
