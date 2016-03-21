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

/* Open questions ...
 *  - how to improuve the handling of these vectors (creating a temporary datatype
 *    can be ONLY a initial solution.
 *
 */

static opal_list_t _create_vector_hooks;
static int _vector_hooks_initialized = 0;

typedef struct create_vector_hook_item_t {
    opal_list_item_t super;
    ompi_datatype_create_vector_hook_fn_t hook;
} create_vector_hook_item_t;

OBJ_CLASS_DECLARATION(create_vector_hook_item_t);
OBJ_CLASS_INSTANCE(create_vector_hook_item_t, opal_list_item_t, NULL, NULL);

int32_t ompi_datatype_create_vector_hook_register(ompi_datatype_create_vector_hook_fn_t hook)
{
    create_vector_hook_item_t *hook_item;
    if (!_vector_hooks_initialized) {
        OBJ_CONSTRUCT(&_create_vector_hooks, opal_list_t);
        _vector_hooks_initialized = 1;
    }
    hook_item = OBJ_NEW(create_vector_hook_item_t);
    hook_item->hook = hook;
    opal_list_append(&_create_vector_hooks,
                     (opal_list_item_t *)hook_item);
    return OMPI_SUCCESS;
}

int32_t ompi_datatype_create_vector_hook_deregister(ompi_datatype_create_vector_hook_fn_t hook)
{
    opal_list_item_t *item, *to_dereg = NULL;
    for (item = opal_list_get_first(&_create_vector_hooks);
         item && (item != opal_list_get_end(&_create_vector_hooks));
         item = opal_list_get_next(item)) {
        if (((create_vector_hook_item_t *)item)->hook == hook) {
            to_dereg = item;
            break;
        }
    }
    if (to_dereg) {
        opal_list_remove_item(&_create_vector_hooks, to_dereg);
        OBJ_RELEASE(to_dereg);
    }

    if (opal_list_is_empty(&_create_vector_hooks)) {
        OBJ_DESTRUCT(&_create_vector_hooks);
        _vector_hooks_initialized = 0;
    }
    return OMPI_SUCCESS;
}

static inline int32_t ompi_datatype_create_vector_call_hooks( int count, int bLength, int stride,
                                                              const ompi_datatype_t* oldType, ompi_datatype_t* newType )
{
    opal_list_item_t *item;
    for (item = opal_list_get_first(&_create_vector_hooks);
         item && (item != opal_list_get_end(&_create_vector_hooks));
         item = opal_list_get_next(item)) {
        ((create_vector_hook_item_t *)item)->hook(count, bLength, stride, oldType, newType);
    }

    return OMPI_SUCCESS;
}

int32_t ompi_datatype_create_vector( int count, int bLength, int stride,
                                     const ompi_datatype_t* oldType, ompi_datatype_t** newType )
{
    ompi_datatype_t *pTempData, *pData;
    OPAL_PTRDIFF_TYPE extent = oldType->super.ub - oldType->super.lb;


    if( 0 == count ) {
        *newType = ompi_datatype_create( 0 );
        ompi_datatype_add( *newType, &ompi_mpi_datatype_null.dt, 0, 0, 0);
        return OMPI_SUCCESS;
    }

    pData = ompi_datatype_create( oldType->super.desc.used + 2 );
    if( (bLength == stride) || (1 >= count) ) {  /* the elements are contiguous */
        ompi_datatype_add( pData, oldType, count * bLength, 0, extent );
    } else {
        if( 1 == bLength ) {
            ompi_datatype_add( pData, oldType, count, 0, extent * stride );
        } else {
            ompi_datatype_add( pData, oldType, bLength, 0, extent );
            pTempData = pData;
            pData = ompi_datatype_create( oldType->super.desc.used + 2 + 2 );
            ompi_datatype_add( pData, pTempData, count, 0, extent * stride );
            OBJ_RELEASE( pTempData );
        }
    }

    ompi_datatype_create_vector_call_hooks(count, bLength, stride, oldType, pData);
    *newType = pData;
    return OMPI_SUCCESS;
}


int32_t ompi_datatype_create_hvector( int count, int bLength, OPAL_PTRDIFF_TYPE stride,
                                      const ompi_datatype_t* oldType, ompi_datatype_t** newType )
{
    ompi_datatype_t *pTempData, *pData;
    OPAL_PTRDIFF_TYPE extent = oldType->super.ub - oldType->super.lb;

    if( 0 == count ) {
        *newType = ompi_datatype_create( 0 );
        ompi_datatype_add( *newType, &ompi_mpi_datatype_null.dt, 0, 0, 0);
        return OMPI_SUCCESS;
    }

    pTempData = ompi_datatype_create( oldType->super.desc.used + 2 );
    if( ((extent * bLength) == stride) || (1 >= count) ) {  /* contiguous */
        pData = pTempData;
        ompi_datatype_add( pData, oldType, count * bLength, 0, extent );
    } else {
        if( 1 == bLength ) {
            pData = pTempData;
            ompi_datatype_add( pData, oldType, count, 0, stride );
        } else {
            ompi_datatype_add( pTempData, oldType, bLength, 0, extent );
            pData = ompi_datatype_create( oldType->super.desc.used + 2 + 2 );
            ompi_datatype_add( pData, pTempData, count, 0, stride );
            OBJ_RELEASE( pTempData );
        }
    }
     *newType = pData;
    return OMPI_SUCCESS;
}
