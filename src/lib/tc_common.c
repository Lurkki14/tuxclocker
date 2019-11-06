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

static tc_bin_node_t *new_bin_node(void *key, void *value) {
    tc_bin_node_t *new_node = calloc(1, sizeof(tc_bin_node_t));
    new_node->key = key;
    new_node->value = value;
    return new_node;
}

static void single_destroy(tc_bin_node_t *node) {
    free(node);
}

static void traverse_postorder(tc_bin_node_t *node, void (*func)(tc_bin_node_t*)) {
    if (node->left) {
        traverse_postorder(node, func);
    }
    if (node->right) {
        traverse_postorder(node, func);
    }
    func(node);
}

tc_bin_node_t *tc_bin_node_insert(tc_bin_node_t *node, void *key, void *value) {
    if (node == NULL) {
        return new_bin_node(key, value);
    }
    
    if (key < node->key) {
        node->left = tc_bin_node_insert(node->left, key, value);
    }
    else if (key > node->key) {
        node->right = tc_bin_node_insert(node->right, key, value);
    }
    return node;
}

void *tc_bin_node_find_value(tc_bin_node_t *node, void *key) {
    if (node == NULL) {
        return NULL;
    }
    if (node->key == key) {
        return node->value;
    }
    
    if (node->key < key) {
        return tc_bin_node_find_value(node->right, key);
    }
    return tc_bin_node_find_value(node->left, key);
}

void tc_bin_node_destroy(tc_bin_node_t *node) {
    traverse_postorder(node, &single_destroy);
}
