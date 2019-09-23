#include <tc_module.h>
#include <tc_common.h>

#include <dlfcn.h>

void *tc_dlopen(const char *path) {
    return dlopen(path, RTLD_LAZY);
}
        
