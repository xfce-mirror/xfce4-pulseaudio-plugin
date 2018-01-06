/*
 *  Copyright (C) 2014-2017 Andrzej <andrzejr@xfce.org>
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

#ifndef __PULSEAUDIO_CONFIG_H__
#define __PULSEAUDIO_CONFIG_H__

#include <glib.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

typedef struct _PulseaudioConfigClass PulseaudioConfigClass;
typedef struct _PulseaudioConfig      PulseaudioConfig;

#define TYPE_PULSEAUDIO_CONFIG             (pulseaudio_config_get_type ())
#define PULSEAUDIO_CONFIG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PULSEAUDIO_CONFIG, PulseaudioConfig))
#define PULSEAUDIO_CONFIG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_PULSEAUDIO_CONFIG, PulseaudioConfigClass))
#define IS_PULSEAUDIO_CONFIG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PULSEAUDIO_CONFIG))
#define IS_PULSEAUDIO_CONFIG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_PULSEAUDIO_CONFIG))
#define PULSEAUDIO_CONFIG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_PULSEAUDIO_CONFIG, PulseaudioConfigClass))

GType              pulseaudio_config_get_type                       (void)                                       G_GNUC_CONST;

PulseaudioConfig  *pulseaudio_config_new                            (const gchar          *property_base);

gboolean           pulseaudio_config_get_enable_keyboard_shortcuts  (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_enable_multimedia_keys     (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_show_notifications         (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_allow_louder_than_hundred  (PulseaudioConfig     *config);
guint              pulseaudio_config_get_volume_step                (PulseaudioConfig     *config);
guint              pulseaudio_config_get_volume_max                 (PulseaudioConfig     *config);
const gchar       *pulseaudio_config_get_mixer_command              (PulseaudioConfig     *config);
gchar            **pulseaudio_config_get_mpris_players              (PulseaudioConfig     *config);

gboolean           pulseaudio_config_get_enable_mpris               (PulseaudioConfig     *config);
void               pulseaudio_config_set_mpris_players              (PulseaudioConfig     *config,
                                                                     gchar               **players);
void               pulseaudio_config_add_mpris_player               (PulseaudioConfig     *config,
                                                                     gchar                *player);

void               pulseaudio_config_set_can_raise_wnck             (PulseaudioConfig     *config,
                                                                     gboolean              can_raise);
gboolean           pulseaudio_config_get_can_raise_wnck             (PulseaudioConfig     *config);

G_END_DECLS

#endif /* !__PULSEAUDIO_CONFIG_H__ */
