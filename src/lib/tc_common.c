#include <tc_common.h>

#include <stdlib.h>
#include <string.h>

char **tc_str_arr_dup(uint16_t str_count, char **const strings) {
    char **ptr_arr = calloc(str_count, sizeof(char*));
    for (uint16_t i = 0; i < str_count; i++) {
        ptr_arr[i] = strdup(strings[i]);
    }
    
    return ptr_arr;
}

void tc_str_arr_free(uint16_t str_count, char **strings) {
    for (uint16_t i = 0; i < str_count; i++) {
        free(strings[i]);
    }
    free(strings);
}
