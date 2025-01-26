/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfilechooser.c
 * Copyright (C) 2025 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_FILE_CHOOSER_H__
#define __GIMP_FILE_CHOOSER_H__

G_BEGIN_DECLS

#define GIMP_TYPE_FILE_CHOOSER (gimp_file_chooser_get_type ())
G_DECLARE_FINAL_TYPE (GimpFileChooser, gimp_file_chooser, GIMP, FILE_CHOOSER, GtkBox)


GtkWidget             * gimp_file_chooser_new              (GimpFileChooserAction  action,
                                                            const gchar           *label,
                                                            const gchar           *title,
                                                            GFile                 *file);

GimpFileChooserAction   gimp_file_chooser_get_action       (GimpFileChooser       *chooser);
void                    gimp_file_chooser_set_action       (GimpFileChooser       *chooser,
                                                            GimpFileChooserAction  action);

GFile                 * gimp_file_chooser_get_file         (GimpFileChooser       *chooser);
void                    gimp_file_chooser_set_file         (GimpFileChooser       *chooser,
                                                            GFile                 *file);

const gchar           * gimp_file_chooser_get_label        (GimpFileChooser       *chooser);
void                    gimp_file_chooser_set_label        (GimpFileChooser       *chooser,
                                                            const gchar           *text);

const gchar           * gimp_file_chooser_get_title        (GimpFileChooser       *chooser);
void                    gimp_file_chooser_set_title        (GimpFileChooser       *chooser,
                                                            const gchar           *text);

GtkWidget             * gimp_file_chooser_get_label_widget (GimpFileChooser       *chooser);


G_END_DECLS

#endif /* __GIMP_FILE_CHOOSER_H__ */
