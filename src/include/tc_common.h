#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
    
// Common definitions for tuxclocker

// Error values
#define TC_SUCCESS 0
#define TC_EGENERIC (-1)
#define TC_ENOMEM (-2)

// Tagged union of data types for simulating function overloading
enum tc_data_types {
    TC_TYPE_NONE,
    TC_TYPE_INT,
    TC_TYPE_UINT,
    TC_TYPE_DOUBLE,
    TC_TYPE_STRING,
    TC_TYPE_STRING_ARR
};

typedef struct {
    enum tc_data_types data_type;
    union {
        int64_t int_value;
        uint64_t uint_value;
        double double_value;
        char *string_value;
    };
} tc_variant_t;

typedef struct {
    enum tc_data_types arg_type;
    union {
        int int_arg;
        char **string_arr_arg;
    };
} tc_arg_t;

// Utility functions
// Allocate a string array on the heap
char **tc_str_arr_dup(uint16_t str_count, char **const strings);
// Deallocate string array
void tc_str_arr_free(uint16_t str_count, char **strings);

// Binary search tree whose left node contains the smaller value
typedef struct tc_bin_node_ {
    void *key;
    void *value;
    
    struct tc_bin_node_ *left;
    struct tc_bin_node_ *right;
} tc_bin_node_t;

// Create a new node with key and data in the appropriate position
tc_bin_node_t *tc_bin_node_insert(tc_bin_node_t* node, void *key, void *value);
// Find the value associated with the key
void *tc_bin_node_find_value(tc_bin_node_t *node, const void *key);
// Destroy a node and its children
void tc_bin_node_destroy(tc_bin_node_t *node);

// Function for const char* -> SHA256. Don't free().
const char *tc_sha256(const char *string, uint32_t string_length);

#ifdef __cplusplus
}
#endif
