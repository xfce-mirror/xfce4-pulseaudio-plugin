sizes = [32, 48]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'xfce4-pulseaudio-plugin.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

install_data(
  'scalable' / 'apps' / 'xfce4-pulseaudio-plugin.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

scalable_status_names = [
  'audio-volume-high-symbolic',
  'audio-volume-low-symbolic',
  'audio-volume-medium-symbolic',
  'audio-volume-muted-symbolic',
  'microphone-sensitivity-high-symbolic',
  'microphone-sensitivity-low-symbolic',
  'microphone-sensitivity-medium-symbolic',
  'microphone-sensitivity-muted-symbolic',
]

foreach name : scalable_status_names
  install_data(
    'scalable' / 'status' / '@0@.svg'.format(name),
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'status',
  )
endforeach
