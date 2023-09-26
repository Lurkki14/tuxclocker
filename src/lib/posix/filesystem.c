#include <tc_filesystem.h>
#include <tc_common.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

char **tc_fs_dir_filenames(const char *dir_name, uint16_t *file_count) {
    char *file_names[256];
    
    struct dirent *entry;
    DIR *dir = opendir(dir_name);
    
    uint16_t i = 0;
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            file_names[i] = strdup(entry->d_name);
            i++;
        }
        closedir(dir);
    }
    else {
        // Couldn't open dir
        return NULL;
    }
    *file_count = i;
    
    char **retval =  tc_str_arr_dup(i, file_names);
    
    for (int j = 0; j < i; j++) {
	    free(file_names[j]);
    }
    
    return retval;
}

bool tc_fs_file_exists(const char *path) {
    struct stat buf;
    return (stat(path, &buf) == 0);
}
