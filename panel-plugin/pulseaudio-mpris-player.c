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
#include <gio/gdesktopappinfo.h>
#include <glib.h>
#include <gtk/gtk.h>

#ifdef HAVE_LIBXFCE4WINDOWING
#include <libxfce4windowing/libxfce4windowing.h>
#elif defined (HAVE_WNCK)
#define WNCK_I_KNOW_THIS_IS_UNSTABLE = 1
#include <libwnck/libwnck.h>
#include <gdk/gdkx.h>
#endif

#include "pulseaudio-mpris-player.h"



struct _PulseaudioMprisPlayer
{
  GObject          __parent__;

  GDBusConnection  *dbus_connection;
  GDBusProxy       *dbus_props_proxy;
  GDBusProxy       *dbus_player_proxy;
  GDBusProxy       *dbus_playlists_proxy;
  gchar            *dbus_name;

  gchar            *player;
  gchar            *player_label;
  gchar            *icon_name;

  gboolean          connected;

  gchar            *title;
  gchar            *artist;
  gchar            *full_path;

  gboolean          can_go_next;
  gboolean          can_go_previous;
  gboolean          can_pause;
  gboolean          can_play;
  gboolean          can_raise;

  PlaybackStatus    playback_status;

  guint             watch_id;

  GHashTable       *playlists;

#ifdef HAVE_LIBXFCE4WINDOWING
  XfwScreen        *screen;
#elif defined (HAVE_WNCK)
  gulong            xid;
#endif

  gint64            timestamp;
};

struct _PulseaudioMprisPlayerClass
{
  GObjectClass            __parent__;
  void (*connection)      (PulseaudioMprisPlayer *player, gboolean        connected);
  void (*playback_status) (PulseaudioMprisPlayer *player, PlaybackStatus  playback_status);
  void (*metadata)        (PulseaudioMprisPlayer *player);
};



static void pulseaudio_mpris_player_finalize     (GObject               *object);
static void pulseaudio_mpris_player_dbus_connect (PulseaudioMprisPlayer *player);

static GVariant *pulseaudio_mpris_player_playlists_get_playlists  (PulseaudioMprisPlayer *player);
static void      pulseaudio_mpris_player_parse_playlists          (PulseaudioMprisPlayer *player,
                                                                   GVariant              *playlists);

enum {
  CONNECTION,
  PLAYBACK_STATUS,
  METADATA,
  VOLUME,
  LAST_SIGNAL
};
static int signals[LAST_SIGNAL] = { 0 };



G_DEFINE_TYPE (PulseaudioMprisPlayer, pulseaudio_mpris_player, G_TYPE_OBJECT)



static void
pulseaudio_mpris_player_class_init (PulseaudioMprisPlayerClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_mpris_player_finalize;

  signals[CONNECTION] =
    g_signal_new ("connection",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PulseaudioMprisPlayerClass, connection),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  signals[PLAYBACK_STATUS] =
    g_signal_new ("playback-status",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PulseaudioMprisPlayerClass, playback_status),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1, G_TYPE_INT);

  signals[METADATA] =
    g_signal_new ("metadata",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (PulseaudioMprisPlayerClass, metadata),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static gboolean
pulseaudio_mpris_player_can_condition_track_info (PulseaudioMprisPlayer *player)
{
  if (player->title == NULL || player->artist == NULL)
    return FALSE;

  if (!g_utf8_validate (player->title, -1, NULL) || !g_utf8_validate (player->artist, -1, NULL))
    return FALSE;

  if (g_utf8_strlen (player->title, -1) <= 0 || g_utf8_strlen (player->artist, -1) <= 0)
    return FALSE;

  return TRUE;
}


static gboolean
pulseaudio_mpris_player_condition_track_info (PulseaudioMprisPlayer *player,
                                              const gchar           *delimiter)
{
  gchar    *lookup = NULL;
  gchar    *replace = NULL;
  gboolean  found = FALSE;

  lookup = g_strconcat (player->artist, delimiter, NULL);
  if (g_str_has_prefix (player->title, lookup))
    {
      replace = g_utf8_substring (player->title, g_utf8_strlen (lookup, -1), g_utf8_strlen (player->title, -1));
      g_free (player->title);
      player->title = replace;
      found = TRUE;
    }
  g_free (lookup);

  if (found)
    return TRUE;

  // Track titles match ARTIST - TITLE
  if (g_str_has_suffix (player->artist, "VEVO"))
    {
      gchar **components = g_strsplit (player->title, delimiter, 2);

      if (g_strv_length (components) == 2)
        {
          g_free (player->artist);
          player->artist = g_strdup (components[0]);

          g_free (player->title);
          player->title = g_strdup (components[1]);

          found = TRUE;
        }

      g_strfreev (components);
    }

  return found;
}



static void
pulseaudio_mpris_player_parse_metadata (PulseaudioMprisPlayer *player,
                                        GVariant              *dictionary)
{
  GVariantIter iter;
  GVariant *value;
  gchar *key;

  gchar **artists;

  if (player->title != NULL)
    {
      g_free(player->title);
      player->title = NULL;
    }

  if (player->artist != NULL)
    {
      g_free(player->artist);
      player->artist = NULL;
    }

  g_variant_iter_init (&iter, dictionary);
  while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
    {
      if (0 == g_ascii_strcasecmp (key, "xesam:title"))
        {
          player->title = g_strdup(g_variant_get_string(value, NULL));
        }
      else if (0 == g_ascii_strcasecmp (key, "xesam:artist"))
        {
          if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
            {
              player->artist = g_strdup (g_variant_get_string(value, NULL));
            }
          else
            {
              artists = g_variant_dup_strv (value, NULL);
              if (artists != NULL)
                {
                  if (g_strv_length (artists) > 0)
                    {
                      player->artist = g_strdup (artists[0]);
                    }
                  else
                    {
                      player->artist = g_strdup ("");
                    }

                  g_strfreev (artists);
                }
            }
        }
    }

  if (!pulseaudio_mpris_player_can_condition_track_info (player))
    return;

  if (pulseaudio_mpris_player_condition_track_info (player, " - "))
    return;

  if (pulseaudio_mpris_player_condition_track_info (player, ": "))
    return;
}



#ifdef HAVE_LIBXFCE4WINDOWING
/**
 * Alternative "Raise" method.
 * Some media players (e.g. Spotify) do not support the "Raise" method.
 * This workaround utilizes libxfce4windowing to find the correct window and raise it.
 */
static void
pulseaudio_mpris_player_raise_wnck (PulseaudioMprisPlayer *player)
{
  for (GList *lp = xfw_screen_get_windows (player->screen); lp != NULL; lp = lp->next)
    {
      if (g_strcmp0 (player->player_label, xfw_window_get_name (lp->data)) == 0)
        {
          xfw_window_activate (lp->data, g_get_monotonic_time () / 1000, NULL);
          break;
        }
    }
}

#elif defined (HAVE_WNCK)

static void
pulseaudio_mpris_player_get_xid (PulseaudioMprisPlayer *player)
{
  WnckScreen           *screen = NULL;
  GList                *window = NULL;

  if (player->xid > 0L || ! GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return;

  screen = wnck_screen_get_default ();
  if (screen != NULL)
    {
      wnck_screen_force_update (screen);
      if (player->xid == 0L)
        {
          for (window = wnck_screen_get_windows (screen); window != NULL; window = window->next)
            {
              if (0 == g_strcmp0 (player->player_label, wnck_window_get_name (WNCK_WINDOW (window->data))))
                {
                  player->xid = wnck_window_get_xid (WNCK_WINDOW (window->data));
                  if (player->xid > 0L)
                    return;
                }
            }
        }
    }
}



/**
 * Alternative "Raise" method.
 * Some media players (e.g. Spotify) do not support the "Raise" method.
 * This workaround utilizes libwnck to find the correct window and raise it.
 */
static void
pulseaudio_mpris_player_raise_wnck (PulseaudioMprisPlayer *player)
{
  WnckWindow           *window = NULL;

  pulseaudio_mpris_player_get_xid (player);

  if (player->xid == 0L)
    return;

  window = wnck_window_get (player->xid);
  if (window != NULL)
    wnck_window_activate (window, g_get_monotonic_time () / 1000);
}
#endif



void
pulseaudio_mpris_player_call_player_method (PulseaudioMprisPlayer *player,
                                            const gchar           *method,
                                            gboolean               update_timestamp)
{
  GDBusMessage *message;
  GError       *error = NULL;
  gchar        *iface = NULL;

  if (g_strcmp0 (method, "Raise") == 0)
    iface = "org.mpris.MediaPlayer2";
#if defined (HAVE_WNCK) || defined (HAVE_LIBXFCE4WINDOWING)
  else if (g_strcmp0 (method, "RaiseWnck") == 0)
    return pulseaudio_mpris_player_raise_wnck (player);
#endif
  else if (g_strcmp0 (method, "Quit") == 0)
    iface = "org.mpris.MediaPlayer2";
  else
    iface = "org.mpris.MediaPlayer2.Player";

  message = g_dbus_message_new_method_call (player->dbus_name,
                                            "/org/mpris/MediaPlayer2",
                                            iface,
                                            method);

  g_dbus_connection_send_message (player->dbus_connection,
                                  message,
                                  G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                  NULL,
                                  &error);
  if (error != NULL)
    {
      g_warning ("unable to send message: %s", error->message);
      g_clear_error (&error);
      error = NULL;
    }

  g_dbus_connection_flush_sync (player->dbus_connection, NULL, &error);
  if (error != NULL)
    {
      g_warning ("unable to flush message queue: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (message);

  if (update_timestamp)
    player->timestamp = g_get_monotonic_time ();
}



GList *
pulseaudio_mpris_player_get_playlists (PulseaudioMprisPlayer *player)
{
  return g_hash_table_get_keys (player->playlists);
}



void pulseaudio_mpris_player_activate_playlist (PulseaudioMprisPlayer *player,
                                                const gchar           *playlist)
{
  gchar *path = g_hash_table_lookup (player->playlists, playlist);

  if (path != NULL)
    {
      g_dbus_connection_call (player->dbus_connection,
                              player->dbus_name,
                              "/org/mpris/MediaPlayer2",
                              "org.mpris.MediaPlayer2.Playlists",
                              "ActivatePlaylist",
                              g_variant_new("(o)", path),
                              NULL,
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,
                              NULL,
                              NULL,
                              NULL);
    }
}



static GVariant *
pulseaudio_mpris_player_get_all_player_properties(PulseaudioMprisPlayer *player)
{
  GVariantIter iter;
  GVariant *result, *child = NULL;

  result = g_dbus_connection_call_sync (player->dbus_connection,
                                        player->dbus_name,
                                        "/org/mpris/MediaPlayer2",
                                        "org.freedesktop.DBus.Properties",
                                        "GetAll",
                                        g_variant_new ("(s)", "org.mpris.MediaPlayer2.Player"),
                                        G_VARIANT_TYPE ("(a{sv})"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        NULL);

  if (result)
    {
      g_variant_iter_init (&iter, result);
      child = g_variant_iter_next_value (&iter);
    }

  return child;
}



static GVariant *
pulseaudio_mpris_player_get_all_media_player_properties (PulseaudioMprisPlayer *player)
{
  GVariantIter iter;
  GVariant *result, *child = NULL;

  result = g_dbus_connection_call_sync (player->dbus_connection,
                                        player->dbus_name,
                                        "/org/mpris/MediaPlayer2",
                                        "org.freedesktop.DBus.Properties",
                                        "GetAll",
                                        g_variant_new ("(s)", "org.mpris.MediaPlayer2"),
                                        G_VARIANT_TYPE ("(a{sv})"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        NULL);

  if (result)
    {
      g_variant_iter_init (&iter, result);
      child = g_variant_iter_next_value (&iter);
    }

  return child;
}



static GVariant *
pulseaudio_mpris_player_playlists_get_playlists (PulseaudioMprisPlayer *player)
{
  GVariantIter iter;
  GVariant *result, *child = NULL;

  result = g_dbus_connection_call_sync(player->dbus_connection,
                                       player->dbus_name,
                                       "/org/mpris/MediaPlayer2",
                                       "org.mpris.MediaPlayer2.Playlists",
                                       "GetPlaylists",
                                       g_variant_new("(uusb)", (guint32)0, (guint32)5, "Played", TRUE),
                                       G_VARIANT_TYPE("(a(oss))"),
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       NULL);

  if (result)
  {
    g_variant_iter_init(&iter, result);
    child = g_variant_iter_next_value(&iter);
  }

  return child;
}



static void
pulseaudio_mpris_player_parse_playback_status (PulseaudioMprisPlayer *player,
                                               const gchar           *playback_status)
{
  if (0 == g_ascii_strcasecmp(playback_status, "Playing"))
    player->playback_status = PLAYING;
  else if (0 == g_ascii_strcasecmp(playback_status, "Paused"))
    player->playback_status = PAUSED;
  else
    player->playback_status = STOPPED;
}



static void
pulseaudio_mpris_player_set_details_from_desktop (PulseaudioMprisPlayer *player,
                                                  const gchar           *player_name);



static void
pulseaudio_mpris_player_parse_player_properties (PulseaudioMprisPlayer *player,
                                                 GVariant              *properties)
{
  GVariantIter  iter;
  GVariant     *value;
  const gchar  *key;
  const gchar  *playback_status = NULL;

  g_variant_iter_init (&iter, properties);

  while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
    {
      if (0 == g_ascii_strcasecmp (key, "PlaybackStatus"))
        {
          playback_status = g_variant_get_string(value, NULL);
        }
      else if (0 == g_ascii_strcasecmp (key, "CanGoNext"))
        {
          player->can_go_next = g_variant_get_boolean(value);
        }
      else if (0 == g_ascii_strcasecmp (key, "CanGoPrevious"))
        {
          player->can_go_previous = g_variant_get_boolean(value);
        }
      else if (0 == g_ascii_strcasecmp (key, "CanPlay"))
        {
          player->can_play = g_variant_get_boolean(value);
        }
      else if (0 == g_ascii_strcasecmp (key, "CanPause"))
        {
          player->can_pause = g_variant_get_boolean(value);
        }
      else if (0 == g_ascii_strcasecmp (key, "Metadata"))
        {
          pulseaudio_mpris_player_parse_metadata (player, value);
          g_signal_emit (player, signals[METADATA], 0, NULL);
        }
      else if (0 == g_ascii_strcasecmp(key, "ActivePlaylist") || 0 == g_ascii_strcasecmp(key, "PlaylistCount"))
        {
          /* Playlists */
          GVariant *reply = pulseaudio_mpris_player_playlists_get_playlists(player);
          if (reply)
          {
            pulseaudio_mpris_player_parse_playlists(player, reply);
            g_variant_unref(reply);
          }
        }
    }

  if (playback_status != NULL)
    {
      pulseaudio_mpris_player_parse_playback_status (player, playback_status);
      g_signal_emit (player, signals[PLAYBACK_STATUS], 0, player->playback_status);
    }
}



static void
pulseaudio_mpris_player_parse_media_player_properties (PulseaudioMprisPlayer *player,
                                                       GVariant              *properties)
{
  GVariantIter  iter;
  GVariant     *value;
  const gchar  *key;
  const gchar  *name_for_desktop = NULL;
  const gchar  *identify = NULL;

  g_variant_iter_init (&iter, properties);

  while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
    {
      if (0 == g_ascii_strcasecmp (key, "CanRaise"))
        {
          player->can_raise = g_variant_get_boolean(value);
        }
      else if (0 == g_ascii_strcasecmp (key, "DesktopEntry"))
        {
          name_for_desktop = g_variant_get_string(value, NULL);
        }
      else if (0 == g_ascii_strcasecmp (key, "Identity"))
        {
          identify = g_variant_get_string(value, NULL);
        }
    }

  if (name_for_desktop == NULL && identify != NULL && *identify != '\0')
    player->player_label = g_strdup (identify);

  if (name_for_desktop == NULL)
    name_for_desktop = player->player_label;
  if (name_for_desktop == NULL)
    name_for_desktop = player->player;
  pulseaudio_mpris_player_set_details_from_desktop (player, name_for_desktop);
}



static void
pulseaudio_mpris_player_parse_playlists (PulseaudioMprisPlayer *player,
                                         GVariant *playlists)
{
  GVariantIter iter;

  gchar *path;
  const gchar *name;
  const gchar *icon;

  g_hash_table_remove_all (player->playlists);

  g_variant_iter_init(&iter, playlists);

  while (g_variant_iter_loop(&iter, "(oss)", &path, &name, &icon))
  {
    g_hash_table_insert (player->playlists, g_strdup (name), g_strdup (path));
  }
}



static void
pulseaudio_mpris_player_on_dbus_property_signal (GDBusProxy *proxy,
                                                 gchar      *sender_name,
                                                 gchar      *signal_name,
                                                 GVariant   *parameters,
                                                 gpointer    user_data)
{
  GVariantIter iter;
  GVariant *child;

  PulseaudioMprisPlayer *player = user_data;

  if (g_ascii_strcasecmp (signal_name, "PropertiesChanged"))
    return;

  g_variant_iter_init (&iter, parameters);

  child = g_variant_iter_next_value (&iter); /* Interface name. */
  if (child)
    {
      g_variant_unref (child);
    }

  child = g_variant_iter_next_value (&iter); /* Property name. */
  if (child)
    {
      pulseaudio_mpris_player_parse_player_properties (player, child);
      g_variant_unref(child);
    }
}



static void
pulseaudio_mpris_player_on_dbus_connected (GDBusConnection *connection,
                                           const gchar     *name,
                                           const gchar     *name_owner,
                                           gpointer         user_data)
{
  GVariant *reply;

  PulseaudioMprisPlayer *player = user_data;

  player->connected = TRUE;
  player->timestamp = g_get_monotonic_time ();

  /* Media player properties */
  reply = pulseaudio_mpris_player_get_all_media_player_properties (player);
  if (reply)
    {
      pulseaudio_mpris_player_parse_media_player_properties (player, reply);
      g_variant_unref (reply);
    }

  if (player->player_label == NULL)
    pulseaudio_mpris_player_set_details_from_desktop (player, player->player);

  /* Notify that connect to a player */
  g_signal_emit (player, signals[CONNECTION], 0, player->connected);

  /* Inform the current status of the player */
  reply = pulseaudio_mpris_player_get_all_player_properties (player);
  if (reply)
    {
      pulseaudio_mpris_player_parse_player_properties (player, reply);
      g_variant_unref (reply);
    }

  /* Playlists */
  reply = pulseaudio_mpris_player_playlists_get_playlists (player);
  if (reply)
  {
    pulseaudio_mpris_player_parse_playlists (player, reply);
    g_variant_unref (reply);
  }

#ifdef HAVE_WNCK
  pulseaudio_mpris_player_get_xid (player);
#endif
}



static void
pulseaudio_mpris_player_on_dbus_lost (GDBusConnection *connection,
                                      const gchar     *name,
                                      gpointer         user_data)
{
  PulseaudioMprisPlayer *player = user_data;

  player->connected       = FALSE;

  /* This will delete the object */
  g_signal_emit (player, signals[CONNECTION], 0, player->connected);
}



gchar *
pulseaudio_mpris_player_find_desktop_entry (const gchar *player_name)
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

  if (file)
    g_free (file);

  return filename;
}



static void
pulseaudio_mpris_player_set_details_from_desktop (PulseaudioMprisPlayer *player,
                                                  const gchar           *player_name)
{
  GKeyFile  *key_file = NULL;
  gchar     *file = NULL;
  gchar     *full_path = NULL;
  gchar     *filename = NULL;

  filename = pulseaudio_mpris_player_find_desktop_entry (player_name);

  g_free (player->icon_name);
  g_free (player->full_path);

  if (filename == NULL)
    {
      if (player->player_label == NULL)
        player->player_label = g_strdup (player->player);
      player->icon_name = g_strdup ("applications-multimedia");
      player->full_path = NULL;
      return;
    }

  file = g_strconcat("applications/", filename, NULL);
  g_free (filename);

  key_file = g_key_file_new();
  if (g_key_file_load_from_data_dirs (key_file, file, &full_path, G_KEY_FILE_NONE, NULL))
    {
      if (player->player_label == NULL)
        player->player_label = g_key_file_get_locale_string (key_file, "Desktop Entry", "Name", NULL, NULL);
      player->icon_name = g_key_file_get_string (key_file, "Desktop Entry", "Icon", NULL);
    }
  else
    {
      if (player->player_label == NULL)
        player->player_label = g_strdup (player->player);
      player->icon_name = g_strdup ("applications-multimedia");
    }

  player->full_path = full_path;

  g_key_file_free (key_file);
  g_free (file);
}



static void
pulseaudio_mpris_player_dbus_connect (PulseaudioMprisPlayer *player)
{
  GDBusProxy *proxy;
  GError     *gerror = NULL;
  guint       watch_id;

  if (player->player == NULL)
    return;

  g_free(player->dbus_name);
  player->dbus_name = g_strdup_printf("org.mpris.MediaPlayer2.%s", player->player);

  watch_id = g_bus_watch_name_on_connection(player->dbus_connection,
                                            player->dbus_name,
                                            G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                            pulseaudio_mpris_player_on_dbus_connected,
                                            pulseaudio_mpris_player_on_dbus_lost,
                                            player,
                                            NULL);

  proxy = g_dbus_proxy_new_sync (player->dbus_connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 player->dbus_name,
                                 "/org/mpris/MediaPlayer2",
                                 "org.freedesktop.DBus.Properties",
                                 NULL, /* GCancellable */
                                 &gerror);

  if (proxy == NULL)
    {
      g_printerr ("Error creating proxy: %s\n", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
    }
  else
    {
      g_signal_connect (proxy, "g-signal",
                        G_CALLBACK (pulseaudio_mpris_player_on_dbus_property_signal), player);
      player->dbus_props_proxy = proxy;
    }

  /* interface=org.mpris.MediaPlayer2.Player */
  proxy = g_dbus_proxy_new_sync (player->dbus_connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 player->dbus_name,
                                 "/org/mpris/MediaPlayer2",
                                 "org.mpris.MediaPlayer2.Player",
                                 NULL, /* GCancellable */
                                 &gerror);

  if (proxy == NULL)
    {
      g_printerr ("Error creating proxy: %s\n", gerror->message);
      g_error_free (gerror);
      gerror = NULL;
    }
  else
    {
      player->dbus_player_proxy = proxy;
    }

  /* interface=org.mpris.MediaPlayer2.Player */
  proxy = g_dbus_proxy_new_sync(player->dbus_connection,
                                G_DBUS_PROXY_FLAGS_NONE,
                                NULL,
                                player->dbus_name,
                                "/org/mpris/MediaPlayer2",
                                "org.mpris.MediaPlayer2.Playlists",
                                NULL, /* GCancellable */
                                &gerror);

  if (proxy == NULL)
    {
      g_printerr("Error creating proxy: %s\n", gerror->message);
      g_error_free(gerror);
      gerror = NULL;
    }
  else
    {
      player->dbus_playlists_proxy = proxy;
    }

  player->watch_id = watch_id;
}



gint64
pulseaudio_mpris_player_get_timestamp (PulseaudioMprisPlayer *player)
{
  return player->timestamp;
}



const gchar *
pulseaudio_mpris_player_get_player (PulseaudioMprisPlayer *player)
{
  return player->player;
}



const gchar *
pulseaudio_mpris_player_get_player_title (PulseaudioMprisPlayer *player)
{
  return player->player_label;
}



const gchar *
pulseaudio_mpris_player_get_icon_name (PulseaudioMprisPlayer *player)
{
  return player->icon_name;
}



const gchar *
pulseaudio_mpris_player_get_title (PulseaudioMprisPlayer *player)
{
  return player->title;
}



const gchar *
pulseaudio_mpris_player_get_artist (PulseaudioMprisPlayer *player)
{
  return player->artist;
}



const gchar *
pulseaudio_mpris_player_get_full_path (PulseaudioMprisPlayer *player)
{
  return player->full_path;
}



gboolean
pulseaudio_mpris_player_is_connected (PulseaudioMprisPlayer *player)
{
  return player->connected;
}



gboolean
pulseaudio_mpris_player_is_playing (PulseaudioMprisPlayer *player)
{
  return player->playback_status == PLAYING;
}



gboolean
pulseaudio_mpris_player_is_stopped (PulseaudioMprisPlayer *player)
{
  return player->playback_status == STOPPED;
}



gboolean
pulseaudio_mpris_player_can_play (PulseaudioMprisPlayer *player)
{
  return player->can_play;
}



gboolean
pulseaudio_mpris_player_can_pause (PulseaudioMprisPlayer *player)
{
  return player->can_pause;
}



gboolean
pulseaudio_mpris_player_can_go_previous (PulseaudioMprisPlayer *player)
{
  return player->can_go_previous;
}



gboolean
pulseaudio_mpris_player_can_go_next (PulseaudioMprisPlayer *player)
{
  return player->can_go_next;
}



gboolean
pulseaudio_mpris_player_can_raise (PulseaudioMprisPlayer *player)
{
  return player->can_raise;
}



static void
pulseaudio_mpris_player_init (PulseaudioMprisPlayer *player)
{
  player->playback_status   = STOPPED;

#ifdef HAVE_LIBXFCE4WINDOWING
  player->screen = xfw_screen_get_default ();
#endif
}



static void
pulseaudio_mpris_player_finalize (GObject *object)
{
  PulseaudioMprisPlayer *player;

  player = PULSEAUDIO_MPRIS_PLAYER (object);

  /* Disconnect dbus */
  if (player->watch_id != 0)
    g_bus_unwatch_name (player->watch_id);
  if (player->dbus_props_proxy != NULL)
    g_object_unref (player->dbus_props_proxy);
  if (player->dbus_player_proxy != NULL)
    g_object_unref (player->dbus_player_proxy);
  if (player->dbus_playlists_proxy != NULL)
    g_object_unref (player->dbus_playlists_proxy);

  g_free (player->player);
  g_free (player->player_label);
  g_free (player->icon_name);

  g_free (player->title);
  g_free (player->artist);
  g_free (player->full_path);

  if (player->playlists != NULL)
    g_hash_table_destroy (player->playlists);

#ifdef HAVE_LIBXFCE4WINDOWING
  g_object_unref (player->screen);
#endif

  (*G_OBJECT_CLASS(pulseaudio_mpris_player_parent_class)->finalize)(object);
}



PulseaudioMprisPlayer *
pulseaudio_mpris_player_new (gchar *name)
{
  PulseaudioMprisPlayer *player;
  GDBusConnection       *gconnection;
  GError                *gerror = NULL;

  gconnection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &gerror);
  if (gconnection == NULL)
    {
      g_message ("Failed to get session bus: %s", gerror->message);
      g_error_free (gerror);
      return NULL;
    }

  player = g_object_new (TYPE_PULSEAUDIO_MPRIS_PLAYER, NULL);

  player->dbus_connection = gconnection;
  player->player = g_strdup (name);
  player->playlists = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  pulseaudio_mpris_player_dbus_connect (player);

  return player;
}
