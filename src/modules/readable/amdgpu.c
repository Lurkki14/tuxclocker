#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <xf86drm.h>

#include <tc_module.h>
#include <tc_readable.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

// Prefix for device files eg. /dev/dri/renderD
#define _AMDGPU_NAME "amdgpu"
#define DEVICE_FILE_PREFIX DRM_DIR_NAME "/" DRM_RENDER_MINOR_NAME
#define RENDERD_OFFSET 128

// Macro abuse for libdrm queries
#define LIBDRM_RETURN_QUERY(type, ret_type, tc_type, node, sensor_type, operator, operand) \
	bool valid = false; \
	if (!node) { \
		return tc_readable_result_create(TC_TYPE_NONE, NULL, valid); \
	} \
	const amd_readable_node *a_node = (const amd_readable_node*) node; \
	type value; \
	if (amdgpu_query_sensor_info(a_node->dev, sensor_type, sizeof(value), &value) == 0) { \
		valid = true; \
	} \
	ret_type retval = value operator operand; \
	return tc_readable_result_create(tc_type, &retval, valid); \
	
#define LIBDRM_ADD_NODE(dev, parent, type_, sensor_type, name, unit, callback) { \
	type_ value; \
	if (amdgpu_query_sensor_info(dev, sensor_type, sizeof(value), &value) == 0) { \
		amd_readable_node *node = amd_readable_node_new(); \
		node->type = NODE_LIBDRM; \
		node->dev = dev; \
		tc_readable_node_t *r_node = &(node->node); \
		if (tc_readable_node_add_child(parent, r_node) != TC_SUCCESS) { \
			return; \
		} \
		tc_readable_node_set_data(r_node, name, unit); \
		r_node->value_callback = callback; \
	} \
} \

enum node_type {
	NODE_LIBDRM, // Node uses libdrm_amdgpu to get its value
	NODE_SYSFS // Node uses sysfs to get its value
};

typedef struct {
	tc_readable_node_t node;
	enum node_type type;
	union {
		amdgpu_device_handle dev;
		char hwmon_dir[64];
	};
} amd_readable_node;

static amd_readable_node *amd_readable_node_new() {
	return calloc(1, sizeof(amd_readable_node));
}

static int8_t init();

// For adding readables
static void add_temp_item(tc_readable_node_t *parent, amdgpu_device_handle dev);

// Reading dynamic values
tc_readable_result_t get_temp(const tc_readable_node_t *node);
tc_readable_result_t get_core_clock(const tc_readable_node_t *node);
tc_readable_result_t get_mem_clock(const tc_readable_node_t *node);
tc_readable_result_t get_voltage(const tc_readable_node_t *node);
tc_readable_result_t get_power(const tc_readable_node_t *node);
tc_readable_result_t get_utilization(const tc_readable_node_t *node);

static tc_readable_module_data_t *readable_mod_data() {
	static tc_readable_module_data_t mod = {
		.category_mask = (TC_READABLE_DYNAMIC)
	};
	
	return &mod;
}

tc_readable_module_data_t cat_data_cb() {
	tc_readable_module_data_t *data = readable_mod_data();
	
	return *data;
}

static tc_module_t *mod_data() {
	static tc_module_t mod = {
		.category = TC_CATEGORY_READABLE,
		.init_callback = &init
	};
	return &mod;
}

static tc_readable_node_t *root_node() {
	static tc_readable_node_t *node;
	return node;
}

tc_module_t *TC_MODULE_INFO_FUNCTION() {
	union module_data_callback_t u = {&cat_data_cb};
	tc_module_category_data_t cat_data = tc_module_category_data_create(TC_READABLE, u);
	
	tc_module_category_data_t cat_list[] = {cat_data};
	tc_module_category_info_t cat_info = tc_module_category_info_create(TC_READABLE, 1, cat_list);
	
	tc_module_t *mod = mod_data();
	
	mod->category_info = cat_info;
	
	return mod;
}

void generate_dynamic_readables(tc_readable_node_t *parent, amdgpu_device_handle dev, int dev_index) {
	// Generates the GPU name item and subitems
	tc_readable_node_t *gpu_name_node = tc_readable_node_new();
	gpu_name_node->name = (char*) amdgpu_get_marketing_name(dev);
	
	tc_readable_node_add_child(parent, gpu_name_node);
	
	add_temp_item(gpu_name_node, dev);
	LIBDRM_ADD_NODE(dev, gpu_name_node, uint32_t, AMDGPU_INFO_SENSOR_GFX_SCLK, "Core Clock", "MHz", &get_core_clock)
	LIBDRM_ADD_NODE(dev, gpu_name_node, uint32_t, AMDGPU_INFO_SENSOR_GFX_MCLK, "Memory Clock", "MHz", &get_mem_clock)
	LIBDRM_ADD_NODE(dev, gpu_name_node, uint32_t, AMDGPU_INFO_SENSOR_VDDGFX, "Core Voltage", "mV", &get_voltage)
	LIBDRM_ADD_NODE(dev, gpu_name_node, double, AMDGPU_INFO_SENSOR_GPU_AVG_POWER, "Power Usage", "W", &get_power)
	LIBDRM_ADD_NODE(dev, gpu_name_node, uint32_t, AMDGPU_INFO_SENSOR_GPU_LOAD, "Core Utilization", "%", &get_utilization)
}

static int8_t init() {
	tc_readable_node_t *r_node = root_node();
	r_node = tc_readable_node_new();
	
	tc_readable_module_data_t *m_data = readable_mod_data();
	m_data->root_node = r_node;
	
	// Get device files using amdgpu
	struct stat buf;
	char dev_filename[64];
	
	int i = 0;
	
	while (1) {
		//snprintf(dev_filename, 64, "%s%d", DEVICE_FILE_PREFIX, i + RENDERD_OFFSET);
		if (stat(dev_filename, &buf) != 0) {
			break;
		}
		// Try to get file descriptor
		drmVersionPtr v_ptr;
		int fd = open(dev_filename, O_RDONLY);
		
		if (fd > 0 && (v_ptr = drmGetVersion(fd)) && strcmp(v_ptr->name, _AMDGPU_NAME) == 0) {
			// This device uses amdgpu
			printf("%s\n", dev_filename);
			drmFreeVersion(v_ptr);
			
			// Generate readable tree for this GPU
			amdgpu_device_handle dev;
			uint32_t mi, ma;
			if (amdgpu_device_initialize(fd, &ma, &mi, &dev) == 0) {
				generate_dynamic_readables(r_node, dev, i + RENDERD_OFFSET);
			}
		}
		i++;
	}
	
	return TC_SUCCESS;
}

void add_temp_item(tc_readable_node_t *parent, amdgpu_device_handle dev) {
	/*uint32_t temp;
	if (amdgpu_query_sensor_info(dev, AMDGPU_INFO_SENSOR_GPU_TEMP, sizeof(temp), &temp) == 0) {
		// Success
		amd_readable_node *node = amd_readable_node_new();
		node->type = NODE_LIBDRM;
		node->dev = dev;
		
		tc_readable_node_t *r_node = &(node->node);
		if (tc_readable_node_add_child(parent, r_node) != TC_SUCCESS) {
			return;
		}
		tc_readable_node_set_data(r_node, "Temperature", "C");
		r_node->value_callback = &get_temp;
	}*/
	LIBDRM_ADD_NODE(dev, parent, uint32_t, AMDGPU_INFO_SENSOR_GPU_TEMP, "Temperature", "C", &get_temp)
}

tc_readable_result_t get_temp(const tc_readable_node_t *node) {
	/*bool valid = false;
	if (!node) {
		return tc_readable_result_create(TC_TYPE_NONE, NULL, valid);
	}
	const amd_readable_node *a_node = (const amd_readable_node*) node;
	uint32_t temp;
	if (amdgpu_query_sensor_info(a_node->dev, AMDGPU_INFO_SENSOR_GPU_TEMP, sizeof(temp), &temp) == 0) {
		valid = true;
	}
	uint64_t temp_ = temp / 1000;
	return tc_readable_result_create(TC_TYPE_UINT, &temp_, valid);*/
	LIBDRM_RETURN_QUERY(uint32_t, uint64_t, TC_TYPE_UINT, node, AMDGPU_INFO_SENSOR_GPU_TEMP, /, 1000)
}

tc_readable_result_t get_core_clock(const tc_readable_node_t *node) {
	LIBDRM_RETURN_QUERY(uint32_t, uint64_t, TC_TYPE_UINT, node, AMDGPU_INFO_SENSOR_GFX_SCLK, +, 0)
}

tc_readable_result_t get_mem_clock(const tc_readable_node_t *node) {
	LIBDRM_RETURN_QUERY(uint32_t, uint64_t, TC_TYPE_UINT, node, AMDGPU_INFO_SENSOR_GFX_MCLK, +, 0)
}

tc_readable_result_t get_voltage(const tc_readable_node_t *node) {
	LIBDRM_RETURN_QUERY(uint32_t, uint64_t, TC_TYPE_UINT, node, AMDGPU_INFO_SENSOR_VDDGFX, +, 0)
}

tc_readable_result_t get_power(const tc_readable_node_t *node) {
	LIBDRM_RETURN_QUERY(double, double, TC_TYPE_DOUBLE, node, AMDGPU_INFO_SENSOR_GPU_AVG_POWER, +, 0)
}

tc_readable_result_t get_utilization(const tc_readable_node_t *node) {
	LIBDRM_RETURN_QUERY(uint32_t, uint64_t, TC_TYPE_UINT, node, AMDGPU_INFO_SENSOR_GPU_LOAD, +, 0)
}
