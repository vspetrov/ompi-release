/*
 * Copyright (c) 2006      The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2006      The Technical University of Chemnitz. All
 *                         rights reserved.
 * Copyright (c) 2016      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */
#include "nbc_internal.h"

/* cannot cache schedules because one cannot check locally if the pattern is the same!! */
#undef NBC_CACHE_SCHEDULE

#ifdef NBC_CACHE_SCHEDULE
/* tree comparison function for schedule cache */
int NBC_Ineighbor_alltoall_args_compare(NBC_Ineighbor_alltoall_args *a, NBC_Ineighbor_alltoall_args *b, void *param) {
  if( (a->sbuf == b->sbuf) &&
      (a->scount == b->scount) &&
      (a->stype == b->stype) &&
      (a->rbuf == b->rbuf) &&
      (a->rcount == b->rcount) &&
      (a->rtype == b->rtype) ) {
    return  0;
  }
  if( a->sbuf < b->sbuf ) {
    return -1;
  }
  return +1;
}
#endif

int ompi_coll_libnbc_ineighbor_alltoall(void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                                        int rcount, MPI_Datatype rtype, struct ompi_communicator_t *comm,
                                        ompi_request_t ** request, struct mca_coll_base_module_2_0_0_t *module) {
  int rank, size, res, worldsize;
  MPI_Aint sndext, rcvext;
  NBC_Handle *handle;
  ompi_coll_libnbc_request_t **coll_req = (ompi_coll_libnbc_request_t**) request;
  ompi_coll_libnbc_module_t *libnbc_module = (ompi_coll_libnbc_module_t*) module;

  res = NBC_Init_handle(comm, coll_req, libnbc_module);
  handle = *coll_req;
  if(res != NBC_OK) { printf("Error in NBC_Init_handle(%i)\n", res); return res; }
  res = MPI_Comm_size(comm, &size);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Comm_size() (%i)\n", res); return res; }
  res = MPI_Comm_size(MPI_COMM_WORLD, &worldsize);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Comm_size() (%i)\n", res); return res; }
  res = MPI_Comm_rank(comm, &rank);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Comm_rank() (%i)\n", res); return res; }

  res = MPI_Type_extent(stype, &sndext);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Type_extent() (%i)\n", res); return res; }
  res = MPI_Type_extent(rtype, &rcvext);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Type_extent() (%i)\n", res); return res; }

  NBC_Schedule *schedule;
#ifdef NBC_CACHE_SCHEDULE
  NBC_Ineighbor_alltoall_args *args, *found, search;
#endif

  handle->tmpbuf=NULL;

#ifdef NBC_CACHE_SCHEDULE
  /* search schedule in communicator specific tree */
  search.sbuf=sbuf;
  search.scount=scount;
  search.stype=stype;
  search.rbuf=rbuf;
  search.rcount=rcount;
  search.rtype=rtype;
  found = (NBC_Ineighbor_alltoall_args*)hb_tree_search((hb_tree*)handle->comminfo->NBC_Dict[NBC_NEIGHBOR_ALLTOALL], &search);
  if(found == NULL) {
#endif
    schedule = (NBC_Schedule*)malloc(sizeof(NBC_Schedule));

    res = NBC_Sched_create(schedule);
    if(res != NBC_OK) { printf("Error in NBC_Sched_create, res = %i\n", res); return res; }

    {
      int indegree, outdegree, weighted, *srcs, *dsts, i;
      res = NBC_Comm_neighbors_count(comm, &indegree, &outdegree, &weighted);
      if(res != NBC_OK) return res;

      srcs = indegree ? (int*)malloc(sizeof(int)*indegree) : NULL;
      dsts = outdegree ? (int*)malloc(sizeof(int)*outdegree) : NULL;

      res = NBC_Comm_neighbors(comm, indegree, srcs, MPI_UNWEIGHTED, outdegree, dsts, MPI_UNWEIGHTED);
      if(res != NBC_OK) return res;

      /* simply loop over neighbors and post send/recv operations */
      for(i = 0; i < indegree; i++) {
        if (MPI_PROC_NULL != srcs[i]) {
          res = NBC_Sched_recv((char*)rbuf+i*rcount*rcvext, false, rcount, rtype, srcs[i], schedule);
          if (NBC_OK != res) { printf("Error in NBC_Sched_recv() (%i)\n", res); return res; }
        }
      }
      for(i = 0; i < outdegree; i++) {
        if (MPI_PROC_NULL != dsts[i]) {
          res = NBC_Sched_send((char*)sbuf+i*scount*sndext, false, scount, stype, dsts[i], schedule);
          if (NBC_OK != res) { printf("Error in NBC_Sched_send() (%i)\n", res); return res; }
        }
      }
    }

    res = NBC_Sched_commit(schedule);
    if (NBC_OK != res) { printf("Error in NBC_Sched_commit() (%i)\n", res); return res; }
#ifdef NBC_CACHE_SCHEDULE
    /* save schedule to tree */
    args = (NBC_Ineighbor_alltoall_args*)malloc(sizeof(NBC_Ineighbor_alltoall_args));
    args->sbuf=sbuf;
    args->scount=scount;
    args->stype=stype;
    args->rbuf=rbuf;
    args->rcount=rcount;
    args->rtype=rtype;
    args->schedule=schedule;
    res = hb_tree_insert ((hb_tree*)handle->comminfo->NBC_Dict[NBC_NEIGHBOR_ALLTOALL], args, args, 0);
    if(res != 0) printf("error in dict_insert() (%i)\n", res);
    /* increase number of elements for A2A */
    if(++handle->comminfo->NBC_Dict_size[NBC_NEIGHBOR_ALLTOALL] > NBC_SCHED_DICT_UPPER) {
      NBC_SchedCache_dictwipe((hb_tree*)handle->comminfo->NBC_Dict[NBC_NEIGHBOR_ALLTOALL], &handle->comminfo->NBC_Dict_size[NBC_NEIGHBOR_ALLTOALL]);
    }
  } else {
    /* found schedule */
    schedule=found->schedule;
  }
#endif

  res = NBC_Start(handle, schedule);
  if (NBC_OK != res) { printf("Error in NBC_Start() (%i)\n", res); return res; }

  return NBC_OK;
}
