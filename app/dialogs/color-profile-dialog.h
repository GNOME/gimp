/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-profile-dialog.h
 * Copyright (C) 2015 Michael Natterer <mitch@gimp.org>
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

#pragma once


typedef enum
{
  COLOR_PROFILE_DIALOG_ASSIGN_PROFILE,
  COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE,
  COLOR_PROFILE_DIALOG_CONVERT_TO_RGB,
  COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY,
  COLOR_PROFILE_DIALOG_SELECT_SOFTPROOF_PROFILE
} ColorProfileDialogType;


typedef void (* GimpColorProfileCallback) (GtkWidget                *dialog,
                                           GimpImage                *image,
                                           GimpColorProfile         *new_profile,
                                           GFile                    *new_file,
                                           GimpColorRenderingIntent  intent,
                                           gboolean                  bpc,
                                           gpointer                  user_data);


GtkWidget * color_profile_dialog_new (ColorProfileDialogType    dialog_type,
                                      GimpImage                *image,
                                      GimpContext              *context,
                                      GtkWidget                *parent,
                                      GimpColorProfile         *current_profile,
                                      GimpColorProfile         *default_profile,
                                      GimpColorRenderingIntent  intent,
                                      gboolean                  bpc,
                                      GimpColorProfileCallback  callback,
                                      gpointer                  user_data);
