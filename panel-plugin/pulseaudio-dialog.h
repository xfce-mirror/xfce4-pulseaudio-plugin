/*
 *  Copyright (C) 2015 Andrzej <andrzejr@xfce.org>
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

#ifndef __PULSEAUDIO_DIALOG_H__
#define __PULSEAUDIO_DIALOG_H__

#include <glib.h>
#include <gtk/gtk.h>
#include "pulseaudio-config.h"

G_BEGIN_DECLS

#define PULSEAUDIO_TYPE_DIALOG (pulseaudio_dialog_get_type ())
G_DECLARE_FINAL_TYPE (PulseaudioDialog, pulseaudio_dialog, PULSEAUDIO, DIALOG, GtkBuilder)

void                pulseaudio_dialog_show     (PulseaudioDialog  *dialog,
                                                GdkScreen         *screen);

PulseaudioDialog   *pulseaudio_dialog_new      (PulseaudioConfig  *config);

G_END_DECLS

#endif /* !__PULSEAUDIO_DIALOG_H__ */
