/*
 *  Copyright (C) 2014-2017 Andrzej <andrzejr@xfce.org>
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
 *  This file implements a configuration store. The class extends GObject.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include "pulseaudio-plugin.h"
#include "pulseaudio-config.h"



#define DEFAULT_ENABLE_KEYBOARD_SHORTCUTS         TRUE
#define DEFAULT_SHOW_NOTIFICATIONS                1
#define DEFAULT_PLAY_SOUND                        FALSE
#define DEFAULT_VOLUME_STEP                       5
#define DEFAULT_VOLUME_MAX                        150

#ifdef HAVE_MPRIS2
#define DEFAULT_ENABLE_MPRIS                      TRUE
#define DEFAULT_ENABLE_MULTIMEDIA_KEYS            TRUE
#else
#define DEFAULT_ENABLE_MPRIS                      FALSE
#define DEFAULT_ENABLE_MULTIMEDIA_KEYS            FALSE
#endif
#define DEFAULT_BLACKLISTED_PLAYERS               ""
#define DEFAULT_PERSISTENT_PLAYERS                ""

#define DEFAULT_MPRIS_PLAYERS                     ""
#define DEFAULT_ENABLE_WNCK                       FALSE



static void                 pulseaudio_config_finalize       (GObject          *object);
static void                 pulseaudio_config_get_property   (GObject          *object,
                                                              guint             prop_id,
                                                              GValue           *value,
                                                              GParamSpec       *pspec);
static void                 pulseaudio_config_set_property   (GObject          *object,
                                                              guint             prop_id,
                                                              const GValue     *value,
                                                              GParamSpec       *pspec);



struct _PulseaudioConfigClass
{
  GObjectClass     __parent__;
};

struct _PulseaudioConfig
{
  GObject          __parent__;

  gboolean         enable_keyboard_shortcuts;
  gboolean         enable_multimedia_keys;
  guint            show_notifications;
#ifdef HAVE_LIBCANBERRA
  gboolean         play_sound;
#endif
  guint            volume_step;
  guint            volume_max;
  gchar           *mixer_command;
  gboolean         enable_mpris;
  gchar           *mpris_players;
  gchar           *blacklisted_players;
  gchar           *persistent_players;
  gboolean         enable_wnck;
};



enum
  {
    PROP_0,
    PROP_ENABLE_KEYBOARD_SHORTCUTS,
    PROP_ENABLE_MULTIMEDIA_KEYS,
    PROP_SHOW_NOTIFICATIONS,
#ifdef HAVE_LIBCANBERRA
    PROP_PLAY_SOUND,
#endif
    PROP_VOLUME_STEP,
    PROP_VOLUME_MAX,
    PROP_MIXER_COMMAND,
    PROP_ENABLE_MPRIS,
    PROP_MPRIS_PLAYERS,
    PROP_BLACKLISTED_PLAYERS,
    PROP_PERSISTENT_PLAYERS,
    PROP_ENABLE_WNCK,
    N_PROPERTIES,
  };

enum
  {
    CONFIGURATION_CHANGED,
    LAST_SIGNAL
  };

static guint pulseaudio_config_signals [LAST_SIGNAL] = { 0, };



G_DEFINE_TYPE (PulseaudioConfig, pulseaudio_config, G_TYPE_OBJECT)



static void
pulseaudio_config_class_init (PulseaudioConfigClass *klass)
{
  GObjectClass                 *gobject_class;

  gobject_class               = G_OBJECT_CLASS (klass);
  gobject_class->finalize     = pulseaudio_config_finalize;
  gobject_class->get_property = pulseaudio_config_get_property;
  gobject_class->set_property = pulseaudio_config_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_KEYBOARD_SHORTCUTS,
                                   g_param_spec_boolean ("enable-keyboard-shortcuts", NULL, NULL,
                                                         DEFAULT_ENABLE_KEYBOARD_SHORTCUTS,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_MULTIMEDIA_KEYS,
                                   g_param_spec_boolean ("enable-multimedia-keys", NULL, NULL,
                                                         DEFAULT_ENABLE_MULTIMEDIA_KEYS,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_NOTIFICATIONS,
                                   g_param_spec_uint ("show-notifications", NULL, NULL,
                                                      VOLUME_NOTIFICATIONS_NONE,
                                                      VOLUME_NOTIFICATIONS_COUNT - 1,
                                                      DEFAULT_SHOW_NOTIFICATIONS,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));


#ifdef HAVE_LIBCANBERRA
  g_object_class_install_property (gobject_class,
                                   PROP_PLAY_SOUND,
                                   g_param_spec_boolean ("play-sound", NULL, NULL,
                                                         DEFAULT_PLAY_SOUND,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));
#endif


  g_object_class_install_property (gobject_class,
                                   PROP_VOLUME_STEP,
                                   g_param_spec_uint ("volume-step", NULL, NULL,
                                                      1, 50, DEFAULT_VOLUME_STEP,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_VOLUME_MAX,
                                   g_param_spec_uint ("volume-max", NULL, NULL,
                                                      1, 300, DEFAULT_VOLUME_MAX,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_MIXER_COMMAND,
                                   g_param_spec_string ("mixer-command",
                                                        NULL, NULL,
                                                        DEFAULT_MIXER_COMMAND,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_MPRIS,
                                   g_param_spec_boolean ("enable-mpris", NULL, NULL,
                                                         DEFAULT_ENABLE_MPRIS,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_MPRIS_PLAYERS,
                                   g_param_spec_string ("mpris-players",
                                                        NULL, NULL,
                                                        DEFAULT_MPRIS_PLAYERS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_BLACKLISTED_PLAYERS,
                                   g_param_spec_string ("blacklisted-players",
                                                        NULL, NULL,
                                                        DEFAULT_BLACKLISTED_PLAYERS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_PERSISTENT_PLAYERS,
                                   g_param_spec_string ("persistent-players",
                                                        NULL, NULL,
                                                        DEFAULT_PERSISTENT_PLAYERS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_WNCK,
                                   g_param_spec_boolean ("enable-wnck", NULL, NULL,
                                                         DEFAULT_ENABLE_WNCK,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));



  pulseaudio_config_signals[CONFIGURATION_CHANGED] =
    g_signal_new (g_intern_static_string ("configuration-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
pulseaudio_config_init (PulseaudioConfig *config)
{
  config->enable_keyboard_shortcuts = DEFAULT_ENABLE_KEYBOARD_SHORTCUTS;
  config->enable_multimedia_keys    = DEFAULT_ENABLE_MULTIMEDIA_KEYS;
  config->show_notifications        = DEFAULT_SHOW_NOTIFICATIONS;
#ifdef HAVE_LIBCANBERRA
  config->play_sound                = DEFAULT_PLAY_SOUND;
#endif
  config->volume_step               = DEFAULT_VOLUME_STEP;
  config->volume_max                = DEFAULT_VOLUME_MAX;
  config->mixer_command             = g_strdup (DEFAULT_MIXER_COMMAND);
  config->enable_mpris              = DEFAULT_ENABLE_MPRIS;
  config->mpris_players             = g_strdup (DEFAULT_MPRIS_PLAYERS);
  config->blacklisted_players       = g_strdup (DEFAULT_BLACKLISTED_PLAYERS);
  config->persistent_players        = g_strdup (DEFAULT_PERSISTENT_PLAYERS);
  config->enable_wnck               = DEFAULT_ENABLE_WNCK;
}



static void
pulseaudio_config_finalize (GObject *object)
{
  PulseaudioConfig *config = PULSEAUDIO_CONFIG (object);

  xfconf_shutdown();
  g_free (config->mixer_command);

  G_OBJECT_CLASS (pulseaudio_config_parent_class)->finalize (object);
}



static void
pulseaudio_config_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PulseaudioConfig     *config = PULSEAUDIO_CONFIG (object);

  switch (prop_id)
    {
    case PROP_ENABLE_KEYBOARD_SHORTCUTS:
      g_value_set_boolean (value, config->enable_keyboard_shortcuts);
      break;

    case PROP_ENABLE_MULTIMEDIA_KEYS:
      g_value_set_boolean (value, config->enable_multimedia_keys);
      break;

    case PROP_SHOW_NOTIFICATIONS:
      g_value_set_uint (value, config->show_notifications);
      break;

#ifdef HAVE_LIBCANBERRA
    case PROP_PLAY_SOUND:
      g_value_set_boolean (value, config->play_sound);
      break;
#endif

    case PROP_VOLUME_STEP:
      g_value_set_uint (value, config->volume_step);
      break;

    case PROP_VOLUME_MAX:
      g_value_set_uint (value, config->volume_max);
      break;

    case PROP_MIXER_COMMAND:
      g_value_set_string (value, config->mixer_command);
      break;

    case PROP_ENABLE_MPRIS:
      g_value_set_boolean (value, config->enable_mpris);
      break;

    case PROP_MPRIS_PLAYERS:
      g_value_set_string (value, config->mpris_players);
      break;

    case PROP_BLACKLISTED_PLAYERS:
      g_value_set_string (value, config->blacklisted_players);
      break;

    case PROP_PERSISTENT_PLAYERS:
      g_value_set_string (value, config->persistent_players);
      break;

    case PROP_ENABLE_WNCK:
      g_value_set_boolean (value, config->enable_wnck);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
pulseaudio_config_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PulseaudioConfig     *config = PULSEAUDIO_CONFIG (object);
  guint                 val_uint;
  gboolean              val_bool;

  switch (prop_id)
    {
    case PROP_ENABLE_KEYBOARD_SHORTCUTS:
      val_bool = g_value_get_boolean (value);
      if (config->enable_keyboard_shortcuts != val_bool)
        {
          config->enable_keyboard_shortcuts = val_bool;
          g_object_notify (G_OBJECT (config), "enable-keyboard-shortcuts");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_ENABLE_MULTIMEDIA_KEYS:
      val_bool = g_value_get_boolean (value);
      if (config->enable_multimedia_keys != val_bool)
        {
          config->enable_multimedia_keys = val_bool;
          g_object_notify (G_OBJECT (config), "enable-multimedia-keys");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_SHOW_NOTIFICATIONS:
      val_uint = g_value_get_uint (value);
      if (config->show_notifications != val_uint)
        {
          config->show_notifications = val_uint;
          g_object_notify (G_OBJECT (config), "show-notifications");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

#ifdef HAVE_LIBCANBERRA
    case PROP_PLAY_SOUND:
      val_bool = g_value_get_boolean (value);
      if (config->play_sound != val_bool)
        {
          config->play_sound = val_bool;
          g_object_notify (G_OBJECT (config), "play-sound");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;
#endif

    case PROP_VOLUME_STEP:
      val_uint = g_value_get_uint (value);
      if (config->volume_step != val_uint)
        {
          config->volume_step = val_uint;
          g_object_notify (G_OBJECT (config), "volume-step");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_VOLUME_MAX:
      val_uint = g_value_get_uint (value);
      if (config->volume_max != val_uint)
        {
          config->volume_max = val_uint;
          g_object_notify (G_OBJECT (config), "volume-max");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_MIXER_COMMAND:
      g_free (config->mixer_command);
      config->mixer_command = g_value_dup_string (value);
      break;

    case PROP_ENABLE_MPRIS:
      val_bool = g_value_get_boolean (value);
      if (config->enable_mpris != val_bool)
        {
          config->enable_mpris = val_bool;
          g_object_notify (G_OBJECT (config), "enable-mpris");

          if (!config->enable_mpris)
            {
              config->enable_multimedia_keys = FALSE;
              g_object_notify(G_OBJECT(config), "enable-multimedia-keys");

              config->enable_wnck = FALSE;
              g_object_notify(G_OBJECT(config), "enable-wnck");
            }

          g_signal_emit(G_OBJECT(config), pulseaudio_config_signals[CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_MPRIS_PLAYERS:
      g_free (config->mpris_players);
      config->mpris_players = g_value_dup_string (value);
      g_object_notify (G_OBJECT (config), "mpris-players");
      g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
      break;

    case PROP_BLACKLISTED_PLAYERS:
      g_free (config->blacklisted_players);
      config->blacklisted_players = g_value_dup_string (value);
      g_object_notify (G_OBJECT (config), "blacklisted-players");
      g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
      break;

    case PROP_PERSISTENT_PLAYERS:
      g_free (config->persistent_players);
      config->persistent_players = g_value_dup_string (value);
      g_object_notify (G_OBJECT (config), "persistent-players");
      g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
      break;

    case PROP_ENABLE_WNCK:
      val_bool = g_value_get_boolean(value);
      if (config->enable_wnck != val_bool)
      {
        config->enable_wnck = val_bool;
        g_object_notify (G_OBJECT (config), "enable-wnck");
        g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



gboolean
pulseaudio_config_get_enable_keyboard_shortcuts (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_ENABLE_KEYBOARD_SHORTCUTS);

  return config->enable_keyboard_shortcuts;
}



gboolean
pulseaudio_config_get_enable_multimedia_keys (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_ENABLE_MULTIMEDIA_KEYS);

  return config->enable_multimedia_keys;
}



guint
pulseaudio_config_get_show_notifications (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_SHOW_NOTIFICATIONS);

  return config->show_notifications;
}



#ifdef HAVE_LIBCANBERRA
gboolean
pulseaudio_config_get_play_sound (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_PLAY_SOUND);

  return config->play_sound;
}
#endif



guint
pulseaudio_config_get_volume_step (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_VOLUME_STEP);

  return config->volume_step;
}



guint
pulseaudio_config_get_volume_max (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_VOLUME_MAX);

  return config->volume_max;
}



const gchar *
pulseaudio_config_get_mixer_command (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_MIXER_COMMAND);

  return config->mixer_command;
}



gboolean
pulseaudio_config_get_enable_mpris (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_ENABLE_MPRIS);

  return config->enable_mpris;
}



gchar **
pulseaudio_config_get_mpris_players (PulseaudioConfig *config)
{
  if (!IS_PULSEAUDIO_CONFIG (config))
    {
      return g_strsplit (DEFAULT_MPRIS_PLAYERS, ";", 1);
    }

  return g_strsplit (config->mpris_players, ";", 0);
}



static gint
compare_players (gconstpointer item1, gconstpointer item2)
{
  return g_ascii_strcasecmp (item1, item2);
}



void
pulseaudio_config_set_mpris_players (PulseaudioConfig  *config,
                                     gchar            **players)
{
  GSList *player_array;
  gchar  *player_string;
  GValue  src = { 0, };
  guint   index = 0;
  guint   i = 0;
  GSList *list = NULL;
  guint   num_players;

  g_return_if_fail (IS_PULSEAUDIO_CONFIG (config));

  num_players = g_strv_length (players);

  player_array = NULL;
  for (i = 0; i < num_players; i++)
    {
      player_array = g_slist_prepend (player_array, players[i]);
    }

  player_array = g_slist_sort (player_array, (GCompareFunc) compare_players);

  for (list = player_array; list != NULL; list = g_slist_next (list))
    {
      players[index] = list->data;
      index++;
    }

  g_slist_free (player_array);

  player_string = g_strjoinv (";", players);

  g_value_init(&src, G_TYPE_STRING);
  g_value_set_static_string(&src, player_string);

  pulseaudio_config_set_property (G_OBJECT (config), PROP_MPRIS_PLAYERS, &src, NULL);

  g_free (player_string);
}



void
pulseaudio_config_add_mpris_player (PulseaudioConfig *config,
                                    const gchar      *player)
{
  gchar **players;
  gchar **player_list;
  gchar  *players_string;
  gchar  *player_string;

  players = pulseaudio_config_get_mpris_players (config);
  if (g_strv_contains ((const char * const *) players, player))
    {
      g_strfreev(players);
      return;
    }

  players_string = g_strjoinv (";", players);
  if (g_strv_length (players) > 0)
    player_string = g_strjoin (";", players_string, player, NULL);
  else
    player_string = g_strdup (player);
  player_list = g_strsplit(player_string, ";", 0);

  pulseaudio_config_set_mpris_players (config, player_list);

  g_strfreev (player_list);
  g_free (player_string);
  g_free (players_string);
  g_strfreev (players);
}



static gchar **
pulseaudio_config_get_blacklisted_players (PulseaudioConfig *config)
{
  if (!IS_PULSEAUDIO_CONFIG (config))
    {
      return g_strsplit (DEFAULT_BLACKLISTED_PLAYERS, ";", 1);
    }

  return g_strsplit (config->blacklisted_players, ";", 0);
}



static gchar **
pulseaudio_config_get_persistent_players (PulseaudioConfig *config)
{
  if (!IS_PULSEAUDIO_CONFIG (config))
    {
      return g_strsplit (DEFAULT_PERSISTENT_PLAYERS, ";", 1);
    }

  return g_strsplit (config->persistent_players, ";", 0);
}



static void
pulseaudio_config_set_players (PulseaudioConfig  *config,
                               gchar            **players,
                               gint               prop)
{
  GSList *player_array;
  gchar  *player_string;
  GValue  src = { 0, };
  guint   index = 0;
  guint   i = 0;
  GSList *list = NULL;
  guint   num_players;

  g_return_if_fail (IS_PULSEAUDIO_CONFIG (config));

  num_players = g_strv_length (players);

  player_array = NULL;
  for (i = 0; i < num_players; i++)
    {
      player_array = g_slist_prepend (player_array, players[i]);
    }

  player_array = g_slist_sort (player_array, (GCompareFunc) compare_players);

  for (list = player_array; list != NULL; list = g_slist_next (list))
    {
      players[index] = list->data;
      index++;
    }

  g_slist_free (player_array);

  player_string = g_strjoinv (";", players);

  g_value_init(&src, G_TYPE_STRING);
  g_value_set_static_string(&src, player_string);

  pulseaudio_config_set_property (G_OBJECT (config), prop, &src, NULL);

  g_free (player_string);
}



static void
pulseaudio_config_player_add (PulseaudioConfig *config,
                              gchar           **players,
                              const gchar      *player,
                              gint              prop)
{
  gchar **player_list;
  gchar  *players_string;
  gchar  *player_string;

  if (g_strv_contains ((const char * const *) players, player))
    {
      g_strfreev(players);
      return;
    }

  players_string = g_strjoinv (";", players);
  if (g_strv_length (players) > 0)
    player_string = g_strjoin (";", players_string, player, NULL);
  else
    player_string = g_strdup (player);

  player_list = g_strsplit(player_string, ";", 0);

  pulseaudio_config_set_players (config, player_list, prop);

  g_strfreev (player_list);
  g_free (player_string);
  g_free (players_string);
  g_strfreev (players);
}



static void
pulseaudio_config_player_remove (PulseaudioConfig *config,
                                 gchar           **players_old,
                                 const gchar      *player,
                                 gint              prop)
{
  gchar   **players_new;
  guint     j = 0;
  guint     len = g_strv_length (players_old);

  players_new = g_new (gchar *, len);

  for (guint i = 0; i < len; i++)
    if (g_strcmp0 (player, players_old[i]) != 0)
      players_new[j++] = players_old[i];

  if (j < len)
    {
      players_new[j] = NULL;
      pulseaudio_config_set_players (config, players_new, prop);
    }

  g_free (players_new);
  g_strfreev (players_old);
}



void
pulseaudio_config_player_blacklist_add (PulseaudioConfig *config,
                                        const gchar      *player)
{
  pulseaudio_config_player_add (config,
                                pulseaudio_config_get_blacklisted_players (config),
                                player,
                                PROP_BLACKLISTED_PLAYERS);
}



void
pulseaudio_config_player_blacklist_remove (PulseaudioConfig *config,
                                           const gchar      *player)
{
  pulseaudio_config_player_remove (config,
                                   pulseaudio_config_get_blacklisted_players (config),
                                   player,
                                   PROP_BLACKLISTED_PLAYERS);
}



gboolean
pulseaudio_config_player_blacklist_lookup (PulseaudioConfig *config,
                                           const gchar      *player)
{
  gchar    **players;
  gboolean   found = FALSE;
  players = pulseaudio_config_get_blacklisted_players (config);
  if (g_strv_contains ((const char * const *) players, player))
    {
      found = TRUE;
    }

  g_strfreev(players);
  return found;
}



void
pulseaudio_config_player_persistent_add (PulseaudioConfig *config,
                                         const gchar      *player)
{
  pulseaudio_config_player_add (config,
                                pulseaudio_config_get_persistent_players (config),
                                player,
                                PROP_PERSISTENT_PLAYERS);
}



void
pulseaudio_config_player_persistent_remove (PulseaudioConfig *config,
                                            const gchar      *player)
{
  pulseaudio_config_player_remove (config,
                                   pulseaudio_config_get_persistent_players (config),
                                   player,
                                   PROP_PERSISTENT_PLAYERS);
}



gboolean
pulseaudio_config_player_persistent_lookup (PulseaudioConfig *config,
                                            const gchar      *player)
{
  gchar    **players;
  gboolean   found = FALSE;
  players = pulseaudio_config_get_persistent_players (config);
  if (g_strv_contains ((const char * const *) players, player))
    {
      found = TRUE;
    }

  g_strfreev(players);
  return found;
}



void
pulseaudio_config_clear_known_players (PulseaudioConfig *config)
{
  gchar  *player_string;
  GValue  src = { 0, };

  g_return_if_fail (IS_PULSEAUDIO_CONFIG (config));

  player_string = g_strdup ("");

  g_value_init(&src, G_TYPE_STRING);
  g_value_set_static_string(&src, player_string);

  pulseaudio_config_set_property (G_OBJECT (config), PROP_BLACKLISTED_PLAYERS, &src, NULL);
  pulseaudio_config_set_property (G_OBJECT (config), PROP_PERSISTENT_PLAYERS, &src, NULL);
  pulseaudio_config_set_property (G_OBJECT (config), PROP_MPRIS_PLAYERS, &src, NULL);

  g_free (player_string);
}



void
pulseaudio_config_set_can_raise_wnck (PulseaudioConfig *config,
                                      gboolean          can_raise)
{
  GValue src = { 0, };

  g_return_if_fail(IS_PULSEAUDIO_CONFIG(config));

  g_value_init (&src, G_TYPE_BOOLEAN);
  g_value_set_boolean (&src, can_raise);

  pulseaudio_config_set_property(G_OBJECT(config), PROP_ENABLE_WNCK, &src, NULL);
}



gboolean
pulseaudio_config_get_can_raise_wnck (PulseaudioConfig *config)
{
  return config->enable_wnck;
}



PulseaudioConfig *
pulseaudio_config_new (const gchar     *property_base)
{
  PulseaudioConfig    *config;
  XfconfChannel       *channel;
  gchar               *property;

  config = g_object_new (TYPE_PULSEAUDIO_CONFIG, NULL);

  if (xfconf_init (NULL))
    {
      channel = xfconf_channel_get ("xfce4-panel");

      property = g_strconcat (property_base, "/enable-keyboard-shortcuts", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, config, "enable-keyboard-shortcuts");
      g_free (property);

      property = g_strconcat (property_base, "/enable-multimedia-keys", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, config, "enable-multimedia-keys");
      g_free (property);

      property = g_strconcat (property_base, "/show-notifications", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_UINT, config, "show-notifications");
      g_free (property);

#ifdef HAVE_LIBCANBERRA
      property = g_strconcat (property_base, "/play-sound", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, config, "play-sound");
      g_free (property);
#endif

      property = g_strconcat (property_base, "/volume-step", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_UINT, config, "volume-step");
      g_free (property);

      property = g_strconcat (property_base, "/volume-max", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_UINT, config, "volume-max");
      g_free (property);

      property = g_strconcat (property_base, "/mixer-command", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_STRING, config, "mixer-command");
      g_free (property);

      property = g_strconcat (property_base, "/enable-mpris", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, config, "enable-mpris");
      g_free (property);

      property = g_strconcat (property_base, "/mpris-players", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_STRING, config, "mpris-players");
      g_free (property);

      property = g_strconcat (property_base, "/blacklisted-players", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_STRING, config, "blacklisted-players");
      g_free (property);

      property = g_strconcat (property_base, "/persistent-players", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_STRING, config, "persistent-players");
      g_free (property);

      property = g_strconcat (property_base, "/enable-wnck", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, config, "enable-wnck");
      g_free (property);

      g_object_notify (G_OBJECT (config), "enable-keyboard-shortcuts");
      g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
    }

  return config;
}
