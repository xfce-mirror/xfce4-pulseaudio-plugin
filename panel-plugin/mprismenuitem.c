/* -*- c-basic-offset: 2 -*- vi:set ts=2 sts=2 sw=2:
 * * 2017 Sean Davis <bluesabre@xfce.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * Based on the scale menu item implementation of the indicator applet:
 * Authors:
 *    Cody Russell <crussell@canonical.com>
 * http://bazaar.launchpad.net/~indicator-applet-developers/ido/trunk.14.10/view/head:/src/idoscalemenuitem.h
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mprismenuitem.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gdesktopappinfo.h>

/* for DBG/TRACE */
#include <libxfce4util/libxfce4util.h>


static gboolean mpris_menu_item_button_press_event      (GtkWidget          *menuitem,
                                                         GdkEventButton     *event);
static gboolean mpris_menu_item_button_release_event    (GtkWidget          *menuitem,
                                                         GdkEventButton     *event);
static void     update_packing                          (MprisMenuItem      *self);



struct _MprisMenuItemPrivate {
  GtkWidget *title_label;
  GtkWidget *artist_label;

  GtkWidget *go_previous;
  GtkWidget *play_pause;
  GtkWidget *go_next;

  gboolean   can_go_previous;
  gboolean   can_play;
  gboolean   can_pause;
  gboolean   can_go_next;
  gboolean   can_raise;

  gboolean   is_running;
  gboolean   is_playing;
  gboolean   is_stopped;

  gchar     *player;
  gchar     *title;
  gchar     *filename;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button_box;
};

enum {
  MEDIA_NOTIFY,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE (MprisMenuItem, mpris_menu_item, GTK_TYPE_IMAGE_MENU_ITEM)
G_GNUC_END_IGNORE_DEPRECATIONS

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_MPRIS_MENU_ITEM, MprisMenuItemPrivate))



static void
mpris_menu_item_class_init (MprisMenuItemClass *item_class)
{
  GtkWidgetClass    *widget_class =    GTK_WIDGET_CLASS    (item_class);

  widget_class->button_press_event   = mpris_menu_item_button_press_event;
  widget_class->button_release_event = mpris_menu_item_button_release_event;

  /**
   * MprisMenuItem::media-notify:
   * @menuitem: the #MprisMenuItem for which the value changed
   * @value: the mpris signal to emit
   *
   * Emitted whenever the a media button is clicked.
   */
  signals[MEDIA_NOTIFY] = g_signal_new ("media-notify",
                                        TYPE_MPRIS_MENU_ITEM,
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL,
                                        g_cclosure_marshal_VOID__STRING,
                                        G_TYPE_NONE,
                                        1, G_TYPE_STRING);

  g_type_class_add_private (item_class, sizeof (MprisMenuItemPrivate));
}

static void
remove_children (GtkContainer *container)
{
  GList * children;
  GList * l;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  children = gtk_container_get_children (container);

  for (l=children; l!=NULL; l=l->next)
    gtk_container_remove (container, l->data);
  g_list_free (children);
}

static void
gtk_label_set_markup_printf_escaped (GtkLabel    *label,
                                     const gchar *format,
                                     ...)
{
  va_list args;
  gchar *str;

  va_start (args, format);
  str = g_markup_vprintf_escaped (format, args);
  gtk_label_set_markup (label, str);
  va_end (args);

  g_free (str);
}

static void
media_notify (MprisMenuItem *item, gchar *message)
{
  g_signal_emit (item, signals[MEDIA_NOTIFY], 0, message);
}

static void
media_play_pause_clicked_event (GtkButton *button, gpointer user_data)
{
  MprisMenuItem *item = user_data;
  media_notify (item, "PlayPause");
}

static void
media_go_previous_clicked_event (GtkButton *button, gpointer user_data)
{
  MprisMenuItem *item = user_data;
  media_notify (item, "Previous");
}

static void
media_go_next_clicked_event (GtkButton *button, gpointer user_data)
{
  MprisMenuItem *item = user_data;
  media_notify (item, "Next");
}

static void
mpris_menu_item_raise (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  if (priv->is_running)
    {
      if (priv->can_raise)
        {
          media_notify (item, "Raise");
        }
    }
}

static void
mpris_menu_item_launch (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;
  GAppInfo             *app_info;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  if (priv->is_running)
    return;

  app_info = (GAppInfo*)g_desktop_app_info_new_from_filename (priv->filename);
  if (app_info != NULL)
    {
      g_app_info_launch (app_info, NULL, NULL, NULL);
      g_object_unref (app_info);
    }
}

static void
mpris_menu_item_raise_or_launch (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  if (priv->is_running)
    mpris_menu_item_raise (item);
  else
    mpris_menu_item_launch (item);
}

static void
menu_item_activate_event (GtkMenuItem *mi, gpointer user_data)
{
  mpris_menu_item_raise_or_launch (MPRIS_MENU_ITEM (mi));
}

static GtkWidget *
track_info_label_new (void)
{
  GtkWidget *label;

  label = gtk_label_new (NULL);
  gtk_label_set_width_chars (GTK_LABEL (label), 25);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 25);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);

  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_widget_set_halign (label, GTK_ALIGN_START);

  return label;
}

static void
update_packing (MprisMenuItem *self)
{
  MprisMenuItemPrivate *priv;
  GtkBox               *hbox;
  GtkBox               *vbox;
  GtkBox               *button_box;
  GtkStyleContext      *ctx;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (self));

  priv = GET_PRIVATE (self);
  hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

  button_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  ctx = gtk_widget_get_style_context (GTK_WIDGET (button_box));
  gtk_style_context_add_class (ctx, "linked");

  TRACE("entering");

  if (priv->vbox)
    remove_children (GTK_CONTAINER (priv->vbox));
  if (priv->hbox)
  {
    remove_children (GTK_CONTAINER (priv->hbox));
    gtk_container_remove (GTK_CONTAINER (self), priv->hbox);
  }

  priv->hbox = GTK_WIDGET(hbox);
  priv->vbox = GTK_WIDGET(vbox);
  priv->button_box = GTK_WIDGET(button_box);

  /* add the new layout */
  /* [IC]  Title   [  CON  ]
   * [ON]  Artist  [ TROLS ]
   */
  priv->go_previous = gtk_button_new_from_icon_name ("media-skip-backward-symbolic", GTK_ICON_SIZE_MENU);
  priv->play_pause = gtk_button_new_from_icon_name ("media-playback-start-symbolic", GTK_ICON_SIZE_MENU);
  priv->go_next = gtk_button_new_from_icon_name ("media-skip-forward-symbolic", GTK_ICON_SIZE_MENU);

  g_signal_connect (priv->play_pause, "clicked", G_CALLBACK (media_play_pause_clicked_event), self);
  g_signal_connect (priv->go_previous, "clicked", G_CALLBACK (media_go_previous_clicked_event), self);
  g_signal_connect (priv->go_next, "clicked", G_CALLBACK (media_go_next_clicked_event), self);
  g_signal_connect (self, "activate", G_CALLBACK (menu_item_activate_event), self);

  priv->title_label = track_info_label_new ();
  priv->artist_label = track_info_label_new ();

  gtk_box_pack_start (button_box, priv->go_previous, FALSE, FALSE, 0);
  gtk_box_pack_start (button_box, priv->play_pause, FALSE, FALSE, 0);
  gtk_box_pack_start (button_box, priv->go_next, FALSE, FALSE, 0);

  gtk_box_pack_start (vbox, priv->title_label, FALSE, FALSE, 0);
  gtk_box_pack_start (vbox, priv->artist_label, FALSE, FALSE, 0);

  gtk_box_pack_start (hbox, GTK_WIDGET (vbox), TRUE, TRUE, 0);
  gtk_box_pack_start (hbox, GTK_WIDGET (button_box), FALSE, FALSE, 0);

  mpris_menu_item_set_title (self, priv->player);
  mpris_menu_item_set_artist (self, _("Not currently playing"));

  gtk_widget_show_all (priv->button_box);
  gtk_widget_show_all (priv->hbox);
  gtk_widget_show_all (priv->vbox);

  gtk_container_add (GTK_CONTAINER (self), priv->hbox);
}

static void
mpris_menu_item_init (MprisMenuItem *self)
{
}

static GtkWidget *
mpris_menu_item_get_widget_at_event (MprisMenuItem  *menuitem,
                                     GdkEventButton *event)
{
  MprisMenuItemPrivate *priv;
  GtkAllocation         alloc;
  gint                  x, y;

  g_return_val_if_fail (IS_MPRIS_MENU_ITEM (menuitem), NULL);

  priv = GET_PRIVATE (menuitem);

  gtk_widget_get_allocation (priv->button_box, &alloc);
  gtk_widget_translate_coordinates (GTK_WIDGET (menuitem), priv->button_box, event->x, event->y, &x, &y);

  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
      gtk_widget_get_allocation (priv->go_previous, &alloc);
      gtk_widget_translate_coordinates (GTK_WIDGET (menuitem), priv->go_previous, event->x, event->y, &x, &y);

      if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
        return GTK_WIDGET (priv->go_previous);

      gtk_widget_get_allocation (priv->play_pause, &alloc);
      gtk_widget_translate_coordinates (GTK_WIDGET (menuitem), priv->play_pause, event->x, event->y, &x, &y);

      if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
        return GTK_WIDGET (priv->play_pause);

      gtk_widget_get_allocation (priv->go_next, &alloc);
      gtk_widget_translate_coordinates (GTK_WIDGET (menuitem), priv->go_next, event->x, event->y, &x, &y);

      if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
        return GTK_WIDGET (priv->go_next);
    }

  return GTK_WIDGET (menuitem);
}

static gboolean
mpris_menu_item_button_press_event (GtkWidget      *menuitem,
                                    GdkEventButton *event)
{
  GtkWidget            *widget;

  g_return_val_if_fail (IS_MPRIS_MENU_ITEM (menuitem), FALSE);

  widget = mpris_menu_item_get_widget_at_event (MPRIS_MENU_ITEM (menuitem), event);

  if (widget == NULL)
    return FALSE;

  if (widget == menuitem)
    return FALSE;

  gtk_widget_event (widget, (GdkEvent*) event);
  return TRUE;
}

static gboolean
mpris_menu_item_button_release_event (GtkWidget      *menuitem,
                                      GdkEventButton *event)
{
  GtkWidget            *widget;

  g_return_val_if_fail (IS_MPRIS_MENU_ITEM (menuitem), FALSE);

  widget = mpris_menu_item_get_widget_at_event (MPRIS_MENU_ITEM (menuitem), event);

  if (widget == NULL)
    return FALSE;

  if (widget == menuitem)
    return FALSE;

  gtk_widget_event (widget, (GdkEvent*) event);
  return TRUE;
}

const gchar *
mpris_menu_item_get_player (MprisMenuItem *mi)
{
  MprisMenuItemPrivate *priv;

  g_return_val_if_fail (IS_MPRIS_MENU_ITEM (mi), NULL);

  priv = GET_PRIVATE (mi);

  return priv->player;
}

void
mpris_menu_item_set_title (MprisMenuItem *item,
                           const gchar   *title)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  if (title == NULL || *title == '\0')
    gtk_label_set_markup_printf_escaped (GTK_LABEL (priv->title_label), "<b>%s</b>", priv->title);
  else
    gtk_label_set_markup_printf_escaped (GTK_LABEL (priv->title_label), "<b>%s</b>", title);
}

void
mpris_menu_item_set_artist (MprisMenuItem *item,
                            const gchar   *artist)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  if (artist == NULL || *artist == '\0')
    gtk_label_set_label (GTK_LABEL (priv->artist_label), NULL);
  else
    gtk_label_set_label (GTK_LABEL (priv->artist_label), artist);
}

void
mpris_menu_item_set_can_go_previous (MprisMenuItem *item,
                                     gboolean       enabled)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->can_go_previous = enabled;

  if (priv->is_running)
    gtk_widget_set_sensitive (priv->go_previous, priv->can_go_previous);
  else
    gtk_widget_set_sensitive (priv->go_previous, FALSE);
}

void
mpris_menu_item_set_can_play (MprisMenuItem *item,
                              gboolean       enabled)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->can_play = enabled;

  if (priv->is_running)
    gtk_widget_set_sensitive (priv->play_pause, priv->can_play);
  else
    gtk_widget_set_sensitive (priv->play_pause, FALSE);
}

void
mpris_menu_item_set_can_pause (MprisMenuItem *item,
                               gboolean       enabled)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->can_pause = enabled;

  if (priv->is_running)
    {
      if (priv->is_playing)
        {
          gtk_widget_set_sensitive (priv->play_pause, priv->can_pause);
        }
    }
  else
    gtk_widget_set_sensitive (priv->play_pause, FALSE);
}

void
mpris_menu_item_set_can_go_next (MprisMenuItem *item,
                                 gboolean       enabled)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->can_go_next = enabled;

  if (priv->is_running)
    gtk_widget_set_sensitive (priv->go_next, priv->can_go_next);
  else
    gtk_widget_set_sensitive (priv->go_next, FALSE);
}

void
mpris_menu_item_set_is_playing (MprisMenuItem *item,
                                gboolean       playing)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->is_playing = playing;

  if (priv->is_playing)
    {
      gtk_image_set_from_icon_name (GTK_IMAGE (gtk_button_get_image (GTK_BUTTON (priv->play_pause))), "media-playback-pause-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
      gtk_widget_set_sensitive (priv->play_pause, priv->can_pause);
      priv->is_stopped = FALSE;
    }
  else
    {
      gtk_image_set_from_icon_name (GTK_IMAGE (gtk_button_get_image (GTK_BUTTON (priv->play_pause))), "media-playback-start-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
      gtk_widget_set_sensitive (priv->play_pause, priv->can_play);
    }

  if (!priv->is_running)
    {
      gtk_widget_set_sensitive (priv->play_pause, FALSE);
    }
}

void
mpris_menu_item_set_is_stopped (MprisMenuItem *item,
                                gboolean       stopped)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->is_stopped = stopped;

  if (priv->is_stopped)
    {
      if (priv->is_playing)
        {
          mpris_menu_item_set_is_playing (item, FALSE);
        }

      mpris_menu_item_set_title (item, NULL);
      mpris_menu_item_set_artist (item, _("Not currently playing"));
    }
}

void
mpris_menu_item_set_is_running (MprisMenuItem *item,
                                gboolean       running)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->is_running = running;

  if (!priv->is_running)
    {
      mpris_menu_item_set_title (item, NULL);
      mpris_menu_item_set_artist (item, _("Not currently playing"));
      mpris_menu_item_set_can_play (item, FALSE);
      mpris_menu_item_set_can_pause (item, FALSE);
      mpris_menu_item_set_can_go_previous (item, FALSE);
      mpris_menu_item_set_can_go_next (item, FALSE);
      mpris_menu_item_set_is_playing (item, FALSE);
      mpris_menu_item_set_is_stopped (item, TRUE);
    }
  else
    {
      mpris_menu_item_set_can_play (item, priv->can_play);
      mpris_menu_item_set_can_pause (item, priv->can_pause);
      mpris_menu_item_set_can_go_next (item, priv->can_go_next);
      mpris_menu_item_set_can_go_previous (item, priv->can_go_previous);
      mpris_menu_item_set_is_playing (item, priv->is_playing);
      mpris_menu_item_set_is_stopped (item, priv->is_stopped);
    }
}

void
mpris_menu_item_set_can_raise (MprisMenuItem *item,
                               gboolean       can_raise)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  priv->can_raise = can_raise;
}

GtkWidget*
mpris_menu_item_new_with_player (const gchar *player,
                                 const gchar *title,
                                 const gchar *icon_name,
                                 const gchar *filename)
{
  MprisMenuItem        *menu_item;
  MprisMenuItemPrivate *priv;
  GtkWidget            *img;

  TRACE("entering");

  menu_item = MPRIS_MENU_ITEM (g_object_new (TYPE_MPRIS_MENU_ITEM, NULL));

  priv = GET_PRIVATE (menu_item);

  priv->player = g_strdup(player);
  if (title != NULL)
    priv->title = g_strdup(title);
  else
    priv->title = g_strdup(player);
  priv->filename = g_strdup(filename);

  priv->vbox = NULL;
  priv->hbox = NULL;
  priv->button_box = NULL;

  update_packing (menu_item);

  gtk_widget_add_events (GTK_WIDGET(menu_item), GDK_SCROLL_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_MOTION_MASK);

  img = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), img);
G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_image_set_pixel_size (GTK_IMAGE (img), 24);

  return GTK_WIDGET(menu_item);
}

static gchar *
find_desktop_entry (const gchar *player_name)
{
  GKeyFile  *key_file;
  gchar     *file;
  gchar     *filename = NULL;
  gchar     *full_path;

  file = g_strconcat ("applications/", player_name, ".desktop", NULL);

  key_file = g_key_file_new();
  if (g_key_file_load_from_data_dirs (key_file, file, &full_path, G_KEY_FILE_NONE, NULL))
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

GtkWidget*
mpris_menu_item_new_from_player_name (const gchar *player)
{
  GtkWidget *widget = NULL;
  GKeyFile  *key_file;
  gchar     *file;
  gchar     *filename;
  gchar     *full_path;

  filename = find_desktop_entry (player);
  if (filename == NULL)
    {
      g_free (filename);
      return NULL;
    }

  file = g_strconcat("applications/", filename, NULL);
  g_free (filename);

  key_file = g_key_file_new();
  if (g_key_file_load_from_data_dirs (key_file, file, &full_path, G_KEY_FILE_NONE, NULL))
    {
      gchar *name = g_key_file_get_string (key_file, "Desktop Entry", "Name", NULL);
      gchar *icon_name = g_key_file_get_string (key_file, "Desktop Entry", "Icon", NULL);

      widget = mpris_menu_item_new_with_player (player, name, icon_name, full_path);

      g_free (name);
      g_free (icon_name);
    }

  g_key_file_free (key_file);
  g_free (file);

  return widget;
}
