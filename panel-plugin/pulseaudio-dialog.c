/*
 *  Copyright (C) 2015 Andrzej <andrzejr@xfce.org>
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
 *  This file implements a preferences dialog. The class extends GtkBuilder.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "pulseaudio-dialog.h"
#include "pulseaudio-dialog_ui.h"
#include "pulseaudio-mpris.h"

#include <exo/exo.h>

#define PLUGIN_WEBSITE  "https://docs.xfce.org/apps/pulseaudio-plugin/start"

#ifdef LIBXFCE4UI_CHECK_VERSION
#if LIBXFCE4UI_CHECK_VERSION (4, 9, 0)
#define HAS_ONLINE_HELP
#endif
#endif



static void              pulseaudio_dialog_build                  (PulseaudioDialog          *dialog);
static void              pulseaudio_dialog_help_button_clicked    (PulseaudioDialog          *dialog,
                                                                   GtkWidget                 *button);
static void              pulseaudio_dialog_mixer_command_changed     (PulseaudioDialog          *dialog);
static void              pulseaudio_dialog_run_mixer              (PulseaudioDialog          *dialog,
                                                                   GtkWidget                 *widget);



struct _PulseaudioDialogClass
{
  GtkBuilderClass   __parent__;
};

struct _PulseaudioDialog
{
  GtkBuilder         __parent__;

  GObject           *dialog;
  PulseaudioConfig  *config;

  GtkWidget         *treeview;
};



G_DEFINE_TYPE (PulseaudioDialog, pulseaudio_dialog, GTK_TYPE_BUILDER)



static void
pulseaudio_dialog_class_init (PulseaudioDialogClass *klass)
{
}



static void
pulseaudio_dialog_init (PulseaudioDialog *dialog)
{
  dialog->dialog = NULL;
  dialog->config = NULL;
}



static void
pulseaudio_dialog_mixer_command_changed (PulseaudioDialog *dialog)
{
  GObject *object;
  gchar   *path;
  gchar  **argvp = NULL;
  gboolean sensitive = FALSE;

  g_return_if_fail (GTK_IS_BUILDER (dialog));
  g_return_if_fail (IS_PULSEAUDIO_CONFIG (dialog->config));

  object = gtk_builder_get_object (GTK_BUILDER (dialog), "button-run-mixer");
  g_return_if_fail (GTK_IS_BUTTON (object));
  if (g_shell_parse_argv (pulseaudio_config_get_mixer_command (dialog->config), NULL, &argvp, NULL))
    {
      path = g_find_program_in_path (argvp[0]);
      if (path)
        sensitive = TRUE;
      g_free (path);
      g_strfreev (argvp);
    }
  gtk_widget_set_sensitive (GTK_WIDGET (object), sensitive);
}



static void
pulseaudio_dialog_run_mixer (PulseaudioDialog *dialog,
                             GtkWidget        *widget)
{
  GError    *error = NULL;
  GtkWidget *message_dialog;

  g_return_if_fail (IS_PULSEAUDIO_DIALOG (dialog));
  g_return_if_fail (GTK_IS_BUTTON (widget));

  if (!xfce_spawn_command_line_on_screen (gtk_widget_get_screen (widget),
                                          pulseaudio_config_get_mixer_command (dialog->config),
                                          FALSE, FALSE, &error))
    {
      message_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_CLOSE,
                                                           _("<big><b>Failed to execute command \"%s\".</b></big>\n\n%s"),
                                                           pulseaudio_config_get_mixer_command (dialog->config),
                                                           error->message);
      gtk_window_set_title (GTK_WINDOW (message_dialog), _("Error"));
      gtk_dialog_run (GTK_DIALOG (message_dialog));
      gtk_widget_destroy (message_dialog);
      g_error_free (error);
    }
}



static void
pulseaudio_dialog_player_toggled_cb (GtkCellRendererToggle *toggle, gchar *path, gpointer user_data)
{
  PulseaudioDialog *dialog = PULSEAUDIO_DIALOG (user_data);
  GtkTreeModel     *model;
  GtkTreePath      *treepath;
  GtkTreeIter       iter;
  GValue            hidden_val = {0,}, player_val = {0,};
  gboolean          hidden;
  const gchar      *player;

  model = GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->treeview)));
  treepath = gtk_tree_path_new_from_string (path);
  gtk_tree_model_get_iter (model, &iter, treepath);

  gtk_tree_model_get_value (model, &iter, 1, &player_val);
  gtk_tree_model_get_value (model, &iter, 3, &hidden_val);
  hidden = !g_value_get_boolean(&hidden_val);
  player = g_value_get_string(&player_val);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 3, hidden, -1);

  if (hidden)
    pulseaudio_config_player_blacklist_add (dialog->config, player);
  else
    pulseaudio_config_player_blacklist_remove (dialog->config, player);
}



static void
pulseaudio_dialog_clear_players_cb (GtkButton *button,
                                    gpointer  *user_data)
{
  PulseaudioDialog *dialog = PULSEAUDIO_DIALOG (user_data);
  GtkListStore     *liststore;

  pulseaudio_config_clear_known_players (dialog->config);

  liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->treeview)));
  gtk_list_store_clear (liststore);
}



static void
pulseaudio_dialog_build (PulseaudioDialog *dialog)
{
  GtkBuilder   *builder = GTK_BUILDER (dialog);
  GObject      *object;
  GtkListStore *liststore;
  GtkTreeIter   iter;
  GError       *error = NULL;
  gchar       **players;
  guint         i;

  if (xfce_titled_dialog_get_type () == 0)
    return;

  /* load the builder data into the object */
  if (gtk_builder_add_from_string (builder, pulseaudio_dialog_ui,
                                   pulseaudio_dialog_ui_length, &error))
    {
      dialog->dialog = gtk_builder_get_object (builder, "dialog");
      g_return_if_fail (XFCE_IS_TITLED_DIALOG (dialog->dialog));

      object = gtk_builder_get_object (builder, "close-button");
      g_return_if_fail (GTK_IS_BUTTON (object));
      g_signal_connect_swapped (G_OBJECT (object), "clicked",
                                G_CALLBACK (gtk_widget_destroy),
                                dialog->dialog);

      object = gtk_builder_get_object (builder, "help-button");
      g_return_if_fail (GTK_IS_BUTTON (object));
      g_signal_connect_swapped (G_OBJECT (object), "clicked",
                                G_CALLBACK (pulseaudio_dialog_help_button_clicked),
                                dialog);

      object = gtk_builder_get_object (builder, "checkbutton-keyboard-shortcuts");
      g_return_if_fail (GTK_IS_CHECK_BUTTON (object));
      g_object_bind_property (G_OBJECT (dialog->config), "enable-keyboard-shortcuts",
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object (builder, "combobox-notifications");
      g_return_if_fail (GTK_IS_COMBO_BOX (object));
#ifdef HAVE_LIBNOTIFY
      g_object_bind_property (G_OBJECT (dialog->config), "show-notifications",
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
#else
      gtk_widget_set_visible (GTK_WIDGET (object), FALSE);
#endif

      object = gtk_builder_get_object (builder, "checkbutton-play-sound");
      g_return_if_fail (GTK_IS_CHECK_BUTTON (object));
#ifdef HAVE_LIBCANBERRA
      g_object_bind_property (G_OBJECT (dialog->config), "play-sound",
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
#else
      gtk_widget_set_visible (GTK_WIDGET (object), FALSE);
#endif

      object = gtk_builder_get_object (builder, "spinbutton-volume-step");
      g_return_if_fail (GTK_IS_ENTRY (object));
      g_object_bind_property (G_OBJECT (dialog->config), "volume-step",
                              G_OBJECT (object), "value",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object (builder, "spinbutton-max-volume");
      g_return_if_fail (GTK_IS_ENTRY (object));
      g_object_bind_property (G_OBJECT (dialog->config), "volume-max",
                              G_OBJECT (object), "value",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object (builder, "entry-mixer-command");
      g_return_if_fail (GTK_IS_ENTRY (object));
      g_object_bind_property (G_OBJECT (dialog->config), "mixer-command",
                              G_OBJECT (object), "text",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object (builder, "button-run-mixer");
      g_return_if_fail (GTK_IS_BUTTON (object));
      g_signal_connect_swapped (G_OBJECT (dialog->config), "notify::mixer-command",
                                G_CALLBACK (pulseaudio_dialog_mixer_command_changed),
                                dialog);
      pulseaudio_dialog_mixer_command_changed (dialog);
      g_signal_connect_swapped (G_OBJECT (object), "clicked",
                                G_CALLBACK (pulseaudio_dialog_run_mixer), dialog);

#ifdef HAVE_MPRIS2
      object = gtk_builder_get_object (builder, "switch-mpris-support");
      g_return_if_fail (GTK_IS_SWITCH (object));
      g_object_bind_property (G_OBJECT (dialog->config), "enable-mpris",
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object(builder, "checkbutton-multimedia-keys");
      g_return_if_fail(GTK_IS_CHECK_BUTTON(object));
      g_object_bind_property(G_OBJECT(dialog->config), "enable-multimedia-keys",
                             G_OBJECT(object), "active",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      g_object_bind_property(G_OBJECT(dialog->config), "enable-mpris",
                             G_OBJECT(object), "sensitive",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object(builder, "section_mp_content_1");
      g_object_bind_property(G_OBJECT(dialog->config), "enable-mpris",
                             G_OBJECT(object), "sensitive",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object(builder, "section_mp_content_2");
      g_object_bind_property(G_OBJECT(dialog->config), "enable-mpris",
                             G_OBJECT(object), "sensitive",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      /* Populate the liststore */
      dialog->treeview = GTK_WIDGET(gtk_builder_get_object(builder, "player_tree_view"));
      liststore = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dialog->treeview)));
      players = pulseaudio_config_get_mpris_players (dialog->config);
      if (players != NULL)
        {
          const guint num_players = g_strv_length (players);
          for (i = 0; i < num_players; i++)
            {
              gchar *name = NULL;
              gchar *icon_name = NULL;
              GIcon *icon = NULL;

              if (pulseaudio_mpris_get_player_summary (players[i], &name, &icon_name))
                {
                  if (g_file_test (icon_name, G_FILE_TEST_EXISTS) && !g_file_test (icon_name, G_FILE_TEST_IS_DIR))
                    {
                      GdkPixbuf *buf = gdk_pixbuf_new_from_file (icon_name, NULL);
                      if (G_LIKELY (buf != NULL))
                        icon = G_ICON (buf);
                    }

                  if (G_LIKELY (icon == NULL))
                    {
                      if (gtk_icon_theme_has_icon (gtk_icon_theme_get_default (), icon_name))
                        icon = g_themed_icon_new (icon_name);
                      else
                        icon = g_themed_icon_new_with_default_fallbacks ("audio-player");
                    }

                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      0, icon,
                                      1, players[i],
                                      2, name,
                                      3, pulseaudio_config_player_blacklist_lookup (dialog->config, players[i]),
                                      -1);

                  g_free (name);
                  g_free (icon_name);
                  if (icon != NULL)
                    g_object_unref (icon);
                }
            }
        }
      g_strfreev (players);

      object = gtk_builder_get_object(builder, "col_hidden_renderer");
      g_signal_connect (object, "toggled", (GCallback) pulseaudio_dialog_player_toggled_cb, dialog);

      object = gtk_builder_get_object(builder, "clear_players");
      g_signal_connect (object, "clicked", (GCallback) pulseaudio_dialog_clear_players_cb, dialog);

      object = gtk_builder_get_object(builder, "checkbutton-wnck");
      g_return_if_fail(GTK_IS_CHECK_BUTTON(object));
#if defined (HAVE_WNCK) || defined (HAVE_LIBXFCE4WINDOWING)
      g_object_bind_property(G_OBJECT(dialog->config), "enable-wnck",
                             G_OBJECT(object), "active",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
#else
      gtk_widget_set_visible(GTK_WIDGET(object), FALSE);
#endif

#else
      object = gtk_builder_get_object (builder, "section_mp_content");
      gtk_widget_set_visible (GTK_WIDGET (object), FALSE);
#endif
    }
  else
    {
      g_critical ("Failed to construct the builder: %s.",
                  error->message);
      g_error_free (error);
    }
}



static void
pulseaudio_dialog_help_button_clicked (PulseaudioDialog *dialog,
                                       GtkWidget        *button)
{
  #ifndef HAS_ONLINE_HELP
  gboolean result;
  #endif

  g_return_if_fail (IS_PULSEAUDIO_DIALOG (dialog));
  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (GTK_IS_WINDOW (dialog->dialog));

  #ifdef HAS_ONLINE_HELP
  xfce_dialog_show_help (GTK_WINDOW (dialog->dialog), "pulseaudio-plugin", "start", NULL);
  #else

  result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

  if (G_UNLIKELY (result == FALSE))
    g_warning ("Unable to open the following url: %s", PLUGIN_WEBSITE);

  #endif
}



void
pulseaudio_dialog_show (PulseaudioDialog *dialog,
                        GdkScreen        *screen)
{
  g_return_if_fail (IS_PULSEAUDIO_DIALOG (dialog));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  pulseaudio_dialog_build (PULSEAUDIO_DIALOG (dialog));
  gtk_widget_show (GTK_WIDGET (dialog->dialog));

  gtk_window_set_screen (GTK_WINDOW (dialog->dialog), screen);
}



PulseaudioDialog *
pulseaudio_dialog_new (PulseaudioConfig *config)
{
  PulseaudioDialog *dialog;

  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);

  dialog = g_object_new (TYPE_PULSEAUDIO_DIALOG, NULL);
  dialog->config = config;

  return dialog;
}
