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
    TC_TYPE_INT,
    TC_TYPE_DOUBLE,
    TC_TYPE_STRING,
    TC_TYPE_STRING_ARR
};

typedef struct {
    enum tc_data_types data_type;
    union {
        int64_t int_value;
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

#ifdef __cplusplus
}
#endif
