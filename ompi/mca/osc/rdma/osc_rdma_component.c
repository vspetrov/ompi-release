/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2008 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2014 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2006-2008 University of Houston.  All rights reserved.
 * Copyright (c) 2010      Oracle and/or its affiliates.  All rights reserved.
 * Copyright (c) 2012-2013 Sandia National Laboratories.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include <string.h>

#include "osc_rdma.h"
#include "osc_rdma_data_move.h"
#include "osc_rdma_frag.h"
#include "osc_rdma_request.h"

#include "opal/threads/condition.h"
#include "opal/threads/mutex.h"
#include "opal/util/arch.h"
#include "opal/align.h"

#include "ompi/info/info.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/osc/osc.h"
#include "ompi/mca/osc/base/base.h"
#include "ompi/mca/osc/base/osc_base_obj_convert.h"
#include "ompi/mca/btl/btl.h"
#include "ompi/mca/pml/pml.h"

static int component_open(void);
static int component_register(void);
static int component_init(bool enable_progress_threads, bool enable_mpi_threads);
static int component_finalize(void);
static int component_query(struct ompi_win_t *win, void **base, size_t size, int disp_unit,
                           struct ompi_communicator_t *comm, struct ompi_info_t *info,
                           int flavor);
static int component_select(struct ompi_win_t *win, void **base, size_t size, int disp_unit,
                            struct ompi_communicator_t *comm, struct ompi_info_t *info,
                            int flavor, int *model);

ompi_osc_rdma_component_t mca_osc_rdma_component = {
    { /* ompi_osc_base_component_t */
        { /* ompi_base_component_t */
            OMPI_OSC_BASE_VERSION_3_0_0,
            "rdma",
            OMPI_MAJOR_VERSION,  /* MCA component major version */
            OMPI_MINOR_VERSION,  /* MCA component minor version */
            OMPI_RELEASE_VERSION,  /* MCA component release version */
            component_open,
            NULL,
            NULL,
            component_register
        },
        { /* mca_base_component_data */
            /* The component is not checkpoint ready */
            MCA_BASE_METADATA_PARAM_NONE
        },
        component_init,
        component_query,
        component_select,
        component_finalize
    }
};


ompi_osc_rdma_module_t ompi_osc_rdma_module_template = {
    {
        NULL, /* shared_query */

        ompi_osc_rdma_attach,
        ompi_osc_rdma_detach,
        ompi_osc_rdma_free,

        ompi_osc_rdma_put,
        ompi_osc_rdma_get,
        ompi_osc_rdma_accumulate,
        ompi_osc_rdma_compare_and_swap,
        ompi_osc_rdma_fetch_and_op,
        ompi_osc_rdma_get_accumulate,

        ompi_osc_rdma_rput,
        ompi_osc_rdma_rget,
        ompi_osc_rdma_raccumulate,
        ompi_osc_rdma_rget_accumulate,

        ompi_osc_rdma_fence,

        ompi_osc_rdma_start,
        ompi_osc_rdma_complete,
        ompi_osc_rdma_post,
        ompi_osc_rdma_wait,
        ompi_osc_rdma_test,

        ompi_osc_rdma_lock,
        ompi_osc_rdma_unlock,
        ompi_osc_rdma_lock_all,
        ompi_osc_rdma_unlock_all,

        ompi_osc_rdma_sync,
        ompi_osc_rdma_flush,
        ompi_osc_rdma_flush_all,
        ompi_osc_rdma_flush_local,
        ompi_osc_rdma_flush_local_all,

        ompi_osc_rdma_set_info,
        ompi_osc_rdma_get_info
    }
};

bool ompi_osc_rdma_no_locks;

/* look up parameters for configuring this window.  The code first
   looks in the info structure passed by the user, then through mca
   parameters. */
static bool
check_config_value_bool(char *key, ompi_info_t *info)
{
    char *value_string;
    int value_len, ret, flag, param;
    const bool *flag_value;
    bool result;

    ret = ompi_info_get_valuelen(info, key, &value_len, &flag);
    if (OMPI_SUCCESS != ret) goto info_not_found;
    if (flag == 0) goto info_not_found;
    value_len++;

    value_string = (char*)malloc(sizeof(char) * value_len + 1); /* Should malloc 1 char for NUL-termination */
    if (NULL == value_string) goto info_not_found;

    ret = ompi_info_get(info, key, value_len, value_string, &flag);
    if (OMPI_SUCCESS != ret) {
        free(value_string);
        goto info_not_found;
    }
    assert(flag != 0);
    ret = ompi_info_value_to_bool(value_string, &result);
    free(value_string);
    if (OMPI_SUCCESS != ret) goto info_not_found;
    return result;

 info_not_found:
    param = mca_base_var_find("ompi", "osc", "rdma", key);
    if (0 > param) return false;

    ret = mca_base_var_get_value(param, &flag_value, NULL, NULL);
    if (OMPI_SUCCESS != ret) return false;

    return flag_value[0];
}


static int
component_open(void)
{
    return OMPI_SUCCESS;
}


static int
component_register(void)
{
    ompi_osc_rdma_no_locks = false;
    (void) mca_base_component_var_register(&mca_osc_rdma_component.super.osc_version,
                                           "no_locks",
                                           "Enable optimizations available only if MPI_LOCK is "
                                           "not used.  "
                                           "Info key of same name overrides this value.",
                                           MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0,
                                           OPAL_INFO_LVL_9,
                                           MCA_BASE_VAR_SCOPE_READONLY,
                                           &ompi_osc_rdma_no_locks);

    mca_osc_rdma_component.buffer_size = 8192;
    (void) mca_base_component_var_register (&mca_osc_rdma_component.super.osc_version, "buffer_size",
					    "Data transfers smaller than this limit may be coalesced before "
					    "being transferred (default: 8k)", MCA_BASE_VAR_TYPE_UNSIGNED_INT,
					    NULL, 0, 0, OPAL_INFO_LVL_9, MCA_BASE_VAR_SCOPE_READONLY,
					    &mca_osc_rdma_component.buffer_size);

    return OMPI_SUCCESS;
}

static int component_progress (void)
{
    ompi_osc_rdma_pending_t *pending, *next;

    if (0 == opal_list_get_size (&mca_osc_rdma_component.pending_operations)) {
	return 0;
    }

    /* process one incomming request */
    OPAL_THREAD_LOCK(&mca_osc_rdma_component.lock);
    OPAL_LIST_FOREACH_SAFE(pending, next, &mca_osc_rdma_component.pending_operations, ompi_osc_rdma_pending_t) {
	int ret;

	switch (pending->header.base.type) {
	case OMPI_OSC_RDMA_HDR_TYPE_FLUSH_REQ:
	    ret = ompi_osc_rdma_process_flush (pending->module, pending->source,
					       &pending->header.flush);
	    break;
	case OMPI_OSC_RDMA_HDR_TYPE_UNLOCK_REQ:
	    ret = ompi_osc_rdma_process_unlock (pending->module, pending->source,
						&pending->header.unlock);
	    break;
	default:
	    /* shouldn't happen */
	    assert (0);
	    abort ();
	}

	if (OMPI_SUCCESS == ret) {
	    opal_list_remove_item (&mca_osc_rdma_component.pending_operations, &pending->super);
	    OBJ_RELEASE(pending);
	}
    }
    OPAL_THREAD_UNLOCK(&mca_osc_rdma_component.lock);

    return 1;
}

static int
component_init(bool enable_progress_threads,
               bool enable_mpi_threads)
{
    int ret;

    OBJ_CONSTRUCT(&mca_osc_rdma_component.lock, opal_mutex_t);
    OBJ_CONSTRUCT(&mca_osc_rdma_component.pending_operations, opal_list_t);

    OBJ_CONSTRUCT(&mca_osc_rdma_component.modules,
                  opal_hash_table_t);
    opal_hash_table_init(&mca_osc_rdma_component.modules, 2);

    mca_osc_rdma_component.progress_enable = false;
    mca_osc_rdma_component.module_count = 0;

    OBJ_CONSTRUCT(&mca_osc_rdma_component.frags, opal_free_list_t);
    ret = opal_free_list_init(&mca_osc_rdma_component.frags,
			      sizeof(ompi_osc_rdma_frag_t),
			      OBJ_CLASS(ompi_osc_rdma_frag_t),
			      1, -1, 1);
    if (OMPI_SUCCESS != ret) {
	opal_output_verbose(1, ompi_osc_base_framework.framework_output,
			    "%s:%d: ompi_free_list_init failed: %d",
			    __FILE__, __LINE__, ret);
	return ret;
    }

    OBJ_CONSTRUCT(&mca_osc_rdma_component.requests, ompi_free_list_t);
    ret = ompi_free_list_init(&mca_osc_rdma_component.requests,
                              sizeof(ompi_osc_rdma_request_t),
                              OBJ_CLASS(ompi_osc_rdma_request_t),
                              0, -1, 32, NULL);
    if (OMPI_SUCCESS != ret) {
        opal_output_verbose(1, ompi_osc_base_framework.framework_output,
                            "%s:%d: ompi_free_list_init failed: %d\n",
                            __FILE__, __LINE__, ret);
        return ret;
    }

    return ret;
}


int 
component_finalize(void)
{
    size_t num_modules;

    if (mca_osc_rdma_component.progress_enable) {
	opal_progress_unregister (component_progress);
    }

    if (0 != 
        (num_modules = opal_hash_table_get_size(&mca_osc_rdma_component.modules))) {
        opal_output(ompi_osc_base_framework.framework_output,
                    "WARNING: There were %d Windows created but not freed.",
                    (int) num_modules);
    }

    OBJ_DESTRUCT(&mca_osc_rdma_component.frags);
    OBJ_DESTRUCT(&mca_osc_rdma_component.modules);
    OBJ_DESTRUCT(&mca_osc_rdma_component.lock);
    OBJ_DESTRUCT(&mca_osc_rdma_component.requests);
    OBJ_DESTRUCT(&mca_osc_rdma_component.pending_operations);

    return OMPI_SUCCESS;
}


static int
component_query(struct ompi_win_t *win, void **base, size_t size, int disp_unit,
                struct ompi_communicator_t *comm, struct ompi_info_t *info,
                int flavor)
{
    if (MPI_WIN_FLAVOR_SHARED == flavor) return -1;

    return 10;
}


static int
component_select(struct ompi_win_t *win, void **base, size_t size, int disp_unit,
                 struct ompi_communicator_t *comm, struct ompi_info_t *info,
                 int flavor, int *model)
{
    ompi_osc_rdma_module_t *module = NULL;
    int ret;
    char *name;
    bool no_locks = false;

    /* We don't support shared windows; that's for the sm onesided
       component */
    if (MPI_WIN_FLAVOR_SHARED == flavor) return OMPI_ERR_NOT_SUPPORTED;

    if (check_config_value_bool("no_locks", info)) {
        no_locks = true;
	ompi_osc_rdma_no_locks = true;
    }

    /* create module structure with all fields initialized to zero */
    module = (ompi_osc_rdma_module_t*)
        calloc(1, sizeof(ompi_osc_rdma_module_t));
    if (NULL == module) return OMPI_ERR_TEMP_OUT_OF_RESOURCE;

    /* fill in the function pointer part */
    memcpy(module, &ompi_osc_rdma_module_template, 
           sizeof(ompi_osc_base_module_t));

    /* initialize the objects, so that always free in cleanup */
    OBJ_CONSTRUCT(&module->lock, opal_mutex_t);
    OBJ_CONSTRUCT(&module->cond, opal_condition_t);
    OBJ_CONSTRUCT(&module->acc_lock, opal_mutex_t);
    OBJ_CONSTRUCT(&module->queued_frags, opal_list_t);
    OBJ_CONSTRUCT(&module->locks_pending, opal_list_t);
    OBJ_CONSTRUCT(&module->outstanding_locks, opal_list_t);
    OBJ_CONSTRUCT(&module->request_gc, opal_list_t);
    OBJ_CONSTRUCT(&module->pending_acc, opal_list_t);

    /* options */
    /* FIX ME: should actually check this value... */
#if 1
    module->accumulate_ordering = 1;
#else
    ompi_osc_base_config_value_equal("accumulate_ordering", info, "none");
#endif

    /* fill in our part */
    if (MPI_WIN_FLAVOR_ALLOCATE == flavor && size) {
	module->free_after = *base = malloc(size);
	if (NULL == *base) {
	    ret = OMPI_ERR_TEMP_OUT_OF_RESOURCE;
	    goto cleanup;
	}
    }

    /* in the dynamic case base is MPI_BOTTOM */
    if (MPI_WIN_FLAVOR_DYNAMIC != flavor) {
	module->baseptr = *base;
    }

    ret = ompi_comm_dup(comm, &module->comm);
    if (OMPI_SUCCESS != ret) goto cleanup;

    OPAL_OUTPUT_VERBOSE((10, ompi_osc_base_framework.framework_output,
                         "rdma component creating window with id %d",
                         ompi_comm_get_cid(module->comm)));

    /* record my displacement unit.  Always resolved at target */
    module->disp_unit = disp_unit;

    /* peer data */
    module->peers = calloc(ompi_comm_size(comm), sizeof(ompi_osc_rdma_peer_t));
    if (NULL == module->peers) {
        ret = OMPI_ERR_TEMP_OUT_OF_RESOURCE;
        goto cleanup;
    }

    /* peer op count data */
    module->epoch_outgoing_frag_count = calloc (ompi_comm_size(comm), sizeof(uint32_t));
    if (NULL == module->epoch_outgoing_frag_count) {
        ret = OMPI_ERR_TEMP_OUT_OF_RESOURCE;
        goto cleanup;
    }

    if (!no_locks) {
        module->passive_incoming_frag_count = calloc(ompi_comm_size(comm), sizeof(uint32_t));
        if (NULL == module->passive_incoming_frag_count) {
            ret = OMPI_ERR_TEMP_OUT_OF_RESOURCE;
            goto cleanup;
        }

        module->passive_incoming_frag_signal_count = calloc(ompi_comm_size(comm), sizeof(uint32_t));
        if (NULL == module->passive_incoming_frag_signal_count) {
            ret = OMPI_ERR_TEMP_OUT_OF_RESOURCE;
            goto cleanup;
        }
    }

    /* initially, we're in that pseudo-fence state, so we allow eager
       sends (yay for Fence).  Other protocols will disable before
       they start their epochs, so this isn't a problem. */
    module->active_eager_send_active = true;

    if (!no_locks) {
        module->passive_eager_send_active = malloc(sizeof(bool) * ompi_comm_size(comm));
        if (NULL == module->passive_eager_send_active) {
            ret = OMPI_ERR_TEMP_OUT_OF_RESOURCE;
            goto cleanup;
        }
    }

    /* lock data */
    if (check_config_value_bool("no_locks", info)) {
        win->w_flags |= OMPI_WIN_NO_LOCKS;
    }

    /* update component data */
    OPAL_THREAD_LOCK(&mca_osc_rdma_component.lock);
    ret = opal_hash_table_set_value_uint32(&mca_osc_rdma_component.modules,
                                           ompi_comm_get_cid(module->comm),
                                           module);
    OPAL_THREAD_UNLOCK(&mca_osc_rdma_component.lock);
    if (OMPI_SUCCESS != ret) goto cleanup;

    /* fill in window information */
    *model = MPI_WIN_UNIFIED;
    win->w_osc_module = (ompi_osc_base_module_t*) module;
    asprintf(&name, "rdma window %d", ompi_comm_get_cid(module->comm));
    ompi_win_set_name(win, name);
    free(name);

    /* sync memory - make sure all initialization completed */
    opal_atomic_mb();

    module->incomming_buffer = malloc (mca_osc_rdma_component.buffer_size + sizeof (ompi_osc_rdma_frag_header_t));
    if (OPAL_UNLIKELY(NULL == module->incomming_buffer)) {
	goto cleanup;
    }

    ret = ompi_osc_rdma_frag_start_receive (module);
    if (OPAL_UNLIKELY(OMPI_SUCCESS != ret)) {
	goto cleanup;
    }

    /* barrier to prevent arrival of lock requests before we're
       fully created */
    ret = module->comm->c_coll.coll_barrier(module->comm,
                                            module->comm->c_coll.coll_barrier_module);
    if (OMPI_SUCCESS != ret) goto cleanup;

    if (!mca_osc_rdma_component.progress_enable) {
	opal_progress_register (component_progress);
	mca_osc_rdma_component.progress_enable = true;
    }

    OPAL_OUTPUT_VERBOSE((10, ompi_osc_base_framework.framework_output,
                         "done creating rdma window %d", ompi_comm_get_cid(module->comm)));

    return OMPI_SUCCESS;

 cleanup:
    ompi_osc_rdma_free (win);

    return ret;
}


int
ompi_osc_rdma_set_info(struct ompi_win_t *win, struct ompi_info_t *info)
{
    ompi_osc_rdma_module_t *module =
        (ompi_osc_rdma_module_t*) win->w_osc_module;

    /* enforce collectiveness... */
    return module->comm->c_coll.coll_barrier(module->comm,
                                             module->comm->c_coll.coll_barrier_module);
}


int
ompi_osc_rdma_get_info(struct ompi_win_t *win, struct ompi_info_t **info_used)
{
    ompi_info_t *info = OBJ_NEW(ompi_info_t);
    if (NULL == info) return OMPI_ERR_TEMP_OUT_OF_RESOURCE;

    *info_used = info;

    return OMPI_SUCCESS;
}

OBJ_CLASS_INSTANCE(ompi_osc_rdma_pending_t, opal_list_item_t, NULL, NULL);
