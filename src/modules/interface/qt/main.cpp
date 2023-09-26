#include "MainWindow.h"

#include <tc_interface.h>
#include <tc_module.h>

#include <QApplication>

int main(int argc, char **argv);

// Don't scramble the interaction point symbols
extern "C" {

// Module Information
static tc_module_t mod_info = {
    .category = TC_CATEGORY_INTERFACE,
    .name = "qt",
    .description = "Qt Interface",
    .init_callback = (int8_t (*)()) &main,
    .init_callback_argc = 2,
    .init_callback_args = {TC_TYPE_INT, TC_TYPE_STRING_ARR},
    .close_callback = NULL
};

tc_module_t *TC_MODULE_INFO_FUNCTION();

tc_module_t *TC_MODULE_INFO_FUNCTION() {
    return &mod_info;
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    
    MainWindow mw;
    mw.show();
    
    return app.exec();
}
