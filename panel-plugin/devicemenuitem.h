/* -*- c-basic-offset: 2 -*- vi:set ts=2 sts=2 sw=2:
 * * Copyright (C) 2017-2020 Sean Davis <bluesabre@xfce.org>
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


#ifndef _DEVICE_MENU_ITEM_H_
#define _DEVICE_MENU_ITEM_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_DEVICE_MENU_ITEM         (device_menu_item_get_type ())
#define DEVICE_MENU_ITEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_DEVICE_MENU_ITEM, DeviceMenuItem))
#define DEVICE_MENU_ITEM_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), TYPE_DEVICE_MENU_ITEM, DeviceMenuItemClass))
#define IS_DEVICE_MENU_ITEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_DEVICE_MENU_ITEM))
#define IS_DEVICE_MENU_ITEM_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), TYPE_DEVICE_MENU_ITEM))
#define DEVICE_MENU_ITEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_DEVICE_MENU_ITEM, DeviceMenuItemClass))


typedef struct _DeviceMenuItem        DeviceMenuItem;
typedef struct _DeviceMenuItemClass   DeviceMenuItemClass;
typedef struct _DeviceMenuItemPrivate DeviceMenuItemPrivate;

struct _DeviceMenuItem
{
  GtkImageMenuItem parent_instance;

  DeviceMenuItemPrivate *priv;
};

struct _DeviceMenuItemClass
{
  GtkImageMenuItemClass parent_class;
};


GType        device_menu_item_get_type                 (void) G_GNUC_CONST;

GtkWidget   *device_menu_item_new_with_label           (const gchar    *label);

void         device_menu_item_add_device               (DeviceMenuItem *item,
                                                        const gchar    *name,
                                                        const gchar    *description);

void         device_menu_item_set_device_by_name       (DeviceMenuItem *item,
                                                        const gchar    *name);


G_END_DECLS

#endif /* _DEVICE_MENU_ITEM_H_ */
