dnl xfce4-pulseaudio-plugin - A pulseaudio plugin for the Xfce panel
dnl
dnl 2009 Mark Trompell <mark@foresightlinux.org>
dnl

dnl ***************************
dnl *** Version information ***
dnl ***************************
XDT_VERSION_INIT([0.4.9])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2014-2023
        The Xfce development team. All rights reserved.])
AC_INIT([xfce4-pulseaudio-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-pulseaudio-plugin], [xfce4-pulseaudio-plugin])
AC_PREREQ([2.69])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET()
AC_REVISION([xdt_version_build])
AC_CONFIG_MACRO_DIRS([m4])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar foreign])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
LT_PATH_LD([])
AC_PROG_INSTALL()

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_CHECK_HEADERS([math.h memory.h stdlib.h string.h sys/stat.h \
                  sys/time.h time.h])

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
AC_CHECK_FUNCS([])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
m4_define([libxfce4panel_version_api], [2.0])
LIBXFCE4PANEL_VERSION_API=libxfce4panel_version_api()
AC_DEFINE([LIBXFCE4PANEL_VERSION_API], "libxfce4panel_version_api()", [libxfce4panel api version])
AC_SUBST([LIBXFCE4PANEL_VERSION_API])

XDT_CHECK_PACKAGE([PULSEAUDIO], [libpulse-mainloop-glib], [0.9.19])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.44.0])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [2.44.0])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [2.44.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.20.0])
XDT_CHECK_PACKAGE([EXO], [exo-2], [0.11.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.9.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.11.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-${LIBXFCE4PANEL_VERSION_API}], [4.11.0])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [4.6.0])

dnl **********************************
dnl *** Optional keybinder Support ***
dnl **********************************
XDT_CHECK_OPTIONAL_PACKAGE([KEYBINDER], [keybinder-3.0], [0.2.2], [keybinder],
                           [keybinder support])

dnl **********************************
dnl *** Optional libnotify Support ***
dnl **********************************
XDT_CHECK_OPTIONAL_PACKAGE([LIBNOTIFY], [libnotify], [0.7.0], [libnotify],
                           [libnotify library])

dnl ************************************
dnl *** Optional libcanberra Support ***
dnl ************************************
XDT_CHECK_OPTIONAL_PACKAGE([LIBCANBERRA], [libcanberra], [0.30], [libcanberra],
                           [libcanberra library])

LT_LIB_M
AC_SUBST(LIBM)

dnl *******************************
dnl *** Optional MPRIS2 Support ***
dnl *******************************
AC_ARG_ENABLE([mpris2],
              [AS_HELP_STRING([--disable-mpris2],
                              [Do not build MPRIS2 support])],
              [enable_mpris2=$enableval], [enable_mpris2=yes])
if test x"$enable_mpris2" != x"no"; then
  AC_DEFINE([HAVE_MPRIS2], [1], [Define to enable MPRIS2 support])
fi

dnl *********************************************************
dnl *** Optional (experimental) Libxfce4windowing Support ***
dnl *********************************************************
XDT_CHECK_OPTIONAL_PACKAGE([LIBXFCE4WINDOWING], [libxfce4windowing-0], [4.19.6], [libxfce4windowing],
                           [Libxfce4windowing support])

dnl To be removed when Libxfce4windowing reaches at least stable version 4.20
if test "$LIBXFCE4WINDOWING_FOUND" = "yes"; then
  enable_wnck=no
fi
XDT_CHECK_OPTIONAL_PACKAGE([WNCK], [libwnck-3.0], [3.20], [wnck], [Libwnck support])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl *****************************
dnl *** Default mixer command ***
dnl *****************************
AC_ARG_WITH([mixer-command],
            [AS_HELP_STRING([--with-mixer-command=CMD], [Default mixer command (default: pavucontrol]))],
            [DEFAULT_MIXER_COMMAND="$withval"],
            [DEFAULT_MIXER_COMMAND="pavucontrol"])
AC_SUBST([DEFAULT_MIXER_COMMAND])

dnl *********************************
dnl *** Substitute platform flags ***
dnl *********************************
AC_MSG_CHECKING([PLATFORM_CPPFLAGS])
AC_MSG_RESULT([$PLATFORM_CPPFLAGS])
AC_SUBST([PLATFORM_CPPFLAGS])
AC_MSG_CHECKING([PLATFORM_CFLAGS])
AC_MSG_RESULT([$PLATFORM_CFLAGS])
AC_SUBST([PLATFORM_CFLAGS])
AC_MSG_CHECKING([PLATFORM_LDFLAGS])
AC_MSG_RESULT([$PLATFORM_LDFLAGS])
AC_SUBST([PLATFORM_LDFLAGS])

AC_CONFIG_FILES([
Makefile
icons/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
icons/scalable/apps/Makefile
icons/scalable/status/Makefile
panel-plugin/Makefile
panel-plugin/pulseaudio.desktop.in
po/Makefile.in
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "* Debug Support:                $enable_debug"
echo "* Use keybinder:                ${KEYBINDER_FOUND:-no}"
echo "* Use libnotify:                ${LIBNOTIFY_FOUND:-no}"
echo "* Use libcanberra:              ${LIBCANBERRA_FOUND:-no}"
echo "* Use MPRIS2:                   $enable_mpris2"
if test "$LIBXFCE4WINDOWING_FOUND" = "yes"; then
echo "* Libxfce4windowing support:    ${LIBXFCE4WINDOWING_FOUND:-no}"
else
echo "* Experimenal libwnck support:  ${WNCK_FOUND:-no}"
fi
echo "* Default Mixer command:        $DEFAULT_MIXER_COMMAND"
echo
