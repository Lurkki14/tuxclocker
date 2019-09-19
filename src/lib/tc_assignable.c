#include <stdlib.h>
#include <string.h>

#include <tc_common.h>
#include <tc_assignable.h>

tc_assignable_node_t *tc_assignable_node_new() {
  tc_assignable_node_t *node = calloc(1, sizeof(tc_assignable_node_t));
  return node;
}

void tc_assignable_node_destroy(tc_assignable_node_t *node) {
  if (node->parent != NULL) {
    // Update the parent node
    // Copy all members not being deleted
    // Allocate memory for n-1 array members and copy nodes not being deleted
    tc_assignable_node_t **new_children_ptr = calloc(node->parent->children_count - 1, sizeof(tc_assignable_node_t));
    uint16_t saved = 0;
    for (uint16_t i = 0; i < node->parent->children_count; i++) {
      if (node != node->parent->children_nodes[i]) {
        memcpy(node->parent->children_nodes[saved], new_children_ptr[saved], sizeof(tc_assignable_node_t));
        saved++;
      }
    }
    // Free memory of previous array
    for (uint16_t i = 0; i < node->parent->children_count; i++) {
      tc_assignable_node_destroy(node->parent->children_nodes[i]);
    }
    free(node->parent->children_nodes);

    node->parent->children_count -= 1;
    node->parent->children_nodes = new_children_ptr;
  }

  // The name is allocated on the heap
  free(node->name);
  free(node->unit);
  free(node);
}

int8_t tc_assignable_node_add_child(tc_assignable_node_t *parent, tc_assignable_node_t *child) {
  parent->children_count++;
  if ((parent->children_nodes = realloc(parent->children_nodes, parent->children_count)) == NULL) {
    return TC_ENOMEM;
  }
  parent->children_nodes[parent->children_count] = child;
  child->parent = parent;
  return TC_SUCCESS;
}
