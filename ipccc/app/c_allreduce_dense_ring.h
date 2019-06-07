#pragma once

#include "c_common.h"

/*
 * Sparse AllReduce following a recoursive doubling like algorithm
 */
template<class IdxType, class ValType> int c_allreduce_dense_ring(const struct stream *sendbuf, struct stream *recvbuf, unsigned dim, MPI_Comm comm) {

  int r, p;
  MPI_Comm_size(comm, &p);
  MPI_Comm_rank(comm, &r);

  if(p == 1) {
    memcpy(recvbuf, sendbuf, countBytes<IdxType, ValType>(sendbuf, dim));
    return MPI_SUCCESS;
  }

  assert(sendbuf->nofitems == dim);

  int i; /* runner */
  int segsize, *segsizes, *segoffsets; /* segment sizes and offsets per segment (number of segments == number of nodes */
  int speer, rpeer; /* send and recvpeer */
  int mycount; /* temporary */
  segsizes = (int*)malloc(sizeof(int)*p);
  segoffsets = (int*)malloc(sizeof(int)*p);
  segsize = dim/p; /* size of the segments */
  if(dim%p != 0) segsize++;
  mycount = dim;
  segoffsets[0] = 0;
  for(i = 0; i<p;i++) {
    mycount -= segsize;
    segsizes[i] = segsize;
    if(mycount < 0) {
      segsizes[i] = segsize+mycount;
      mycount = 0;
    }
    if(i) segoffsets[i] = segoffsets[i-1] + segsizes[i-1];
    //if(!r) printf("count: %i, (%i) size: %i, offset: %i\n", count, i, segsizes[i], segoffsets[i]);
  }

  /* reduce peers */
  speer = (r+1)%p;
  rpeer = (r-1+p)%p;

  ValType *snd = (ValType*)sendbuf->items;
  ValType *rcv = (ValType*)recvbuf->items;
  recvbuf->nofitems = dim;

  int round = 0;
  /* first p-1 rounds are reductions */
  do {
    int selement = (r+1-round + 2*p /*2*p avoids negative mod*/)%p; /* the element I am sending */
    int soffset = segoffsets[selement];
    int relement = (r-round + 2*p /*2*p avoids negative mod*/)%p; /* the element that I receive from my neighbor */
    int roffset = segoffsets[relement];

    /* first message come out of sendbuf */
    if(round == 0) {
      MPI_Sendrecv(snd+soffset, sizeof(ValType) * segsizes[selement], MPI_BYTE, speer, 1, rcv+roffset, sizeof(ValType) * segsizes[relement], MPI_BYTE, rpeer, 1, comm, MPI_STATUS_IGNORE);
    } else {
      MPI_Sendrecv(rcv+soffset, sizeof(ValType) * segsizes[selement], MPI_BYTE, speer, 1, rcv+roffset, sizeof(ValType) * segsizes[relement], MPI_BYTE, rpeer, 1, comm, MPI_STATUS_IGNORE);
    }
    //printf("[%i] round %i - sending %i\n", r, round, selement);
    //printf("[%i] round %i - receiving %i\n", r, round, relement);

    //printf("[%i] round %i - reducing %i\n", r, round, relement);
    for(i = 0; i < segsizes[relement]; ++i) {
      (rcv+roffset)[i] = (rcv+roffset)[i] + (snd+roffset)[i];
    }

    round++;
  } while(round < p-1);

  do {
    int selement = (r+1-round + 2*p /*2*p avoids negative mod*/)%p; /* the element I am sending */
    int soffset = segoffsets[selement];
    int relement = (r-round + 2*p /*2*p avoids negative mod*/)%p; /* the element that I receive from my neighbor */
    int roffset = segoffsets[relement];

    //printf("[%i] round %i receiving %i sending %i\n", r, round, relement, selement);
    MPI_Sendrecv(rcv+soffset, sizeof(ValType) * segsizes[selement], MPI_BYTE, speer, 1, rcv+roffset, sizeof(ValType) * segsizes[relement], MPI_BYTE, rpeer, 1, comm, MPI_STATUS_IGNORE);
    round++;  
  } while (round < 2*p-2);

  return MPI_SUCCESS;
}
