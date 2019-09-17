#include <stdlib.h>

#include <tc_common.h>
#include <tc_assignable.h>

tc_assignable_node_t *tc_assignable_node_new() {
  tc_assignable_node_t *node = calloc(1, sizeof(tc_assignable_node_t));
  return node;
}

void tc_assignable_node_destroy(tc_assignable_node_t *node) {
  // The name is allocated on the heap
  if (node->name != NULL) {
    free(node->name);
  }

  free(node);
}

int8_t tc_assignable_node_add_child(tc_assignable_node_t *parent, tc_assignable_node_t *child) {
  parent->children_count++;
  if ((parent->children_nodes = realloc(parent->children_nodes, parent->children_count)) == NULL) {
    return TC_ENOMEM;
  }
  parent->children_nodes[parent->children_count] = child;
  return TC_SUCCESS;
}
