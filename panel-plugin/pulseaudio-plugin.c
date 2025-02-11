/*  Copyright (c) 2014-2017 Andrzej <andrzejr@xfce.org>
 *                2017      Viktor Odintsev <zakhams@gmail.com>
 *                2017      Matthieu Mota <matthieumota@gmail.com>
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
 *  This file implements the main plugin class.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "pulseaudio-debug.h"
#include "pulseaudio-plugin.h"
#include "pulseaudio-config.h"
#include "pulseaudio-volume.h"
#include "pulseaudio-button.h"
#include "pulseaudio-dialog.h"
#include "pulseaudio-notify.h"
#include "pulseaudio-mpris.h"


#if defined (HAVE_KEYBINDER) && defined (GDK_WINDOWING_X11)
#define ENABLE_KEYBINDER 1
#include <keybinder.h>
#include <gdk/gdkx.h>

#define PULSEAUDIO_PLUGIN_RAISE_VOLUME_KEY  "XF86AudioRaiseVolume"
#define PULSEAUDIO_PLUGIN_LOWER_VOLUME_KEY  "XF86AudioLowerVolume"
#define PULSEAUDIO_PLUGIN_MUTE_KEY          "XF86AudioMute"
#define PULSEAUDIO_PLUGIN_MIC_MUTE_KEY      "XF86AudioMicMute"
#define PULSEAUDIO_PLUGIN_PLAY_KEY          "XF86AudioPlay"
#define PULSEAUDIO_PLUGIN_STOP_KEY          "XF86AudioStop"
#define PULSEAUDIO_PLUGIN_PREV_KEY          "XF86AudioPrev"
#define PULSEAUDIO_PLUGIN_NEXT_KEY          "XF86AudioNext"
#endif

#ifdef HAVE_LIBCANBERRA
#include <canberra.h>
#endif

/* prototypes */
static void             pulseaudio_plugin_init_debug                       (void);
static void             pulseaudio_plugin_construct                        (XfcePanelPlugin       *plugin);
static void             pulseaudio_plugin_free_data                        (XfcePanelPlugin       *plugin);
static void             pulseaudio_plugin_show_about                       (XfcePanelPlugin       *plugin);
static void             pulseaudio_plugin_configure_plugin                 (XfcePanelPlugin       *plugin);
static gboolean         pulseaudio_plugin_size_changed                     (XfcePanelPlugin       *plugin,
                                                                            gint                   size);

static void             pulseaudio_plugin_orientation_changed              (XfcePanelPlugin       *plugin,
                                                                            GtkOrientation         orientation);
static void             pulseaudio_plugin_set_orientation                  (PulseaudioPlugin *pulseaudio_plugin);
static void             pulseaudio_plugin_set_small                        (PulseaudioPlugin *pulseaudio_plugin);
#ifdef ENABLE_KEYBINDER
static void             pulseaudio_plugin_bind_keys_cb                     (PulseaudioPlugin      *pulseaudio_plugin,
                                                                            PulseaudioConfig      *pulseaudio_config);
static gboolean         pulseaudio_plugin_bind_keys                        (PulseaudioPlugin      *pulseaudio_plugin);
static void             pulseaudio_plugin_unbind_keys                      (PulseaudioPlugin      *pulseaudio_plugin);
static void             pulseaudio_plugin_volume_key_pressed               (const char            *keystring,
                                                                            void                  *user_data);
static void             pulseaudio_plugin_mute_pressed                     (const char            *keystring,
                                                                            void                  *user_data);
static void             pulseaudio_plugin_mic_mute_pressed                 (const char            *keystring,
                                                                            void                  *user_data);

static void             pulseaudio_plugin_bind_multimedia_keys_cb          (PulseaudioPlugin      *pulseaudio_plugin,
                                                                            PulseaudioConfig      *pulseaudio_config);
static gboolean         pulseaudio_plugin_bind_multimedia_keys             (PulseaudioPlugin      *pulseaudio_plugin);
static void             pulseaudio_plugin_unbind_multimedia_keys           (PulseaudioPlugin      *pulseaudio_plugin);
static void             pulseaudio_plugin_play_key_pressed                 (const char            *keystring,
                                                                            void                  *user_data);
static void             pulseaudio_plugin_stop_key_pressed                 (const char            *keystring,
                                                                            void                  *user_data);
static void             pulseaudio_plugin_prev_key_pressed                 (const char            *keystring,
                                                                            void                  *user_data);
static void             pulseaudio_plugin_next_key_pressed                 (const char            *keystring,
                                                                            void                  *user_data);
#endif



struct _PulseaudioPluginClass
{
  XfcePanelPluginClass __parent__;
};

/* plugin structure */
struct _PulseaudioPlugin
{
  XfcePanelPlugin      __parent__;

  PulseaudioConfig    *config;
  PulseaudioVolume    *volume;
#ifdef HAVE_LIBNOTIFY
  PulseaudioNotify    *notify;
#endif

  /* panel widgets */
  PulseaudioButton    *button;

  /* config dialog builder */
  PulseaudioDialog    *dialog;

  /* mpris */
  PulseaudioMpris     *mpris;

#ifdef HAVE_LIBCANBERRA
  ca_context          *canberra;
#endif
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (PulseaudioPlugin, pulseaudio_plugin)



static void
pulseaudio_plugin_class_init (PulseaudioPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = pulseaudio_plugin_construct;
  plugin_class->free_data = pulseaudio_plugin_free_data;
  plugin_class->about = pulseaudio_plugin_show_about;
  plugin_class->configure_plugin = pulseaudio_plugin_configure_plugin;
  plugin_class->size_changed = pulseaudio_plugin_size_changed;
  plugin_class->orientation_changed = pulseaudio_plugin_orientation_changed;
}



static void
pulseaudio_plugin_init (PulseaudioPlugin *pulseaudio_plugin)
{
  /* initialize debug logging */
  pulseaudio_plugin_init_debug ();
  pulseaudio_debug("Pulseaudio Panel Plugin initialized");

  pulseaudio_plugin->volume            = NULL;
  pulseaudio_plugin->button            = NULL;
#ifdef HAVE_LIBNOTIFY
  pulseaudio_plugin->notify            = NULL;
#endif

  pulseaudio_plugin->mpris             = NULL;

#ifdef HAVE_LIBCANBERRA
  pulseaudio_plugin->canberra          = NULL;
#endif
}



static void
pulseaudio_plugin_free_data (XfcePanelPlugin *plugin)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  if (pulseaudio_plugin->volume)
    g_object_unref (pulseaudio_plugin->volume);

  if (pulseaudio_plugin->dialog)
    g_object_unref (pulseaudio_plugin->dialog);

#ifdef ENABLE_KEYBINDER
  /* release keybindings */
  pulseaudio_plugin_unbind_keys (pulseaudio_plugin);
  pulseaudio_plugin_unbind_multimedia_keys (pulseaudio_plugin);
#endif
#ifdef HAVE_MPRIS2
  if (pulseaudio_plugin->mpris)
    g_object_unref (pulseaudio_plugin->mpris);
#endif
#ifdef HAVE_LIBCANBERRA
  if (pulseaudio_plugin->canberra)
    ca_context_destroy (pulseaudio_plugin->canberra);
#endif
#ifdef HAVE_LIBNOTIFY
  if (pulseaudio_plugin->notify)
    g_object_unref (pulseaudio_plugin->notify);
#endif

}



static void
pulseaudio_plugin_init_debug (void)
{
  const gchar  *debug_env;
  gchar       **debug_domains;
  gsize         i;
  gchar        *message_debug_env;

  /* enable debug output if the PANEL_DEBUG is set to "all" */
  debug_env = g_getenv ("PANEL_DEBUG");
  if (debug_env != NULL)
    {
      debug_domains = g_strsplit (debug_env, ",", -1);
      for (i = 0; debug_domains[i] != NULL; i++)
        {
          g_strstrip (debug_domains[i]);

          if (g_str_equal (debug_domains[i], G_LOG_DOMAIN))
            break;
          else if (g_str_equal (debug_domains[i], "all"))
            {
              message_debug_env = g_strjoin (" ", G_LOG_DOMAIN, g_getenv ("G_MESSAGES_DEBUG"), NULL);
              g_setenv ("G_MESSAGES_DEBUG", message_debug_env, TRUE);
              g_free (message_debug_env);
              break;
            }
        }
      g_strfreev (debug_domains);
    }
}



static void
pulseaudio_plugin_show_about (XfcePanelPlugin *plugin)
{
  const gchar *auth[] =
    {
      "Andrzej Radecki <andrzejr@xfce.org>",
      "Guido Berhoerster <guido+xfce@berhoerster.name>",
      "Simon Steinbeiss <simon@xfce.org>",
      "Steve Dodier-Lazaro <sidi@xfce.org>",
      "Viktor Odintsev <zakhams@gmail.com>",
      "Matthieu Mota <matthieumota@gmail.com>",
      "Sean Davis <bluesabre@xfce.org>",
      "Błażej Szczygieł <mumei6102@gmail.com>",
      NULL
    };

  g_return_if_fail (IS_PULSEAUDIO_PLUGIN (plugin));

  gtk_show_about_dialog (NULL,
                         "logo-icon-name", "xfce4-pulseaudio-plugin",
                         "license",        xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
                         "version",        VERSION_FULL,
                         "program-name",   PACKAGE_NAME,
                         "comments",       _("Adjust the audio volume of the PulseAudio sound system"),
                         "website",        "https://docs.xfce.org/panel-plugins/xfce4-pulseaudio-plugin",
                         "copyright",      "Copyright \xc2\xa9 2014-" COPYRIGHT_YEAR " The Xfce development team",
                         "authors",        auth,
                         NULL);
}



static void
pulseaudio_plugin_configure_plugin (XfcePanelPlugin *plugin)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  pulseaudio_dialog_show (pulseaudio_plugin->dialog, gtk_widget_get_screen (GTK_WIDGET (plugin)));
}



static gboolean
pulseaudio_plugin_size_changed (XfcePanelPlugin *plugin,
                                gint             size)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);
  gint              icon_size;
  
  /* The plugin only occupies a single row */
  size /= xfce_panel_plugin_get_nrows(plugin);

  icon_size = xfce_panel_plugin_get_icon_size (plugin);
  pulseaudio_button_set_size (pulseaudio_plugin->button, size, icon_size);

  pulseaudio_plugin_set_orientation (pulseaudio_plugin);

  return TRUE;
}



static void
pulseaudio_plugin_orientation_changed (XfcePanelPlugin *plugin,
                                       GtkOrientation   orientation)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  pulseaudio_plugin_set_orientation (pulseaudio_plugin);
}



static void
pulseaudio_plugin_set_orientation (PulseaudioPlugin *pulseaudio_plugin)
{
  GtkOrientation orientation;
  gint           size;
  gint           icon_size;

  orientation = xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (pulseaudio_plugin));
  size = xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (pulseaudio_plugin));
  icon_size = xfce_panel_plugin_get_icon_size (XFCE_PANEL_PLUGIN (pulseaudio_plugin));

  if (size > icon_size * 2 + XFCE_PANEL_PLUGIN_ICON_PADDING )
    {
      if (orientation == GTK_ORIENTATION_VERTICAL)
        pulseaudio_button_set_orientation (pulseaudio_plugin->button, GTK_ORIENTATION_HORIZONTAL);
      else
        pulseaudio_button_set_orientation (pulseaudio_plugin->button, GTK_ORIENTATION_VERTICAL);
    }
  else
    {
      pulseaudio_button_set_orientation (pulseaudio_plugin->button, orientation);
    }
}



static void
pulseaudio_plugin_set_small (PulseaudioPlugin *pulseaudio_plugin)
{
  gboolean recording = pulseaudio_volume_get_recording (pulseaudio_plugin->volume);
  gboolean rec_indicator_persistent = pulseaudio_config_get_rec_indicator_persistent (pulseaudio_plugin->config);

  if (recording || rec_indicator_persistent)
  {
    xfce_panel_plugin_set_small (XFCE_PANEL_PLUGIN (pulseaudio_plugin), FALSE);
  }
  else
  {
    xfce_panel_plugin_set_small (XFCE_PANEL_PLUGIN (pulseaudio_plugin), TRUE);
  }
}



#ifdef ENABLE_KEYBINDER
static void
pulseaudio_plugin_bind_keys_cb (PulseaudioPlugin      *pulseaudio_plugin,
                                PulseaudioConfig      *pulseaudio_config)
{
  g_return_if_fail (IS_PULSEAUDIO_PLUGIN (pulseaudio_plugin));

  if (pulseaudio_config_get_enable_keyboard_shortcuts (pulseaudio_plugin->config))
    pulseaudio_plugin_bind_keys (pulseaudio_plugin);
  else
    pulseaudio_plugin_unbind_keys (pulseaudio_plugin);
}



static gboolean
pulseaudio_plugin_bind_keys (PulseaudioPlugin      *pulseaudio_plugin)
{
  gboolean success = TRUE;
  g_return_val_if_fail (IS_PULSEAUDIO_PLUGIN (pulseaudio_plugin), FALSE);
  pulseaudio_debug ("Grabbing volume control keys");

  success &= keybinder_bind (PULSEAUDIO_PLUGIN_LOWER_VOLUME_KEY, pulseaudio_plugin_volume_key_pressed, pulseaudio_plugin);
  success &= keybinder_bind (PULSEAUDIO_PLUGIN_RAISE_VOLUME_KEY, pulseaudio_plugin_volume_key_pressed, pulseaudio_plugin);
  success &= keybinder_bind (PULSEAUDIO_PLUGIN_MUTE_KEY, pulseaudio_plugin_mute_pressed, pulseaudio_plugin);
  success &= keybinder_bind (PULSEAUDIO_PLUGIN_MIC_MUTE_KEY, pulseaudio_plugin_mic_mute_pressed, pulseaudio_plugin);

  if (!success)
    g_warning ("Could not have grabbed volume control keys. Is another volume control application (xfce4-volumed) running?");

  return success;
}



static void
pulseaudio_plugin_unbind_keys (PulseaudioPlugin      *pulseaudio_plugin)
{
  g_return_if_fail (IS_PULSEAUDIO_PLUGIN (pulseaudio_plugin));
  pulseaudio_debug ("Releasing volume control keys");

  keybinder_unbind (PULSEAUDIO_PLUGIN_LOWER_VOLUME_KEY, pulseaudio_plugin_volume_key_pressed);
  keybinder_unbind (PULSEAUDIO_PLUGIN_RAISE_VOLUME_KEY, pulseaudio_plugin_volume_key_pressed);
  keybinder_unbind (PULSEAUDIO_PLUGIN_MUTE_KEY, pulseaudio_plugin_mute_pressed);
  keybinder_unbind (PULSEAUDIO_PLUGIN_MIC_MUTE_KEY, pulseaudio_plugin_mic_mute_pressed);
}

static void
pulseaudio_plugin_volume_key_pressed (const char            *keystring,
                                      void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (user_data);
  gdouble           volume            = pulseaudio_volume_get_volume (pulseaudio_plugin->volume);
  gdouble           volume_step       = pulseaudio_config_get_volume_step (pulseaudio_plugin->config) / 100.0;
  gboolean          notify            = pulseaudio_volume_get_show_notifications (pulseaudio_plugin->volume, VOLUME_NOTIFICATIONS_OUTPUT);

  pulseaudio_debug ("%s pressed", keystring);

  if (strcmp (keystring, PULSEAUDIO_PLUGIN_RAISE_VOLUME_KEY) == 0) {
    pulseaudio_volume_set_volume (pulseaudio_plugin->volume, MIN (volume + volume_step, MAX (volume, 1.0)));
#ifdef HAVE_LIBNOTIFY
    /* Also send notification when volume is already at 100% */
    if (notify && volume > 1.0 - 2e-3)
      pulseaudio_notify_volume_changed (pulseaudio_plugin->notify, TRUE, pulseaudio_plugin->volume);
#endif
  } else if (strcmp (keystring, PULSEAUDIO_PLUGIN_LOWER_VOLUME_KEY) == 0) {
    pulseaudio_volume_set_volume (pulseaudio_plugin->volume, MAX (0.0, volume - volume_step));
#ifdef HAVE_LIBNOTIFY
    /* Also send notification when volume is already at 0% */
    if (notify && volume < 2e-3)
      pulseaudio_notify_volume_changed (pulseaudio_plugin->notify, TRUE, pulseaudio_plugin->volume);
#endif
  }
}



static void
pulseaudio_plugin_mute_pressed (const char            *keystring,
                                void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (user_data);

  pulseaudio_debug ("%s pressed", keystring);

  pulseaudio_volume_toggle_muted (pulseaudio_plugin->volume);
}



static void
pulseaudio_plugin_mic_mute_pressed (const char            *keystring,
                                    void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (user_data);

  pulseaudio_debug ("%s pressed", keystring);

  pulseaudio_volume_toggle_muted_mic (pulseaudio_plugin->volume);
}



static void
pulseaudio_plugin_bind_multimedia_keys_cb (PulseaudioPlugin      *pulseaudio_plugin,
                                           PulseaudioConfig      *pulseaudio_config)
{
  g_return_if_fail (IS_PULSEAUDIO_PLUGIN (pulseaudio_plugin));

  if (pulseaudio_config_get_enable_multimedia_keys (pulseaudio_plugin->config))
    pulseaudio_plugin_bind_multimedia_keys (pulseaudio_plugin);
  else
    pulseaudio_plugin_unbind_multimedia_keys (pulseaudio_plugin);
}



static gboolean
pulseaudio_plugin_bind_multimedia_keys (PulseaudioPlugin      *pulseaudio_plugin)
{
  gboolean success = TRUE;
  g_return_val_if_fail (IS_PULSEAUDIO_PLUGIN (pulseaudio_plugin), FALSE);
  pulseaudio_debug ("Grabbing multimedia control keys");

  success &= keybinder_bind (PULSEAUDIO_PLUGIN_PLAY_KEY, pulseaudio_plugin_play_key_pressed, pulseaudio_plugin);
  success &= keybinder_bind (PULSEAUDIO_PLUGIN_STOP_KEY, pulseaudio_plugin_stop_key_pressed, pulseaudio_plugin);
  success &= keybinder_bind (PULSEAUDIO_PLUGIN_PREV_KEY, pulseaudio_plugin_prev_key_pressed, pulseaudio_plugin);
  success &= keybinder_bind (PULSEAUDIO_PLUGIN_NEXT_KEY, pulseaudio_plugin_next_key_pressed, pulseaudio_plugin);

  if (!success)
    g_warning ("Could not have grabbed multimedia control keys.");

  return success;
}



static void
pulseaudio_plugin_unbind_multimedia_keys (PulseaudioPlugin      *pulseaudio_plugin)
{
  g_return_if_fail (IS_PULSEAUDIO_PLUGIN (pulseaudio_plugin));
  pulseaudio_debug ("Releasing multimedia control keys");

  keybinder_unbind (PULSEAUDIO_PLUGIN_PLAY_KEY, pulseaudio_plugin_play_key_pressed);
  keybinder_unbind (PULSEAUDIO_PLUGIN_STOP_KEY, pulseaudio_plugin_stop_key_pressed);
  keybinder_unbind (PULSEAUDIO_PLUGIN_PREV_KEY, pulseaudio_plugin_prev_key_pressed);
  keybinder_unbind (PULSEAUDIO_PLUGIN_NEXT_KEY, pulseaudio_plugin_next_key_pressed);
}



static void
pulseaudio_plugin_play_key_pressed (const char            *keystring,
                                    void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN(user_data);

  pulseaudio_debug ("%s pressed", keystring);

  pulseaudio_mpris_notify_any_player (pulseaudio_plugin->mpris, "PlayPause");
}



static void
pulseaudio_plugin_stop_key_pressed (const char            *keystring,
                                    void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN(user_data);

  pulseaudio_debug("%s pressed", keystring);

  pulseaudio_mpris_notify_any_player(pulseaudio_plugin->mpris, "Stop");
}



static void
pulseaudio_plugin_prev_key_pressed (const char            *keystring,
                                    void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN(user_data);

  pulseaudio_debug("%s pressed", keystring);

  pulseaudio_mpris_notify_any_player(pulseaudio_plugin->mpris, "Previous");
}



static void
pulseaudio_plugin_next_key_pressed (const char            *keystring,
                                    void                  *user_data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN(user_data);

  pulseaudio_debug("%s pressed", keystring);

  pulseaudio_mpris_notify_any_player(pulseaudio_plugin->mpris, "Next");
}
#endif



#ifdef HAVE_LIBCANBERRA
void
pulseaudio_plugin_play_sound (PulseaudioPlugin      *pulseaudio_plugin,
                              const char            *event_id,
                              const char            *event_desc)
{
  pulseaudio_debug("play %s", event_id);

  if (pulseaudio_config_get_play_sound (pulseaudio_plugin->config))
    {
      ca_context_play (pulseaudio_plugin->canberra, 0,
                       CA_PROP_EVENT_ID, event_id,
                       CA_PROP_EVENT_DESCRIPTION, event_desc,
                       NULL);
    }
}
#endif



static void
pulseaudio_plugin_construct (XfcePanelPlugin *plugin)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  xfce_panel_plugin_menu_show_configure (plugin);
  xfce_panel_plugin_menu_show_about (plugin);

  /* setup transation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* initialize xfconf */
  pulseaudio_plugin->config = pulseaudio_config_new (xfce_panel_plugin_get_property_base (plugin));

  g_signal_connect_swapped (pulseaudio_plugin->config, "notify::rec-indicator-persistent", G_CALLBACK (pulseaudio_plugin_set_small), pulseaudio_plugin);

  /* instantiate preference dialog builder */
  pulseaudio_plugin->dialog = pulseaudio_dialog_new (pulseaudio_plugin->config);

#ifdef ENABLE_KEYBINDER
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    {
      /* Initialize libkeybinder */
      keybinder_init ();
      g_signal_connect_swapped (G_OBJECT (pulseaudio_plugin->config), "notify::enable-keyboard-shortcuts",
                                G_CALLBACK (pulseaudio_plugin_bind_keys_cb), pulseaudio_plugin);
      g_signal_connect_swapped (G_OBJECT (pulseaudio_plugin->config), "notify::enable-multimedia-keys",
                                G_CALLBACK (pulseaudio_plugin_bind_multimedia_keys_cb), pulseaudio_plugin);
      pulseaudio_plugin_bind_keys_cb (pulseaudio_plugin, NULL);
      pulseaudio_plugin_bind_multimedia_keys_cb (pulseaudio_plugin, NULL);
    }
#endif

  /* volume controller */
  pulseaudio_plugin->volume = pulseaudio_volume_new (pulseaudio_plugin,
                                                     pulseaudio_plugin->config);

  g_signal_connect_swapped (pulseaudio_plugin->volume, "recording_changed", G_CALLBACK (pulseaudio_plugin_set_small), pulseaudio_plugin);

  /* initialize mpris support */
#ifdef HAVE_MPRIS2
  pulseaudio_plugin->mpris = pulseaudio_mpris_new (pulseaudio_plugin->config);
#endif

  /* instantiate a button box */
  pulseaudio_plugin->button = pulseaudio_button_new (pulseaudio_plugin,
                                                     pulseaudio_plugin->config,
                                                     pulseaudio_plugin->mpris,
                                                     pulseaudio_plugin->volume);

  /* initialize notify wrapper */
#ifdef HAVE_LIBNOTIFY
  pulseaudio_plugin->notify = pulseaudio_notify_new (pulseaudio_plugin->config,
                                                     pulseaudio_plugin->volume,
                                                     pulseaudio_plugin->button);
#endif

  /* initialize canberra support */
#ifdef HAVE_LIBCANBERRA
  ca_context_create (&pulseaudio_plugin->canberra);
#endif

  gtk_container_add (GTK_CONTAINER (plugin), GTK_WIDGET (pulseaudio_plugin->button));
  gtk_widget_show (GTK_WIDGET (pulseaudio_plugin->button));
}
