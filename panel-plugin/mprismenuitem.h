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
/*
 * Based on the scale menu item implementation of the indicator applet:
 * Authors:
 *    Cody Russell <crussell@canonical.com>
 * http://bazaar.launchpad.net/~indicator-applet-developers/ido/trunk.14.10/view/head:/src/idoscalemenuitem.h
 */


#ifndef _MPRIS_MENU_ITEM_H_
#define _MPRIS_MENU_ITEM_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MPRIS_TYPE_MENU_ITEM (mpris_menu_item_get_type ())
#ifndef GTK_IMAGE_MENU_ITEM_AUTOPTR_CLEANUP_FUNC_ALREADY_DEFINED
#define GTK_IMAGE_MENU_ITEM_AUTOPTR_CLEANUP_FUNC_ALREADY_DEFINED 1
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkImageMenuItem, g_object_unref)
#endif
G_DECLARE_FINAL_TYPE (MprisMenuItem, mpris_menu_item, MPRIS, MENU_ITEM, GtkImageMenuItem)

GtkWidget   *mpris_menu_item_new_from_player_name  (const gchar *player);

const gchar *mpris_menu_item_get_player            (MprisMenuItem *item);

void         mpris_menu_item_set_title             (MprisMenuItem *item,
                                                    const gchar   *title);

void         mpris_menu_item_set_artist            (MprisMenuItem *item,
                                                    const gchar   *artist);

void         mpris_menu_item_set_can_go_previous   (MprisMenuItem *item,
                                                    gboolean enabled);

void         mpris_menu_item_set_can_play          (MprisMenuItem *item,
                                                    gboolean enabled);

void         mpris_menu_item_set_can_pause         (MprisMenuItem *item,
                                                    gboolean enabled);

void         mpris_menu_item_set_can_go_next       (MprisMenuItem *item,
                                                    gboolean enabled);

void         mpris_menu_item_set_can_raise         (MprisMenuItem *item,
                                                    gboolean can_raise);

void         mpris_menu_item_set_can_raise_wnck    (MprisMenuItem *item,
                                                    gboolean can_raise);

void         mpris_menu_item_set_is_running        (MprisMenuItem *item,
                                                    gboolean running);

void         mpris_menu_item_set_is_playing        (MprisMenuItem *item,
                                                    gboolean playing);

void         mpris_menu_item_set_is_stopped        (MprisMenuItem *item,
                                                    gboolean stopped);

G_END_DECLS

#endif /* _MPRIS_MENU_ITEM_H_ */
