/*  Copyright (c) 2017-2020 Sean Davis <bluesabre@xfce.org>
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

#ifndef __PULSEAUDIO_MPRIS_H__
#define __PULSEAUDIO_MPRIS_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "pulseaudio-config.h"

G_BEGIN_DECLS

GType pulseaudio_mpris_get_type (void);

#define TYPE_PULSEAUDIO_MPRIS            (pulseaudio_mpris_get_type())
#define PULSEAUDIO_MPRIS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_PULSEAUDIO_MPRIS, PulseaudioMpris))
#define PULSEAUDIO_MPRIS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_PULSEAUDIO_MPRIS, PulseaudioMprisClass))
#define IS_PULSEAUDIO_MPRIS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PULSEAUDIO_MPRIS))
#define IS_PULSEAUDIO_MPRIS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_PULSEAUDIO_MPRIS))
#define PULSEAUDIO_MPRIS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_PULSEAUDIO_MPRIS, PulseaudioMprisClass))

typedef struct          _PulseaudioMpris                       PulseaudioMpris;
typedef struct          _PulseaudioMprisClass                  PulseaudioMprisClass;

PulseaudioMpris        *pulseaudio_mpris_new                   (PulseaudioConfig *config);

gboolean                pulseaudio_mpris_get_player_snapshot   (PulseaudioMpris  *mpris,
                                                                const gchar      *name,
                                                                gchar           **title,
                                                                gchar           **artist,
                                                                gboolean         *is_playing,
                                                                gboolean         *is_stopped,
                                                                gboolean         *can_play,
                                                                gboolean         *can_pause,
                                                                gboolean         *can_go_previous,
                                                                gboolean         *can_go_next,
                                                                gboolean         *can_raise,
                                                                GList           **playlists);

gboolean                pulseaudio_mpris_get_player_summary    (const gchar      *player_name,
                                                                gchar           **player_label,
                                                                gchar           **icon_name,
                                                                gchar           **full_path);

gboolean                pulseaudio_mpris_notify_player         (PulseaudioMpris  *mpris,
                                                                const gchar      *name,
                                                                const gchar      *message);

gboolean                pulseaudio_mpris_notify_any_player     (PulseaudioMpris *mpris,
                                                                const gchar     *message);

gboolean                pulseaudio_mpris_activate_playlist     (PulseaudioMpris *mpris,
                                                                const gchar     *name,
                                                                const gchar     *playlist);

    G_END_DECLS

#endif /* !__PULSEAUDIO_MPRIS_H__ */
