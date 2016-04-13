#ifndef OMPI_HOOKS_H_HAS_BEEN_INCLUDED
#define OMPI_HOOKS_H_HAS_BEEN_INCLUDED

#include "ompi_config.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "ompi/constants.h"
#include "ompi/datatype/ompi_datatype.h"


/* datatype create hooks allow other modules to intercept user call and do extra dtype handling */
typedef enum {
    OMPI_HOOK_CREATE_TYPE_VECTOR,
    OMPI_HOOK_CREATE_TYPE_STRUCT,
    OMPI_HOOK_DESTROY_TYPE,
    OMPI_HOOK_ALLOC_MEM,
    OMPI_HOOK_FREE_MEM,
    OMPI_HOOK_NUM
} ompi_hook_type_t;

int32_t ompi_register_hook(ompi_hook_type_t hook_type, uint64_t hook);
int32_t ompi_deregister_hook(ompi_hook_type_t hook_type, uint64_t hook);

struct ompi_hooks{
    opal_list_t hooks[OMPI_HOOK_NUM];
};
extern struct ompi_hooks ompi_hooks;

typedef struct ompi_hook_item_t {
    opal_list_item_t super;
    uint64_t hook;
} ompi_hook_item_t;

OBJ_CLASS_DECLARATION(ompi_hook_item_t);

#define  OMPI_CALL_HOOKS(__hook_id, __hook_fn_type, ...) do{            \
        opal_list_item_t *item;                                         \
        opal_list_t *hooks_list = &ompi_hooks.hooks[__hook_id];         \
        for (item = opal_list_get_first(hooks_list);                    \
             item && (item != opal_list_get_end(hooks_list));           \
             item = opal_list_get_next(item)) {                         \
            __hook_fn_type __hook_fn = (__hook_fn_type)                 \
                ((ompi_hook_item_t *)item)->hook;                       \
            __hook_fn(__VA_ARGS__);                                     \
        }                                                               \
    }while(0)

typedef int32_t (*ompi_datatype_create_struct_hook_fn_t)( int count, const int* pBlockLength, const OPAL_PTRDIFF_TYPE* pDisp,
                                                          ompi_datatype_t* const* pTypes, ompi_datatype_t* newType );
typedef int32_t (*ompi_datatype_create_vector_hook_fn_t)( int count, int bLength, int stride,
                                                          const ompi_datatype_t* oldType, ompi_datatype_t* newType );
typedef int32_t (*ompi_datatype_destroy_hook_fn_t)( ompi_datatype_t* type );

typedef int32_t (*ompi_alloc_mem_hook_fn_t)(void *addr, size_t len);
typedef int32_t (*ompi_free_mem_hook_fn_t)(void *addr);

#define OMPI_DECLARE_HOOK_REG_DEREG(_hook_name,_hook_id,_action)               \
    static inline int32_t _hook_name ##_hook_ ##_action (_hook_name ##_hook_fn_t hook) {\
        return ompi_ ##_action ## _hook(_hook_id, (uint64_t)hook); }
#define OMPI_DECLARE_HOOK(_hook_name,_hook_id)                \
    OMPI_DECLARE_HOOK_REG_DEREG(_hook_name, _hook_id, register) \
    OMPI_DECLARE_HOOK_REG_DEREG(_hook_name, _hook_id, deregister)


OMPI_DECLARE_HOOK(ompi_datatype_create_struct, OMPI_HOOK_CREATE_TYPE_STRUCT)
OMPI_DECLARE_HOOK(ompi_datatype_create_vector, OMPI_HOOK_CREATE_TYPE_VECTOR)
OMPI_DECLARE_HOOK(ompi_datatype_destroy,       OMPI_HOOK_DESTROY_TYPE)
OMPI_DECLARE_HOOK(ompi_alloc_mem,              OMPI_HOOK_ALLOC_MEM)
OMPI_DECLARE_HOOK(ompi_free_mem,               OMPI_HOOK_FREE_MEM)


int32_t ompi_hooks_init(void);
int32_t ompi_hooks_finalize(void);
#endif /* OMPI_HOOKS_H_HAS_BEEN_INCLUDED */
