/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorProfileChooserDialog
 * Copyright (C) 2006-2014 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COLOR_PROFILE_CHOOSER_DIALOG_H__
#define __GIMP_COLOR_PROFILE_CHOOSER_DIALOG_H__

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG (gimp_color_profile_chooser_dialog_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorProfileChooserDialog, gimp_color_profile_chooser_dialog, GIMP, COLOR_PROFILE_CHOOSER_DIALOG, GtkFileChooserDialog)

GtkWidget * gimp_color_profile_chooser_dialog_new      (const gchar          *title,
                                                        GtkWindow            *parent,
                                                        GtkFileChooserAction  action);


G_END_DECLS

#endif /* __GIMP_COLOR_PROFILE_CHOOSER_DIALOG_H__ */
