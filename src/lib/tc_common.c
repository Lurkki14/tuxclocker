#include <tc_common.h>

#include <stdlib.h>
#include <string.h>

char **tc_str_arr_dup(int8_t str_count, char **const strings) {
    char **ptr_arr = calloc(str_count, sizeof(char*));
    for (int8_t i = 0; i < str_count; i++) {
        ptr_arr[i] = strdup(strings[i]);
    }
    
    return NULL;
}
