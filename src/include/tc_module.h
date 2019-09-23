#pragma once

#include <stdint.h>

// Categories for modules.
enum tc_module_category {
  TC_CATEGORY_ASSIGNABLE,
  TC_CATEGORY_PROPERTY,
  TC_CATEGORY_INTERFACE
};

// Function name the module loader uses to get the module_t describing the module
#define TC_MODULE_INFO_FUNCTION_NAME "tc_get_module_handle"

typedef struct tc_module_t {
  enum tc_module_category category;
  // Short name of the module like nvidia, qt
  const char *name;
  // Longer description
  const char *description;

  // Initializes the module's internal state
  int8_t (*init_callback)();
  // Frees the internal memory of the module
  int8_t (*close_callback)();

} tc_module_t;

// Try to return the module handle matching the category and name. If it doesn't exist or there was a problem loading the module, returns NULL.
tc_module_t *tc_module_find(enum tc_module_category category, const char *name);

// Wrappers for platform-specific functions for loading libraries (modules) at runtime
void *tc_dlopen(const char *path);
void *tc_dlsym(void *handle, const char *name);
void tc_dlclose(void *handle);
char *tc_dlerror();
