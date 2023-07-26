#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines the structure of the read-only properties of hardware

#include <tc_common.h>

#include <stdint.h>
#include <stdbool.h>

// Return value type for querying values
typedef struct {
	bool valid;
	tc_variant_t data;
} tc_readable_result_t;

typedef struct tc_readable_node_t_ {
	char *name;
	char *unit;

	// Is the value of this node constant
	bool constant;
	// Instead of an update callback store the value for constants
	union {
		tc_readable_result_t (*value_callback)(const struct tc_readable_node_t_ *);
		tc_variant_t data;
	};

	uint16_t children_count;
	struct tc_readable_node_t_ **children_nodes;
} tc_readable_node_t;

// Master data structure loaded by module loader
typedef struct {
	uint64_t category_mask; // Which types of readable (dynamic or static) are implemented
	tc_readable_node_t *root_static_node;
	tc_readable_node_t *root_node;
	const char *(*sha256_hash)(
	    const tc_readable_node_t *); // Callback to get a unique hash for a node
} tc_readable_module_data_t;

// Utility functions
// Create a new node
tc_readable_node_t *tc_readable_node_new();

// Destroys a node and all its children
void tc_readable_node_destroy(tc_readable_node_t *node);

// Add a child to a node
int8_t tc_readable_node_add_child(tc_readable_node_t *parent, tc_readable_node_t *child);

// Convinience function for creating a new node and adding it to parent
tc_readable_node_t *tc_readable_node_add_new_child(tc_readable_node_t *parent);

// Set node data
void tc_readable_node_set_data(tc_readable_node_t *node, const char *name, const char *unit);

// Create a tc_readable_result from data, data type and validity. Avoids boilerplate in returning
// values from readable nodes.
tc_readable_result_t tc_readable_result_create(enum tc_data_types type, void *data, bool valid);

#ifdef __cplusplus
}
#endif
