/* -*- c-basic-offset: 2 -*- vi:set ts=2 sts=2 sw=2:
 * * Copyright (C) 2017 Sean Davis <bluesabre@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "devicemenuitem.h"

/* for DBG/TRACE */
#include <libxfce4util/libxfce4util.h>



struct _DeviceMenuItemPrivate {
  GtkWidget *submenu;
  GtkWidget *label;
  GtkWidget *image;
  GSList    *group;
  gchar     *title;
};



static void device_menu_item_finalize (GObject *object);



enum {
  DEVICE_CHANGED,
  LAST_SIGNAL
};




static guint signals[LAST_SIGNAL] = { 0 };

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE (DeviceMenuItem, device_menu_item, GTK_TYPE_IMAGE_MENU_ITEM)
G_GNUC_END_IGNORE_DEPRECATIONS

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_DEVICE_MENU_ITEM, DeviceMenuItemPrivate))



static void
device_menu_item_class_init (DeviceMenuItemClass *item_class)
{
  GObjectClass      *gobject_class =   G_OBJECT_CLASS      (item_class);

  gobject_class->finalize = device_menu_item_finalize;

  /**
   * DeviceMenuItem::value-changed:
   * @menuitem: the #DeviceMenuItem for which the value changed
   * @value: the new value
   *
   * Emitted whenever the value of the contained scale changes because
   * of user input.
   */
  signals[DEVICE_CHANGED] = g_signal_new ("device-changed",
                                         TYPE_DEVICE_MENU_ITEM,
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__STRING,
                                         G_TYPE_NONE,
                                         1, G_TYPE_STRING);


  g_type_class_add_private (item_class, sizeof (DeviceMenuItemPrivate));
}

static void
device_menu_item_init (DeviceMenuItem *self)
{
  DeviceMenuItemPrivate *priv;

  priv = GET_PRIVATE (self);

  priv->submenu = NULL;
  priv->label = NULL;
  priv->image = NULL;
  priv->title = NULL;
  priv->group = NULL;
}

static void
device_menu_item_finalize (GObject *object)
{
  DeviceMenuItem        *self;
  DeviceMenuItemPrivate *priv;

  self = DEVICE_MENU_ITEM (object);
  priv = GET_PRIVATE (self);

  if (priv->title)
    g_free (priv->title);

  g_object_unref (priv->submenu);
  g_object_unref (priv->image);
  g_object_unref (priv->label);

  (*G_OBJECT_CLASS (device_menu_item_parent_class)->finalize) (object);
}

GtkWidget*
device_menu_item_new_with_label (const gchar *label)
{
  DeviceMenuItem        *device_item;
  DeviceMenuItemPrivate *priv;

  TRACE("entering");

  device_item = DEVICE_MENU_ITEM (g_object_new (TYPE_DEVICE_MENU_ITEM, NULL));

  priv = GET_PRIVATE (device_item);

  priv->submenu = gtk_menu_new ();
  priv->title = g_strdup (label);
  priv->group = NULL;

  /* Configure menu item image */
  priv->image = gtk_image_new_from_icon_name ("audio-card", GTK_ICON_SIZE_LARGE_TOOLBAR);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (device_item), priv->image);
G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_image_set_pixel_size (GTK_IMAGE (priv->image), 24);

  /* Configure menu item label */
  gtk_menu_item_set_label (GTK_MENU_ITEM (device_item), priv->title);
  priv->label = gtk_bin_get_child (GTK_BIN (device_item));
  gtk_label_set_width_chars (GTK_LABEL (priv->label), 30);
  gtk_label_set_max_width_chars (GTK_LABEL (priv->label), 30);
  gtk_label_set_ellipsize (GTK_LABEL (priv->label), PANGO_ELLIPSIZE_MIDDLE);

  /* Configure menu item submenu */
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (device_item), priv->submenu);

  /* Keep references to the widgets */
  g_object_ref (priv->submenu);
  g_object_ref (priv->image);
  g_object_ref (priv->label);

  return GTK_WIDGET(device_item);
}

static void
device_menu_item_device_toggled (DeviceMenuItem   *self,
                                 GtkCheckMenuItem *menu_item)
{
  g_return_if_fail (IS_DEVICE_MENU_ITEM (self));

  if (gtk_check_menu_item_get_active (menu_item))
    {
      g_signal_emit (self, signals[DEVICE_CHANGED], 0, (gchar *)g_object_get_data (G_OBJECT(menu_item), "name"));
    }
}

void
device_menu_item_add_device (DeviceMenuItem *item,
                             const gchar    *name,
                             const gchar    *description)
{
  DeviceMenuItemPrivate *priv;
  GtkWidget             *mi;

  priv = GET_PRIVATE (item);

  mi = gtk_radio_menu_item_new_with_label (priv->group, description);
  g_object_set_data_full (G_OBJECT (mi), "name", g_strdup (name), (GDestroyNotify) g_free);

  priv->group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mi));

  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->submenu), mi);

  g_signal_connect_swapped (G_OBJECT (mi), "toggled", G_CALLBACK (device_menu_item_device_toggled), item);
}

void
device_menu_item_set_device_by_name       (DeviceMenuItem *item,
                                           const gchar    *name)
{
  DeviceMenuItemPrivate *priv;
  GList                 *children = NULL;
  GList                 *iter = NULL;
  gchar                 *markup = NULL;

  g_return_if_fail (IS_DEVICE_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  children = gtk_container_get_children (GTK_CONTAINER (priv->submenu));

  for (iter = children; iter != NULL; iter = g_list_next (iter)) {
    if (g_strcmp0 (name, (gchar *)g_object_get_data (G_OBJECT(iter->data), "name")) == 0)
      {
        if (markup != NULL)
          g_free (markup);
        markup = g_strconcat ("<b>", priv->title, " (", gtk_menu_item_get_label (GTK_MENU_ITEM (iter->data)), ")</b>", NULL);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (iter->data), TRUE);
      } else {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (iter->data), FALSE);
      }
  }

  if (markup == NULL)
    markup = g_strconcat ("<b>", priv->title, "</b>", NULL);

  gtk_label_set_markup (GTK_LABEL (priv->label), markup);
  g_free (markup);

  g_list_free (children);
}

void
device_menu_item_set_image_from_icon_name (DeviceMenuItem *item,
                                           const gchar    *icon_name)
{
  DeviceMenuItemPrivate *priv;

  g_return_if_fail (IS_DEVICE_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->image), icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
}
