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

#define DEVICE_TYPE_MENU_ITEM (device_menu_item_get_type ())
#ifndef GTK_IMAGE_MENU_ITEM_AUTOPTR_CLEANUP_FUNC_ALREADY_DEFINED
#define GTK_IMAGE_MENU_ITEM_AUTOPTR_CLEANUP_FUNC_ALREADY_DEFINED 1
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkImageMenuItem, g_object_unref)
#endif
G_DECLARE_FINAL_TYPE (DeviceMenuItem, device_menu_item, DEVICE, MENU_ITEM, GtkImageMenuItem)

GtkWidget   *device_menu_item_new_with_label           (const gchar    *label);

void         device_menu_item_add_device               (DeviceMenuItem *item,
                                                        const gchar    *name,
                                                        const gchar    *description,
                                                        gboolean        sensitive);

void         device_menu_item_set_device_by_name       (DeviceMenuItem *item,
                                                        const gchar    *name);


G_END_DECLS

#endif /* _DEVICE_MENU_ITEM_H_ */
