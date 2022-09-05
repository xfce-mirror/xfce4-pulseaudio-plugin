/*  Copyright (c) 2014-2017 Andrzej <andrzejr@xfce.org>
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
 *  This file implements a pulseaudio volume class abstracting out
 *  operations on pulseaudio mixer.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

#include "pulseaudio-config.h"
#include "pulseaudio-debug.h"
#include "pulseaudio-volume.h"


static void                 pulseaudio_volume_finalize                  (GObject              *object);
static void                 pulseaudio_volume_connect                   (PulseaudioVolume     *volume);
static gdouble              pulseaudio_volume_v2d                       (PulseaudioVolume     *volume,
                                                                         pa_volume_t           vol);
static pa_volume_t          pulseaudio_volume_d2v                       (PulseaudioVolume     *volume,
                                                                         gdouble               vol);
static gboolean             pulseaudio_volume_reconnect_timeout         (gpointer              userdata);
static void                 pulseaudio_volume_get_sink_list_cb          (pa_context           *context,
                                                                         const pa_sink_info   *i,
                                                                         int                   eol,
                                                                         void                 *userdata);
static void                 pulseaudio_volume_get_source_list_cb        (pa_context           *context,
                                                                         const pa_source_info *i,
                                                                         int                   eol,
                                                                         void                 *userdata);
static void                 pulseaudio_volume_get_source_output_info_cb (pa_context           *context,
                                                                         const pa_source_output_info *i,
                                                                         int                   eol,
                                                                         void                 *userdata);
static void                 pulseaudio_volume_move_sink_input           (pa_context           *context,
                                                                         const                 pa_sink_input_info *i,
                                                                         int                   eol,
                                                                         void                 *userdata);



struct _PulseaudioVolume
{
  GObject               __parent__;

  PulseaudioConfig     *config;

  pa_glib_mainloop     *pa_mainloop;
  pa_context           *pa_context;
  gboolean              connected;
  gboolean              sink_connected;
  gboolean              source_connected;

  gdouble               volume;
  gboolean              muted;
  gboolean              recording;

  gdouble               volume_mic;
  gboolean              muted_mic;

  guint                 reconnect_timer_id;

  /* Device management */
  GHashTable           *sinks;
  GHashTable           *sources;

  guint                 sink_index;
  guint                 source_index;

  gchar                *default_sink_name;
  gchar                *default_source_name;
};

struct _PulseaudioVolumeClass
{
  GObjectClass          __parent__;
};



enum
{
  VOLUME_CHANGED,
  VOLUME_MIC_CHANGED,
  RECORDING_CHANGED,
  LAST_SIGNAL
};

static guint pulseaudio_volume_signals[LAST_SIGNAL] = { 0, };



G_DEFINE_TYPE (PulseaudioVolume, pulseaudio_volume, G_TYPE_OBJECT)



static void
pulseaudio_volume_class_init (PulseaudioVolumeClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_volume_finalize;

  pulseaudio_volume_signals[VOLUME_CHANGED] =
    g_signal_new (g_intern_static_string ("volume-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  pulseaudio_volume_signals[VOLUME_MIC_CHANGED] =
    g_signal_new (g_intern_static_string ("volume-mic-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  pulseaudio_volume_signals[RECORDING_CHANGED] =
    g_signal_new (g_intern_static_string ("recording-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}



static void
pulseaudio_volume_init (PulseaudioVolume *volume)
{
  volume->connected = FALSE;
  volume->volume = 0.0;
  volume->muted = FALSE;
  volume->recording = FALSE;
  volume->volume_mic = 0.0;
  volume->muted_mic = FALSE;
  volume->reconnect_timer_id = 0;

  volume->default_sink_name = NULL;
  volume->default_source_name = NULL;

  volume->pa_mainloop = pa_glib_mainloop_new (NULL);

  volume->sinks = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);
  volume->sources = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

  pulseaudio_volume_connect (volume);
}



static void
pulseaudio_volume_finalize (GObject *object)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (object);

  volume->config = NULL;

  if (volume->default_sink_name)
    g_free (volume->default_sink_name);

  if (volume->default_source_name)
    g_free (volume->default_source_name);

  g_hash_table_destroy (volume->sinks);
  g_hash_table_destroy (volume->sources);

  pa_glib_mainloop_free (volume->pa_mainloop);

  (*G_OBJECT_CLASS (pulseaudio_volume_parent_class)->finalize) (object);
}



/* sink event callbacks */
static void
pulseaudio_volume_sink_info_cb (pa_context         *context,
                                const pa_sink_info *i,
                                int                 eol,
                                void               *userdata)
{
  gboolean  muted;
  gdouble   vol;

  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pulseaudio_debug ("sink info: %s, %s", i->name, i->description);

  volume->sink_index = (guint)i->index;

  muted = !!(i->mute);
  vol = pulseaudio_volume_v2d (volume, i->volume.values[0]);

  if (volume->muted != muted)
    {
      pulseaudio_debug ("Updated Mute: %d -> %d", volume->muted, muted);
      volume->muted = muted;

      if (volume->sink_connected)
        g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0, TRUE);
    }

  if (ABS (volume->volume - vol) > 2e-3)
    {
      pulseaudio_debug ("Updated Volume: %04.3f -> %04.3f", volume->volume, vol);
      volume->volume = vol;

      if (volume->sink_connected)
        g_signal_emit(G_OBJECT(volume), pulseaudio_volume_signals[VOLUME_CHANGED], 0, TRUE);
    }

  pulseaudio_debug ("volume: %f, muted: %d", vol, muted);
  volume->sink_connected = TRUE;
}



static void
pulseaudio_volume_source_info_cb (pa_context           *context,
                                  const pa_source_info *i,
                                  int                   eol,
                                  void                 *userdata)
{
  gboolean  muted_mic;
  gdouble   vol_mic;

  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pulseaudio_debug ("source info: %s, %s", i->name, i->description);

  volume->source_index = (guint)i->index;

  muted_mic = !!(i->mute);
  vol_mic = pulseaudio_volume_v2d (volume, i->volume.values[0]);

  if (volume->muted_mic != muted_mic)
    {
      pulseaudio_debug ("Updated Mute Mic: %d -> %d", volume->muted_mic, muted_mic);
      volume->muted_mic = muted_mic;

      if (volume->source_connected)
        g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_MIC_CHANGED], 0, FALSE);
    }

  if (ABS (volume->volume_mic - vol_mic) > 2e-3)
    {
      pulseaudio_debug ("Updated Volume Mic: %04.3f -> %04.3f", volume->volume_mic, vol_mic);
      volume->volume_mic = vol_mic;

      if (volume->source_connected)
        g_signal_emit(G_OBJECT(volume), pulseaudio_volume_signals[VOLUME_MIC_CHANGED], 0, FALSE);
    }

  pulseaudio_debug ("volume mic: %f, muted mic: %d", vol_mic, muted_mic);
  volume->source_connected = TRUE;
}



static void
pulseaudio_volume_server_info_cb (pa_context           *context,
                                  const pa_server_info *i,
                                  void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pulseaudio_volume_set_default_input (volume, i->default_source_name);
  pulseaudio_volume_set_default_output (volume, i->default_sink_name);

  pulseaudio_debug ("server: %s@%s, v.%s", i->user_name, i->server_name, i->server_version);
  pa_context_get_sink_info_by_name (context, i->default_sink_name, pulseaudio_volume_sink_info_cb, volume);
  pa_context_get_source_info_by_name (context, i->default_source_name, pulseaudio_volume_source_info_cb, volume);
}



static void
pulseaudio_volume_sink_source_check (PulseaudioVolume *volume,
                                     pa_context       *context)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pa_context_get_server_info (context, pulseaudio_volume_server_info_cb, volume);

  g_hash_table_remove_all (volume->sinks);
  g_hash_table_remove_all (volume->sources);

  pa_context_get_sink_info_list (volume->pa_context, pulseaudio_volume_get_sink_list_cb, volume);
  pa_context_get_source_info_list (volume->pa_context, pulseaudio_volume_get_source_list_cb, volume);
}



static void
pulseaudio_volume_subscribe_cb (pa_context                   *context,
                                pa_subscription_event_type_t  t,
                                uint32_t                      idx,
                                void                         *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
    {
    case PA_SUBSCRIPTION_EVENT_SINK          :
      pulseaudio_volume_sink_source_check (volume, context);
      pulseaudio_debug ("received sink event");
      break;

    case PA_SUBSCRIPTION_EVENT_SINK_INPUT :
      pulseaudio_volume_sink_source_check (volume, context);
      pulseaudio_debug ("received sink output event");
      break;

    case PA_SUBSCRIPTION_EVENT_SOURCE        :
      pulseaudio_volume_sink_source_check (volume, context);
      pulseaudio_debug ("received source event");
      break;

    case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT :
      gboolean recording = volume->recording;

      volume->recording = FALSE;
      pa_context_get_source_output_info_list (context, pulseaudio_volume_get_source_output_info_cb, volume);

      if (volume->recording != recording)
        g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [RECORDING_CHANGED], 0, volume->recording);

      pulseaudio_volume_sink_source_check (volume, context);
      pulseaudio_debug ("received source output event");
      break;

    case PA_SUBSCRIPTION_EVENT_SERVER        :
      pulseaudio_volume_sink_source_check (volume, context);
      pulseaudio_debug ("received server event");
      break;

    default                                  :
      pulseaudio_debug ("received unknown pulseaudio event");
      break;
    }
}



static void
pulseaudio_volume_get_sink_list_cb (pa_context         *context,
                                    const pa_sink_info *i,
                                    int                 eol,
                                    void               *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  if (eol > 0)
    return;

  g_hash_table_insert (volume->sinks, g_strdup (i->name), g_strdup (i->description));
}



static void
pulseaudio_volume_get_source_output_info_cb (pa_context                  *context,
                                             const pa_source_output_info *i,
                                             int                          eol,
                                             void                        *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (eol > 0)
    return;

  if (i)
    {
      if (g_strcmp0("PulseAudio Volume Control",
                    pa_proplist_gets (i->proplist, PA_PROP_APPLICATION_NAME)) == 0)
        {
          pulseaudio_debug ("Don't show recording indicator for pavucontrol");
          return;
        }

      volume->recording = TRUE;
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [RECORDING_CHANGED], 0, TRUE);
    }
  else
    {
      volume->recording = FALSE;
    }
}


static void
pulseaudio_volume_get_source_list_cb (pa_context           *context,
                                      const pa_source_info *i,
                                      int                   eol,
                                      void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  if (eol > 0)
    return;

  /* Ignore sink monitors, not relevant for users */
  if (i->monitor_of_sink != PA_INVALID_INDEX)
    return;

  g_hash_table_insert (volume->sources, g_strdup (i->name), g_strdup (i->description));
}



static void
pulseaudio_volume_get_server_info_cb (pa_context           *context,
                                      const pa_server_info *i,
                                      void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  g_free (volume->default_sink_name);
  g_free (volume->default_source_name);

  volume->default_sink_name = g_strdup (i->default_sink_name);
  volume->default_source_name = g_strdup (i->default_source_name);
}



static void
pulseaudio_volume_context_state_cb (pa_context *context,
                                    void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  switch (pa_context_get_state (context))
    {
    case PA_CONTEXT_READY        :
      pa_context_subscribe (context, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SINK_INPUT | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT | PA_SUBSCRIPTION_MASK_SERVER , NULL, NULL);
      pa_context_set_subscribe_callback (context, pulseaudio_volume_subscribe_cb, volume);

      pulseaudio_debug ("PulseAudio connection established");
      volume->connected = TRUE;
      // Check current sink and source volume manually. PA sink events usually not emitted.
      pulseaudio_volume_sink_source_check (volume, context);

      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0, FALSE);
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_MIC_CHANGED], 0, FALSE);
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [RECORDING_CHANGED], 0, FALSE);

      volume->sink_connected = FALSE;
      volume->source_connected = FALSE;

      pa_context_get_server_info (volume->pa_context, pulseaudio_volume_get_server_info_cb, volume);

      break;

    case PA_CONTEXT_FAILED       :
    case PA_CONTEXT_TERMINATED   :
      g_warning ("Disconnected from the PulseAudio server. Attempting to reconnect in 5 seconds...");
      volume->pa_context = NULL;
      volume->connected = FALSE;
      volume->volume = 0.0;
      volume->muted = FALSE;
      volume->recording = FALSE;
      volume->volume_mic = 0.0;
      volume->muted_mic = FALSE;

      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0, FALSE);
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_MIC_CHANGED], 0, FALSE);
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [RECORDING_CHANGED], 0, FALSE);

      g_hash_table_remove_all (volume->sinks);
      g_hash_table_remove_all (volume->sources);

      if (volume->reconnect_timer_id == 0)
        volume->reconnect_timer_id = g_timeout_add_seconds
          (5, pulseaudio_volume_reconnect_timeout, volume);
      break;

    case PA_CONTEXT_CONNECTING   :
      pulseaudio_debug ("Connecting to PulseAudio server");
      break;

    case PA_CONTEXT_SETTING_NAME :
      pulseaudio_debug ("Setting application name");
      break;

    case PA_CONTEXT_AUTHORIZING  :
      pulseaudio_debug ("Authorizing");
      break;

    case PA_CONTEXT_UNCONNECTED  :
      pulseaudio_debug ("Not connected to PulseAudio server");
      break;

    default                      :
      g_warning ("Unknown PulseAudio context state");
      break;
    }
}



static void
pulseaudio_volume_connect (PulseaudioVolume *volume)
{
  pa_proplist  *proplist;
  gint          err;

  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (!volume->connected);

  proplist = pa_proplist_new ();
#ifdef HAVE_CONFIG_H
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_ID, "org.xfce.pulseaudio-plugin");
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
#endif

  volume->pa_context = pa_context_new_with_proplist (pa_glib_mainloop_get_api (volume->pa_mainloop), NULL, proplist);
  pa_context_set_state_callback(volume->pa_context, pulseaudio_volume_context_state_cb, volume);

  err = pa_context_connect (volume->pa_context, NULL, PA_CONTEXT_NOFAIL, NULL);
  if (err < 0)
    g_warning ("pa_context_connect() failed: %s", pa_strerror (err));
}



static gboolean
pulseaudio_volume_reconnect_timeout  (gpointer userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  volume->reconnect_timer_id = 0;
  pulseaudio_volume_connect (volume);

  return FALSE;  // stop the timer
}



gboolean
pulseaudio_volume_get_connected (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->connected;
}



gboolean
pulseaudio_volume_get_sink_connected (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->sink_connected;
}



gboolean
pulseaudio_volume_get_source_connected (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->source_connected;
}



static gdouble
pulseaudio_volume_v2d (PulseaudioVolume *volume,
                       pa_volume_t       pa_volume)
{
  gdouble vol;
  gdouble vol_max;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), 0.0);

  vol_max = pulseaudio_config_get_volume_max (volume->config) / 100.0;

  vol = (gdouble) pa_volume - PA_VOLUME_MUTED;
  vol /= (gdouble) (PA_VOLUME_NORM - PA_VOLUME_MUTED);
  /* for safety */
  vol = MIN (MAX (vol, 0.0), vol_max);
  return vol;
}



static pa_volume_t
pulseaudio_volume_d2v (PulseaudioVolume *volume,
                       gdouble           vol)
{
  pa_volume_t pa_volume;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), PA_VOLUME_MUTED);

  pa_volume = (pa_volume_t) ((PA_VOLUME_NORM - PA_VOLUME_MUTED) * vol);
  pa_volume = pa_volume + PA_VOLUME_MUTED;
  /* for safety */
  pa_volume = MIN (MAX (pa_volume, PA_VOLUME_MUTED), PA_VOLUME_MAX);
  return pa_volume;
}



gboolean
pulseaudio_volume_get_muted (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->muted;
}



gboolean
pulseaudio_volume_get_recording (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->recording;
}



/* final callback for volume/mute changes */
/* pa_context_success_cb_t */
static void
pulseaudio_volume_sink_volume_changed (pa_context *context,
                                       int         success,
                                       void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (success)
    g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0, TRUE);
}



void
pulseaudio_volume_set_muted (PulseaudioVolume *volume,
                             gboolean          muted)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (volume->pa_context != NULL);
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  if (volume->muted != muted)
    {
      volume->muted = muted;
      pa_context_set_sink_mute_by_index (volume->pa_context, volume->sink_index, volume->muted, pulseaudio_volume_sink_volume_changed, volume);
    }
}



void
pulseaudio_volume_toggle_muted (PulseaudioVolume *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pulseaudio_volume_set_muted (volume, !volume->muted);
}



gboolean
pulseaudio_volume_get_muted_mic (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->muted_mic;
}



/* final callback for mic volume/mute changes */
/* pa_context_success_cb_t */
static void
pulseaudio_volume_source_volume_changed (pa_context *context,
                                         int         success,
                                         void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (success)
    g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_MIC_CHANGED], 0, TRUE);
}



void
pulseaudio_volume_set_muted_mic (PulseaudioVolume *volume,
                                 gboolean          muted_mic)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (volume->pa_context != NULL);
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  if (volume->muted_mic != muted_mic)
    {
      volume->muted_mic = muted_mic;
      pa_context_set_source_mute_by_index (volume->pa_context, volume->source_index, volume->muted_mic, pulseaudio_volume_source_volume_changed, volume);
    }
}



void
pulseaudio_volume_toggle_muted_mic (PulseaudioVolume *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pulseaudio_volume_set_muted_mic (volume, !volume->muted_mic);
}



gdouble
pulseaudio_volume_get_volume (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), 0.0);

  return volume->volume;
}



/* volume setting callbacks */
/* pa_sink_info_cb_t */
static void
pulseaudio_volume_set_volume_cb2 (pa_context         *context,
                                  const pa_sink_info *i,
                                  int                 eol,
                                  void               *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pa_cvolume_set ((pa_cvolume *)&i->volume, 1, pulseaudio_volume_d2v (volume, volume->volume));
  pa_context_set_sink_volume_by_index (context, i->index, &i->volume, pulseaudio_volume_sink_volume_changed, volume);
}



/* pa_server_info_cb_t */
static void
pulseaudio_volume_set_volume_cb1 (pa_context           *context,
                                  const pa_server_info *i,
                                  void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pa_context_get_sink_info_by_name (context, i->default_sink_name, pulseaudio_volume_set_volume_cb2, volume);
}



void
pulseaudio_volume_set_volume (PulseaudioVolume *volume,
                              gdouble           vol)
{
  gdouble vol_max;
  gdouble vol_trim;

  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (volume->pa_context != NULL);
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  vol_max = pulseaudio_config_get_volume_max (volume->config) / 100.0;
  vol_trim = MIN (MAX (vol, 0.0), vol_max);

  if (volume->volume != vol_trim)
  {
    volume->volume = vol_trim;
    pa_context_get_server_info (volume->pa_context, pulseaudio_volume_set_volume_cb1, volume);
  }
}



gdouble
pulseaudio_volume_get_volume_mic (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), 0.0);

  return volume->volume_mic;
}



/* volume setting callbacks */
/* pa_source_info_cb_t */
static void
pulseaudio_volume_set_volume_mic_cb2 (pa_context           *context,
                                      const pa_source_info *i,
                                      int                   eol,
                                      void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pa_cvolume_set ((pa_cvolume *)&i->volume, 1, pulseaudio_volume_d2v (volume, volume->volume_mic));
  pa_context_set_source_volume_by_index (context, i->index, &i->volume, pulseaudio_volume_source_volume_changed, volume);
}



/* pa_server_info_cb_t */
static void
pulseaudio_volume_set_volume_mic_cb1 (pa_context           *context,
                                      const pa_server_info *i,
                                      void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pa_context_get_source_info_by_name (context, i->default_source_name, pulseaudio_volume_set_volume_mic_cb2, volume);
}



void
pulseaudio_volume_set_volume_mic (PulseaudioVolume *volume,
                                  gdouble           vol)
{
  gdouble vol_max;
  gdouble vol_trim;

  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (volume->pa_context != NULL);
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  vol_max = pulseaudio_config_get_volume_max (volume->config) / 100.0;
  vol_trim = MIN (MAX (vol, 0.0), vol_max);

  if (volume->volume_mic != vol_trim)
    {
      volume->volume_mic = vol_trim;
      pa_context_get_server_info (volume->pa_context, pulseaudio_volume_set_volume_mic_cb1, volume);
    }
}



static gint
sort_device_list (gchar *a,
                  gchar *b,
                  void  *hash_table)
{
  GHashTable *table = (GHashTable *)hash_table;
  gchar      *a_val = (gchar *) g_hash_table_lookup (table, a);
  gchar      *b_val = (gchar *) g_hash_table_lookup (table, b);
  return g_strcmp0 (a_val, b_val);
}



GList *
pulseaudio_volume_get_output_list (PulseaudioVolume *volume)
{
  GList *list;
  GList *sorted;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);

  list = g_hash_table_get_keys (volume->sinks);
  sorted = g_list_sort_with_data (list, (GCompareDataFunc) sort_device_list, volume->sinks);

  return sorted;
}



gchar *
pulseaudio_volume_get_output_by_name (PulseaudioVolume *volume,
                                      gchar            *name)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);
  return (gchar *) g_hash_table_lookup (volume->sinks, name);
}



GList *
pulseaudio_volume_get_input_list (PulseaudioVolume *volume)
{
  GList *list;
  GList *sorted;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);

  list = g_hash_table_get_keys (volume->sources);
  sorted = g_list_sort_with_data (list, (GCompareDataFunc) sort_device_list, volume->sources);

  return sorted;
}



gchar *
pulseaudio_volume_get_input_by_name (PulseaudioVolume *volume,
                                     gchar            *name)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);
  return (gchar *) g_hash_table_lookup (volume->sources, name);
}



const gchar *
pulseaudio_volume_get_default_output (PulseaudioVolume *volume)
{
  return volume->default_sink_name;
}



const gchar *
pulseaudio_volume_get_default_input (PulseaudioVolume *volume)
{
  return volume->default_source_name;
}



static void
pulseaudio_volume_default_sink_changed_info_cb (pa_context         *context,
                                                const pa_sink_info *i,
                                                int                 eol,
                                                void               *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pa_context_move_sink_input_by_index (context, volume->sink_index, i->index, NULL, NULL);
  volume->sink_index = (guint)i->index;

  pa_context_get_sink_input_info_list (volume->pa_context, pulseaudio_volume_move_sink_input, volume);
}



static void
pulseaudio_volume_default_sink_changed (pa_context *context,
                                        int         success,
                                        void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (success)
    pa_context_get_sink_info_by_name (volume->pa_context, volume->default_sink_name, pulseaudio_volume_default_sink_changed_info_cb, volume);
}



static void
pulseaudio_volume_move_sink_input (pa_context               *context,
                                   const pa_sink_input_info *i,
                                   int                       eol,
                                   void                     *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL) return;
  if (eol > 0) return;

  pa_context_move_sink_input_by_name (context, i->index, volume->default_sink_name, NULL, NULL);
}



void
pulseaudio_volume_set_default_output (PulseaudioVolume *volume,
                                      const gchar      *name)
{
  if (g_strcmp0(name, volume->default_sink_name) == 0)
    return;

  g_free (volume->default_sink_name);
  volume->default_sink_name = g_strdup (name);

  pa_context_set_default_sink (volume->pa_context, name, pulseaudio_volume_default_sink_changed, volume);
}



static void
pulseaudio_volume_default_source_changed_info_cb (pa_context         *context,
                                                  const pa_source_info *i,
                                                  int                 eol,
                                                  void               *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (i == NULL)
    return;

  pa_context_move_source_output_by_index (context, volume->source_index, i->index, NULL, NULL);
  volume->source_index = (guint)i->index;
}



static void
pulseaudio_volume_default_source_changed (pa_context *context,
                                          int         success,
                                          void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (success)
    pa_context_get_source_info_by_name (volume->pa_context, volume->default_source_name, pulseaudio_volume_default_source_changed_info_cb, volume);
}



void
pulseaudio_volume_set_default_input (PulseaudioVolume *volume,
                                     const gchar      *name)
{
  if (g_strcmp0 (name, volume->default_source_name) == 0)
    return;

  g_free (volume->default_source_name);
  volume->default_source_name = g_strdup (name);

  pa_context_set_default_source (volume->pa_context, name, pulseaudio_volume_default_source_changed, volume);
}



PulseaudioVolume *
pulseaudio_volume_new (PulseaudioConfig *config)
{
  PulseaudioVolume *volume;

  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);

  volume = g_object_new (TYPE_PULSEAUDIO_VOLUME, NULL);
  volume->config = config;

  return volume;
}
