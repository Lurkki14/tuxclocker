#pragma once

#include <stdint.h>

#include <tc_common.h>

// Use unmangled symbols for C++
#ifdef __cplusplus
extern "C" {
#endif
  
// Categories for modules.
enum tc_module_category {
  TC_CATEGORY_ASSIGNABLE,
  TC_CATEGORY_PROPERTY,
  TC_CATEGORY_INTERFACE
};

// Maximum amount of modules loaded at once
#define TC_MAX_LOADED_MODULES 64

// Default module path in case not defined
#ifndef TC_MODULE_PATH
#define TC_MODULE_PATH "/usr/lib/tuxclocker/modules"
#endif

// Env variable name to load modules from in addition
#define TC_MODULE_PATH_ENV "TC_MODULE_PATH"

// Function name the module loader uses to get the module_t describing the module
#define TC_MODULE_INFO_FUNCTION_NAME "tc_get_module_handle"
#define TC_MODULE_INFO_FUNCTION tc_get_module_handle

// Maximum argument count for "overloaded" functions
#define TC_MAX_FUNCTION_ARGC 16

typedef struct tc_module_t {
  enum tc_module_category category;
  // Short name of the module like nvidia, qt
  const char *name;
  // Longer description
  const char *description;

  // Initializes the module's internal state
  int8_t (*init_callback)();
  // Arguments for init_callback
  uint8_t init_callback_argc;
  enum tc_arg_types init_callback_args[TC_MAX_FUNCTION_ARGC];
  
  // Frees the internal memory of the module
  int8_t (*close_callback)();

  // Callback for category specific main data structure of the module
  void *(*category_data_callback)();
} tc_module_t;

// Try to return the module handle matching the category and name. If it doesn't exist or there was a problem loading the module, returns NULL.
tc_module_t *tc_module_find(enum tc_module_category category, const char *name);
// Try to return all module handles matching 'category'
tc_module_t **tc_module_find_all_from_category(enum tc_module_category category, uint16_t *count);

// Close the module after successful find
void tc_module_close(tc_module_t *module);

// Wrappers for platform-specific functions for loading libraries (modules) at runtime
void *tc_dlopen(const char *path);
void *tc_dlsym(void *handle, const char *name);
void tc_dlclose(void *handle);
char *tc_dlerror();

#ifdef __cplusplus
}
#endif
