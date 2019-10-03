#include "/opt/cuda/include/nvml.h"
//#include <nvml.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdio.h>

#include <tc_assignable.h>
#include <tc_common.h>
#include <tc_module.h>

#define MAX_GPUS 32

// Function for module loader to get the module structure
tc_module_t *TC_MODULE_INFO_FUNCTION();

// Local function declarations
static int8_t init();
static int8_t close();
static tc_assignable_node_t *category_callback();
static int8_t generate_assignable_tree();
// Functions for adding nodes to the GPU
void add_power_limit_item(tc_assignable_node_t *parent, nvmlDevice_t dev);

static uint32_t gpu_count;
static nvmlDevice_t nvml_handles[MAX_GPUS];
static Display *dpy;
static tc_assignable_node_t *root_node;

tc_module_t mod_info = {
    .category = TC_CATEGORY_ASSIGNABLE,
    .name = "nvidia",
    .description = "Nvidia assignables",
    .init_callback = &init,
    .close_callback = &close,
    .category_data_callback = (void *(*)()) &category_callback
};

tc_module_t *TC_MODULE_INFO_FUNCTION() {
    return &mod_info;
}

static tc_assignable_node_t *category_callback() {
    return root_node;
}

static int8_t init() {
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
  generate_assignable_tree();

  return TC_SUCCESS;
}

static int8_t close() {

}

static int8_t generate_assignable_tree() {
  // Allocate memory for root node
  root_node = tc_assignable_node_new();

  for (uint32_t i = 0; i < gpu_count; i++) {
    // Get GPU name and use it as the root item for GPU
    char gpu_name[NVML_DEVICE_NAME_BUFFER_SIZE];

    if (nvmlDeviceGetName(nvml_handles[i], gpu_name, NVML_DEVICE_NAME_BUFFER_SIZE) != NVML_SUCCESS) {
        continue;
    }
    // Got the name, append the item to the root item
    tc_assignable_node_t *gpu_name_node = tc_assignable_node_new();
    gpu_name_node->name = strdup(gpu_name);
    printf("%s\n", gpu_name_node->name);
    
    // Append to the root node
    if (tc_assignable_node_add_child(root_node, gpu_name_node) != TC_SUCCESS) {
      // Couldn't allocate memory, destroy the node
      tc_assignable_node_destroy(gpu_name_node);
      continue;
    }
    // Try to add tunables that don't have children first
    add_power_limit_item(gpu_name_node, nvml_handles[i]);
  }

  return TC_SUCCESS;
}

void add_power_limit_item(tc_assignable_node_t *parent, nvmlDevice_t dev) {
  uint32_t min, max;
  if (nvmlDeviceGetPowerManagementLimitConstraints(dev, &min, &max) != NVML_SUCCESS) {
    return;
  }
  // Create a new node
  tc_assignable_node_t *power_node = tc_assignable_node_new();
  if (power_node == NULL) {
    return;
  }
  // Assign the parent
  if (tc_assignable_node_add_child(parent, power_node) != TC_SUCCESS) {
    return;
  }

  // Create the assignable range
  tc_assignable_range_int_t int_range = {
    .min = (min / 1000),
    .max = (max / 1000)
  };
  tc_assignable_range_t range = {
    .range_data_type = TC_ASSIGNABLE_RANGE_INT,
    .int_range = int_range
  };
  power_node->value_category = TC_ASSIGNABLE_RANGE;
  power_node->range_info = range;
}
