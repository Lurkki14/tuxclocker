#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

// Contains functions and definitions for dealing with files

#ifdef __cplusplus
extern "C" {
#endif

// Get filename list from a directory. Return value needs to be freed.
char **tc_fs_dir_filenames(const char *dir_name, uint16_t *file_count);

// Check if a file exists
bool tc_fs_file_exists(const char *path);

#ifdef __cplusplus
}
#endif
