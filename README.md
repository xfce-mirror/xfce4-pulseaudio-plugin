[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-pulseaudio-plugin/-/blob/master/COPYING)

# xfce4-pulseaudio-plugin

The Xfce PulseAudio Plugin is a plugin for the Xfce panel which provides a
convenient way to adjust the audio volume of the [PulseAudio sound system](http://www.freedesktop.org/wiki/Software/PulseAudio/) and to an auto mixer tool like pavucontrol.

It can optionally handle multimedia keys for controlling the audio volume.

----

### Homepage

[Xfce4-pulseaudio-plugin documentation](https://docs.xfce.org/panel-plugins/xfce4-pulseaudio-plugin)

### Changelog

See [NEWS](https://gitlab.xfce.org/panel-plugins/xfce4-pulseaudio-plugin/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[Xfce4-pulseaudio-plugin source code](https://gitlab.xfce.org/panel-plugins/xfce4-pulseaudio-plugin)

### Download a Release Tarball

[Xfce4-pulseaudio-plugin archive](https://archive.xfce.org/src/panel-plugins/xfce4-pulseaudio-plugin)
    or
[Xfce4-pulseaudio-plugin tags](https://gitlab.xfce.org/panel-plugins/xfce4-pulseaudio-plugin/-/tags)

### Installation

From source code repository: 

    % cd xfce4-pulseaudio-plugin
    % meson setup build
    % meson compile -C build
    % meson install -C build

From release tarball:

    % tar xf xfce4-pulseaudio-plugin-<version>.tar.xz
    % cd xfce4-pulseaudio-plugin-<version>
    % meson setup build
    % meson compile -C build
    % meson install -C build

### Uninstallation

    % ninja uninstall -C build

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/panel-plugins/xfce4-pulseaudio-plugin/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

