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
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PULSEAUDIO_TYPE_CONFIG (pulseaudio_config_get_type ())
G_DECLARE_FINAL_TYPE (PulseaudioConfig, pulseaudio_config, PULSEAUDIO, CONFIG, GObject)

enum
{
  /* First two values keeps the backward compatibility */
  VOLUME_NOTIFICATIONS_NONE,
  VOLUME_NOTIFICATIONS_ALL,
  VOLUME_NOTIFICATIONS_OUTPUT,
  VOLUME_NOTIFICATIONS_INPUT,

  VOLUME_NOTIFICATIONS_COUNT,
};

PulseaudioConfig  *pulseaudio_config_new                            (const gchar          *property_base);

gboolean           pulseaudio_config_get_enable_keyboard_shortcuts  (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_enable_multimedia_keys     (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_multimedia_keys_to_all     (PulseaudioConfig     *config);
guint              pulseaudio_config_get_show_notifications         (PulseaudioConfig     *config);
#ifdef HAVE_LIBCANBERRA
gboolean           pulseaudio_config_get_play_sound                 (PulseaudioConfig     *config);
#endif
gboolean           pulseaudio_config_get_rec_indicator_persistent   (PulseaudioConfig     *config);
guint              pulseaudio_config_get_volume_step                (PulseaudioConfig     *config);
guint              pulseaudio_config_get_volume_max                 (PulseaudioConfig     *config);
const gchar       *pulseaudio_config_get_mixer_command              (PulseaudioConfig     *config);
gchar            **pulseaudio_config_get_known_players              (PulseaudioConfig     *config);

gboolean           pulseaudio_config_get_enable_mpris               (PulseaudioConfig     *config);
void               pulseaudio_config_set_mpris_players              (PulseaudioConfig     *config,
                                                                     gchar               **players);
void               pulseaudio_config_add_known_player               (PulseaudioConfig     *config,
                                                                     const gchar          *player);

void               pulseaudio_config_player_ignored_add             (PulseaudioConfig     *config,
                                                                     const gchar          *player);
void               pulseaudio_config_player_ignored_remove          (PulseaudioConfig     *config,
                                                                     const gchar          *player);
gboolean           pulseaudio_config_player_ignored_lookup          (PulseaudioConfig     *config,
                                                                     const gchar          *player);

void               pulseaudio_config_player_persistent_add          (PulseaudioConfig     *config,
                                                                     const gchar          *player);
void               pulseaudio_config_player_persistent_remove       (PulseaudioConfig     *config,
                                                                     const gchar          *player);
gboolean           pulseaudio_config_player_persistent_lookup       (PulseaudioConfig     *config,
                                                                     const gchar          *player);

void               pulseaudio_config_clear_known_players            (PulseaudioConfig     *config);

void               pulseaudio_config_set_can_raise_wnck             (PulseaudioConfig     *config,
                                                                     gboolean              can_raise);
gboolean           pulseaudio_config_get_can_raise_wnck             (PulseaudioConfig     *config);

G_END_DECLS

#endif /* !__PULSEAUDIO_CONFIG_H__ */
