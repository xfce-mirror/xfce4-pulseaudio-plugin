/*  Copyright (c) 2009-2015 Steve Dodier-Lazaro <sidi@xfce.org>
 *  Copyright (c) 2015-2017 Andrzej <andrzejr@xfce.org>
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

#ifndef __PULSEAUDIO_NOTIFY_H__
#define __PULSEAUDIO_NOTIFY_H__

#ifdef HAVE_LIBNOTIFY

#include <glib-object.h>
#include "pulseaudio-config.h"
#include "pulseaudio-volume.h"
#include "pulseaudio-button.h"

G_BEGIN_DECLS

#define PULSEAUDIO_TYPE_NOTIFY (pulseaudio_notify_get_type ())
G_DECLARE_FINAL_TYPE (PulseaudioNotify, pulseaudio_notify, PULSEAUDIO, NOTIFY, GObject)

PulseaudioNotify       *pulseaudio_notify_new             (PulseaudioConfig *config,
                                                           PulseaudioVolume *volume,
                                                           PulseaudioButton *button);
void                    pulseaudio_notify_volume_changed  (PulseaudioNotify  *notify,
                                                           gboolean           should_notify,
                                                           PulseaudioVolume  *volume);

G_END_DECLS

#endif /* HAVE_LIBNOTIFY */
#endif /* !__PULSEAUDIO_NOTIFY_H__ */
