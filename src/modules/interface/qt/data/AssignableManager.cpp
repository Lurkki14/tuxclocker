#include "AssignableManager.h"

AssignableManager::AssignableManager() {
    // Open all assignable modules
    uint16_t mod_count;
    tc_module_t **assignableModules =  tc_module_find_all_from_category(TC_CATEGORY_ASSIGNABLE, &mod_count);
    
    // Try to initialize every module
    for (uint16_t i = 0; i < mod_count; i++) {
        if (assignableModules[i]->init_callback != NULL) {
            if (assignableModules[i]->init_callback() != TC_SUCCESS) {
                // Couldn't initialize, close module
                tc_module_close(assignableModules[i]);
            }
            // Add module to the list
            m_assignableModules.append(assignableModules[i]);
        }
    }
    // Obtain root node from each module
    for (tc_module_t *module : m_assignableModules) {
        tc_assignable_node_t *root = static_cast<tc_assignable_node_t*>(module->category_data_callback());
        if (root == NULL) {
            m_assignableModules.remove(m_assignableModules.indexOf(module));
        }
        m_assignableRootNodes.append(root);
    }
            
    delete assignableModules;
}

AssignableManager::~AssignableManager() {
    // Close all modules
    for (tc_module_t *module : m_assignableModules) {
        tc_module_close(module);
    }
}
