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

typedef struct _PulseaudioConfigClass PulseaudioConfigClass;
typedef struct _PulseaudioConfig      PulseaudioConfig;

#define TYPE_PULSEAUDIO_CONFIG             (pulseaudio_config_get_type ())
#define PULSEAUDIO_CONFIG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PULSEAUDIO_CONFIG, PulseaudioConfig))
#define PULSEAUDIO_CONFIG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_PULSEAUDIO_CONFIG, PulseaudioConfigClass))
#define IS_PULSEAUDIO_CONFIG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PULSEAUDIO_CONFIG))
#define IS_PULSEAUDIO_CONFIG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_PULSEAUDIO_CONFIG))
#define PULSEAUDIO_CONFIG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_PULSEAUDIO_CONFIG, PulseaudioConfigClass))

enum
{
  /* First two values keeps the backward compatibility */
  VOLUME_NOTIFICATIONS_NONE,
  VOLUME_NOTIFICATIONS_ALL,
  VOLUME_NOTIFICATIONS_OUTPUT,
  VOLUME_NOTIFICATIONS_INPUT,

  VOLUME_NOTIFICATIONS_COUNT,
};

enum
{
  REC_IND_POSITION_CENTER,
  REC_IND_POSITION_TOPLEFT,
  REC_IND_POSITION_TOP,
  REC_IND_POSITION_TOPRIGHT,
  REC_IND_POSITION_RIGHT,
  REC_IND_POSITION_BOTTOMRIGHT,
  REC_IND_POSITION_BOTTOM,
  REC_IND_POSITION_BOTTOMLEFT,
  REC_IND_POSITION_LEFT
};

GType              pulseaudio_config_get_type                       (void)                                       G_GNUC_CONST;

PulseaudioConfig  *pulseaudio_config_new                            (const gchar          *property_base);

gboolean           pulseaudio_config_get_enable_keyboard_shortcuts  (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_enable_multimedia_keys     (PulseaudioConfig     *config);
gboolean           pulseaudio_config_get_multimedia_keys_to_all     (PulseaudioConfig     *config);
guint              pulseaudio_config_get_show_notifications         (PulseaudioConfig     *config);
#ifdef HAVE_LIBCANBERRA
gboolean           pulseaudio_config_get_play_sound                 (PulseaudioConfig     *config);
#endif
gboolean           pulseaudio_config_get_rec_indicator_persistent   (PulseaudioConfig     *config);
guint              pulseaudio_config_get_rec_indicator_position     (PulseaudioConfig     *config);
void               pulseaudio_config_set_rec_indicator_position     (PulseaudioConfig     *config,
                                                                     gint                  rec_ind_position);
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
