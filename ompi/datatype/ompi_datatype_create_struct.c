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
#include "opal/class/opal_list.h"
#include "ompi/datatype/ompi_datatype.h"

static opal_list_t _create_struct_hooks;
static int _struct_hooks_initialized = 0;

typedef struct create_struct_hook_item_t {
    opal_list_item_t super;
    ompi_datatype_create_struct_hook_fn_t hook;
} create_struct_hook_item_t;

OBJ_CLASS_DECLARATION(create_struct_hook_item_t);
OBJ_CLASS_INSTANCE(create_struct_hook_item_t, opal_list_item_t, NULL, NULL);

int32_t ompi_datatype_create_struct_hook_register(ompi_datatype_create_struct_hook_fn_t hook)
{
    create_struct_hook_item_t *hook_item;
    if (!_struct_hooks_initialized) {
        OBJ_CONSTRUCT(&_create_struct_hooks, opal_list_t);
        _struct_hooks_initialized = 1;
    }
    hook_item = OBJ_NEW(create_struct_hook_item_t);
    hook_item->hook = hook;
    opal_list_append(&_create_struct_hooks,
                     (opal_list_item_t *)hook_item);
    return OMPI_SUCCESS;
}

int32_t ompi_datatype_create_struct_hook_deregister(ompi_datatype_create_struct_hook_fn_t hook)
{
    opal_list_item_t *item, *to_dereg = NULL;
    for (item = opal_list_get_first(&_create_struct_hooks);
         item && (item != opal_list_get_end(&_create_struct_hooks));
         item = opal_list_get_next(item)) {
        if (((create_struct_hook_item_t *)item)->hook == hook) {
            to_dereg = item;
            break;
        }
    }
    if (to_dereg) {
        opal_list_remove_item(&_create_struct_hooks, to_dereg);
        OBJ_RELEASE(to_dereg);
    }

    if (opal_list_is_empty(&_create_struct_hooks)) {
        OBJ_DESTRUCT(&_create_struct_hooks);
        _struct_hooks_initialized = 0;
    }
    return OMPI_SUCCESS;
}

static inline int32_t ompi_datatype_create_struct_call_hooks( int count, const int* pBlockLength, const OPAL_PTRDIFF_TYPE* pDisp,
                                                              ompi_datatype_t* const * pTypes, ompi_datatype_t* newType)
{
    opal_list_item_t *item;
    for (item = opal_list_get_first(&_create_struct_hooks);
         item && (item != opal_list_get_end(&_create_struct_hooks));
         item = opal_list_get_next(item)) {
        ((create_struct_hook_item_t *)item)->hook(count, pBlockLength, pDisp, pTypes, newType);
    }

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
