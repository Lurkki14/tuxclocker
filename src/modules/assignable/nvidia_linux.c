//#include "/opt/cuda/include/nvml.h"
#include <nvml.h>
#include <X11/Xlib.h>
#include <NVCtrl/NVCtrlLib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <tc_assignable.h>
#include <tc_common.h>
#include <tc_module.h>

#define MAX_GPUS 32

enum node_type {
	NODE_BASE,
	NODE_CLOCK,
	NODE_FAN
};

typedef struct {
	tc_assignable_node_t node;
	struct base_data {
		nvmlDevice_t dev;
		uint8_t device_index;
	} base_data;
	enum node_type type;
	union {
		struct clock_info {
			uint32_t max_perf_state, clock_type;
		} clock_info;
		uint8_t fan_index;
	};
} callback_data;


static callback_data *callback_data_new() {
	tc_assignable_node_t node;
	// Zero the node structure
	memset(&node, 0, sizeof(node));
	
	callback_data *cd = calloc(1, sizeof(callback_data));
	cd->node = node;
	return cd;
}

static void callback_data_destroy(callback_data *data) {
	free(data);
}

// Function for module loader to get the module structure
tc_module_t *TC_MODULE_INFO_FUNCTION();

// Local function declarations
static int8_t init();
static int8_t close();
static tc_assignable_node_t *category_callback();

tc_assignable_module_data_t cat_data_cb();

static int8_t generate_assignable_tree();
// Functions for creating fan speed and mode nodes
static void add_fanmode_item(tc_assignable_node_t *node, uint32_t dev_index);
static void add_fanspeed_item(tc_assignable_node_t *node, uint32_t dev_index, uint8_t fan_index);
// Functions for adding nodes to the GPU
void add_power_limit_item(tc_assignable_node_t *parent, nvmlDevice_t dev);
void add_fan_items(tc_assignable_node_t *parent, int32_t index);
void add_clock_items(tc_assignable_node_t *parent, int32_t index, nvmlDevice_t dev);

// Assignment functions
int8_t assign_power_limit(tc_variant_t value, const tc_assignable_node_t *node);
int8_t assign_fan_mode(tc_variant_t value, const tc_assignable_node_t *node);
int8_t assign_fan_speed(tc_variant_t value, const tc_assignable_node_t *node);
int8_t assign_clock(tc_variant_t value, const tc_assignable_node_t *node);

static uint32_t gpu_count;
static nvmlDevice_t nvml_handles[MAX_GPUS];
static Display *dpy;
static tc_assignable_node_t *root_node;
static callback_data *root_callback_data;
static tc_assignable_module_data_t mod_data;

tc_module_t mod_info = {
    .category = TC_CATEGORY_ASSIGNABLE,
    .name = "nvidia",
    .description = "Nvidia assignables",
    .init_callback = &init,
    .close_callback = &close,
    .category_data_callback = (void *(*)()) &category_callback
};

tc_module_t *TC_MODULE_INFO_FUNCTION() {
	tc_module_category_data_t cat_data = {
		.category = TC_ASSIGNABLE,
		.assignable_data = &cat_data_cb
	};
	
	tc_module_category_info_t cat_info = {
		.category_mask = TC_ASSIGNABLE,
		.num_categories = 1
	};
	cat_info.category_data_list[0] = cat_data;
	
	mod_info.category_info = cat_info;
	
	return &mod_info;
}

static tc_assignable_node_t *category_callback() {
    return root_node;
}

tc_assignable_module_data_t cat_data_cb() {
	return mod_data;
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
	root_callback_data = callback_data_new();
	root_node = &(root_callback_data->node);
	
	root_node->value_category = TC_ASSIGNABLE_NONE;
	
	// Assign module data
	mod_data.root_node = root_node;

	for (uint32_t i = 0; i < gpu_count; i++) {
		// Get GPU name and use it as the root item for GPU
		char gpu_name[NVML_DEVICE_NAME_BUFFER_SIZE];

		if (nvmlDeviceGetName(nvml_handles[i], gpu_name, NVML_DEVICE_NAME_BUFFER_SIZE) != NVML_SUCCESS) {
			continue;
		}
		// Got the name, append the item to the root item
		tc_assignable_node_t *gpu_name_node = tc_assignable_node_new();
		gpu_name_node->name = strdup(gpu_name);

		// Append to the root node
		if (tc_assignable_node_add_child(root_node, gpu_name_node) != TC_SUCCESS) {
			// Couldn't allocate memory, destroy the node
			tc_assignable_node_destroy(gpu_name_node);
			continue;
		}
		// Try to add tunables that don't have children first
		add_power_limit_item(gpu_name_node, nvml_handles[i]);

		add_fan_items(gpu_name_node, i);
		
		add_clock_items(gpu_name_node, i, nvml_handles[i]);
	}

	return TC_SUCCESS;
}

static void add_fanmode_item(tc_assignable_node_t *parent, uint32_t dev_index) {
	callback_data *fanmode_data = callback_data_new();
	fanmode_data->base_data.device_index = dev_index;
	
	tc_assignable_node_t *fanmode_node = &(fanmode_data->node);
	if (tc_assignable_node_add_child(parent, fanmode_node) != TC_SUCCESS) {
		callback_data_destroy(fanmode_data);
		return;
	}
	
	// Create the string array of the options
	char *opts[64] = {"auto", "manual"};
	char **list = tc_str_arr_dup(2, opts);
	
	tc_assignable_enum_t enum_info = {
		2,
		list
	};
	
	fanmode_node->value_category = TC_ASSIGNABLE_ENUM;
	fanmode_node->enum_info = enum_info;
	
	tc_assignable_node_set_data(fanmode_node, NULL, "Fan Mode",  &assign_fan_mode);	
}

static void add_fanspeed_item(tc_assignable_node_t *parent, uint32_t dev_index, uint8_t fan_index) {
	// Fan speed node
	callback_data *fanspeed_data = callback_data_new();
	fanspeed_data->base_data.device_index = dev_index;
	fanspeed_data->type = NODE_FAN;
	fanspeed_data->fan_index = fan_index;
	
	tc_assignable_node_t *fanspeed_node = &(fanspeed_data->node);
	if (tc_assignable_node_add_child(parent, fanspeed_node) != TC_SUCCESS) {
		callback_data_destroy(fanspeed_data);
		return;
	}
	
	tc_assignable_range_int_t speed_range = {
		.min = 0,
		.max = 100
	};
		
	tc_assignable_range_t range = {
		.range_data_type = TC_ASSIGNABLE_RANGE_INT,
		.int_range = speed_range
	};
		
	fanspeed_node->value_category = TC_ASSIGNABLE_RANGE;
	fanspeed_node->range_info = range;
	
	tc_assignable_node_set_data(fanspeed_node, "%", "Fan Speed", &assign_fan_speed);
}

void add_power_limit_item(tc_assignable_node_t *parent, nvmlDevice_t dev) {
  uint32_t min, max;
  if (nvmlDeviceGetPowerManagementLimitConstraints(dev, &min, &max) != NVML_SUCCESS) {
    return;
  }
	// Create a new node
	callback_data *power_data = callback_data_new();
	power_data->base_data.dev = dev;
	
	tc_assignable_node_t *power_node = &(power_data->node);
  if (power_node == NULL) {
    return;
  }
  // Assign the parent
  if (tc_assignable_node_add_child(parent, power_node) != TC_SUCCESS) {
    tc_assignable_node_destroy(power_node);
    return;
  }

  // Create the assignable range
  tc_assignable_range_double_t double_range = {
    .min = (min / 1000),
    .max = (max / 1000)
  };
  tc_assignable_range_t range = {
    .range_data_type = TC_ASSIGNABLE_RANGE_DOUBLE,
    .double_range = double_range
  };
  
	power_node->value_category = TC_ASSIGNABLE_RANGE;
	power_node->range_info = range;
	power_node->name = "Power Limit";
	power_node->assign_callback = &assign_power_limit;
	
	tc_variant_t v = {
		.data_type = TC_TYPE_DOUBLE,
		.double_value = 3.14f
	};
	
	//assign_power_limit(v, power_node);
}

void add_fan_items(tc_assignable_node_t* parent, int32_t index) {
    // Query fan count for GPU
    unsigned char *data;
    int32_t data_len;
    
    if (!XNVCTRLQueryTargetBinaryData(dpy,
                                                                              NV_CTRL_TARGET_TYPE_GPU,
                                                                              0,
                                                                              index,
                                                                              NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                                                                              &data,
                                                                              &data_len)) {
        return;
    }
    int gpu_fan_count = (int) *data;
    
	// Check if manual fan mode is available
	NVCTRLAttributeValidValuesRec values;
        
	if (!XNVCTRLQueryValidTargetAttributeValues(dpy,
			NV_CTRL_TARGET_TYPE_GPU,
			index,
			0,
			NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
			&values)) {
		return;
        }
        
        if (!(values.permissions & ATTRIBUTE_TYPE_WRITE)) {
		// Fan mode nor speed will be writable
		return;
	}
    
	if (gpu_fan_count == 1) {
		// Only add direct children
		add_fanmode_item(parent, index);
		
		add_fanspeed_item(parent, index, 0);
		return;
	}
	
	else if (gpu_fan_count < 1) {
		// Multiple controllable fans
		// Add a node as a parent for fan mode
		tc_assignable_node_t *fan_parent = tc_assignable_node_new();
		tc_assignable_node_set_data(fan_parent, NULL, "Fans", NULL);
		
		if (tc_assignable_node_add_child(parent, fan_parent) != TC_SUCCESS) {
			tc_assignable_node_destroy(fan_parent);
			return;
		}
		// Add the fan mode node (should control the mode of both fans)
		add_fanmode_item(fan_parent, index);
		
		for (int i = 0; i < gpu_fan_count; i++) {
			tc_assignable_node_t *fanspeed_parent = tc_assignable_node_new();
			char n_str[16];
			snprintf(n_str, 16, "%d", i);
			tc_assignable_node_set_data(fanspeed_parent, NULL, strdup(n_str), NULL);
			
			if (tc_assignable_node_add_child(fan_parent, fanspeed_parent) != TC_SUCCESS) {
				tc_assignable_node_destroy(fanspeed_parent);
				return;
			}
			
			add_fanspeed_item(fanspeed_parent, index, (uint8_t) i);
		}
	}
}

void add_clock_items(tc_assignable_node_t *parent, int32_t index, nvmlDevice_t dev) {
	// Get the amount of performance states
	uint32_t mem_clocks[16];
	uint32_t len = 16;
	if (nvmlDeviceGetSupportedMemoryClocks(dev, &len, mem_clocks) != NVML_SUCCESS) {
		return;
	}

	NVCTRLAttributeValidValuesRec values;
	// Query writability of memory clock (transfer rate / 2)
	do {
		if (XNVCTRLQueryValidTargetAttributeValues(dpy,
				NV_CTRL_TARGET_TYPE_GPU,
				index,
				len - 1,
				NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,
				&values)) {
			if (values.permissions & ATTRIBUTE_TYPE_WRITE) {
				// Is writable
				callback_data *memclock_data = callback_data_new();
				memclock_data->base_data.device_index = index;
				memclock_data->type = NODE_CLOCK;
				memclock_data->clock_info.max_perf_state = len - 1;
				memclock_data->clock_info.clock_type = NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET;
				
				tc_assignable_node_t *memclock_node = &(memclock_data->node);
				if (tc_assignable_node_add_child(parent, memclock_node) != TC_SUCCESS) {
					tc_assignable_node_destroy(memclock_node);
					break;
				}
				tc_assignable_range_int_t clock_range = {
					.min = values.u.range.min / 2,
					.max = values.u.range.max / 2
				};
				
				tc_assignable_range_t range = {
					.range_data_type = TC_ASSIGNABLE_RANGE_INT,
					.int_range = clock_range
				};
				
				memclock_node->value_category = TC_ASSIGNABLE_RANGE;
				memclock_node->range_info = range;
				
				tc_assignable_node_set_data(memclock_node, "MHz", "Memory Clock Offset", &assign_clock);
			}
		}
	} while (0);
	
	if (XNVCTRLQueryValidTargetAttributeValues(dpy,
			NV_CTRL_TARGET_TYPE_GPU,
			index,
			len - 1,
			NV_CTRL_GPU_NVCLOCK_OFFSET,
			&values)) {
		if (values.permissions & ATTRIBUTE_TYPE_WRITE) {
			callback_data *coreclock_data = callback_data_new();
			coreclock_data->base_data.device_index = index;
			coreclock_data->type = NODE_CLOCK;
			coreclock_data->clock_info.max_perf_state = len - 1;
			coreclock_data->clock_info.clock_type = NV_CTRL_GPU_NVCLOCK_OFFSET;
			
			tc_assignable_node_t *coreclock_node = &(coreclock_data->node);
			if (tc_assignable_node_add_child(parent, coreclock_node) != TC_SUCCESS) {
				tc_assignable_node_destroy(coreclock_node);
				return;
			}
			tc_assignable_range_int_t clock_range = {
				.min = values.u.range.min,
				.max = values.u.range.max
			};
			
			tc_assignable_range_t range = {
				.range_data_type = TC_ASSIGNABLE_RANGE_INT,
				.int_range = clock_range
			};
			
			coreclock_node->value_category = TC_ASSIGNABLE_RANGE;
			coreclock_node->range_info = range;
			
			tc_assignable_node_set_data(coreclock_node, "MHz", "Core Clock Offset", &assign_clock);
		}
	}
}

int8_t assign_power_limit(tc_variant_t value, const tc_assignable_node_t *node) {
	const callback_data *data = (const callback_data*) node;
	
	if (value.data_type != TC_TYPE_DOUBLE) {
		return TC_EINVAL;
	}
	
	// Watts -> milliwatts
	uint32_t in_arg = (uint32_t) (value.double_value * 1000);
	nvmlReturn_t retval = nvmlDeviceSetPowerManagementLimit(data->base_data.dev, in_arg);
	
	switch (retval) {
		case NVML_SUCCESS:
			return TC_SUCCESS;
		case NVML_ERROR_NO_PERMISSION:
			return TC_ENOPERM;
		case NVML_ERROR_INVALID_ARGUMENT:
			return TC_EINVAL;
		default:
			return TC_EGENERIC;
	}
	
	return TC_EGENERIC;
}

int8_t assign_fan_mode(tc_variant_t value, const tc_assignable_node_t *node) {
	if (!node || value.data_type != TC_TYPE_UINT || (value.uint_value > node->enum_info.property_count + 1)) {
		return TC_EINVAL;
	}
	
	// TODO : add a mapping in the node for NVCtrl enumerations
	const callback_data *data = (const callback_data*) node;
	
	switch (value.uint_value) {
		case 0:
			if (!XNVCTRLSetTargetAttributeAndGetStatus(dpy,
					NV_CTRL_TARGET_TYPE_GPU,
					data->base_data.device_index,
					0,
					NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
					NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE)) {
				return TC_EINVAL;
			}
			return TC_SUCCESS;
		case 1:
			if (!XNVCTRLSetTargetAttributeAndGetStatus(dpy,
					NV_CTRL_TARGET_TYPE_GPU,
					data->base_data.device_index,
					0,
					NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
					NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE)) {
				return TC_EINVAL;
			}
			return TC_SUCCESS;
		default:
			return TC_EINVAL;
	}
	return TC_EGENERIC;
}

int8_t assign_fan_speed(tc_variant_t value, const tc_assignable_node_t *node) {
	if (!node || value.data_type != TC_TYPE_INT) {
		return TC_EINVAL;
	}
	
	const callback_data *data = (const callback_data*) node;	
	
	if (!XNVCTRLSetTargetAttributeAndGetStatus(dpy,
			NV_CTRL_TARGET_TYPE_COOLER,
			data->base_data.device_index,
			0,
			NV_CTRL_THERMAL_COOLER_LEVEL,
			value.uint_value)) {
		// Return TC_EINVALPREREQ if fan mode is automatic
		int value;
	
		if (!XNVCTRLQueryTargetAttribute(dpy,
				NV_CTRL_TARGET_TYPE_GPU,
				data->base_data.device_index,
				0,
				NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
				&value)) {
			return TC_EINVAL;
		}
		
		if (!(value & NV_CTRL_GPU_COOLER_MANUAL_CONTROL)) {
			return TC_EINVALPREREQ;
		}
		
		return TC_EINVAL;
	}
	return TC_SUCCESS;
}

int8_t assign_clock(tc_variant_t value, const tc_assignable_node_t *node) {
	if (!node || value.data_type != TC_TYPE_INT) {
		return TC_EINVAL;
	}
	
	const callback_data *data = (const callback_data*) node;
	
	if (data->type != NODE_CLOCK) {
		return TC_EINVAL;
	}
	
	// Memory clock -> transfer rate
	int arg = (data->clock_info.clock_type == NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET) ? value.int_value * 2 : value.int_value;
	// Nvidia driver might crash on an invalid argument so check it here (good job novideo)
	if (arg > node->range_info.int_range.max || arg < node->range_info.int_range.min) {
		return TC_EINVAL;
	}
	
	if (!XNVCTRLSetTargetAttributeAndGetStatus(dpy,
			NV_CTRL_TARGET_TYPE_GPU,
			data->base_data.device_index,
			data->clock_info.max_perf_state,
			data->clock_info.clock_type,
			arg)) {
		return TC_EINVAL;
	}
	return TC_SUCCESS;
}
