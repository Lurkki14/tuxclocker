#include "ReadableManager.h"

ReadableManager::ReadableManager() {
    uint16_t mod_count = 0;
    tc_module_t **modules = tc_module_find_all_from_category(TC_CATEGORY_READABLE, &mod_count);
    
    if (!modules) {
        return;
    }
    
    for (uint16_t i = 0; i < mod_count; i++) {
        if (modules[i]->init_callback  && modules[i]->init_callback() == TC_SUCCESS) {
            // Module was initialized successfully
            
            tc_readable_node_t *root_node = static_cast<tc_readable_node_t*>(modules[i]->category_data_callback());
            if (root_node) {
                // Add to the list
                m_rootNodes.append(root_node);
                m_root = root_node;
            }
        }
    }
    
   /* tc_module_t *nv_mod = tc_module_find(TC_CATEGORY_READABLE, "nvidia");
    nv_mod->init_callback();
    tc_readable_node_t *nv_node = static_cast<tc_readable_node_t*>(nv_mod->category_data_callback());
    m_rootNodes.append(nv_node);*/
}

ReadableManager::~ReadableManager() {
}
