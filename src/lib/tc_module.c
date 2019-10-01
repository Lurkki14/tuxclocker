#include <stdio.h>
#include <stdlib.h>

#include <tc_module.h>


tc_module_t *tc_module_find(enum tc_module_category category, const char *name) {
    // The library  is configured with runpath pointing to module root on posix
    char mod_abs_path[128];
    
    // Find the folder where the module should reside
    switch (category) {
        case TC_CATEGORY_ASSIGNABLE:
            snprintf(mod_abs_path, 128, "%s/%s",  "assignable", name);
            break;
        case TC_CATEGORY_INTERFACE:
            snprintf(mod_abs_path, 128, "%s/%s",  "interface", name);
            break;
        default:
            return NULL;
    }
    void *handle = tc_dlopen(mod_abs_path);
    if (handle == NULL) {
        return NULL;
    }
    
    // Call the function to get the module handle
    tc_module_t *(*mod_info_func)() = tc_dlsym(handle, TC_MODULE_INFO_FUNCTION_NAME);
    return mod_info_func();
}

tc_module_t **tc_module_find_all_from_category(enum tc_module_category category, uint16_t *count) {
    return NULL;
}
