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

#define TYPE_MPRIS_MENU_ITEM         (mpris_menu_item_get_type ())
#define MPRIS_MENU_ITEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_MPRIS_MENU_ITEM, MprisMenuItem))
#define MPRIS_MENU_ITEM_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), TYPE_MPRIS_MENU_ITEM, MprisMenuItemClass))
#define IS_MPRIS_MENU_ITEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_MPRIS_MENU_ITEM))
#define IS_MPRIS_MENU_ITEM_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), TYPE_MPRIS_MENU_ITEM))
#define MPRIS_MENU_ITEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_MPRIS_MENU_ITEM, MprisMenuItemClass))


typedef struct _MprisMenuItem        MprisMenuItem;
typedef struct _MprisMenuItemClass   MprisMenuItemClass;
typedef struct _MprisMenuItemPrivate MprisMenuItemPrivate;

struct _MprisMenuItem
{
  GtkImageMenuItem parent_instance;

  MprisMenuItemPrivate *priv;
};

struct _MprisMenuItemClass
{
  GtkImageMenuItemClass parent_class;
};


GType        mpris_menu_item_get_type              (void) G_GNUC_CONST;

GtkWidget   *mpris_menu_item_new_with_player       (const gchar *player,
                                                    const gchar *title,
                                                    const gchar *icon_name,
                                                    const gchar *filename);

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
