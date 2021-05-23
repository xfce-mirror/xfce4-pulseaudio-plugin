/*  Copyright (c) 2014-2017 Andrzej <andrzejr@xfce.org>
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

#ifndef __PULSEAUDIO_BUTTON_H__
#define __PULSEAUDIO_BUTTON_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "pulseaudio-plugin.h"
#include "pulseaudio-config.h"
#include "pulseaudio-volume.h"
#include "pulseaudio-menu.h"
#include "pulseaudio-mpris.h"

G_BEGIN_DECLS

GType pulseaudio_button_get_type (void);

#define TYPE_PULSEAUDIO_BUTTON             (pulseaudio_button_get_type())
#define PULSEAUDIO_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_PULSEAUDIO_BUTTON, PulseaudioButton))
#define PULSEAUDIO_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_PULSEAUDIO_BUTTON, PulseaudioButtonClass))
#define IS_PULSEAUDIO_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PULSEAUDIO_BUTTON))
#define IS_PULSEAUDIO_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_PULSEAUDIO_BUTTON))
#define PULSEAUDIO_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_PULSEAUDIO_BUTTON, PulseaudioButtonClass))

typedef struct          _PulseaudioButton              PulseaudioButton;
typedef struct          _PulseaudioButtonClass         PulseaudioButtonClass;

PulseaudioButton       *pulseaudio_button_new         (PulseaudioPlugin *plugin,
                                                       PulseaudioConfig *config,
                                                       PulseaudioMpris  *mpris,
                                                       PulseaudioVolume *volume,
                                                       gboolean          mic);

void                    pulseaudio_button_set_size    (PulseaudioButton *button,
                                                       gint              size,
                                                       gint              icon_size);

PulseaudioMenu         *pulseaudio_button_get_menu    (PulseaudioButton *button);

G_END_DECLS

#endif /* !__PULSEAUDIO_BUTTON_H__ */
