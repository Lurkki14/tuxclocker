

#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <xf86drm.h>

#include <tc_module.h>
#include <tc_readable.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
	
#include <vector>

extern int errno;

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

#define DEV_PATH_SIZE 64
#define HWMON_PATH_SIZE 96

enum node_type {
	NODE_LIBDRM, // Node uses libdrm_amdgpu to get its value
	NODE_SYSFS // Node uses sysfs to get its value
};

typedef struct {
	char dev_file_path[DEV_PATH_SIZE];
	char hwmon_path[HWMON_PATH_SIZE];
	amdgpu_device_handle dev;
} amd_fs_info;

typedef struct {
	tc_readable_node_t node;
	enum node_type type;
	union {
		amdgpu_device_handle dev;
		char hwmon_dir[HWMON_PATH_SIZE];
	};
} amd_readable_node;

typedef struct {
	tc_assignable_node_t node;
	amd_fs_info fs_info;
} amd_assignable_node;

struct module_state {
	int refcount;
	std::vector <amd_fs_info> amd_fs_info_list;
};

static module_state mod_state = {
	.refcount = 0
};

// Writes a string to specified path in sysfs
static int8_t sysfs_write(const char *value, size_t size, const char *hwmon_path, const char *filename) {
	char buf[128];
	snprintf(buf, 128, "%s/%s", hwmon_path, filename);
	
	int fd = open(buf, O_WRONLY);
	if (fd < 1) {
		return TC_EGENERIC;
	}
	
	if (write(fd, value, size) < 0) {
		// Error
		return TC_EGENERIC;
	}
	return TC_SUCCESS;
}

static void init_shared_resources() {
	struct stat buf;
	char s_buf[64];
	char hwmon_buf[128];
	
	int i = 0, j = 0;;
	
	while (1) {
		snprintf(s_buf, 64, "%s%d", DEVICE_FILE_PREFIX, i + RENDERD_OFFSET);
		if (stat(s_buf, &buf) != 0) {
			break;
		}
		// Try to get file descriptor
		drmVersionPtr v_ptr;
		int fd = open(s_buf, O_RDONLY);

		if (fd > 0 && (v_ptr = drmGetVersion(fd)) && strcmp(v_ptr->name, _AMDGPU_NAME) == 0) {
			// This device uses amdgpu
			drmFreeVersion(v_ptr);
			amd_fs_info fs_info;
			
			// Find what the hwmon folder is named (why do I have to do this?)
			DIR *dir;
			while (1) {
				snprintf(hwmon_buf, 128, "/sys/class/drm/%s%d/device/hwmon/hwmon%d", DRM_RENDER_MINOR_NAME, i + RENDERD_OFFSET, j);
				if ((dir = opendir(hwmon_buf))) {
					// Found hwmon directory
					snprintf(fs_info.dev_file_path, DEV_PATH_SIZE, "%s", s_buf);
					snprintf(fs_info.hwmon_path, HWMON_PATH_SIZE, "%s", hwmon_buf);
					closedir(dir);
					break;
				}
				j++;
			}
			amdgpu_device_handle dev;
			uint32_t mi, ma;
			if (amdgpu_device_initialize(fd, &ma, &mi, &dev) != 0) {
				continue;
			}
			fs_info.dev = dev;
			mod_state.amd_fs_info_list.push_back(fs_info);
		}
		i++;
	}
}

extern "C" {
static amd_readable_node *amd_readable_node_new() {
	return (amd_readable_node*) calloc(1, sizeof(amd_readable_node));
}

static amd_assignable_node *amd_assignable_node_new() {
	return (amd_assignable_node*) calloc(1, sizeof(amd_assignable_node));
}

static int8_t init();
static int8_t assignable_init();

// For adding readables
static void add_temp_item(tc_readable_node_t *parent, amdgpu_device_handle dev);

// Reading dynamic values
tc_readable_result_t get_temp(const tc_readable_node_t *node);
tc_readable_result_t get_core_clock(const tc_readable_node_t *node);
tc_readable_result_t get_mem_clock(const tc_readable_node_t *node);
tc_readable_result_t get_voltage(const tc_readable_node_t *node);
tc_readable_result_t get_power(const tc_readable_node_t *node);
tc_readable_result_t get_utilization(const tc_readable_node_t *node);

// Creating assignables
static void add_fanmode_item(tc_assignable_node_t *parent, amd_fs_info fs_info);
static void add_fanspeed_item(tc_assignable_node_t *parent, amd_fs_info fs_info);

// Assignment functions
static int8_t assign_fanmode(tc_variant_t value, const tc_assignable_node_t *node);
static int8_t assign_fanspeed(tc_variant_t value, const tc_assignable_node_t *node);

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

/*
 * Assignables
 */

static tc_assignable_module_data_t *assignable_mod_data() {
	static tc_assignable_module_data_t mod;
	return &mod;
}

static tc_assignable_module_data_t a_cat_data_cb() {
	tc_assignable_module_data_t *data = assignable_mod_data();
	return *data;
}

static tc_assignable_node_t *root_assignable_node() {
	static tc_assignable_node_t *node;
	return node;
}

tc_module_t *TC_MODULE_INFO_FUNCTION() {
	union module_data_callback_t u = {&cat_data_cb};
	tc_module_category_data_t cat_data = tc_module_category_data_create(TC_READABLE, u);
	cat_data.init = &init;
	
	union module_data_callback_t au = {.assignable_data = &a_cat_data_cb};
	tc_module_category_data_t a_cat_data = tc_module_category_data_create(TC_ASSIGNABLE, au);
	a_cat_data.init = &assignable_init;
	
	tc_module_category_data_t cat_list[] = {cat_data, a_cat_data};
	tc_module_category_info_t cat_info = tc_module_category_info_create(TC_READABLE | TC_ASSIGNABLE, 2, cat_list);
	
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
	if (mod_state.refcount < 1) {
		init_shared_resources();
	}
	
	for (auto &item : mod_state.amd_fs_info_list) {
		generate_dynamic_readables(r_node, item.dev, 0);
	}
	
	mod_state.refcount++;
	return TC_SUCCESS;
}

static void generate_assignables(tc_assignable_node_t *parent, amd_fs_info fs_info) {
	auto gpu_name_node = tc_assignable_node_new();
	gpu_name_node->name = (char*) amdgpu_get_marketing_name(fs_info.dev);
	
	tc_assignable_node_add_child(parent, gpu_name_node);
	
	add_fanmode_item(gpu_name_node, fs_info);
	
	add_fanspeed_item(gpu_name_node, fs_info);
}

static int8_t assignable_init() {
	auto a_node = root_assignable_node();
	a_node = tc_assignable_node_new();
	
	auto m_data = assignable_mod_data();
	m_data->root_node = a_node;
	
	if (mod_state.refcount < 1) {
		init_shared_resources();
	}
	
	for (auto &item : mod_state.amd_fs_info_list) {
		generate_assignables(a_node, item);
	}
	
	mod_state.refcount++;
	return TC_SUCCESS;
}

static void add_fanmode_item(tc_assignable_node_t *parent, amd_fs_info fs_info) {
	// Check if pwm1_enable exists
	struct stat buf;
	char path[128];
	snprintf(path, 128, "%s/pwm1_enable", fs_info.hwmon_path);
	
	if (stat(path, &buf) != 0) {
		return;
	}
	auto item = amd_assignable_node_new();
	item->fs_info = fs_info;
	
	auto node = &(item->node);
	if (tc_assignable_node_add_child(parent, node) != TC_SUCCESS) {
		delete item;
		return;
	}
	
	// Create options
	 char *opts[] = {"auto", "manual"};
	auto list = tc_str_arr_dup(2, opts);
	
	tc_assignable_enum_t enum_info = {
		2,
		list
	};
	node->value_category = TC_ASSIGNABLE_ENUM;
	node->enum_info = enum_info;
	
	tc_assignable_node_set_data(node, NULL, "Fan Mode", &assign_fanmode);
}

static void add_fanspeed_item(tc_assignable_node_t *parent, amd_fs_info fs_info) {
	struct stat buf;
	char path[128];
	snprintf(path, 128, "%s/pwm1", fs_info.hwmon_path);
	
	if (stat(path, &buf) != 0) {
		return;
	}
	auto item = amd_assignable_node_new();
	item->fs_info = fs_info;
	
	auto node = &(item->node);
	if (tc_assignable_node_add_child(parent, node) != TC_SUCCESS) {
		delete item;
		return;
	}
	
	tc_assignable_range_int_t range = {
		.min = 0,
		.max = 100
	};
	node->value_category = TC_ASSIGNABLE_RANGE;
	node->range_info.range_data_type = TC_ASSIGNABLE_RANGE_INT;
	node->range_info.int_range = range;
	
	tc_assignable_node_set_data(node, "%", "Fan Speed", &assign_fanspeed);
}

static int8_t assign_fanmode(tc_variant_t value, const tc_assignable_node_t *node) {
	if (value.data_type != TC_TYPE_UINT || !node || value.uint_value > 1) {
		return TC_EINVAL;
	}
	auto a_node = (const amd_assignable_node*) node;
	// Fanmode enum depends on the family
	amdgpu_gpu_info info;
	if (amdgpu_query_gpu_info(a_node->fs_info.dev, &info) != 0) {
		return TC_EGENERIC;
	}
	int target = 1;
	if (value.uint_value == 0) {
		target = (info.family_id < AMDGPU_FAMILY_KV) ? 0 : 2;
	}
	
	char buf[2];
	snprintf(buf, 2, "%d", target);
	
	return sysfs_write(buf, strlen(buf), a_node->fs_info.hwmon_path, "pwm1_enable");
}

static int8_t assign_fanspeed(tc_variant_t value, const tc_assignable_node_t *node) {
	if (value.data_type != TC_TYPE_INT || !node || value.int_value < 0 || value.int_value > 100) {
		return TC_EINVAL;
	}
	auto a_node = (const amd_assignable_node*) node;
	char buf[4];
	snprintf(buf, 4, "%d", (int) round(value.int_value * 2.55));
	return sysfs_write(buf, strlen(buf), a_node->fs_info.hwmon_path, "pwm1");
}

void add_temp_item(tc_readable_node_t *parent, amdgpu_device_handle dev) {
	LIBDRM_ADD_NODE(dev, parent, uint32_t, AMDGPU_INFO_SENSOR_GPU_TEMP, "Temperature", "C", &get_temp)
}

tc_readable_result_t get_temp(const tc_readable_node_t *node) {
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

}
