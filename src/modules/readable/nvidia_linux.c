//#include "/opt/cuda/include/nvml.h"
#include <nvml.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrlLib.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#include <tc_readable.h>
#include <tc_module.h>
#include <tc_common.h>

#define MAX_GPUS 32

// Function for module loader to get the module structure
tc_module_t *TC_MODULE_INFO_FUNCTION();

int8_t init();
int8_t close();
tc_readable_node_t *category_callback();

// Local data structure for mapping nodes to GPU's

enum map_type {MAP_BASE,
    MAP_FAN,
    MAP_CLOCK
};

typedef struct {
    struct base_map {
        tc_readable_node_t *node;
        nvmlDevice_t dev;
        uint8_t device_index;
    } base_map;
    enum map_type map_type;
    union {
        uint8_t fan_index;
        nvmlClockType_t clock_type;
    };
} callback_map;

// Create new callback map on the heap
static callback_map *callback_map_new() {
    return calloc(1, sizeof(callback_map));
}

static callback_map *callback_map_new_copy(const callback_map *map) {
    callback_map *new_map = callback_map_new();
    // FIXME : add copying not only the base map
    new_map->base_map = map->base_map;
    new_map->map_type = map->map_type;
    
    return new_map;
}

// Local functions
void generate_readable_tree();
void add_temp_item(tc_readable_node_t *parent, nvmlDevice_t dev, callback_map *map);
void add_power_item(tc_readable_node_t *parent, callback_map *map);
void add_clock_items(tc_readable_node_t *parent, callback_map *map);

// Value updating functions
tc_readable_result_t get_temp(const tc_readable_node_t *node);
tc_readable_result_t get_power(const tc_readable_node_t *node);
tc_readable_result_t get_clock(const tc_readable_node_t *node);

static uint32_t gpu_count;
static nvmlDevice_t nvml_handles[MAX_GPUS];
static Display *dpy;
static tc_readable_node_t *root_node = NULL;
// Root node for maps
static tc_bin_node_t *root_search_node;

tc_module_t mod_info = {
    .category = TC_CATEGORY_READABLE,
    .name = "nvidia",
    .description = "Nvidia readables",
    .init_callback = &init,
    .close_callback = &close,
    .category_data_callback = (void *(*)()) &category_callback
};

tc_module_t *TC_MODULE_INFO_FUNCTION() {
    return &mod_info;
}

tc_readable_node_t *category_callback() {
    return root_node;
}

int8_t init() {
    // Initialize library
  if (nvmlInit_v2() != NVML_SUCCESS) {
    return TC_EGENERIC;
  }

  // Query GPU count
  if (nvmlDeviceGetCount(&gpu_count) != NVML_SUCCESS) {
    return TC_EGENERIC;
  }

  if (gpu_count > MAX_GPUS) {
    return TC_ENOMEM;
  }

  // Get nvml handles
  uint32_t valid_count = 0;
  for (uint8_t i = 0; i < gpu_count; i++) {
    nvmlDevice_t dev;

    if (nvmlDeviceGetHandleByIndex_v2(i, &dev) == NVML_SUCCESS) {
      nvml_handles[valid_count] = dev;
      valid_count++;
    }
  }
  gpu_count = valid_count;

  // Get X11 display
  if ((dpy = XOpenDisplay(NULL)) == NULL) {
    return TC_EGENERIC;
  }

  // Generate the tree structure of assignables for every GPU. Free in close().
    generate_readable_tree();
    
    return TC_SUCCESS;
}

void generate_readable_tree() {
    root_node = tc_readable_node_new();
    
    tc_variant_t data = {
        .data_type = TC_TYPE_NONE
    };
    
    root_node->constant = true;
    root_node->data = data;
    
  for (uint32_t i = 0; i < gpu_count; i++) {
    // Get GPU name and use it as the root item for GPU
    char gpu_name[NVML_DEVICE_NAME_BUFFER_SIZE];

    if (nvmlDeviceGetName(nvml_handles[i], gpu_name, NVML_DEVICE_NAME_BUFFER_SIZE) != NVML_SUCCESS) {
        continue;
    }
    // Got the name, append the item to the root item
    tc_readable_node_t *gpu_name_node = tc_readable_node_new();
    gpu_name_node->name = strdup(gpu_name);
    
    // Append to the root node
    if (tc_readable_node_add_child(root_node, gpu_name_node) != TC_SUCCESS) {
      // Couldn't allocate memory, destroy the node
      tc_readable_node_destroy(gpu_name_node);
      continue;
    }
    
    // Create the base map from this GPU
    callback_map *map = callback_map_new();
    map->map_type = MAP_BASE;
    map->base_map.dev = nvml_handles[i];
    map->base_map.device_index = i;
    
    // Add the root map  node
    if (!root_search_node) {
        root_search_node = tc_bin_node_insert(root_search_node, gpu_name_node, map);
    }
    else if (root_search_node) {
        tc_bin_node_insert(root_search_node, gpu_name_node, map);
    }
    
    // Add nodes to the GPU
    add_temp_item(gpu_name_node, nvml_handles[i], map);
    
    add_power_item(gpu_name_node, map);
    
    add_clock_items(gpu_name_node, map);
  }
}

void add_temp_item(tc_readable_node_t *parent, nvmlDevice_t dev, callback_map *map) {
    uint32_t reading;
    if (nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &reading) != NVML_SUCCESS) {
        return;
    }
    // Create new node
    tc_readable_node_t *temp_node = tc_readable_node_add_new_child(parent);
    
    if (!temp_node) {
        return;
    }
    
    temp_node->name = strdup("Temperature");
    temp_node->value_callback = &get_temp;
    
    tc_bin_node_insert(root_search_node, temp_node, map);
}

void add_power_item(tc_readable_node_t *parent, callback_map *map) {
    uint32_t reading;
    if (nvmlDeviceGetPowerUsage(map->base_map.dev, &reading) != NVML_SUCCESS) {
        return;
    }
    
    tc_readable_node_t *node = tc_readable_node_add_new_child(parent);
    if (!node) {
        return;
    }
    
    node->name = strdup("Power Usage");
    node->value_callback = &get_power;
    
    tc_bin_node_insert(root_search_node, node, map);
}

void add_clock_items(tc_readable_node_t *parent, callback_map *map) {
    uint32_t reading;
    // Create copy of the map
    callback_map *c_map = callback_map_new_copy(map);
    
    if (nvmlDeviceGetClockInfo(c_map->base_map.dev, NVML_CLOCK_GRAPHICS, &reading) == NVML_SUCCESS) {
        do {
            // FIXME : make this a function
            tc_readable_node_t *node = tc_readable_node_add_new_child(parent);
            if (!node) {
                free(c_map);
                break;
            }
            tc_bin_node_t *s_node = NULL;
            
            if (!(s_node = tc_bin_node_insert(root_search_node, node, c_map))) {
                tc_bin_node_destroy(s_node);
                tc_readable_node_destroy(node);
                free(c_map);
                break;
            }
            node->name = strdup("Core Clock");
            node->value_callback = &get_clock;
            
            c_map->map_type = MAP_CLOCK;
            c_map->clock_type = NVML_CLOCK_GRAPHICS;
        } while (0);
    }
    
}

tc_readable_result_t get_temp(const tc_readable_node_t *node) {
    // Find the data mapped to the node
    callback_map *map = (callback_map*) tc_bin_node_find_value(root_search_node, node);
    
    tc_readable_result_t res;
    res.valid = false;
    
    if (!map) {
        return res;
    }
    
    uint32_t temp;
    if (nvmlDeviceGetTemperature(map->base_map.dev, NVML_TEMPERATURE_GPU,  &temp) != NVML_SUCCESS) {
        return res;
    }
    res.valid = true;
    res.data.data_type = TC_TYPE_UINT;
    res.data.uint_value = temp;
    
    return res;
}

tc_readable_result_t get_power(const tc_readable_node_t *node) {
    callback_map *map = (callback_map*) tc_bin_node_find_value(root_search_node, node);
    
    tc_readable_result_t res;
    res.valid = false;
    
    if (!map) {
        return res;
    }
    
    uint32_t power;
    if (nvmlDeviceGetPowerUsage(map->base_map.dev, &power) != NVML_SUCCESS) {
        return res;
    }
    
    res.valid = true;
    res.data.data_type = TC_TYPE_UINT;
    res.data.uint_value = power;
    
    return res;
}

tc_readable_result_t get_clock(const tc_readable_node_t *node) {
    tc_readable_result_t res;
    res.valid = false;
    
    callback_map *map = (callback_map*) tc_bin_node_find_value(root_search_node, node);
    
    if (!map || map->map_type != MAP_CLOCK) {
        return res;
    }
    
    uint32_t reading;
    if (nvmlDeviceGetClockInfo(map->base_map.dev, map->clock_type, &reading) != NVML_SUCCESS) {
        return res;
    }
    res.valid = true;
    res.data.data_type = TC_TYPE_UINT;
    res.data.uint_value = reading;
    
    return res;
}