#pragma once

#include <stdint.h>

// Categories for modules.
enum tc_module_category {
  TC_CATEGORY_ASSIGNABLE,
  TC_CATEGORY_PROPERTY
};



typedef struct tc_module_t {
  enum tc_module_category category;
  const char *name;

  // Initializes the module's internal state
  int8_t (*init_callback)();
  // Frees the internal memory of the module
  int8_t (*close_callback)();

} tc_module_t;
