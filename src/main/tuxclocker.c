// Main executable for TuxClocker. Responsible for loading an interface

#include <tc_common.h>
#include <tc_module.h>

#include <stdio.h>

int main(int argc, char **argv) {
    // Load an interface here
    tc_module_t *mod = tc_module_find(TC_CATEGORY_INTERFACE, "qt");
    
    if (mod != NULL) {
        printf("successful load for %s\n", mod->name);
        mod->init_callback(argc, argv);
    }
    
    return 0;
}
