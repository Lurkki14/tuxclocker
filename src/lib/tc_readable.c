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
    return calloc(1, sizeof(tc_readable_node_t));
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
