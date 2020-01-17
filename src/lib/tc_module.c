#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tc_module.h>
#include <tc_filesystem.h>

// Local data structure that contains the handle of the module and the module data
typedef struct {
    tc_module_t *module;
    void *lib_handle;
} mod_handle_pair;

static mod_handle_pair mod_handle_pairs[TC_MAX_LOADED_MODULES];
static uint32_t mod_handle_pairs_len = 0;

// Local function for getting the path of the module. Requires freeing.
static char *module_filename(const char *category, const char *mod_name);
// Local function for adding a mod_handle_pair to the array
static int8_t add_mod_handle_pair(mod_handle_pair pair);
// Local function for closing a library matching mod
static void close_lib_by_module(const tc_module_t *mod);

static int8_t add_mod_handle_pair(mod_handle_pair pair) {
    mod_handle_pairs_len++;
    if (mod_handle_pairs_len > TC_MAX_LOADED_MODULES) {
        return TC_ENOMEM;
    }
    mod_handle_pairs[mod_handle_pairs_len - 1] = pair;
    return TC_SUCCESS;
}

static void close_lib_by_module(const tc_module_t* mod) {
    // Copy entries not being deleted to a new array
    mod_handle_pair new_pairs[TC_MAX_LOADED_MODULES];
    
    uint32_t saved = 0;
    for (uint32_t i = 0; i < mod_handle_pairs_len; i++) {
        if (mod_handle_pairs[i].module != mod) {
            saved++;
            new_pairs[saved] = mod_handle_pairs[i];
        }
        else {
            // Close the library
            tc_dlclose(mod_handle_pairs[i].lib_handle);
        }
    }
    
    mod_handle_pairs_len = saved;
    for (uint32_t i = 0; i < saved; i++) {
        mod_handle_pairs[i] = new_pairs[i];
    }
}

static char *module_filename(const char *category, const char *mod_name) {
    // TC_MODULE_PATH should be defined as an absolute path
    // Check if file by the exact name exists
    char path[128];
    snprintf(path, 128, "%s/%s/%s", TC_MODULE_PATH, category, mod_name);
    if (tc_fs_file_exists(path)) {
        return strdup(path);
    }
    
    // Form a filename of the form lib<mod_name>.so
    // Posix
    char affixed_path[128];
    snprintf(affixed_path, 128,  "%s/%s/lib%s.so", TC_MODULE_PATH, category, mod_name);
    return strdup(affixed_path);
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
        case TC_CATEGORY_READABLE:
            mod_abs_path = module_filename("readable", name);
            break;
        default:
            return NULL;
    }
    
    void *handle = tc_dlopen(mod_abs_path);
    if (handle == NULL) {
        free(mod_abs_path);
        return NULL;
    }
    
    free(mod_abs_path);
    
    // Call the function to get the module handle
    tc_module_t *(*mod_info_func)() = tc_dlsym(handle, TC_MODULE_INFO_FUNCTION_NAME);
    tc_module_t *mod;
    if ((mod = mod_info_func()) == NULL) {
        tc_dlclose(handle);
        return NULL;
    }
    // Loading was successful, save the data
    mod_handle_pair pair = {
        .module = mod,
        .lib_handle = handle
    };
    
    if (add_mod_handle_pair(pair) != TC_SUCCESS) {
        // Not enough space to add to module list
        tc_dlclose(handle);
        return NULL;
    }
    return mod;
}

tc_module_t **tc_module_find_all_from_category(enum tc_module_category category, uint16_t *count) {
    // Get the file name list of the category modules
    uint16_t file_count = 0;
    char mod_dir_name[512];
    
    switch (category) {
        case TC_CATEGORY_ASSIGNABLE:
            snprintf(mod_dir_name, 512, "%s/%s", TC_MODULE_PATH, "assignable");
            break;
        case TC_CATEGORY_READABLE:
            snprintf(mod_dir_name, 512, "%s/%s", TC_MODULE_PATH, "readable");
            break;
        default:
            return NULL;
    }
    
    // FIXME: file_names members are sometimes corrupted
    char **file_names = tc_fs_dir_filenames(mod_dir_name, &file_count);
    if (file_names == NULL) {
        return NULL;
    }
    // Open all modules using the file names
    tc_module_t *mod_list[TC_MAX_LOADED_MODULES];
    uint16_t mod_count = 0;
    tc_module_t *mod;
    for (uint16_t i = 0; i < file_count; i++) {
        if ((mod = tc_module_find(category, file_names[i])) == NULL) {
            continue;
        }
        mod_list[mod_count] = mod;
        mod_count++;
    }
    
    // Allocate pointer array on the heap
    tc_module_t **retval = calloc(mod_count, sizeof(tc_module_t*));
    for (uint16_t i = 0; i < mod_count; i++) {
        retval[i] = mod_list[i];
    }
    
    *count = mod_count;
    return retval;
}

void tc_module_close(tc_module_t* module) {
    // Free internal state of module
    if (module->close_callback != NULL) {
        if (module->close_callback() != TC_SUCCESS) {
            // Add some sort of log message here
            // It's probably good to abort here
            //abort();
        }
    }
    
    // Free the library matching the module
    close_lib_by_module(module);
}

tc_module_category_info_t tc_module_category_info_create(uint64_t mask, uint16_t num_categories, const tc_module_category_data_t *categories) {
	tc_module_category_info_t retval = {
		.category_mask = mask,
		.num_categories = num_categories
	};
	
	if (num_categories > 0 && categories) {
		for (uint16_t i = 0; i < num_categories; i++) {
			retval.category_data_list[i] = categories[i];
		}
	}
	
	return retval;
}

tc_module_category_data_t tc_module_category_data_create(uint64_t category, union module_data_callback_t u) {
	tc_module_category_data_t retval = {
		.category = category
	};
	
	switch (category) {
		case TC_READABLE:
			retval.readable_data = u.readable_data;
			return retval;
		case TC_ASSIGNABLE:
			retval.assignable_data = u.assignable_data;
			return retval;
		default:
			break;
	}
	return retval;
}
