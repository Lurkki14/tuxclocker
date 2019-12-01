#include <tc_readable.h>

#include <stdlib.h>

// Local function for postorder traversal

static void destroy_node(tc_readable_node_t *node) {
    free(node->name);
    free(node->unit);
    free(node->children_nodes);
    free(node);
}

static void postorder_traverse(tc_readable_node_t *node, void (*func)(tc_readable_node_t*)) {
    for (uint16_t i = 0; i < node->children_count; i++) {
        postorder_traverse(node->children_nodes[i], func);
    }
    func(node);
}

tc_readable_node_t *tc_readable_node_new() {
    tc_readable_node_t *node = calloc(1, sizeof(tc_readable_node_t));
    return node;
}

void tc_readable_node_destroy(tc_readable_node_t *node) {
    postorder_traverse(node, &destroy_node);
}


int8_t tc_readable_node_add_child(tc_readable_node_t *parent, tc_readable_node_t *child) {
    parent->children_count++;
    if ((parent->children_nodes = realloc(parent->children_nodes, parent->children_count)) == NULL) {
        return TC_ENOMEM;
    }
    parent->children_nodes[parent->children_count - 1] = child;
    child->parent = parent;
    return TC_SUCCESS;
}

tc_readable_node_t *tc_readable_node_add_new_child(tc_readable_node_t *parent) {
    tc_readable_node_t *node = tc_readable_node_new();
    
    if (!node) {
        return NULL;
    }
    if (tc_readable_node_add_child(parent, node) != TC_SUCCESS) {
        tc_readable_node_destroy(node);
        return NULL;
    }
    return node;
}

tc_readable_result_t tc_readable_result_create(enum tc_data_types type, void *data, bool valid) {
    tc_readable_result_t res;
    res.valid = valid;
    res.data.data_type = type;
    
    if (!data || !valid) {
        return res;
    }
    
    switch (type) {
        case TC_TYPE_UINT:
            res.data.uint_value = *(uint64_t*) data;
            break;
        case TC_TYPE_INT:
            res.data.int_value = *(int64_t*) data;
            break;
        case TC_TYPE_DOUBLE:
            res.data.double_value = *(double*) data;
            break;
        default:
            res.valid = false;
            break;
    }
    return res;
}
