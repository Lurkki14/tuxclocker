#pragma once

#include <stdint.h>

#include <tc_common.h>
#include <tc_readable.h>
#include <tc_assignable.h>

// Use unmangled symbols for C++
#ifdef __cplusplus
extern "C" {
#endif
  
// Bitmask values for module categories
#define TC_ASSIGNABLE (1)
#define TC_READABLE (1 << 1)
#define TC_INTERFACE (1 << 2)

#define TC_READABLE_DYNAMIC (1)
#define TC_READABLE_STATIC (1 << 1)
	
// Categories for modules.
enum tc_module_category {
  TC_CATEGORY_ASSIGNABLE,
  TC_CATEGORY_READABLE,
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

union module_data_callback_t {
	tc_readable_module_data_t (*readable_data)();
	tc_assignable_module_data_t (*assignable_data)();
};

// Tagged union for category specific data
typedef struct {
	uint64_t category;
	// Since category specific data might be generated after calling module's 'init' callback, use function pointers to fetch it.
	union {
		tc_readable_module_data_t (*readable_data)();
		tc_assignable_module_data_t (*assignable_data)();
	};
} tc_module_category_data_t;

typedef struct {
	uint64_t category_mask;
	uint16_t num_categories;
	tc_module_category_data_t category_data_list[64];
} tc_module_category_info_t;

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
	enum tc_data_types init_callback_args[TC_MAX_FUNCTION_ARGC];

	// Frees the internal memory of the module
	int8_t (*close_callback)();

	// Callback for category specific main data structure of the module
	void *(*category_data_callback)();
	
	tc_module_category_info_t category_info;
} tc_module_t;

// Try to return the module handle matching the category and name. If it doesn't exist or there was a problem loading the module, returns NULL.
tc_module_t *tc_module_find(enum tc_module_category category, const char *name);
// Try to return all module handles matching 'category'. The return value needs to be freed in addition to tc_module_close()
tc_module_t **tc_module_find_all_from_category(enum tc_module_category category, uint16_t *count);

// Convenience functions
tc_module_category_info_t tc_module_category_info_create(uint64_t mask, uint16_t num_categories, const tc_module_category_data_t *categories);

tc_module_category_data_t tc_module_category_data_create(uint64_t category, union module_data_callback_t u);

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
