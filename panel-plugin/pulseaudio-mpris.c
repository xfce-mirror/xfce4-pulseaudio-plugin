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



static gchar *
find_desktop_entry (const gchar *player_name)
{
  GKeyFile  *key_file;
  gchar     *file = NULL;
  gchar     *filename = NULL;

  file = g_strconcat ("applications/", player_name, ".desktop", NULL);

  key_file = g_key_file_new();
  if (g_key_file_load_from_data_dirs (key_file, file, NULL, G_KEY_FILE_NONE, NULL))
    {
      filename = g_strconcat (player_name, ".desktop", NULL);
    }
  else
    {
      /* Support reverse domain name (RDN) formatted launchers. */
      gchar ***results = g_desktop_app_info_search (player_name);
      gint i, j;

      for (i = 0; results[i]; i++)
        {
          for (j = 0; results[i][j]; j++)
            {
              if (filename == NULL)
                {
                  filename = g_strdup (results[i][j]);
                }
            }
          g_strfreev (results[i]);
      }
      g_free (results);
    }

  g_key_file_free (key_file);

  g_free (file);

  return filename;
}



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
  const gchar     *player_name;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));

  player_name = pulseaudio_mpris_player_get_player (player);

  g_signal_emit (mpris, signals[UPDATE], 0, player_name);

  if (!pulseaudio_mpris_player_is_connected (player))
    g_hash_table_remove (mpris->players, player_name);
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
      if (!g_hash_table_contains (mpris->players, players[i]))
        {
          player = pulseaudio_mpris_player_new (players[i]);
          if (!player)
            continue;

          g_signal_connect (player, "connection", G_CALLBACK (pulseaudio_mpris_player_connection_cb), mpris);
          g_signal_connect (player, "playback-status", G_CALLBACK (pulseaudio_mpris_player_update_cb), mpris);
          g_signal_connect (player, "metadata", G_CALLBACK (pulseaudio_mpris_player_metadata_cb), mpris);

          g_hash_table_insert (mpris->players, g_strdup (players[i]), player);

          pulseaudio_config_add_mpris_player (mpris->config, players[i]);
        }
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
                                      gboolean         *is_running,
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
          if (playlists)
            *playlists        = pulseaudio_mpris_player_get_playlists (player);
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
          if (playlists)
            *playlists        = NULL;
        }
      if (*title == NULL || g_strcmp0 (*title, "") == 0)
        *title = g_strdup(pulseaudio_mpris_player_get_player_title (player));
      return TRUE;
    }

  return FALSE;
}



static gboolean
pulseaudio_mpris_get_player_summary_from_desktop (const gchar  *player_id,
                                                  gchar       **name,
                                                  gchar       **icon_name,
                                                  gchar       **full_path)
{
  GKeyFile  *key_file;
  gchar     *file;
  gchar     *filename;
  gchar     *path;

  filename = find_desktop_entry (player_id);
  if (filename == NULL)
    {
      return FALSE;
    }

  file = g_strconcat("applications/", filename, NULL);
  g_free (filename);

  key_file = g_key_file_new();
  if (g_key_file_load_from_data_dirs (key_file, file, &path, G_KEY_FILE_NONE, NULL))
    {
      *name = g_key_file_get_string (key_file, "Desktop Entry", "Name", NULL);
      *icon_name = g_key_file_get_string (key_file, "Desktop Entry", "Icon", NULL);
      *full_path = g_strdup (path);
      g_free (path);
    }

  g_key_file_free (key_file);
  g_free (file);

  return TRUE;
}



gboolean
pulseaudio_mpris_get_player_summary (const gchar  *player_id,
                                     gchar       **name,
                                     gchar       **icon_name,
                                     gchar       **full_path)
{
  PulseaudioMprisPlayer *player;
  player = PULSEAUDIO_MPRIS_PLAYER (g_hash_table_lookup (mpris_instance->players, player_id));

  if (player == NULL)
    return pulseaudio_mpris_get_player_summary_from_desktop (player_id, name, icon_name, full_path);

  *name = g_strdup (pulseaudio_mpris_player_get_player_title (player));
  *icon_name = g_strdup (pulseaudio_mpris_player_get_icon_name (player));
  *full_path = g_strdup (pulseaudio_mpris_player_get_full_path (player));

  if (*full_path == NULL)
    return FALSE;

  if (!pulseaudio_mpris_player_is_connected (player) && !pulseaudio_mpris_player_can_launch (player))
    return FALSE;

  return TRUE;
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



gboolean
pulseaudio_mpris_activate_playlist (PulseaudioMpris *mpris,
                                    const gchar     *name,
                                    const gchar     *playlist)
{
  PulseaudioMprisPlayer *player;

  g_return_val_if_fail(IS_PULSEAUDIO_MPRIS(mpris), FALSE);

  player = g_hash_table_lookup(mpris->players, name);

  if (player != NULL)
  {
    if (pulseaudio_mpris_player_is_connected (player))
    {
      pulseaudio_mpris_player_activate_playlist (player, playlist);
      return TRUE;
    }
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
