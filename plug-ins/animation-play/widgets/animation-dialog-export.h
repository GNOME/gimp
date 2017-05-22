/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-dialog-export.h
 * Copyright (C) 2017 Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ANIMATION_DIALOG_EXPORT_H__
#define __ANIMATION_DIALOG_EXPORT_H__

#include <gtk/gtk.h>

void         animation_dialog_export              (GtkWindow         *main_dialog,
                                                   AnimationPlayback *playback);

#endif  /*  __ANIMATION_DIALOG_EXPORT_H__  */
