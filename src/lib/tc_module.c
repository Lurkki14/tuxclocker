#include <stdio.h>
#include <stdlib.h>

#include <tc_module.h>

// Local function for returning the top level paths to look for eg "/usr/lib/tuxclocker/modules/"
static char **module_search_paths(uint8_t *count) {
    return NULL;
}

tc_module_t *tc_module_find(enum tc_module_category category, const char *name) {
    // How do we find out where the library path is?
    // Not like this, this is bad
    char abs_path[128];
    char abs_env_path[128];
    
    const char  *env_module_path = getenv(TC_MODULE_PATH_ENV);
    
    // Find the folder where the module should reside
    switch (category) {
        case TC_CATEGORY_ASSIGNABLE:
            snprintf(abs_env_path, 128, "%s/%s/%s", env_module_path, "assignable", name);
            break;
        case TC_CATEGORY_INTERFACE:
            snprintf(abs_env_path, 128, "%s/%s/%s", env_module_path, "interface", name);
            break;
        default:
            return NULL;
    }
    void *handle = tc_dlopen(abs_env_path);
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
