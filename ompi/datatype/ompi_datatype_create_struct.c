/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2013 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009      Sun Microsystems, Inc. All rights reserved.
 * Copyright (c) 2009      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include <stddef.h>

#include "ompi/datatype/ompi_datatype.h"

static ompi_datatype_create_struct_hook_fn_t create_struct_hook = NULL;
int32_t ompi_datatype_create_struct_hook_register(ompi_datatype_create_struct_hook_fn_t hook)
{
    create_struct_hook = hook;
    return OMPI_SUCCESS;
}

int32_t ompi_datatype_create_struct_hook_deregister(ompi_datatype_create_struct_hook_fn_t hook)
{
    create_struct_hook = NULL;
    return OMPI_SUCCESS;
}

static inline int32_t ompi_datatype_create_struct_call_hooks( int count, const int* pBlockLength, const OPAL_PTRDIFF_TYPE* pDisp,
                                                              ompi_datatype_t* const * pTypes, ompi_datatype_t* newType)
{
    if (create_struct_hook)
        create_struct_hook(count, pBlockLength, pDisp, pTypes, newType);
    return OMPI_SUCCESS;
}

int32_t ompi_datatype_create_struct( int count, const int* pBlockLength, const OPAL_PTRDIFF_TYPE* pDisp,
                                     ompi_datatype_t* const * pTypes, ompi_datatype_t** newType )
{
    int i;
    OPAL_PTRDIFF_TYPE disp = 0, endto, lastExtent, lastDisp;
    int lastBlock;
    ompi_datatype_t *pdt, *lastType;

    if( 0 == count ) {
        *newType = ompi_datatype_create( 0 );
        ompi_datatype_add( *newType, &ompi_mpi_datatype_null.dt, 0, 0, 0);
        return OMPI_SUCCESS;
    }

    /* if we compute the total number of elements before we can
     * avoid increasing the size of the desc array often.
     */
    lastType = (ompi_datatype_t*)pTypes[0];
    lastBlock = pBlockLength[0];
    lastExtent = lastType->super.ub - lastType->super.lb;
    lastDisp = pDisp[0];
    endto = pDisp[0] + lastExtent * lastBlock;

    for( i = 1; i < count; i++ ) {
        if( (pTypes[i] == lastType) && (pDisp[i] == endto) ) {
            lastBlock += pBlockLength[i];
            endto = lastDisp + lastBlock * lastExtent;
        } else {
            disp += lastType->super.desc.used;
            if( lastBlock > 1 ) disp += 2;
            lastType = (ompi_datatype_t*)pTypes[i];
            lastExtent = lastType->super.ub - lastType->super.lb;
            lastBlock = pBlockLength[i];
            lastDisp = pDisp[i];
            endto = lastDisp + lastExtent * lastBlock;
        }
    }
    disp += lastType->super.desc.used;
    if( lastBlock != 1 ) disp += 2;

    lastType = (ompi_datatype_t*)pTypes[0];
    lastBlock = pBlockLength[0];
    lastExtent = lastType->super.ub - lastType->super.lb;
    lastDisp = pDisp[0];
    endto = pDisp[0] + lastExtent * lastBlock;

    pdt = ompi_datatype_create( (int32_t)disp );

    /* Do again the same loop but now add the elements */
    for( i = 1; i < count; i++ ) {
        if( (pTypes[i] == lastType) && (pDisp[i] == endto) ) {
            lastBlock += pBlockLength[i];
            endto = lastDisp + lastBlock * lastExtent;
        } else {
            ompi_datatype_add( pdt, lastType, lastBlock, lastDisp, lastExtent );
            lastType = (ompi_datatype_t*)pTypes[i];
            lastExtent = lastType->super.ub - lastType->super.lb;
            lastBlock = pBlockLength[i];
            lastDisp = pDisp[i];
            endto = lastDisp + lastExtent * lastBlock;
        }
    }
    ompi_datatype_add( pdt, lastType, lastBlock, lastDisp, lastExtent );
    ompi_datatype_create_struct_call_hooks(count, pBlockLength, pDisp, pTypes, pdt);
     *newType = pdt;
    return OMPI_SUCCESS;
}
