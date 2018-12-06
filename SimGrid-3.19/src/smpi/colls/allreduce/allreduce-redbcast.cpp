/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
namespace simgrid{
namespace smpi{
int Coll_allreduce_redbcast::allreduce(void *buf, void *buf2, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPI_Comm comm)
{
	int size, rank;
	size = comm->size(); 
	rank = comm->rank();
	XBT_WARN("[NNNN] [%d] Start function",comm->rank());
	XBT_WARN("[NNNN] [%d] Start algorithm",rank);
	XBT_WARN("[NNNN] [%d] reduce inter-communication",rank);
	Colls::reduce(buf, buf2, count, datatype, op, 0, comm);
	XBT_WARN("[NNNN] [%d] broadcast inter-communication",rank);
	Colls::bcast(buf2, count, datatype, 0, comm);
	XBT_WARN("[NNNN] [%d] Finish algorithm",rank);	
	return MPI_SUCCESS;
}
}
}
