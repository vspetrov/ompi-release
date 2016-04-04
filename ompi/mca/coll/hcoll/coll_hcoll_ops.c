/**
  Copyright (c) 2011      Mellanox Technologies. All rights reserved.
  Copyright (c) 2015      Research Organization for Information Science
                          and Technology (RIST). All rights reserved.
  $COPYRIGHT$

  Additional copyrights may follow

  $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"
#include "coll_hcoll.h"
#include "hcoll/api/hcoll_constants.h"
#include "coll_hcoll_dtypes.h"
#include "hcoll/api/hcoll_dte.h"
int mca_coll_hcoll_barrier(struct ompi_communicator_t *comm,
                         mca_coll_base_module_t *module){
    int rc;
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    HCOL_VERBOSE(20,"RUNNING HCOL BARRIER");

    if (OPAL_UNLIKELY(ompi_mpi_finalize_started)) {
        HCOL_VERBOSE(5, "In finalize, reverting to previous barrier");
        goto orig_barrier;
    }
    rc = hcoll_collectives.coll_barrier(hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK BARRIER");
        rc = hcoll_module->previous_barrier(comm,hcoll_module->previous_barrier_module);
    }
    return rc;
orig_barrier:
    return hcoll_module->previous_barrier(comm,hcoll_module->previous_barrier_module);
}

int mca_coll_hcoll_bcast(void *buff, int count,
                        struct ompi_datatype_t *datatype, int root,
                        struct ompi_communicator_t *comm,
                        mca_coll_base_module_t *module)
{
    dte_data_representation_t dtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL BCAST");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    dtype = ompi_dtype_2_dte_dtype(datatype, TRY_FIND_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(dtype) || HCOL_DTE_IS_COMPLEX(dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: %s; calling fallback bcast;",datatype->super.name);
        rc = hcoll_module->previous_bcast(buff,count,datatype,root,
                                         comm,hcoll_module->previous_bcast_module);
        return rc;
    }
    rc = hcoll_collectives.coll_bcast(buff,count,dtype,root,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK BCAST");
        rc = hcoll_module->previous_bcast(buff,count,datatype,root,
                                         comm,hcoll_module->previous_bcast_module);
    }
    return rc;
}

int mca_coll_hcoll_allgather(void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, int rcount,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLGATHER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback allgather;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_allgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             hcoll_module->previous_allgather_module);
        return rc;
    }
    rc = hcoll_collectives.coll_allgather(sbuf,scount,stype,rbuf,rcount,rtype,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLGATHER");
        rc = hcoll_module->previous_allgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             hcoll_module->previous_allgather_module);
    }
    return rc;
}

int mca_coll_hcoll_allgatherv(const void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, const int *rcount,
                            const int *displs,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLGATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback allgatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_allgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             hcoll_module->previous_allgatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_allgatherv((void *)sbuf,scount,stype,rbuf,rcount,displs,rtype,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLGATHERV");
        rc = hcoll_module->previous_allgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             hcoll_module->previous_allgatherv_module);
    }
    return rc;
}

int mca_coll_hcoll_gather(const void *sbuf, int scount,
                          struct ompi_datatype_t *sdtype,
                          void *rbuf, int rcount,
                          struct ompi_datatype_t *rdtype,
                          int root,
                          struct ompi_communicator_t *comm,
                          mca_coll_base_module_t *module){
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL GATHER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback gather;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_gather(sbuf,scount,sdtype,
                                           rbuf,rcount,rdtype,root,
                                           comm,
                                           hcoll_module->previous_allgather_module);
        return rc;
    }
    rc = hcoll_collectives.coll_gather(sbuf,scount,stype,rbuf,rcount,rtype,root,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK GATHER");
        rc = hcoll_module->previous_gather(sbuf,scount,sdtype,
                                              rbuf,rcount,rdtype,root,
                                              comm,
                                              hcoll_module->previous_allgather_module);
    }
    return rc;

}

int mca_coll_hcoll_allreduce(void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLREDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback allreduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_allreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, hcoll_module->previous_allreduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback allreduce;",
                     op->o_name);
        rc = hcoll_module->previous_allreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, hcoll_module->previous_allreduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_allreduce(sbuf,rbuf,count,Dtype,Op,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLREDUCE");
        rc = hcoll_module->previous_allreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, hcoll_module->previous_allreduce_module);
    }
    return rc;
}

int mca_coll_hcoll_reduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            int root,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL REDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback reduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_reduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, hcoll_module->previous_reduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback reduce;",
                     op->o_name);
        rc = hcoll_module->previous_reduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, hcoll_module->previous_reduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_reduce((void *)sbuf,rbuf,count,Dtype,Op,root,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK REDUCE");
        rc = hcoll_module->previous_reduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, hcoll_module->previous_reduce_module);
    }
    return rc;
}

int mca_coll_hcoll_alltoall(const void *sbuf, int scount,
                           struct ompi_datatype_t *sdtype,
                           void* rbuf, int rcount,
                           struct ompi_datatype_t *rdtype,
                           struct ompi_communicator_t *comm,
                           mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLTOALL");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback alltoall;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_alltoall(sbuf,scount,sdtype,
                                            rbuf,rcount,rdtype,
                                            comm,
                                            hcoll_module->previous_alltoall_module);
        return rc;
    }
    rc = hcoll_collectives.coll_alltoall(sbuf,scount,stype,rbuf,rcount,rtype,hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLTOALL");
        rc = hcoll_module->previous_alltoall(sbuf,scount,sdtype,
                                            rbuf,rcount,rdtype,
                                            comm,
                                            hcoll_module->previous_alltoall_module);
    }
    return rc;
}

int mca_coll_hcoll_alltoallv(void *sbuf, int *scounts, int *sdisps,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, int *rcounts, int *rdisps,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL ALLTOALLV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback alltoallv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_alltoallv(sbuf, scounts, sdisps, sdtype,
                                            rbuf, rcounts, rdisps, rdtype,
                                            comm, hcoll_module->previous_alltoallv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_alltoallv(sbuf, scounts, sdisps, stype,
                                            rbuf, rcounts, rdisps, rtype,
                                                hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK ALLTOALLV");
        rc = hcoll_module->previous_alltoallv(sbuf, scounts, sdisps, sdtype,
                                            rbuf, rcounts, rdisps, rdtype,
                                            comm, hcoll_module->previous_alltoallv_module);
    }
    return rc;
}

int mca_coll_hcoll_gatherv(void* sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void* rbuf, int *rcounts, int *displs,
                            struct ompi_datatype_t *rdtype,
                            int root,
                            struct ompi_communicator_t *comm,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL GATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback gatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_gatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, hcoll_module->previous_gatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_gatherv(sbuf,scount,stype,rbuf,rcounts,displs, rtype, root, hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK GATHERV");
        rc = hcoll_module->previous_gatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, hcoll_module->previous_igatherv_module);
    }
    return rc;

}

int mca_coll_hcoll_ibarrier(struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING BARRIER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    rc = hcoll_collectives.coll_ibarrier(hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING BARRIER");
        rc = hcoll_module->previous_ibarrier(comm, request, hcoll_module->previous_ibarrier_module);
    }
    return rc;
}

int mca_coll_hcoll_ibcast(void *buff, int count,
                        struct ompi_datatype_t *datatype, int root,
                        struct ompi_communicator_t *comm,
                        ompi_request_t ** request,
                        mca_coll_base_module_t *module)
{
    dte_data_representation_t dtype;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING BCAST");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    dtype = ompi_dtype_2_dte_dtype(datatype, TRY_FIND_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(dtype) || HCOL_DTE_IS_COMPLEX(dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: %s; calling fallback non-blocking bcast;",datatype->super.name);
        rc = hcoll_module->previous_ibcast(buff,count,datatype,root,
                                         comm, request, hcoll_module->previous_ibcast_module);
        return rc;
    }
    rc = hcoll_collectives.coll_ibcast(buff, count, dtype, root, rt_handle, hcoll_module->hcoll_context);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING BCAST");
        rc = hcoll_module->previous_ibcast(buff,count,datatype,root,
                                         comm, request, hcoll_module->previous_ibcast_module);
    }
    return rc;
}

int mca_coll_hcoll_iallgather(void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, int rcount,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING ALLGATHER");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback non-blocking allgather;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_iallgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgather_module);
        return rc;
    }
    rc = hcoll_collectives.coll_iallgather(sbuf, scount, stype, rbuf, rcount, rtype, hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING ALLGATHER");
        rc = hcoll_module->previous_iallgather(sbuf,scount,sdtype,
                                             rbuf,rcount,rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgather_module);
    }
    return rc;
}

#if HCOLL_API >= HCOLL_VERSION(3,5)
int mca_coll_hcoll_iallgatherv(const void *sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void *rbuf, const int *rcount,
                            const int *displs,
                            struct ompi_datatype_t *rdtype,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING ALLGATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    void **rt_handle = (void **) request;
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback non-blocking allgatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_iallgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_iallgatherv((void *)sbuf,scount,stype,rbuf,rcount,displs,rtype,
            hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
       HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING ALLGATHER");
        rc = hcoll_module->previous_iallgatherv(sbuf,scount,sdtype,
                                             rbuf,rcount,
                                             displs, 
                                             rdtype,
                                             comm,
                                             request,
                                             hcoll_module->previous_iallgatherv_module);
    }
    return rc;
}
#endif
int mca_coll_hcoll_iallreduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING ALLREDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback non-blocking allreduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_iallreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, request, hcoll_module->previous_iallreduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback non-blocking allreduce;",
                     op->o_name);
        rc = hcoll_module->previous_iallreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, request, hcoll_module->previous_iallreduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_iallreduce(sbuf, rbuf, count, Dtype, Op, hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING ALLREDUCE");
        rc = hcoll_module->previous_iallreduce(sbuf,rbuf,
                                             count,dtype,op,
                                             comm, request, hcoll_module->previous_iallreduce_module);
    }
    return rc;
}
#if HCOLL_API >= HCOLL_VERSION(3,5)
int mca_coll_hcoll_ireduce(const void *sbuf, void *rbuf, int count,
                            struct ompi_datatype_t *dtype,
                            struct ompi_op_t *op,
                            int root,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t Dtype;
    hcoll_dte_op_t *Op;
    int rc;
    HCOL_VERBOSE(20,"RUNNING HCOL NON-BLOCKING REDUCE");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    Dtype = ompi_dtype_2_dte_dtype(dtype, NO_DERIVED);
    void **rt_handle = (void**) request;
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(Dtype) || HCOL_DTE_IS_COMPLEX(Dtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: dtype = %s; calling fallback non-blocking reduce;",
                     dtype->super.name);
        rc = hcoll_module->previous_ireduce(sbuf,rbuf,count,dtype,op,
                                             root,
                                             comm, request, 
                                             hcoll_module->previous_ireduce_module);
        return rc;
    }

    Op = ompi_op_2_hcolrte_op(op);
    if (OPAL_UNLIKELY(HCOL_DTE_OP_NULL == Op->id)){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"ompi_op_t is not supported: op = %s; calling fallback non-blocking reduce;",
                     op->o_name);
        rc = hcoll_module->previous_ireduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, request,
                                             hcoll_module->previous_ireduce_module);
        return rc;
    }

    rc = hcoll_collectives.coll_ireduce((void *)sbuf,rbuf,count,Dtype,Op,root,hcoll_module->hcoll_context,rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK NON-BLOCKING REDUCE");
        rc = hcoll_module->previous_ireduce(sbuf,rbuf,
                                             count,dtype,op,
                                             root,
                                             comm, 
                                             request,
                                             hcoll_module->previous_ireduce_module);
    }
    return rc;
}
#endif
int mca_coll_hcoll_igatherv(const void* sbuf, int scount,
                            struct ompi_datatype_t *sdtype,
                            void* rbuf, int *rcounts, int *displs,
                            struct ompi_datatype_t *rdtype,
                            int root,
                            struct ompi_communicator_t *comm,
                            ompi_request_t ** request,
                            mca_coll_base_module_t *module)
{
    dte_data_representation_t stype;
    dte_data_representation_t rtype;
    int rc;
    void** rt_handle;
    HCOL_VERBOSE(20,"RUNNING HCOL IGATHERV");
    mca_coll_hcoll_module_t *hcoll_module = (mca_coll_hcoll_module_t*)module;
    rt_handle = (void**) request;
    stype = ompi_dtype_2_dte_dtype(sdtype, NO_DERIVED);
    rtype = ompi_dtype_2_dte_dtype(rdtype, NO_DERIVED);
    if (OPAL_UNLIKELY((HCOL_DTE_IS_ZERO(stype) || HCOL_DTE_IS_ZERO(rtype)
                        || HCOL_DTE_IS_COMPLEX(stype) || HCOL_DTE_IS_COMPLEX(rtype)))
                        && mca_coll_hcoll_component.hcoll_datatype_fallback){
        /*If we are here then datatype is not simple predefined datatype */
        /*In future we need to add more complex mapping to the dte_data_representation_t */
        /* Now use fallback */
        HCOL_VERBOSE(20,"Ompi_datatype is not supported: sdtype = %s, rdtype = %s; calling fallback igatherv;",
                     sdtype->super.name,
                     rdtype->super.name);
        rc = hcoll_module->previous_igatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, request,
                                           hcoll_module->previous_igatherv_module);
        return rc;
    }
    rc = hcoll_collectives.coll_igatherv(sbuf,scount,stype,rbuf,rcounts,displs, rtype, root, hcoll_module->hcoll_context, rt_handle);
    if (HCOLL_SUCCESS != rc){
        HCOL_VERBOSE(20,"RUNNING FALLBACK IGATHERV");
        rc = hcoll_module->previous_igatherv(sbuf,scount,sdtype,
                                           rbuf, rcounts, displs, rdtype,root,
                                           comm, request,
                                           hcoll_module->previous_igatherv_module);
    }
    return rc;

}


int32_t hcoll_dtype_create_vector_hook( int count, int bLength, int stride,
                                        const ompi_datatype_t* oldType, ompi_datatype_t* newType )
{
    dte_data_representation_t parent_dte, *new_dte;
    parent_dte = ompi_dtype_2_dte_dtype(oldType, TRY_FIND_DERIVED);
    if (HCOL_DTE_IS_ZERO(parent_dte) || HCOL_DTE_IS_COMPLEX(parent_dte)){
        return OMPI_SUCCESS;
    }
    hcoll_dte_create_vector(count, bLength, stride, parent_dte, &new_dte);
    opal_hash_table_set_value_uint32(&mca_coll_hcoll_component.derived_types_map,
                                     newType->id,(void*)new_dte);
    return OMPI_SUCCESS;
}

int32_t hcoll_dtype_create_struct_hook(int count, const int* pBlockLength, const OPAL_PTRDIFF_TYPE* pDisp,
                                       ompi_datatype_t* const* pTypes, ompi_datatype_t* newType )
{
    int i;
    dte_data_representation_t ptype, *new_dte;
    int is_const_length = 1;
    int is_const_stride = 1;
    int is_const_type   = 1;
    OPAL_PTRDIFF_TYPE stride = pDisp[1] - pDisp[0];
    for (i=0; i<count; i++) {
        ptype = ompi_dtype_2_dte_dtype(pTypes[i], TRY_FIND_DERIVED);
        if (HCOL_DTE_IS_ZERO(ptype) || HCOL_DTE_IS_COMPLEX(ptype)){
            /* we found not a simple dtype as an input for the new struct
               dtype, we will not support this for now */
            return OMPI_SUCCESS;
        }
        if (pBlockLength[i] != pBlockLength[0])
            is_const_length = 0;
        if (pTypes[i] != pTypes[0])
            is_const_type = 0;
        if (i > 1 && (pDisp[i]-pDisp[i-1] != stride))
            is_const_stride = 0;
    }
    if (is_const_length && is_const_stride && is_const_type && stride < INT_MAX)
        return hcoll_dtype_create_vector_hook(count,pBlockLength[0],(int)stride,pTypes[0],newType);

    dte_data_representation_t *dte_types = (dte_data_representation_t *)
        malloc(count*sizeof(*dte_types));
    for (i=0; i<count; i++)
        dte_types[i] = ompi_dtype_2_dte_dtype(pTypes[i], TRY_FIND_DERIVED);

    hcoll_dte_create_struct(count, (int*)pBlockLength, (ptrdiff_t*)pDisp, dte_types, &new_dte);
    opal_hash_table_set_value_uint32(&mca_coll_hcoll_component.derived_types_map,
                                     newType->id,(void*)new_dte);

    free(dte_types);
    return OMPI_SUCCESS;
}

int32_t hcoll_dtype_destroy_hook( ompi_datatype_t* dtype )
{
    int ret = OMPI_SUCCESS;
    dte_data_representation_t *dte =
        find_derived_mapping(dtype);
    if (dte) {
        ret = hcoll_dte_destroy(dte);
        opal_hash_table_remove_value_uint32(&mca_coll_hcoll_component.derived_types_map,
                                            dtype->id);
    }

    return ret;
}


int hcoll_alloc_mem_hook(void *addr, size_t len) {
    hcoll_register_mem(addr, len);
    return OMPI_SUCCESS;
}
