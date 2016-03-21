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
#include <string.h>

#include "opal/class/opal_pointer_array.h"
#include "ompi/datatype/ompi_datatype.h"
#include "ompi/attribute/attribute.h"

static opal_list_t _destroy_hooks;
static int _destroy_hooks_initialized = 0;

typedef struct destroy_hook_item_t {
    opal_list_item_t super;
    ompi_datatype_destroy_hook_fn_t hook;
} destroy_hook_item_t;

OBJ_CLASS_DECLARATION(destroy_hook_item_t);
OBJ_CLASS_INSTANCE(destroy_hook_item_t, opal_list_item_t, NULL, NULL);

int32_t ompi_datatype_destroy_hook_register(ompi_datatype_destroy_hook_fn_t hook)
{
    destroy_hook_item_t *hook_item;
    if (!_destroy_hooks_initialized) {
        OBJ_CONSTRUCT(&_destroy_hooks, opal_list_t);
        _destroy_hooks_initialized = 1;
    }
    hook_item = OBJ_NEW(destroy_hook_item_t);
    hook_item->hook = hook;
    opal_list_append(&_destroy_hooks,
                     (opal_list_item_t *)hook_item);
    return OMPI_SUCCESS;
}

int32_t ompi_datatype_destroy_hook_deregister(ompi_datatype_destroy_hook_fn_t hook)
{
    opal_list_item_t *item, *to_dereg = NULL;
    for (item = opal_list_get_first(&_destroy_hooks);
         item && (item != opal_list_get_end(&_destroy_hooks));
         item = opal_list_get_next(item)) {
        if (((destroy_hook_item_t *)item)->hook == hook) {
            to_dereg = item;
            break;
        }
    }
    if (to_dereg) {
        opal_list_remove_item(&_destroy_hooks, to_dereg);
        OBJ_RELEASE(to_dereg);
    }

    if (opal_list_is_empty(&_destroy_hooks)) {
        OBJ_DESTRUCT(&_destroy_hooks);
        _destroy_hooks_initialized = 0;
    }
    return OMPI_SUCCESS;
}

static inline int32_t ompi_datatype_destroy_call_hooks( ompi_datatype_t* type )
{
    opal_list_item_t *item;
    for (item = opal_list_get_first(&_destroy_hooks);
         item && (item != opal_list_get_end(&_destroy_hooks));
         item = opal_list_get_next(item)) {
        ((destroy_hook_item_t *)item)->hook(type);
    }

    return OMPI_SUCCESS;
}

static void __ompi_datatype_allocate( ompi_datatype_t* datatype )
{
    datatype->args               = NULL;
    datatype->d_f_to_c_index     = opal_pointer_array_add(&ompi_datatype_f_to_c_table, datatype);
    /* Later generated datatypes will have their id according to the Fortran ID, as ALL types are registered */
    datatype->id                 = datatype->d_f_to_c_index;
    datatype->d_keyhash          = NULL;
    datatype->name[0]            = '\0';
    datatype->packed_description = NULL;
    datatype->pml_data           = 0;
}

static void __ompi_datatype_release(ompi_datatype_t * datatype)
{
    if( NULL != datatype->args ) {
        ompi_datatype_release_args( datatype );
        datatype->args = NULL;
    }
    if( NULL != datatype->packed_description ) {
        free( datatype->packed_description );
        datatype->packed_description = NULL;
    }
    if( NULL != opal_pointer_array_get_item(&ompi_datatype_f_to_c_table, datatype->d_f_to_c_index) ){
        opal_pointer_array_set_item( &ompi_datatype_f_to_c_table, datatype->d_f_to_c_index, NULL );
    }
    /* any pending attributes ? */
    if (NULL != datatype->d_keyhash) {
        ompi_attr_delete_all( TYPE_ATTR, datatype, datatype->d_keyhash );
        OBJ_RELEASE( datatype->d_keyhash );
    }
    /* make sure the name is set to empty */
    datatype->name[0] = '\0';
}

OBJ_CLASS_INSTANCE(ompi_datatype_t, opal_datatype_t, __ompi_datatype_allocate, __ompi_datatype_release);

ompi_datatype_t * ompi_datatype_create( int32_t expectedSize )
{
    int ret;
    ompi_datatype_t * datatype = (ompi_datatype_t*)OBJ_NEW(ompi_datatype_t);

    ret = opal_datatype_create_desc( &(datatype->super), expectedSize);
    if (OPAL_SUCCESS != ret)
        return NULL;

    return datatype;
}

int32_t ompi_datatype_destroy( ompi_datatype_t** type)
{
    ompi_datatype_t* pData = *type;

    if( ompi_datatype_is_predefined(pData) && (pData->super.super.obj_reference_count <= 1) )
        return OMPI_ERROR;

    ompi_datatype_destroy_call_hooks(pData);

    OBJ_RELEASE(pData);
    *type = NULL;
    return OMPI_SUCCESS;
}

int32_t
ompi_datatype_duplicate( const ompi_datatype_t* oldType, ompi_datatype_t** newType )
{
    ompi_datatype_t * new_ompi_datatype = ompi_datatype_create( oldType->super.desc.used + 2 );

    *newType = new_ompi_datatype;
    if( NULL == new_ompi_datatype ) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    opal_datatype_clone( &oldType->super, &new_ompi_datatype->super);
    /* Strip the predefined flag at the OMPI level. */
    new_ompi_datatype->super.flags &= ~OMPI_DATATYPE_FLAG_PREDEFINED;
    /* By default maintain the relationships related to the old data (such as ops) */
    new_ompi_datatype->id = oldType->id;

    /* Set the keyhash to NULL -- copying attributes is *only* done at
       the top level (specifically, MPI_TYPE_DUP). */
    new_ompi_datatype->d_keyhash = NULL;
    new_ompi_datatype->args = NULL;
    snprintf (new_ompi_datatype->name, MPI_MAX_OBJECT_NAME, "Dup %s",
              oldType->name);

    return OMPI_SUCCESS;
}

