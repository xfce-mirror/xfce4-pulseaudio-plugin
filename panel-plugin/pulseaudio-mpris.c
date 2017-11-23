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



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "pulseaudio-mpris.h"
#include "pulseaudio-mpris-player.h"

struct _PulseaudioMpris
{
  GObject           __parent__;
  PulseaudioConfig *config;

  GDBusConnection  *dbus_connection;
  GHashTable       *players;

  guint             player_timer_id;
};

struct _PulseaudioMprisClass
{
  GObjectClass          __parent__;
  void (*update)        (PulseaudioMpris *mpris);
};


static void             pulseaudio_mpris_finalize         (GObject         *object);


enum
{
  UPDATE,
  LAST_SIGNAL
};
static int signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (PulseaudioMpris, pulseaudio_mpris, G_TYPE_OBJECT)

static void
pulseaudio_mpris_class_init (PulseaudioMprisClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_mpris_finalize;

  signals[UPDATE] =
      g_signal_new ("update",
                    G_TYPE_FROM_CLASS (gobject_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (PulseaudioMprisClass, update),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING);
}


gchar **
pulseaudio_mpris_get_available_players (PulseaudioMpris *mpris)
{
  GError *error = NULL;
  GVariant *v;
  GVariantIter *iter;
  const gchar *str = NULL;
  gchar **res = NULL;
  guint items = 0;

  v = g_dbus_connection_call_sync (mpris->dbus_connection,
                                   "org.freedesktop.DBus",
                                   "/org/freedesktop/DBus",
                                   "org.freedesktop.DBus",
                                   "ListNames",
                                   NULL,
                                   G_VARIANT_TYPE ("(as)"),
                                   G_DBUS_CALL_FLAGS_NONE,
                                   -1,
                                   NULL,
                                   &error);
  if (error)
    {
      g_critical ("Could not get a list of names registered on the session bus, %s",
                  error ? error->message : "no error given");
      g_clear_error (&error);
      return NULL;
    }

  g_variant_get (v, "(as)", &iter);
  while (g_variant_iter_loop (iter, "&s", &str))
    {
      if (g_str_has_prefix(str, "org.mpris.MediaPlayer2."))
        {
          res = (gchar**)g_realloc(res, (items + 1) * sizeof(gchar*));
          res[items] = g_strdup(str + 23);
          items++;
        }
    }

  /* Add NULL termination to the res vector */
  if (items > 0)
    {
      res = g_realloc(res, (items + 1) * sizeof(gchar*));
      res[items] = NULL;
    }

  g_variant_iter_free (iter);
  g_variant_unref (v);

  return res;
}

static void
pulseaudio_mpris_player_update_cb (PulseaudioMprisPlayer *player,
                                   gchar                 *sender_name,
                                   gpointer               user_data)
{
  PulseaudioMpris *mpris = user_data;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));

  g_signal_emit (mpris, signals[UPDATE], 0, pulseaudio_mpris_player_get_player (player));
}

static void
pulseaudio_mpris_player_metadata_cb (PulseaudioMprisPlayer *player,
                                     gpointer               user_data)
{
  PulseaudioMpris *mpris = user_data;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));

  g_signal_emit (mpris, signals[UPDATE], 0, pulseaudio_mpris_player_get_player (player));
}

static gboolean
pulseaudio_mpris_tick_cb (gpointer user_data)
{
  PulseaudioMpris        *mpris = user_data;
  PulseaudioMprisPlayer  *player;
  gchar                 **players;

  players = pulseaudio_mpris_get_available_players (mpris);
  if (players == NULL)
    return TRUE;

  for (guint i = 0; i < g_strv_length (players); i++)
    {
      if (!g_hash_table_contains (mpris->players, players[i]))
        {
          player = pulseaudio_mpris_player_new (players[i]);

          g_signal_connect (player, "connection", G_CALLBACK (pulseaudio_mpris_player_update_cb), mpris);
          g_signal_connect (player, "playback-status", G_CALLBACK (pulseaudio_mpris_player_update_cb), mpris);
          g_signal_connect (player, "metadata", G_CALLBACK (pulseaudio_mpris_player_metadata_cb), mpris);

          g_hash_table_insert (mpris->players, players[i], player);

          pulseaudio_config_add_mpris_player (mpris->config, players[i]);
        }
    }

  return TRUE;
}

gboolean
pulseaudio_mpris_get_player_snapshot (PulseaudioMpris  *mpris,
                                      const gchar      *name,
                                      gchar           **title,
                                      gchar           **artist,
                                      gboolean         *is_running,
                                      gboolean         *is_playing,
                                      gboolean         *is_stopped,
                                      gboolean         *can_play,
                                      gboolean         *can_pause,
                                      gboolean         *can_go_previous,
                                      gboolean         *can_go_next,
                                      gboolean         *can_raise)
{
  PulseaudioMprisPlayer *player;
  player = PULSEAUDIO_MPRIS_PLAYER (g_hash_table_lookup (mpris->players, name));

  if (player != NULL)
    {
      if (pulseaudio_mpris_player_is_connected (player))
        {
          *title = g_strdup(pulseaudio_mpris_player_get_title (player));
          *artist = g_strdup(pulseaudio_mpris_player_get_artist (player));

          *is_running         = TRUE;
          *is_playing         = pulseaudio_mpris_player_is_playing (player);
          *is_stopped         = pulseaudio_mpris_player_is_stopped (player);
          *can_play           = pulseaudio_mpris_player_can_play (player);
          *can_pause          = pulseaudio_mpris_player_can_pause (player);
          *can_go_previous    = pulseaudio_mpris_player_can_go_previous (player);
          *can_go_next        = pulseaudio_mpris_player_can_go_next (player);
          *can_raise          = pulseaudio_mpris_player_can_raise (player);
        }
      else
        {
          *title = g_strdup(pulseaudio_mpris_player_get_player_title (player));
          *artist = g_strdup("Not currently playing");

          *is_running         = FALSE;
          *is_playing         = FALSE;
          *is_stopped         = TRUE;
          *can_play           = FALSE;
          *can_pause          = FALSE;
          *can_go_previous    = FALSE;
          *can_go_next        = FALSE;
          *can_raise          = FALSE;
        }
      if (*title == NULL || g_strcmp0 (*title, "") == 0)
        *title = g_strdup(pulseaudio_mpris_player_get_player_title (player));
      return TRUE;
    }

  return FALSE;
}

gboolean
pulseaudio_mpris_notify_player (PulseaudioMpris  *mpris,
                                const gchar      *name,
                                const gchar      *message)
{
  PulseaudioMprisPlayer *player;

  g_return_val_if_fail (IS_PULSEAUDIO_MPRIS (mpris), FALSE);

  player = g_hash_table_lookup (mpris->players, name);

  if (player != NULL)
    {
      if (pulseaudio_mpris_player_is_connected (player))
        {
          pulseaudio_mpris_player_call_player_method (player, message);
          return TRUE;
        }
    }

  return FALSE;
}



gboolean
pulseaudio_mpris_notify_any_player (PulseaudioMpris *mpris,
                                    const gchar     *message)
{
  PulseaudioMprisPlayer *player;
  GHashTableIter iter;
  const gchar *key;
  gboolean found = FALSE;

  g_return_val_if_fail(IS_PULSEAUDIO_MPRIS(mpris), FALSE);

  g_hash_table_iter_init (&iter, mpris->players);
  while (g_hash_table_iter_next(&iter, (gpointer *) &key, (gpointer) &player))
  {
    if (player != NULL)
    {
      if (pulseaudio_mpris_player_is_connected(player))
      {
        pulseaudio_mpris_player_call_player_method(player, message);
        found = TRUE;
      }
    }
  }

  return found;
}

static void
pulseaudio_mpris_init (PulseaudioMpris *mpris)
{
  mpris->config            = NULL;
  mpris->dbus_connection   = NULL;
}

static void
pulseaudio_mpris_finalize (GObject *object)
{
  PulseaudioMpris *mpris;

  mpris = PULSEAUDIO_MPRIS (object);

  mpris->config            = NULL;
  mpris->dbus_connection   = NULL;

  (*G_OBJECT_CLASS (pulseaudio_mpris_parent_class)->finalize) (object);
}

PulseaudioMpris *
pulseaudio_mpris_new (PulseaudioConfig *config)
{
  PulseaudioMpris *mpris;
  GDBusConnection *gconnection;
  GError          *gerror = NULL;

  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);

  gconnection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &gerror);
  if (gconnection == NULL)
    {
      g_message ("Failed to get session bus: %s", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
    }

  mpris = g_object_new (TYPE_PULSEAUDIO_MPRIS, NULL);

  mpris->config = config;
  mpris->dbus_connection = gconnection;
  mpris->players = g_hash_table_new (g_str_hash, g_str_equal);
  mpris->player_timer_id = g_timeout_add_seconds (1, pulseaudio_mpris_tick_cb, mpris);

  return mpris;
}
