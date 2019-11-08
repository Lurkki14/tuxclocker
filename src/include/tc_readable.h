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
    
typedef struct tc_readable_node_t_{
    char *name;
    char *unit;
    
    // Is the value of this node constant
    bool constant;
    // Instead of an update callback store the value for constants
    union {
        tc_readable_result_t (*value_callback)(struct tc_readable_node_t_*);
        tc_variant_t data;
    };
    struct tc_readable_node_t_ *parent;
    
    uint16_t children_count;
    struct tc_readable_node_t_ **children_nodes;
} tc_readable_node_t;

// Utility functions
// Create a new node
tc_readable_node_t *tc_readable_node_new();

// Destroys a node and all its children
void tc_readable_node_destroy(tc_readable_node_t *node);

// Add a child to a node
int8_t tc_readable_node_add_child(tc_readable_node_t *parent, tc_readable_node_t *child);

// Convinience function for creating a new node and adding it to parent
tc_readable_node_t *tc_readable_node_add_new_child(tc_readable_node_t *parent);

#ifdef __cplusplus
}
#endif
