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



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#include "pulseaudio-mpris.h"
#include "pulseaudio-mpris-player.h"



struct _PulseaudioMpris
{
  GObject           __parent__;
  PulseaudioConfig *config;

  GDBusConnection  *dbus_connection;
  GHashTable       *players;
  GHashTable       *players_by_title;

  guint             dbus_signal_id;
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



static PulseaudioMpris *mpris_instance;



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


static gboolean
player_is_usable (PulseaudioMpris *mpris,
                  const gchar     *player) {
  GVariantIter iter;
  GVariant *result, *child = NULL;
  gboolean usable = FALSE;

  result = g_dbus_connection_call_sync (mpris->dbus_connection,
                                        player,
                                        "/org/mpris/MediaPlayer2",
                                        "org.freedesktop.DBus.Properties",
                                        "GetAll",
                                        g_variant_new ("(s)", "org.mpris.MediaPlayer2"),
                                        G_VARIANT_TYPE ("(a{sv})"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        5000,
                                        NULL,
                                        NULL);

  if (result)
    {
      g_variant_iter_init (&iter, result);
      child = g_variant_iter_next_value (&iter);
      if (child)
        {
          usable = TRUE;
          g_variant_unref (child);
        }
      g_variant_unref (result);
    }

  return usable;
}


static gchar **
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
      g_warning ("Could not get a list of names registered on the session bus, %s",
                 error ? error->message : "no error given");
      g_clear_error (&error);
      return NULL;
    }

  g_variant_get (v, "(as)", &iter);
  while (g_variant_iter_loop (iter, "&s", &str))
    {
      if (g_str_has_prefix(str, "org.mpris.MediaPlayer2."))
        {
          if (player_is_usable (mpris, str)) {
            res = (gchar**)g_realloc(res, (items + 1) * sizeof(gchar*));
            res[items] = g_strdup(str + 23);
            items++;
          }
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
pulseaudio_mpris_player_connection_cb (PulseaudioMprisPlayer *player,
                                       gchar                 *sender_name,
                                       gpointer               user_data)
{
  PulseaudioMpris *mpris = user_data;
  const gchar     *player_title;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));

  player = g_object_ref (player);

  player_title = pulseaudio_mpris_player_get_player_title (player);

  if (!pulseaudio_mpris_player_is_connected (player))
    {
      g_hash_table_remove (mpris->players, pulseaudio_mpris_player_get_player (player));
      g_hash_table_remove (mpris->players_by_title, player_title);
    }
  else if (G_LIKELY (!g_hash_table_contains (mpris->players_by_title, (gpointer) player_title)))
    {
      g_hash_table_insert (mpris->players_by_title, g_strdup (player_title), g_object_ref (player));
      pulseaudio_config_add_known_player (mpris->config, player_title);
    }

  g_signal_emit (mpris, signals[UPDATE], 0, player_title);

  g_object_unref (player);
}



static void
pulseaudio_mpris_player_update_cb (PulseaudioMprisPlayer *player,
                                   gchar                 *sender_name,
                                   gpointer               user_data)
{
  PulseaudioMpris *mpris = user_data;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));

  g_signal_emit (mpris, signals[UPDATE], 0, pulseaudio_mpris_player_get_player_title (player));
}



static void
pulseaudio_mpris_player_metadata_cb (PulseaudioMprisPlayer *player,
                                     gpointer               user_data)
{
  PulseaudioMpris *mpris = user_data;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));

  g_signal_emit (mpris, signals[UPDATE], 0, pulseaudio_mpris_player_get_player_title (player));
}



static void
pulseaudio_mpris_manage_players (PulseaudioMpris *mpris)
{
  PulseaudioMprisPlayer  *player;
  gchar                 **players;
  guint                   i;
  guint                   num_players;

  players = pulseaudio_mpris_get_available_players (mpris);
  if (players == NULL)
    return;

  num_players = g_strv_length (players);
  for (i = 0; i < num_players; i++)
    {
      if (g_hash_table_contains (mpris->players, players[i]))
        continue;

      player = pulseaudio_mpris_player_new (players[i]);
      if (!player)
        continue;

      g_signal_connect (player, "connection", G_CALLBACK (pulseaudio_mpris_player_connection_cb), mpris);
      g_signal_connect (player, "playback-status", G_CALLBACK (pulseaudio_mpris_player_update_cb), mpris);
      g_signal_connect (player, "metadata", G_CALLBACK (pulseaudio_mpris_player_metadata_cb), mpris);

      g_hash_table_insert (mpris->players, g_strdup (players[i]), player);
    }

  if (players != NULL)
    g_strfreev (players);
}



static void
pulseaudio_mpris_changed_cb (GDBusConnection *connection,
                             const gchar *sender_name,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *signal_name,
                             GVariant *parameters,
                             gpointer user_data)
{
  PulseaudioMpris        *mpris = user_data;
  pulseaudio_mpris_manage_players (mpris);
}



gboolean
pulseaudio_mpris_get_player_snapshot (PulseaudioMpris  *mpris,
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
                                      GList           **playlists)
{
  PulseaudioMprisPlayer *player;
  player = PULSEAUDIO_MPRIS_PLAYER (g_hash_table_lookup (mpris->players_by_title, name));

  if (player == NULL || G_UNLIKELY (!pulseaudio_mpris_player_is_connected (player)))
    return FALSE;

  *title = g_strdup (pulseaudio_mpris_player_get_title (player));
  *artist = g_strdup (pulseaudio_mpris_player_get_artist (player));

  *is_playing         = pulseaudio_mpris_player_is_playing (player);
  *is_stopped         = pulseaudio_mpris_player_is_stopped (player);
  *can_play           = pulseaudio_mpris_player_can_play (player);
  *can_pause          = pulseaudio_mpris_player_can_pause (player);
  *can_go_previous    = pulseaudio_mpris_player_can_go_previous (player);
  *can_go_next        = pulseaudio_mpris_player_can_go_next (player);
  *can_raise          = pulseaudio_mpris_player_can_raise (player);
  if (playlists)
    *playlists        = pulseaudio_mpris_player_get_playlists (player);

  if (*title && **title == '\0')
    {
      g_free (*title);
      *title = NULL;
    }
  if (*title == NULL)
    {
      *title = g_strdup (pulseaudio_mpris_player_get_player_title (player));
    }

  return TRUE;
}



static gboolean
pulseaudio_mpris_get_player_summary_from_desktop (const gchar  *player_title,
                                                  gchar       **icon_name,
                                                  gchar       **full_path)
{
  GKeyFile  *key_file;
  gchar     *file;
  gchar     *filename;
  gchar     *path;
  gboolean   ret = FALSE;

  filename = pulseaudio_mpris_player_find_desktop_entry (player_title);
  if (filename == NULL)
    {
      return ret;
    }

  file = g_strconcat("applications/", filename, NULL);
  g_free (filename);

  key_file = g_key_file_new();
  if (g_key_file_load_from_data_dirs (key_file, file, &path, G_KEY_FILE_NONE, NULL))
    {
      *icon_name = g_key_file_get_string (key_file, "Desktop Entry", "Icon", NULL);
      if (full_path)
        *full_path = g_strdup (path);
      g_free (path);
      ret = TRUE;
    }

  g_key_file_free (key_file);
  g_free (file);

  return ret;
}



gboolean
pulseaudio_mpris_get_player_summary (const gchar  *player_title,
                                     gchar       **icon_name,
                                     gchar       **full_path)
{
  PulseaudioMprisPlayer *player;
  player = PULSEAUDIO_MPRIS_PLAYER (g_hash_table_lookup (mpris_instance->players_by_title, player_title));

  if (player == NULL)
    return pulseaudio_mpris_get_player_summary_from_desktop (player_title, icon_name, full_path);

  if (G_UNLIKELY (!pulseaudio_mpris_player_is_connected (player)))
    return FALSE;

  *icon_name = g_strdup (pulseaudio_mpris_player_get_icon_name (player));
  if (full_path)
    *full_path = g_strdup (pulseaudio_mpris_player_get_full_path (player));

  return TRUE;
}



gboolean
pulseaudio_mpris_notify_player (PulseaudioMpris  *mpris,
                                const gchar      *name,
                                const gchar      *message)
{
  PulseaudioMprisPlayer *player;

  g_return_val_if_fail (IS_PULSEAUDIO_MPRIS (mpris), FALSE);

  player = g_hash_table_lookup (mpris->players_by_title, name);

  if (player != NULL && G_LIKELY (pulseaudio_mpris_player_is_connected (player)))
    {
      pulseaudio_mpris_player_call_player_method (player, message, TRUE);
      return TRUE;
    }

  return FALSE;
}



gboolean
pulseaudio_mpris_notify_any_player (PulseaudioMpris *mpris,
                                    const gchar     *message)
{
  PulseaudioMprisPlayer *player;
  PulseaudioMprisPlayer *recent_player = NULL;
  GHashTableIter iter;
  const gchar *key;
  gint64 timestamp;
  gint64 highest_timestamp = 0;
  gboolean found = FALSE;

  g_return_val_if_fail(IS_PULSEAUDIO_MPRIS(mpris), FALSE);

  g_hash_table_iter_init (&iter, mpris->players_by_title);
  while (g_hash_table_iter_next(&iter, (gpointer *) &key, (gpointer) &player))
    {
      if (player == NULL)
        continue;

      if (G_UNLIKELY (!pulseaudio_mpris_player_is_connected (player)))
        continue;

      if (pulseaudio_config_player_ignored_lookup (mpris->config, key))
        continue;

      timestamp = pulseaudio_mpris_player_get_timestamp (player);
      if (timestamp > highest_timestamp || !recent_player)
        {
          highest_timestamp = timestamp;
          recent_player = player;
        }
    }

    if (recent_player)
      {
        pulseaudio_mpris_player_call_player_method (recent_player, message, FALSE);
        found = TRUE;
      }

  return found;
}



gboolean
pulseaudio_mpris_activate_playlist (PulseaudioMpris *mpris,
                                    const gchar     *name,
                                    const gchar     *playlist)
{
  PulseaudioMprisPlayer *player;

  g_return_val_if_fail(IS_PULSEAUDIO_MPRIS(mpris), FALSE);

  player = g_hash_table_lookup(mpris->players_by_title, name);

  if (player != NULL && G_LIKELY (pulseaudio_mpris_player_is_connected (player)))
  {
    pulseaudio_mpris_player_activate_playlist (player, playlist);
    return TRUE;
  }

  return FALSE;
}



static void
pulseaudio_mpris_init (PulseaudioMpris *mpris)
{
}



static void
pulseaudio_mpris_finalize (GObject *object)
{
  PulseaudioMpris *mpris;

  mpris = PULSEAUDIO_MPRIS (object);

  mpris_instance = NULL;

  if (mpris->dbus_signal_id != 0 && mpris->dbus_connection != NULL)
    g_dbus_connection_signal_unsubscribe (mpris->dbus_connection, mpris->dbus_signal_id);

  if (mpris->players != NULL)
    g_hash_table_destroy (mpris->players);
  if (mpris->players_by_title != NULL)
    g_hash_table_destroy (mpris->players_by_title);

  (*G_OBJECT_CLASS (pulseaudio_mpris_parent_class)->finalize) (object);
}



PulseaudioMpris *
pulseaudio_mpris_new (PulseaudioConfig *config)
{
  PulseaudioMpris *mpris;
  GDBusConnection *gconnection;
  GError          *gerror = NULL;

  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);

  if (mpris_instance)
    return NULL;

  gconnection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &gerror);
  if (gconnection == NULL)
    {
      g_message ("Failed to get session bus: %s", gerror->message);
      g_error_free (gerror);
      return NULL;
    }

  mpris = g_object_new (TYPE_PULSEAUDIO_MPRIS, NULL);

  mpris->config = config;
  mpris->dbus_connection = gconnection;
  mpris->players = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  mpris->players_by_title = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  mpris->dbus_signal_id = g_dbus_connection_signal_subscribe (gconnection,
                                                              "org.freedesktop.DBus",
                                                              "org.freedesktop.DBus",
                                                              "NameOwnerChanged",
                                                              "/org/freedesktop/DBus",
                                                              "org.mpris.MediaPlayer2",
                                                              G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE,
                                                              pulseaudio_mpris_changed_cb,
                                                              mpris,
                                                              NULL);
  if (mpris->dbus_signal_id != 0)
    pulseaudio_mpris_manage_players (mpris);

  mpris_instance = mpris;

  return mpris;
}
