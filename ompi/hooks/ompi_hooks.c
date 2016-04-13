#include "ompi_config.h"

#include <stddef.h>
#include <stdio.h>
#include "ompi_hooks.h"

OBJ_CLASS_INSTANCE(ompi_hook_item_t, opal_list_item_t, NULL, NULL);
struct ompi_hooks ompi_hooks;

int32_t ompi_register_hook(ompi_hook_type_t hook_type, uint64_t hook)
{
    ompi_hook_item_t *hook_item;
    hook_item = OBJ_NEW(ompi_hook_item_t);
    hook_item->hook = hook;
    opal_list_append(&ompi_hooks.hooks[hook_type],
                     (opal_list_item_t *)hook_item);
    return OMPI_SUCCESS;
}

int32_t ompi_deregister_hook(ompi_hook_type_t hook_type, uint64_t hook)
{
    opal_list_item_t *item, *to_dereg = NULL;
    opal_list_t *hooks_list = &ompi_hooks.hooks[hook_type];
    for (item = opal_list_get_first(hooks_list);
         item && (item != opal_list_get_end(hooks_list));
         item = opal_list_get_next(item)) {
        if (((ompi_hook_item_t *)item)->hook == hook) {
            to_dereg = item;
            break;
        }
    }
    if (to_dereg) {
        opal_list_remove_item(hooks_list, to_dereg);
        OBJ_RELEASE(to_dereg);
    }
    return OMPI_SUCCESS;
}

int32_t ompi_hooks_init() {
    /* Initialize hooks lists */
    for ( int i = 0; i < OMPI_HOOK_NUM; i++ ) {
        OBJ_CONSTRUCT(&ompi_hooks.hooks[i],
                      opal_list_t);
    }

    return OMPI_SUCCESS;
}

int32_t ompi_hooks_finalize() {
    for ( int i = 0; i < OMPI_HOOK_NUM; i++ ) {
        OBJ_DESTRUCT(&ompi_hooks.hooks[i]);
    }

    return OMPI_SUCCESS;
}
