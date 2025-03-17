/* -*- c-basic-offset: 2 -*- vi:set ts=2 sts=2 sw=2:
 * Copyright (c) 2017-2020 Sean Davis <bluesabre@xfce.org>
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
#include "pulseaudio-mpris.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gdesktopappinfo.h>
#include <libxfce4ui/libxfce4ui.h>

#if !LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
#include <exo/exo.h>
#endif


/* for DBG/TRACE */
#include <libxfce4util/libxfce4util.h>

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
  gboolean   can_raise_wnck;

  gboolean   is_running;
  gboolean   is_playing;
  gboolean   is_stopped;

  gchar     *player;
  gchar     *title;
  gchar     *filename;

  GtkWidget *image;
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
G_DEFINE_TYPE_WITH_PRIVATE (MprisMenuItem, mpris_menu_item, GTK_TYPE_IMAGE_MENU_ITEM)
G_GNUC_END_IGNORE_DEPRECATIONS



/* Static Declarations */
static void         mpris_menu_item_finalize                (GObject        *object);
static void         mpris_menu_item_raise                   (MprisMenuItem  *item);

static void         mpris_menu_item_launch                  (MprisMenuItem  *item);
static void         mpris_menu_item_raise_or_launch         (MprisMenuItem  *item);
static GtkWidget *  mpris_menu_item_get_widget_at_event     (MprisMenuItem  *item,
                                                             GdkEventButton *event);
static gboolean     mpris_menu_item_button_press_event      (GtkWidget      *item,
                                                             GdkEventButton *event);
static gboolean     mpris_menu_item_button_release_event    (GtkWidget      *item,
                                                             GdkEventButton *event);
static void         media_play_pause_clicked_event          (GtkButton      *button,
                                                             gpointer        user_data);
static void         media_go_previous_clicked_event         (GtkButton      *button,
                                                             gpointer        user_data);
static void         media_go_next_clicked_event             (GtkButton      *button,
                                                             gpointer        user_data);
static void         menu_item_activate_event                (GtkMenuItem    *item,
                                                             gpointer        user_data);
static void         media_notify                            (MprisMenuItem  *item,
                                                             gchar          *message);
static GtkWidget *  track_info_label_new                    (void);
static void         gtk_label_set_markup_printf_escaped     (GtkLabel       *label,
                                                             const gchar    *format,
                                                                             ...);
static void         update_packing                          (MprisMenuItem  *item);



static GtkWidget*
mpris_menu_item_new_with_player (const gchar *player,
                                 gchar *player_label,
                                 gchar *icon_name,
                                 gchar *filename)
{
  MprisMenuItem        *menu_item;
  MprisMenuItemPrivate *priv;

  TRACE("entering");

  menu_item = MPRIS_MENU_ITEM (g_object_new (TYPE_MPRIS_MENU_ITEM, NULL));

  priv = mpris_menu_item_get_instance_private (menu_item);

  priv->player = g_strdup (player);
  priv->title = player_label;
  priv->filename = filename;

  update_packing (menu_item);

  gtk_widget_add_events (GTK_WIDGET(menu_item), GDK_SCROLL_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_MOTION_MASK);

  if (g_file_test (icon_name, G_FILE_TEST_EXISTS) && !g_file_test (icon_name, G_FILE_TEST_IS_DIR)) {
    GdkPixbuf *buf;
    gint scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (menu_item));
    gint size;

    if (! gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &size, NULL))
      size = 24;
    size *= scale_factor;

#if LIBXFCE4UI_CHECK_VERSION(4, 21, 0)
    buf = xfce_gdk_pixbuf_new_from_file_at_max_size (icon_name, size, size, TRUE, NULL);
#else
    buf = exo_gdk_pixbuf_new_from_file_at_max_size (icon_name, size, size, TRUE, NULL);
#endif
    if (buf != NULL) {
      cairo_surface_t *surface = gdk_cairo_surface_create_from_pixbuf (buf, scale_factor, NULL);
      gtk_image_set_from_surface (GTK_IMAGE (priv->image), surface);
      cairo_surface_destroy(surface);
      g_object_unref (buf);
    } else {
      gtk_image_set_from_icon_name (GTK_IMAGE (priv->image), "audio-player", GTK_ICON_SIZE_LARGE_TOOLBAR);
    }
  } else {
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->image), icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
  }

  return GTK_WIDGET(menu_item);
}



/* Public API */
GtkWidget*
mpris_menu_item_new_from_player_name (const gchar *player)
{
  GtkWidget *widget = NULL;
  gchar     *player_label;
  gchar     *icon_name;
  gchar     *full_path;

  if (pulseaudio_mpris_get_player_summary (player, &player_label, &icon_name, &full_path))
    {
      widget = mpris_menu_item_new_with_player (player, player_label, icon_name, full_path);
      g_free (icon_name);
    }

  return widget;
}



const gchar *
mpris_menu_item_get_player (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;

  g_return_val_if_fail (IS_MPRIS_MENU_ITEM (item), NULL);

  priv = mpris_menu_item_get_instance_private (item);

  return priv->player;
}



void
mpris_menu_item_set_title (MprisMenuItem *item,
                           const gchar   *title)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

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

  priv = mpris_menu_item_get_instance_private (item);

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

  priv = mpris_menu_item_get_instance_private (item);

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

  priv = mpris_menu_item_get_instance_private (item);

  priv->can_play = enabled;

  if (priv->is_running)
    {
      if (!priv->is_playing)
        {
          gtk_widget_set_sensitive (priv->play_pause, priv->can_play);
        }
    }
  else
    gtk_widget_set_sensitive (priv->play_pause, FALSE);
}



void
mpris_menu_item_set_can_pause (MprisMenuItem *item,
                               gboolean       enabled)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

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

  priv = mpris_menu_item_get_instance_private (item);

  priv->can_go_next = enabled;

  if (priv->is_running)
    gtk_widget_set_sensitive (priv->go_next, priv->can_go_next);
  else
    gtk_widget_set_sensitive (priv->go_next, FALSE);
}



void
mpris_menu_item_set_can_raise (MprisMenuItem *item,
                               gboolean       can_raise)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

  priv->can_raise = can_raise;
}



void
mpris_menu_item_set_can_raise_wnck (MprisMenuItem *item,
                                    gboolean       can_raise)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

  priv->can_raise_wnck = can_raise;
}



void
mpris_menu_item_set_is_running (MprisMenuItem *item,
                                gboolean       running)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

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
mpris_menu_item_set_is_playing (MprisMenuItem *item,
                                gboolean       playing)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

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

  priv = mpris_menu_item_get_instance_private (item);

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



/* Private API */
static void
mpris_menu_item_class_init (MprisMenuItemClass *item_class)
{
  GObjectClass      *gobject_class =   G_OBJECT_CLASS      (item_class);
  GtkWidgetClass    *widget_class =    GTK_WIDGET_CLASS    (item_class);

  gobject_class->finalize = mpris_menu_item_finalize;
  widget_class->button_press_event   = mpris_menu_item_button_press_event;
  widget_class->button_release_event = mpris_menu_item_button_release_event;

  /**
   * MprisMenuItem::media-notify:
   * @menuitem: the #MprisMenuItem for which the value changed
   * @value: the mpris signal to emit
   *
   * Emitted whenever a media button is clicked.
   */
  signals[MEDIA_NOTIFY] = g_signal_new ("media-notify",
                                        TYPE_MPRIS_MENU_ITEM,
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL,
                                        g_cclosure_marshal_VOID__STRING,
                                        G_TYPE_NONE,
                                        1, G_TYPE_STRING);
}



static void
mpris_menu_item_init (MprisMenuItem *item)
{
}



static void
mpris_menu_item_finalize (GObject *object)
{
  MprisMenuItem        *item;
  MprisMenuItemPrivate *priv;

  item = MPRIS_MENU_ITEM (object);
  priv = mpris_menu_item_get_instance_private (item);

  g_free (priv->player);
  g_free (priv->title);
  g_free (priv->filename);

  g_object_unref (priv->title_label);
  g_object_unref (priv->artist_label);
  g_object_unref (priv->button_box);
  g_object_unref (priv->vbox);
  g_object_unref (priv->hbox);
  g_object_unref (priv->go_previous);
  g_object_unref (priv->play_pause);
  g_object_unref (priv->go_next);
  g_object_unref (priv->image);

  (*G_OBJECT_CLASS (mpris_menu_item_parent_class)->finalize) (object);
}



static void
mpris_menu_item_raise (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

  if (priv->is_running)
    {
      if (priv->can_raise)
        {
          media_notify (item, "Raise");
        }
#ifdef HAVE_LIBXFCE4WINDOWING
      else if (priv->can_raise_wnck)
        {
          media_notify (item, "RaiseWnck");
        }
#endif
    }
}



static void
mpris_menu_item_launch (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;
  GAppInfo             *app_info;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

  if (priv->is_running || !priv->filename)
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

  priv = mpris_menu_item_get_instance_private (item);

  if (priv->is_running)
    mpris_menu_item_raise (item);
  else
    mpris_menu_item_launch (item);
}



static GtkWidget *
mpris_menu_item_get_widget_at_event (MprisMenuItem  *item,
                                     GdkEventButton *event)
{
  MprisMenuItemPrivate *priv;
  GtkAllocation         alloc;
  gint                  x, y;

  g_return_val_if_fail (IS_MPRIS_MENU_ITEM (item), NULL);

  priv = mpris_menu_item_get_instance_private (item);

  gtk_widget_get_allocation (priv->button_box, &alloc);
  gtk_widget_translate_coordinates (GTK_WIDGET (item), priv->button_box, event->x, event->y, &x, &y);

  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
      gtk_widget_get_allocation (priv->go_previous, &alloc);
      gtk_widget_translate_coordinates (GTK_WIDGET (item), priv->go_previous, event->x, event->y, &x, &y);

      if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
        return GTK_WIDGET (priv->go_previous);

      gtk_widget_get_allocation (priv->play_pause, &alloc);
      gtk_widget_translate_coordinates (GTK_WIDGET (item), priv->play_pause, event->x, event->y, &x, &y);

      if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
        return GTK_WIDGET (priv->play_pause);

      gtk_widget_get_allocation (priv->go_next, &alloc);
      gtk_widget_translate_coordinates (GTK_WIDGET (item), priv->go_next, event->x, event->y, &x, &y);

      if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
        return GTK_WIDGET (priv->go_next);
    }

  return GTK_WIDGET (item);
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
menu_item_activate_event (GtkMenuItem *mi, gpointer user_data)
{
  mpris_menu_item_raise_or_launch (MPRIS_MENU_ITEM (mi));
}



static void
media_notify (MprisMenuItem *item,
              gchar         *message)
{
  g_signal_emit (item, signals[MEDIA_NOTIFY], 0, message);
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
update_packing (MprisMenuItem *item)
{
  MprisMenuItemPrivate *priv;
  GtkBox               *hbox;
  GtkBox               *vbox;
  GtkBox               *button_box;
  GtkStyleContext      *ctx;

  g_return_if_fail (IS_MPRIS_MENU_ITEM (item));

  priv = mpris_menu_item_get_instance_private (item);

  TRACE("entering");

  /* Create the widgets */
  hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

  button_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  ctx = gtk_widget_get_style_context (GTK_WIDGET (button_box));
  gtk_style_context_add_class (ctx, "linked");

  priv->hbox = GTK_WIDGET(hbox);
  priv->vbox = GTK_WIDGET(vbox);
  priv->button_box = GTK_WIDGET(button_box);

  priv->go_previous = gtk_button_new_from_icon_name ("media-skip-backward-symbolic", GTK_ICON_SIZE_MENU);
  priv->play_pause = gtk_button_new_from_icon_name ("media-playback-start-symbolic", GTK_ICON_SIZE_MENU);
  priv->go_next = gtk_button_new_from_icon_name ("media-skip-forward-symbolic", GTK_ICON_SIZE_MENU);

  priv->title_label = track_info_label_new ();
  priv->artist_label = track_info_label_new ();

  priv->image = gtk_image_new_from_icon_name ("audio-x-generic", GTK_ICON_SIZE_LARGE_TOOLBAR);

  /* Pack the widgets */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), priv->image);
G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_image_set_pixel_size (GTK_IMAGE (priv->image), 24);

  gtk_box_pack_start (button_box, priv->go_previous, FALSE, FALSE, 0);
  gtk_box_pack_start (button_box, priv->play_pause, FALSE, FALSE, 0);
  gtk_box_pack_start (button_box, priv->go_next, FALSE, FALSE, 0);

  gtk_box_pack_start (vbox, priv->title_label, FALSE, FALSE, 0);
  gtk_box_pack_start (vbox, priv->artist_label, FALSE, FALSE, 0);

  gtk_box_pack_start (hbox, GTK_WIDGET (vbox), TRUE, TRUE, 0);
  gtk_box_pack_start (hbox, GTK_WIDGET (button_box), FALSE, FALSE, 0);

  /* Configure the widgets */
  mpris_menu_item_set_title (item, priv->player);
  mpris_menu_item_set_artist (item, _("Not currently playing"));

  g_signal_connect (priv->play_pause, "clicked", G_CALLBACK (media_play_pause_clicked_event), item);
  g_signal_connect (priv->go_previous, "clicked", G_CALLBACK (media_go_previous_clicked_event), item);
  g_signal_connect (priv->go_next, "clicked", G_CALLBACK (media_go_next_clicked_event), item);
  g_signal_connect (item, "activate", G_CALLBACK (menu_item_activate_event), item);

  /* Keep a reference to each */
  g_object_ref (priv->title_label);
  g_object_ref (priv->artist_label);
  g_object_ref (priv->button_box);
  g_object_ref (priv->vbox);
  g_object_ref (priv->hbox);
  g_object_ref (priv->go_previous);
  g_object_ref (priv->play_pause);
  g_object_ref (priv->go_next);
  g_object_ref (priv->image);

  /* And finally display them */
  gtk_widget_show_all (priv->button_box);
  gtk_widget_show_all (priv->hbox);
  gtk_widget_show_all (priv->vbox);

  gtk_container_add (GTK_CONTAINER (item), priv->hbox);
}
