#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tc_module.h>

// Local function for getting the path of the module. Requires freeing.
static char *module_filename(const char *category, const char *mod_name);
static char *module_filename(const char *category, const char *mod_name) {
    // TC_MODULE_PATH should be defined as an absolute path
    // Posix
    char path[128];
    snprintf(path, 128,  "%s/%s/lib%s.so", TC_MODULE_PATH, category, mod_name);
    return strdup(path);
}

tc_module_t *tc_module_find(enum tc_module_category category, const char *name) {
    char *mod_abs_path = NULL;
    
    switch (category) {
        case TC_CATEGORY_ASSIGNABLE:
            mod_abs_path = module_filename("assignable", name);
            break;
        case TC_CATEGORY_INTERFACE:
           mod_abs_path = module_filename("interface", name);
            break;
        default:
            return NULL;
    }
    
    void *handle = tc_dlopen(mod_abs_path);
    if (handle == NULL) {
        printf("%s\n", tc_dlerror());
        return NULL;
    }
    
    // Call the function to get the module handle
    tc_module_t *(*mod_info_func)() = tc_dlsym(handle, TC_MODULE_INFO_FUNCTION_NAME);
    return mod_info_func();
}

tc_module_t **tc_module_find_all_from_category(enum tc_module_category category, uint16_t *count) {
    
    
    return NULL;
}
