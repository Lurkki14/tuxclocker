i18n = import('i18n')
# define GETTEXT_PACKAGE
add_project_arguments('-DGETTEXT_PACKAGE="tuxclocker"', language:'cpp')
i18n.gettext(
        meson.project_name(),
        args : '--from-code=UTF-8')
