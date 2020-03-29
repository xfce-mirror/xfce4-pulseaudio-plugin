/*  Copyright (c) 2017 Sean Davis <bluesabre@xfce.org>
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

#ifndef __PULSEAUDIO_MPRIS_PLAYER_PLAYER_H__
#define __PULSEAUDIO_MPRIS_PLAYER_PLAYER_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * PlaybackStatus:
 * @PLAYING: A track is currently playing.
 * @PAUSED: A track is currently paused.
 * @STOPPED: There is no track currently playing.
 *
 * The current playback status:
 * See mpris2 specification <ulink url="http://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html#Property:PlaybackStatus">PlaybackStatus</ulink>
 */
typedef enum {
  PLAYING = 1,
  PAUSED,
  STOPPED
} PlaybackStatus;

GType pulseaudio_mpris_player_get_type (void);

#define TYPE_PULSEAUDIO_MPRIS_PLAYER            (pulseaudio_mpris_player_get_type())
#define PULSEAUDIO_MPRIS_PLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_PULSEAUDIO_MPRIS_PLAYER, PulseaudioMprisPlayer))
#define PULSEAUDIO_MPRIS_PLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_PULSEAUDIO_MPRIS_PLAYER, PulseaudioMprisPlayerClass))
#define IS_PULSEAUDIO_MPRIS_PLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PULSEAUDIO_MPRIS_PLAYER))
#define IS_PULSEAUDIO_MPRIS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_PULSEAUDIO_MPRIS_PLAYER))
#define PULSEAUDIO_MPRIS_PLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_PULSEAUDIO_MPRIS_PLAYER, PulseaudioMprisPlayerClass))

typedef struct          _PulseaudioMprisPlayer                     PulseaudioMprisPlayer;
typedef struct          _PulseaudioMprisPlayerClass                PulseaudioMprisPlayerClass;

PulseaudioMprisPlayer  *pulseaudio_mpris_player_new                (gchar *name);

const gchar            *pulseaudio_mpris_player_get_player         (PulseaudioMprisPlayer *player);
const gchar            *pulseaudio_mpris_player_get_player_title   (PulseaudioMprisPlayer *player);
const gchar            *pulseaudio_mpris_player_get_icon_name      (PulseaudioMprisPlayer *player);
const gchar            *pulseaudio_mpris_player_get_title          (PulseaudioMprisPlayer *player);
const gchar            *pulseaudio_mpris_player_get_artist         (PulseaudioMprisPlayer *player);
const gchar            *pulseaudio_mpris_player_get_full_path      (PulseaudioMprisPlayer *player);

gboolean                pulseaudio_mpris_player_is_connected       (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_is_playing         (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_is_stopped         (PulseaudioMprisPlayer *player);

gboolean                pulseaudio_mpris_player_can_play           (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_can_pause          (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_can_go_previous    (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_can_go_next        (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_can_raise          (PulseaudioMprisPlayer *player);
gboolean                pulseaudio_mpris_player_can_launch         (PulseaudioMprisPlayer *player);

gboolean                pulseaudio_mpris_player_is_equal           (PulseaudioMprisPlayer *a,
                                                                    PulseaudioMprisPlayer *b);

void                    pulseaudio_mpris_player_call_player_method (PulseaudioMprisPlayer *player,
                                                                    const gchar           *method);

GList                  *pulseaudio_mpris_player_get_playlists      (PulseaudioMprisPlayer *player);
void                    pulseaudio_mpris_player_activate_playlist  (PulseaudioMprisPlayer *player,
                                                                    const gchar           *playlist);

G_END_DECLS

#endif /* !__PULSEAUDIO_MPRIS_PLAYER_H__ */
