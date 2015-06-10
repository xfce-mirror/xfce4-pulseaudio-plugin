/*  Copyright (c) 2009-2015 Steve Dodier-Lazaro <sidi@xfce.org>
 *  Copyright (c) 2015      Andrzej <ndrwrdck@gmail.com>
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
 *  This file implements a wrapper for libnotify
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#include <libxfce4util/libxfce4util.h>


#define SYNCHRONOUS      "x-canonical-private-synchronous"
#define LAYOUT_ICON_ONLY "x-canonical-private-icon-only"

#include "pulseaudio-notify.h"

#define V_MUTED  0
#define V_LOW    1
#define V_MEDIUM 2
#define V_HIGH   3


/* Icons for different volume levels */
static const char *icons[] = {
  "audio-volume-muted-symbolic",
  "audio-volume-low-symbolic",
  "audio-volume-medium-symbolic",
  "audio-volume-high-symbolic",
  NULL
};


static void                 pulseaudio_notify_finalize        (GObject            *object);


struct _PulseaudioNotify
{
  GObject               __parent__;

  PulseaudioConfig     *config;
  PulseaudioVolume     *volume;

  gboolean              gauge_notifications;
  NotifyNotification   *notification;

  gulong                volume_changed_id;
};

struct _PulseaudioNotifyClass
{
  GObjectClass          __parent__;
};




G_DEFINE_TYPE (PulseaudioNotify, pulseaudio_notify, G_TYPE_OBJECT)

static void
pulseaudio_notify_class_init (PulseaudioNotifyClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_notify_finalize;
}



static void
pulseaudio_notify_init (PulseaudioNotify *notify)
{
  GList *caps_list;
  GList *node;

  notify->gauge_notifications = TRUE;
  notify->notification = NULL;
  notify->volume_changed_id = 0;

  //g_set_application_name ("Xfce volume control");
  notify_init ("Xfce volume control");

  caps_list = notify_get_server_caps ();

  if (caps_list)
    {
      node = g_list_find_custom (caps_list, LAYOUT_ICON_ONLY, (GCompareFunc) g_strcmp0);
      if (!node)
        notify->gauge_notifications = FALSE;
      /*		node = g_list_find_custom (caps_list, SYNCHRONOUS, (GCompareFunc) g_strcmp0);*/
      /*		if (!node)*/
      /*			Inst->gauge_notifications = FALSE;*/
      g_list_free (caps_list);
    }
  notify->notification = notify_notification_new ("xfce4-pulseaudio-plugin", NULL, NULL);
  notify_notification_set_timeout (notify->notification, 2000);
}



static void
pulseaudio_notify_finalize (GObject *object)
{
  PulseaudioNotify *notify = PULSEAUDIO_NOTIFY (object);

  notify->config = NULL;

  g_object_unref (G_OBJECT (notify->notification));
  notify->notification = NULL;
  notify_uninit ();

  (*G_OBJECT_CLASS (pulseaudio_notify_parent_class)->finalize) (object);
}



static void
pulseaudio_notify_notify (PulseaudioNotify *notify)
{
  GError      *error = NULL;
  gdouble      volume;
  gint         volume_i;
  gboolean     muted;
  gboolean     connected;
  gchar       *title = NULL;
  const gchar *icon = NULL;

  g_return_if_fail (IS_PULSEAUDIO_NOTIFY (notify));
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (notify->volume));

  if (!pulseaudio_config_get_show_notifications (notify->config))
    return;

  volume = pulseaudio_volume_get_volume (notify->volume);
  muted = pulseaudio_volume_get_muted (notify->volume);
  connected = pulseaudio_volume_get_connected (notify->volume);
  volume_i = (gint) round (volume * 100);

  if (!connected)
    volume_i = 0;

  if (!connected)
    title = g_strdup_printf ( _("Not connected to the PulseAudio server"));
  else if (muted)
    title = g_strdup_printf ( _("Volume %d%c (muted)"), volume_i, '%');
  else
    title = g_strdup_printf ( _("Volume %d%c"), volume_i, '%');

  if (!connected)
    icon = icons[V_MUTED];
  else if (muted)
    icon = icons[V_MUTED];
  else if (volume <= 0.0)
    icon = icons[V_MUTED];
  else if (volume <= 0.3)
    icon = icons[V_LOW];
  else if (volume <= 0.7)
    icon = icons[V_MEDIUM];
  else
    icon = icons[V_HIGH];


  notify_notification_update (notify->notification,
                              title,
                              NULL,
                              icon);
  g_free (title);

  if (notify->gauge_notifications) {
    notify_notification_set_hint_int32 (notify->notification,
                                        "value",
                                        volume_i);
    notify_notification_set_hint_string (notify->notification,
                                         "x-canonical-private-synchronous",
                                         "");
  }

  if (!notify_notification_show (notify->notification, &error))
    {
      g_warning ("Error while sending notification : %s\n", error->message);
      g_error_free (error);
    }
}



static void
pulseaudio_notify_volume_changed (PulseaudioNotify  *notify,
                                  PulseaudioVolume  *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_NOTIFY (notify));

  pulseaudio_notify_notify (notify);
}



PulseaudioNotify *
pulseaudio_notify_new (PulseaudioConfig *config,
                       PulseaudioVolume *volume)
{
  PulseaudioNotify *notify;

  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);

  notify = g_object_new (TYPE_PULSEAUDIO_NOTIFY, NULL);

  notify->config = config;
  notify->volume = volume;
  notify->volume_changed_id =
    g_signal_connect_swapped (G_OBJECT (notify->volume), "volume-changed",
                              G_CALLBACK (pulseaudio_notify_volume_changed), notify);

  return notify;
}


#endif /* HAVE_LIBNOTIFY */
