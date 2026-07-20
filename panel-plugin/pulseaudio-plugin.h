/*  Copyright (c) 2014-2017 Andrzej <andrzejr@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __PULSEAUDIO_PLUGIN_H__
#define __PULSEAUDIO_PLUGIN_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

#define PULSEAUDIO_TYPE_PLUGIN (pulseaudio_plugin_get_type ())
G_DECLARE_FINAL_TYPE (PulseaudioPlugin, pulseaudio_plugin, PULSEAUDIO, PLUGIN, XfcePanelPlugin)

void  pulseaudio_plugin_register_type (XfcePanelTypeModule *type_module);

#ifdef HAVE_LIBCANBERRA
void  pulseaudio_plugin_play_sound    (PulseaudioPlugin    *pulseaudio_plugin,
                                       const char          *event_id,
                                       const char          *event_desc);
#endif

G_END_DECLS

#endif /* !__PULSEAUDIO_PLUGIN_H__ */
