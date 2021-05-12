/* -*- c-basic-offset: 2 -*- vi:set ts=2 sts=2 sw=2:
 * * Copyright (C) 2014 Eric Koegel <eric@xfce.org>
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

#include "scalemenuitem.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

/* for DBG/TRACE */
#include <libxfce4util/libxfce4util.h>



struct _ScaleMenuItemPrivate {
  GtkWidget            *scale;
  GtkWidget            *hbox;
  GtkWidget            *vbox;
  GtkWidget            *image;
  GtkWidget            *mute_toggle;
  gchar                *icon_name;

  gboolean              grabbed;
};



enum {
  SLIDER_GRABBED,
  SLIDER_RELEASED,
  VALUE_CHANGED,
  TOGGLED,
  LAST_SIGNAL
};



static guint signals[LAST_SIGNAL] = { 0 };



G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE_WITH_PRIVATE (ScaleMenuItem, scale_menu_item, GTK_TYPE_IMAGE_MENU_ITEM)
G_GNUC_END_IGNORE_DEPRECATIONS



/* Static Declarations */
static void         scale_menu_item_finalize                (GObject            *object);
static void         scale_menu_item_update_icon             (ScaleMenuItem      *self);
static void         scale_menu_item_scale_value_changed     (GtkRange           *range,
                                                             gpointer            user_data);
static gboolean     scale_menu_item_button_press_event      (GtkWidget          *item,
                                                             GdkEventButton     *event);
static gboolean     scale_menu_item_button_release_event    (GtkWidget          *item,
                                                             GdkEventButton     *event);
static gboolean     scale_menu_item_motion_notify_event     (GtkWidget          *item,
                                                             GdkEventMotion     *event);
static gboolean     scale_menu_item_leave_notify_event      (GtkWidget          *item,
                                                             GdkEventCrossing   *event);
static gboolean     scale_menu_item_mute_toggle_state_set   (GtkWidget          *mute_toggle,
                                                             gboolean            state,
                                                             gpointer            user_data);
static void         menu_hidden                             (GtkWidget          *menu,
                                                             ScaleMenuItem      *scale);
static void         scale_menu_item_parent_set              (GtkWidget          *item,
                                                             GtkWidget          *previous_parent);



/* Public API */
GtkWidget*
scale_menu_item_new_with_range (gdouble min,
                                gdouble max,
                                gdouble step)
{
  ScaleMenuItem        *scale_item;
  ScaleMenuItemPrivate *priv;

  TRACE("entering");

  scale_item = SCALE_MENU_ITEM (g_object_new (TYPE_SCALE_MENU_ITEM, NULL));

  priv = scale_menu_item_get_instance_private (scale_item);

  /* Configure the menu item image */
  priv->image = gtk_image_new ();
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (scale_item), priv->image);
G_GNUC_END_IGNORE_DEPRECATIONS

  /* Configure the menu item scale */
  priv->scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request (priv->scale, 100, -1);
  gtk_range_set_inverted (GTK_RANGE(priv->scale), FALSE);
  gtk_scale_set_draw_value (GTK_SCALE(priv->scale), FALSE);
  gtk_range_set_round_digits(GTK_RANGE(priv->scale), 0);

  if (max > 100.0)
    gtk_scale_add_mark (GTK_SCALE (priv->scale), 100.0, GTK_POS_BOTTOM, NULL);

  /* Configure the mute toggle */
  priv->mute_toggle = gtk_switch_new ();

  /* Pack the scale widget */
  priv->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (priv->hbox), priv->scale, TRUE, TRUE, 0);
  gtk_box_set_center_widget (GTK_BOX (priv->vbox), priv->mute_toggle);
  gtk_box_pack_start (GTK_BOX (priv->hbox), priv->vbox, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (scale_item), priv->hbox);
  gtk_widget_show_all (priv->hbox);

  /* Connect events */
  g_signal_connect (priv->scale, "value-changed", G_CALLBACK (scale_menu_item_scale_value_changed), scale_item);
  g_signal_connect (priv->mute_toggle, "state-set", G_CALLBACK (scale_menu_item_mute_toggle_state_set), NULL);
  gtk_widget_add_events (GTK_WIDGET(scale_item), GDK_SCROLL_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_MOTION_MASK);

  /* Keep references to the widgets */
  g_object_ref (priv->image);
  g_object_ref (priv->scale);
  g_object_ref (priv->mute_toggle);
  g_object_ref (priv->hbox);
  g_object_ref (priv->vbox);

  return GTK_WIDGET(scale_item);
}



gdouble
scale_menu_item_get_value (ScaleMenuItem *item)
{
  ScaleMenuItemPrivate *priv;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (item), 0.0);

  priv = scale_menu_item_get_instance_private (item);

  return gtk_range_get_value (GTK_RANGE (priv->scale));
}



void
scale_menu_item_set_value (ScaleMenuItem *item,
                           gdouble        value)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (item));

  priv = scale_menu_item_get_instance_private (item);

  gtk_range_set_value (GTK_RANGE (priv->scale), value);
}



gboolean
scale_menu_item_get_muted (ScaleMenuItem *item)
{
  ScaleMenuItemPrivate *priv;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (item), TRUE);

  priv = scale_menu_item_get_instance_private (item);

  return !gtk_switch_get_state (GTK_SWITCH (priv->mute_toggle));
}



void
scale_menu_item_set_muted (ScaleMenuItem *item,
                           gboolean       muted)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (item));

  priv = scale_menu_item_get_instance_private (item);

  gtk_switch_set_active (GTK_SWITCH (priv->mute_toggle), !muted);
  gtk_switch_set_state (GTK_SWITCH (priv->mute_toggle), !muted);

  scale_menu_item_update_icon (item);
}



void
scale_menu_item_set_base_icon_name (ScaleMenuItem *item,
                                    const gchar   *base_icon_name)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (item));

  priv = scale_menu_item_get_instance_private (item);

  if (priv->icon_name)
    g_free (priv->icon_name);

  priv->icon_name = g_strdup (base_icon_name);
}



/* Private API */
static void
scale_menu_item_class_init (ScaleMenuItemClass *item_class)
{
  GObjectClass      *gobject_class =   G_OBJECT_CLASS      (item_class);
  GtkWidgetClass    *widget_class =    GTK_WIDGET_CLASS    (item_class);

  widget_class->button_press_event   = scale_menu_item_button_press_event;
  widget_class->button_release_event = scale_menu_item_button_release_event;
  widget_class->motion_notify_event  = scale_menu_item_motion_notify_event;
  widget_class->leave_notify_event   = scale_menu_item_leave_notify_event;
  widget_class->parent_set           = scale_menu_item_parent_set;

  gobject_class->finalize = scale_menu_item_finalize;

  /**
   * ScaleMenuItem::slider-grabbed:
   * @menuitem: The #ScaleMenuItem emitting the signal.
   *
   * The ::slider-grabbed signal is emitted when the pointer selects the slider.
   */
  signals[SLIDER_GRABBED] = g_signal_new ("slider-grabbed",
                                          G_OBJECT_CLASS_TYPE (gobject_class),
                                          G_SIGNAL_RUN_FIRST,
                                          0,
                                          NULL, NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);

  /**
   * ScaleMenuItem::slider-released:
   * @menuitem: The #ScaleMenuItem emitting the signal.
   *
   * The ::slider-released signal is emitted when the pointer releases the slider.
   */
  signals[SLIDER_RELEASED] = g_signal_new ("slider-released",
                                           G_OBJECT_CLASS_TYPE (gobject_class),
                                           G_SIGNAL_RUN_FIRST,
                                           0,
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__VOID,
                                           G_TYPE_NONE, 0);


  /**
   * ScaleMenuItem::value-changed:
   * @menuitem: the #ScaleMenuItem for which the value changed
   * @value: the new value
   *
   * Emitted whenever the value of the contained scale changes because
   * of user input.
   */
  signals[VALUE_CHANGED] = g_signal_new ("value-changed",
                                         TYPE_SCALE_MENU_ITEM,
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__DOUBLE,
                                         G_TYPE_NONE,
                                         1, G_TYPE_DOUBLE);

   /**
    * ScaleMenuItem::toggled:
    * @menuitem: the #ScaleMenuItem emitting the signal.
    *
    * Emitted whenever the mute switch is toggled.
    */
   signals[TOGGLED] = g_signal_new ("toggled",
                                    G_OBJECT_CLASS_TYPE (gobject_class),
                                    G_SIGNAL_RUN_FIRST,
                                    0,
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
}



static void
scale_menu_item_init (ScaleMenuItem *self)
{
  ScaleMenuItemPrivate *priv;

  priv = scale_menu_item_get_instance_private (self);

  priv->scale = NULL;
  priv->image = NULL;
  priv->hbox = NULL;
  priv->vbox = NULL;
  priv->mute_toggle = NULL;
  priv->icon_name = NULL;
}



static void
scale_menu_item_finalize (GObject *object)
{
  ScaleMenuItem        *self;
  ScaleMenuItemPrivate *priv;

  self = SCALE_MENU_ITEM (object);
  priv = scale_menu_item_get_instance_private (self);

  if (priv->icon_name)
    g_free (priv->icon_name);

  g_object_unref (priv->scale);
  g_object_unref (priv->image);
  g_object_unref (priv->mute_toggle);
  g_object_unref (priv->vbox);
  g_object_unref (priv->hbox);

  (*G_OBJECT_CLASS (scale_menu_item_parent_class)->finalize) (object);
}



static void
scale_menu_item_update_icon (ScaleMenuItem *self)
{
  ScaleMenuItemPrivate *priv = scale_menu_item_get_instance_private (self);
  gchar                *icon_name = NULL;
  gdouble               value = gtk_range_get_value (GTK_RANGE (priv->scale));

  /* Update the menuitem icon */
  if (scale_menu_item_get_muted (self) || value <= 0.0)
    icon_name = g_strconcat(priv->icon_name, "-muted-symbolic", NULL);
  else if (value < 30.0)
    icon_name = g_strconcat(priv->icon_name, "-low-symbolic", NULL);
  else if (value < 70.0)
    icon_name = g_strconcat(priv->icon_name, "-medium-symbolic", NULL);
  else
    icon_name = g_strconcat(priv->icon_name, "-high-symbolic", NULL);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->image), icon_name, GTK_ICON_SIZE_MENU);
  g_free (icon_name);
}



static void
scale_menu_item_scale_value_changed (GtkRange *range,
                                     gpointer  user_data)
{
  ScaleMenuItem *self = SCALE_MENU_ITEM (user_data);
  gdouble        value = gtk_range_get_value (range);

  g_signal_emit (self, signals[VALUE_CHANGED], 0, value);

  scale_menu_item_update_icon (self);
}



static gboolean
scale_menu_item_button_press_event (GtkWidget      *item,
                                    GdkEventButton *event)
{
  ScaleMenuItemPrivate *priv;
  GtkAllocation         alloc;
  GtkSwitch            *mute_toggle;
  gint                  x, y;

  TRACE("entering");

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (item), FALSE);

  if ((event->type == GDK_2BUTTON_PRESS) || (event->type == GDK_3BUTTON_PRESS))
    return TRUE;

  priv = scale_menu_item_get_instance_private (SCALE_MENU_ITEM (item));

  gtk_widget_get_allocation (priv->mute_toggle, &alloc);
  gtk_widget_translate_coordinates (GTK_WIDGET (item), priv->mute_toggle, event->x, event->y, &x, &y);
  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
      /* toggle the desired state, but keep the underlying state as is */
      mute_toggle = GTK_SWITCH (priv->mute_toggle);
      gtk_switch_set_active (mute_toggle, !gtk_switch_get_active (mute_toggle));
      return TRUE;
    }

  gtk_widget_get_allocation (priv->scale, &alloc);
  gtk_widget_translate_coordinates (item, priv->scale, event->x, event->y, &x, &y);
  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
      gtk_widget_event (priv->scale, (GdkEvent*) event);
    }

  if (!priv->grabbed)
    {
      priv->grabbed = TRUE;
      g_signal_emit (item, signals[SLIDER_GRABBED], 0);
    }

  return TRUE;
}



static gboolean
scale_menu_item_button_release_event (GtkWidget      *item,
                                      GdkEventButton *event)
{
  ScaleMenuItemPrivate *priv;
  GtkAllocation         alloc;
  GtkSwitch            *mute_toggle;
  gint                  x, y;
  gboolean              cur_mute_state, new_mute_state;

  TRACE("entering");

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (item), FALSE);

  priv = scale_menu_item_get_instance_private (SCALE_MENU_ITEM (item));

  gtk_widget_get_allocation (priv->mute_toggle, &alloc);
  gtk_widget_translate_coordinates (GTK_WIDGET (item), priv->mute_toggle, event->x, event->y, &x, &y);

  /* toggle the switch's underlying state if the pointer is still within the
   * widget area, otherwise reset its desired state */
  mute_toggle = GTK_SWITCH (priv->mute_toggle);
  cur_mute_state = gtk_switch_get_state (mute_toggle);
  new_mute_state = (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
                   ? gtk_switch_get_active (mute_toggle) : cur_mute_state;
  if (new_mute_state != cur_mute_state)
    {
      gtk_switch_set_state (mute_toggle, new_mute_state);
      g_signal_emit (item, signals[TOGGLED], 0);
    }
  else
    {
      gtk_switch_set_active (mute_toggle, cur_mute_state);
    }

  gtk_widget_get_allocation (priv->scale, &alloc);
  gtk_widget_translate_coordinates (item, priv->scale, event->x, event->y, &x, &y);
  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
      gtk_widget_event (priv->scale, (GdkEvent*) event);
    }

  if (priv->grabbed)
    {
      priv->grabbed = FALSE;
      g_signal_emit (item, signals[SLIDER_RELEASED], 0);
    }

  return TRUE;
}



static gboolean
scale_menu_item_motion_notify_event (GtkWidget      *item,
                                     GdkEventMotion *event)
{
  ScaleMenuItemPrivate *priv;
  GtkWidget            *scale;
  GtkAllocation         alloc;
  gint                  x, y;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (item), FALSE);

  priv = scale_menu_item_get_instance_private (SCALE_MENU_ITEM (item));
  scale = priv->scale;

  gtk_widget_get_allocation (priv->scale, &alloc);
  gtk_widget_translate_coordinates (item, priv->scale, event->x, event->y, &x, &y);

  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
      gtk_widget_event (scale, (GdkEvent*) event);
    }

  return TRUE;
}

static gboolean
scale_menu_item_leave_notify_event (GtkWidget        *item,
                                    GdkEventCrossing *event)
{
  ScaleMenuItemPrivate *priv;
  GtkSwitch            *mute_toggle;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (item), FALSE);

  priv = scale_menu_item_get_instance_private (SCALE_MENU_ITEM (item));
  mute_toggle = GTK_SWITCH (priv->mute_toggle);

  /* reset the switch to its current underlying state */
  gtk_switch_set_active (mute_toggle, gtk_switch_get_state (mute_toggle));

  return TRUE;
}

gboolean scale_menu_item_mute_toggle_state_set (GtkWidget *mute_toggle,
                                                gboolean   state,
                                                gpointer   user_data)
{
  /* do nothing, the underlying state is updated on leave-notify or buttun-release events */
  return TRUE;
}


static void
menu_hidden (GtkWidget     *menu,
             ScaleMenuItem *scale)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (scale));
  g_return_if_fail (GTK_IS_MENU (menu));

  priv = scale_menu_item_get_instance_private (scale);

  if (priv->grabbed)
    {
      priv->grabbed = FALSE;
      g_signal_emit (scale, signals[SLIDER_RELEASED], 0);
    }
}



static void
scale_menu_item_parent_set (GtkWidget *item,
                            GtkWidget *previous_parent)

{
  GtkWidget *parent;

  g_return_if_fail (IS_SCALE_MENU_ITEM (item));

  if (previous_parent != NULL)
    {
      g_signal_handlers_disconnect_by_func (previous_parent, menu_hidden, item);
    }

  parent = gtk_widget_get_parent (item);

  if (parent != NULL)
    {
      g_signal_connect (parent, "hide", G_CALLBACK (menu_hidden), item);
    }
}
