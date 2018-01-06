/*
 *  Copyright (C) 2015 Andrzej <ndrwrdck@gmail.com>
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

  g_return_if_fail (GTK_IS_BUILDER (dialog));
  g_return_if_fail (IS_PULSEAUDIO_CONFIG (dialog->config));

  object = gtk_builder_get_object (GTK_BUILDER (dialog), "button-run-mixer");
  g_return_if_fail (GTK_IS_BUTTON (object));
  path = g_find_program_in_path (pulseaudio_config_get_mixer_command (dialog->config));
  gtk_widget_set_sensitive (GTK_WIDGET (object), path != NULL);
  g_free (path);
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
pulseaudio_dialog_build (PulseaudioDialog *dialog)
{
  GtkBuilder  *builder = GTK_BUILDER (dialog);
  GObject     *object;
  GError      *error = NULL;

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

      object = gtk_builder_get_object (builder, "checkbutton-allow-louder-than-hundred");
      g_return_if_fail (GTK_IS_CHECK_BUTTON (object));
      g_object_bind_property (G_OBJECT (dialog->config), "allow-louder-than-hundred",
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      object = gtk_builder_get_object (builder, "checkbutton-show-notifications");
      g_return_if_fail (GTK_IS_CHECK_BUTTON (object));
#ifdef HAVE_LIBNOTIFY
      g_object_bind_property (G_OBJECT (dialog->config), "show-notifications",
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
#else
      gtk_widget_set_visible (GTK_WIDGET (object), FALSE);
#endif

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
      object = gtk_builder_get_object (builder, "checkbutton-mpris-support");
      g_return_if_fail (GTK_IS_CHECK_BUTTON (object));
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

      object = gtk_builder_get_object(builder, "checkbutton-wnck");
      g_return_if_fail(GTK_IS_CHECK_BUTTON(object));
#ifdef HAVE_WNCK
      g_object_bind_property(G_OBJECT(dialog->config), "enable-wnck",
                             G_OBJECT(object), "active",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

      g_object_bind_property(G_OBJECT(dialog->config), "enable-mpris",
                             G_OBJECT(object), "sensitive",
                             G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
#else
      gtk_widget_set_visible(GTK_WIDGET(object), FALSE);
#endif

#else
      object = gtk_builder_get_object (builder, "media-player-frame");
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
