#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Common definitions for tuxclocker

// Error values
#define TC_SUCCESS 0
#define TC_EGENERIC (-1)
#define TC_ENOMEM (-2)

// Tagged union of data types for simulating function overloading
enum tc_arg_types {
    TC_TYPE_INT,
    TC_TYPE_STRING_ARR
};

typedef struct {
    enum tc_arg_types arg_type;
    union {
        int int_arg;
        char **string_arr_arg;
    };
} tc_arg_t;

#ifdef __cplusplus
}
#endif
