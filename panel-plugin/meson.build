plugin_sources = [
  'devicemenuitem.c',
  'devicemenuitem.h',
  'mprismenuitem.c',
  'mprismenuitem.h',
  'pulseaudio-button.c',
  'pulseaudio-button.h',
  'pulseaudio-config.c',
  'pulseaudio-config.h',
  'pulseaudio-debug.c',
  'pulseaudio-debug.h',
  'pulseaudio-dialog.c',
  'pulseaudio-dialog.h',
  'pulseaudio-menu.c',
  'pulseaudio-menu.h',
  'pulseaudio-mpris.c',
  'pulseaudio-mpris.h',
  'pulseaudio-mpris-player.c',
  'pulseaudio-mpris-player.h',
  'pulseaudio-notify.c',
  'pulseaudio-notify.h',
  'pulseaudio-plugin.c',
  'pulseaudio-plugin.h',
  'pulseaudio-volume.c',
  'pulseaudio-volume.h',
  'scalemenuitem.c',
  'scalemenuitem.h',
  xfce_revision_h,
]

plugin_sources += gnome.compile_resources(
  'pulseaudio-dialog-resources',
  'pulseaudio-dialog.gresource.xml',
  export: false,
)

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_module(
  'pulseaudio-plugin',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-pulseaudio-plugin'),
    '-DDEFAULT_MIXER_COMMAND="@0@"'.format(get_option('mixer-command')),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    exo,
    gio,
    gio_unix,
    glib,
    gtk,
    keybinder,
    libcanberra,
    libm,
    libnotify,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    libxfce4windowing,
    pulseaudio,
    xfconf,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: configure_file(
    input: 'pulseaudio.desktop.in.in',
    output: 'pulseaudio.desktop.in',
    configuration: {
      'LIBXFCE4PANEL_VERSION_API': libxfce4panel.get_variable(pkgconfig: 'api'),
    },
    install: false,
  ),
  output: 'pulseaudio.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
