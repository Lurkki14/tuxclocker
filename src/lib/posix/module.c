#include <tc_module.h>
#include <tc_common.h>

#include <dlfcn.h>

void *tc_dlopen(const char *path) {
    return dlopen(path, RTLD_LAZY);
}

void *tc_dlsym(void *handle, const char *name) {
    return dlsym(handle, name);
}

void tc_dlclose(void *handle) {
    dlclose(handle);
}

char *tc_dlerror() {
    return dlerror();
}
