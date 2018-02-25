/*  Copyright (c) 2015-2017 Andrzej <andrzejr@xfce.org>
 *                2015      Simon Steinbeiss <ochosi@xfce.org>
 *                2017      Viktor Odintsev <zakhams@gmail.com>
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



/*
 *  This file implements a plugin menu
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "pulseaudio-menu.h"
#include "pulseaudio-mpris.h"
#include "devicemenuitem.h"
#include "mprismenuitem.h"
#include "scalemenuitem.h"



struct _PulseaudioMenu
{
  GtkMenu             __parent__;

  PulseaudioVolume     *volume;
  PulseaudioConfig     *config;
  PulseaudioMpris      *mpris;
  GtkWidget            *button;

  GtkWidget            *output_scale;
  GtkWidget            *input_scale;

  gulong                volume_changed_id;
  gulong                volume_mic_changed_id;
};

struct _PulseaudioMenuClass
{
  GtkMenuClass        __parent__;
};



static void             pulseaudio_menu_finalize         (GObject       *object);



G_DEFINE_TYPE (PulseaudioMenu, pulseaudio_menu, GTK_TYPE_MENU)



static void
pulseaudio_menu_class_init (PulseaudioMenuClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_menu_finalize;
}



static void
pulseaudio_menu_init (PulseaudioMenu *menu)
{
  menu->volume                         = NULL;
  menu->config                         = NULL;
  menu->mpris                          = NULL;
  menu->button                         = NULL;
  menu->output_scale                   = NULL;
  menu->input_scale                    = NULL;
  menu->volume_changed_id              = 0;
  menu->volume_mic_changed_id          = 0;
}



static void
pulseaudio_menu_finalize (GObject *object)
{
  PulseaudioMenu *menu;

  menu = PULSEAUDIO_MENU (object);

  if (menu->volume_changed_id != 0)
    g_signal_handler_disconnect (G_OBJECT (menu->volume), menu->volume_changed_id);

  if (menu->volume_mic_changed_id != 0)
    g_signal_handler_disconnect (G_OBJECT (menu->volume), menu->volume_mic_changed_id);

  menu->volume                         = NULL;
  menu->config                         = NULL;
  menu->mpris                          = NULL;
  menu->button                         = NULL;
  menu->output_scale                   = NULL;
  menu->input_scale                    = NULL;
  menu->volume_changed_id              = 0;
  menu->volume_mic_changed_id          = 0;

  G_OBJECT_CLASS (pulseaudio_menu_parent_class)->finalize (object);
}



static void
pulseaudio_menu_output_range_scroll (GtkWidget        *widget,
                                     GdkEvent         *event,
                                     PulseaudioMenu   *menu)
{
  gdouble         new_volume;
  gdouble         volume;
  gdouble         volume_step;
  GdkEventScroll *scroll_event;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  volume =  pulseaudio_volume_get_volume (menu->volume);
  volume_step = pulseaudio_config_get_volume_step (menu->config) / 100.0;

  scroll_event = (GdkEventScroll*)event;

  new_volume = volume + (1.0 - 2.0 * scroll_event->direction) * volume_step;
  pulseaudio_volume_set_volume (menu->volume, new_volume);
}



static void
pulseaudio_menu_output_range_value_changed (PulseaudioMenu   *menu,
                                            GtkWidget        *widget)
{
  gdouble  new_volume;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  new_volume = scale_menu_item_get_value (SCALE_MENU_ITEM (menu->output_scale)) / 100.0;
  pulseaudio_volume_set_volume (menu->volume, new_volume);
}



static void
pulseaudio_menu_mute_output_item_toggled (PulseaudioMenu   *menu,
                                          ScaleMenuItem    *menu_item)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  pulseaudio_volume_set_muted (menu->volume, scale_menu_item_get_muted (menu_item));
}



static void
pulseaudio_menu_default_output_changed (PulseaudioMenu *menu,
                                        gchar          *name,
                                        DeviceMenuItem *menu_item)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  pulseaudio_volume_set_default_output (menu->volume, name);
}



static void
pulseaudio_menu_default_input_changed (PulseaudioMenu *menu,
                                       gchar          *name,
                                       DeviceMenuItem *menu_item)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  pulseaudio_volume_set_default_input (menu->volume, name);
}



static void
pulseaudio_menu_input_range_scroll (GtkWidget        *widget,
                                    GdkEvent         *event,
                                    PulseaudioMenu   *menu)
{
  gdouble         new_volume_mic;
  gdouble         volume_mic;
  gdouble         volume_step;
  GdkEventScroll *scroll_event;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  volume_mic =  pulseaudio_volume_get_volume_mic (menu->volume);
  volume_step = pulseaudio_config_get_volume_step (menu->config) / 100.0;

  scroll_event = (GdkEventScroll*)event;

  new_volume_mic = volume_mic + (1.0 - 2.0 * scroll_event->direction) * volume_step;
  pulseaudio_volume_set_volume_mic (menu->volume, new_volume_mic);
}



static void
pulseaudio_menu_input_range_value_changed (PulseaudioMenu   *menu,
                                           GtkWidget        *widget)
{
  gdouble  new_volume_mic;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  new_volume_mic = scale_menu_item_get_value (SCALE_MENU_ITEM (menu->input_scale)) / 100.0;
  pulseaudio_volume_set_volume_mic (menu->volume, new_volume_mic);
}



static void
pulseaudio_menu_mute_input_item_toggled (PulseaudioMenu   *menu,
                                         ScaleMenuItem    *menu_item)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  pulseaudio_volume_set_muted_mic (menu->volume, scale_menu_item_get_muted (menu_item));
}



static void
pulseaudio_menu_run_audio_mixer (PulseaudioMenu   *menu,
                                 GtkCheckMenuItem *menu_item)
{
  GError    *error = NULL;
  GtkWidget *message_dialog;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  if (!xfce_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (menu)),
                                          pulseaudio_config_get_mixer_command (menu->config),
                                          FALSE, FALSE, &error))
    {
      message_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_CLOSE,
                                                           _("<big><b>Failed to execute command \"%s\".</b></big>\n\n%s"),
                                                           pulseaudio_config_get_mixer_command (menu->config),
                                                           error->message);
      gtk_window_set_title (GTK_WINDOW (message_dialog), _("Error"));
      gtk_dialog_run (GTK_DIALOG (message_dialog));
      gtk_widget_destroy (message_dialog);
      g_error_free (error);
    }
}



static void
pulseaudio_menu_activate_playlist (PulseaudioMenu *menu,
                                   GtkMenuItem    *menu_item)
{
  gchar *player;
  gchar *playlist;

  g_return_if_fail(IS_PULSEAUDIO_MENU(menu));

  player = g_strdup (g_object_get_data (G_OBJECT (menu_item), "player"));
  playlist = g_strdup (g_object_get_data (G_OBJECT (menu_item), "playlist"));

  pulseaudio_mpris_activate_playlist (menu->mpris, player, playlist);

  g_free (player);
  g_free (playlist);
}



static void
pulseaudio_menu_volume_changed (PulseaudioMenu   *menu,
                                gboolean          should_notify,
                                PulseaudioVolume *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  g_signal_handlers_block_by_func (G_OBJECT (menu->output_scale),
                                   pulseaudio_menu_mute_output_item_toggled,
                                   menu);
  scale_menu_item_set_muted (SCALE_MENU_ITEM (menu->output_scale),
                                  pulseaudio_volume_get_muted (volume));
  g_signal_handlers_unblock_by_func (G_OBJECT (menu->output_scale),
                                     pulseaudio_menu_mute_output_item_toggled,
                                     menu);

  scale_menu_item_set_value (SCALE_MENU_ITEM (menu->output_scale), pulseaudio_volume_get_volume (menu->volume) * 100.0);

  g_signal_handlers_block_by_func (G_OBJECT (menu->input_scale),
                                   pulseaudio_menu_mute_input_item_toggled,
                                   menu);
  scale_menu_item_set_muted (SCALE_MENU_ITEM (menu->input_scale),
                                  pulseaudio_volume_get_muted_mic (volume));
  g_signal_handlers_unblock_by_func (G_OBJECT (menu->input_scale),
                                     pulseaudio_menu_mute_input_item_toggled,
                                     menu);

  scale_menu_item_set_value (SCALE_MENU_ITEM (menu->input_scale), pulseaudio_volume_get_volume_mic (menu->volume) * 100.0);
}



#ifdef HAVE_MPRIS2
static void
media_notify_cb (GtkWidget  *widget,
                 gchar      *message,
                 gpointer    user_data)
{
  PulseaudioMenu *menu = user_data;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  g_return_if_fail (IS_MPRIS_MENU_ITEM (widget));

  pulseaudio_mpris_notify_player (menu->mpris, mpris_menu_item_get_player (MPRIS_MENU_ITEM (widget)), message);
}



static void
mpris_update_cb (PulseaudioMpris *mpris,
                 gchar           *player,
                 gpointer         user_data)
{
  MprisMenuItem *menu_item = user_data;

  gchar          *title;
  gchar          *artist;
  gboolean        is_running;
  gboolean        is_playing;
  gboolean        is_stopped;
  gboolean        can_play;
  gboolean        can_pause;
  gboolean        can_go_previous;
  gboolean        can_go_next;
  gboolean        can_raise;
  GList          *playlists;

  g_return_if_fail (IS_PULSEAUDIO_MPRIS (mpris));
  g_return_if_fail (IS_MPRIS_MENU_ITEM (menu_item));

  if (mpris_menu_item_get_player (menu_item) == NULL)
    return;

  if (g_strcmp0 (player, mpris_menu_item_get_player (menu_item)) == 0)
    {
      if (pulseaudio_mpris_get_player_snapshot (mpris,
                                                player,
                                                &title,
                                                &artist,
                                                &is_running,
                                                &is_playing,
                                                &is_stopped,
                                                &can_play,
                                                &can_pause,
                                                &can_go_previous,
                                                &can_go_next,
                                                &can_raise,
                                                &playlists))
        {
          mpris_menu_item_set_is_running (menu_item, is_running);
          mpris_menu_item_set_title (menu_item, title);
          mpris_menu_item_set_artist (menu_item, artist);

          mpris_menu_item_set_can_play (menu_item, can_play);
          mpris_menu_item_set_can_pause (menu_item, can_pause);

          mpris_menu_item_set_can_go_previous (menu_item, can_go_previous);
          mpris_menu_item_set_can_go_next (menu_item, can_go_next);

          mpris_menu_item_set_is_playing (menu_item, is_playing);
          mpris_menu_item_set_is_stopped (menu_item, is_stopped);
        }

      if (title != NULL)
        g_free (title);
      if (artist != NULL)
        g_free (artist);
      if (playlists != NULL)
        g_list_free (playlists);
    }
}



static void
item_destroy_cb (GtkWidget  *widget,
                 gpointer    user_data)
{
  PulseaudioMenu *menu = user_data;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  g_return_if_fail (IS_MPRIS_MENU_ITEM (widget));

  g_signal_handlers_disconnect_by_func (G_OBJECT (menu->mpris), G_CALLBACK (mpris_update_cb), widget);
}
#endif



PulseaudioMenu *
pulseaudio_menu_new (PulseaudioVolume *volume,
                     PulseaudioConfig *config,
                     PulseaudioMpris  *mpris,
                     GtkWidget        *widget)
{
  PulseaudioMenu *menu;
  GdkScreen      *gscreen;
  GtkWidget      *mi;
  GtkWidget      *device_mi;
  gdouble         volume_max;

  GList          *sources = NULL;
  GList          *list = NULL;
  guint           i = 0;

#ifdef HAVE_MPRIS2
  gchar         **players;
  gchar          *title = NULL;
  gchar          *artist = NULL;
  gboolean        is_running;
  gboolean        is_playing;
  gboolean        is_stopped;
  gboolean        can_play;
  gboolean        can_pause;
  gboolean        can_go_previous;
  gboolean        can_go_next;
  gboolean        can_raise;
  GList          *playlists = NULL;
  GtkWidget      *submenu = NULL;
#endif

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  if (gtk_widget_has_screen (widget))
    gscreen = gtk_widget_get_screen (widget);
  else
    gscreen = gdk_display_get_default_screen (gdk_display_get_default ());

  menu = g_object_new (TYPE_PULSEAUDIO_MENU, NULL);
  gtk_menu_set_screen (GTK_MENU (menu), gscreen);

  menu->volume = volume;
  menu->config = config;
  menu->mpris = mpris;
  menu->button = widget;
  menu->volume_changed_id =
    g_signal_connect_swapped (G_OBJECT (menu->volume), "volume-changed",
                              G_CALLBACK (pulseaudio_menu_volume_changed), menu);
  menu->volume_mic_changed_id =
    g_signal_connect_swapped (G_OBJECT (menu->volume), "volume-mic-changed",
                              G_CALLBACK (pulseaudio_menu_volume_changed), menu);

  volume_max = pulseaudio_config_get_volume_max (menu->config);

  /* Output Devices */
  sources = pulseaudio_volume_get_output_list (menu->volume);
  if (g_list_length (sources) > 0)
    {
      /* output volume slider */
      menu->output_scale = scale_menu_item_new_with_range (0.0, volume_max, 1.0);
      scale_menu_item_set_base_icon_name (SCALE_MENU_ITEM (menu->output_scale), "audio-volume");

      g_signal_connect_swapped (menu->output_scale, "value-changed", G_CALLBACK (pulseaudio_menu_output_range_value_changed), menu);
      g_signal_connect_swapped (menu->output_scale, "toggled", G_CALLBACK (pulseaudio_menu_mute_output_item_toggled), menu);
      g_signal_connect (menu->output_scale, "scroll-event", G_CALLBACK (pulseaudio_menu_output_range_scroll), menu);

      gtk_widget_show_all (menu->output_scale);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu->output_scale);

      /* output device items */
      if (g_list_length (sources) > 1)
        {
          device_mi = device_menu_item_new_with_label(_("Output"));
          for (list = sources; list != NULL; list = g_list_next(list))
            {
              device_menu_item_add_device(DEVICE_MENU_ITEM(device_mi), (gchar *)list->data, pulseaudio_volume_get_output_by_name(menu->volume, list->data));
            }

          device_menu_item_set_device_by_name(DEVICE_MENU_ITEM(device_mi), pulseaudio_volume_get_default_output(menu->volume));
          gtk_widget_show(device_mi);

          g_signal_connect_swapped(G_OBJECT(device_mi), "device-changed", G_CALLBACK(pulseaudio_menu_default_output_changed), menu);

          gtk_menu_shell_append(GTK_MENU_SHELL(menu), device_mi);
        }

      /* separator */
      mi = gtk_separator_menu_item_new ();
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
  g_list_free (sources);

  /* Input Devices */
  sources = pulseaudio_volume_get_input_list (menu->volume);
  if (g_list_length (sources) > 0)
    {
      /* input volume slider */
      menu->input_scale = scale_menu_item_new_with_range (0.0, volume_max, 1.0);
      scale_menu_item_set_base_icon_name (SCALE_MENU_ITEM (menu->input_scale), "microphone-sensitivity");

      g_signal_connect_swapped (menu->input_scale, "value-changed", G_CALLBACK (pulseaudio_menu_input_range_value_changed), menu);
      g_signal_connect_swapped (menu->input_scale, "toggled", G_CALLBACK (pulseaudio_menu_mute_input_item_toggled), menu);
      g_signal_connect (menu->input_scale, "scroll-event", G_CALLBACK (pulseaudio_menu_input_range_scroll), menu);

      gtk_widget_show_all (menu->input_scale);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu->input_scale);

      /* input device items */
      if (g_list_length(sources) > 1)
        {
          device_mi = device_menu_item_new_with_label(_("Input"));
          for (list = sources; list != NULL; list = g_list_next(list))
            {
              device_menu_item_add_device(DEVICE_MENU_ITEM(device_mi), (gchar *)list->data, pulseaudio_volume_get_input_by_name(menu->volume, list->data));
            }

          device_menu_item_set_device_by_name(DEVICE_MENU_ITEM(device_mi), pulseaudio_volume_get_default_input(menu->volume));
          gtk_widget_show(device_mi);

          g_signal_connect_swapped(G_OBJECT(device_mi), "device-changed", G_CALLBACK(pulseaudio_menu_default_input_changed), menu);

          gtk_menu_shell_append(GTK_MENU_SHELL(menu), device_mi);
        }

      /* separator */
      mi = gtk_separator_menu_item_new();
      gtk_widget_show(mi);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
  g_list_free (sources);

  /* MPRIS2 */
#ifdef HAVE_MPRIS2
  if (pulseaudio_config_get_enable_mpris (menu->config))
    {
      players = pulseaudio_config_get_mpris_players (menu->config);
      if (players != NULL)
        {
          for (i = 0; i < g_strv_length (players); i++)
            {
              if (pulseaudio_config_player_blacklist_lookup (menu->config, players[i]))
                continue;

              mi = mpris_menu_item_new_from_player_name (players[i]);
              if (mi != NULL)
                {
                  if (pulseaudio_mpris_get_player_snapshot (menu->mpris,
                                                            players[i],
                                                            &title,
                                                            &artist,
                                                            &is_running,
                                                            &is_playing,
                                                            &is_stopped,
                                                            &can_play,
                                                            &can_pause,
                                                            &can_go_previous,
                                                            &can_go_next,
                                                            &can_raise,
                                                            &playlists))
                    {
                      mpris_menu_item_set_is_running (MPRIS_MENU_ITEM (mi), is_running);
                      mpris_menu_item_set_title (MPRIS_MENU_ITEM (mi), title);
                      mpris_menu_item_set_artist (MPRIS_MENU_ITEM (mi), artist);

                      mpris_menu_item_set_can_raise (MPRIS_MENU_ITEM (mi), can_raise);
                      mpris_menu_item_set_can_raise_wnck (MPRIS_MENU_ITEM (mi), pulseaudio_config_get_can_raise_wnck (menu->config));

                      mpris_menu_item_set_can_play (MPRIS_MENU_ITEM (mi), can_play);
                      mpris_menu_item_set_can_pause (MPRIS_MENU_ITEM (mi), can_pause);

                      mpris_menu_item_set_can_go_previous (MPRIS_MENU_ITEM (mi), can_go_previous);
                      mpris_menu_item_set_can_go_next (MPRIS_MENU_ITEM (mi), can_go_next);

                      mpris_menu_item_set_is_playing (MPRIS_MENU_ITEM (mi), is_playing);
                      mpris_menu_item_set_is_stopped (MPRIS_MENU_ITEM (mi), is_stopped);

                      if (title != NULL)
                        g_free (title);
                      if (artist != NULL)
                        g_free (artist);
                    }
                  else
                    {
                      mpris_menu_item_set_is_running (MPRIS_MENU_ITEM (mi), FALSE);
                      mpris_menu_item_set_is_stopped (MPRIS_MENU_ITEM (mi), TRUE);
                    }

                  g_signal_connect (mi, "media-notify", G_CALLBACK (media_notify_cb), menu);
                  g_signal_connect (menu->mpris, "update", G_CALLBACK (mpris_update_cb), mi);
                  g_signal_connect (mi, "destroy", G_CALLBACK(item_destroy_cb), menu);

                  gtk_widget_show(mi);
                  gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

                  if (playlists != NULL)
                    {
                      if (g_list_length(playlists) > 0)
                      {
                        mi = gtk_menu_item_new_with_label(_("Choose Playlist"));
                        gtk_widget_show(mi);
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

                        submenu = gtk_menu_new();
                        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), submenu);

                        for (list = playlists; list != NULL; list = g_list_next(list))
                        {
                          mi = gtk_menu_item_new_with_label((gchar *)list->data);
                          gtk_widget_show(mi);
                          gtk_menu_shell_append(GTK_MENU_SHELL(submenu), mi);

                          g_object_set_data (G_OBJECT(mi), "player", g_strdup (players[i]));
                          g_object_set_data (G_OBJECT(mi), "playlist", g_strdup ((gchar *)list->data));

                          g_signal_connect_swapped(G_OBJECT(mi), "activate", G_CALLBACK (pulseaudio_menu_activate_playlist), menu);
                        }
                      }

                      g_list_free(playlists);
                      playlists = NULL;
                    }

                  /* separator */
                  mi = gtk_separator_menu_item_new ();
                  gtk_widget_show (mi);
                  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
                }
            }

          g_strfreev (players);
        }
    }
#endif

  /* Audio mixers */
  mi = gtk_menu_item_new_with_mnemonic (_("_Audio mixer..."));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (G_OBJECT (mi), "activate", G_CALLBACK (pulseaudio_menu_run_audio_mixer), menu);

  pulseaudio_menu_volume_changed (menu, FALSE, menu->volume);

  return menu;
}
