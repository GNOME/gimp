/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppatheditor.h
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PATH_EDITOR_H__
#define __GIMP_PATH_EDITOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PATH_EDITOR (gimp_path_editor_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpPathEditor, gimp_path_editor, GIMP, PATH_EDITOR, GtkBox)

struct _GimpPathEditorClass
{
  GtkBoxClass  parent_class;

  void (* path_changed)     (GimpPathEditor *editor);
  void (* writable_changed) (GimpPathEditor *editor);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


/* For information look into the C source or the html documentation */

GtkWidget * gimp_path_editor_new               (const gchar    *title,
                                                const gchar    *path);

gchar     * gimp_path_editor_get_path          (GimpPathEditor *editor);
void        gimp_path_editor_set_path          (GimpPathEditor *editor,
                                                const gchar    *path);

gchar     * gimp_path_editor_get_writable_path (GimpPathEditor *editor);
void        gimp_path_editor_set_writable_path (GimpPathEditor *editor,
                                                const gchar    *path);

gboolean    gimp_path_editor_get_dir_writable  (GimpPathEditor *editor,
                                                const gchar    *directory);
void        gimp_path_editor_set_dir_writable  (GimpPathEditor *editor,
                                                const gchar    *directory,
                                                gboolean        writable);

G_END_DECLS

#endif /* __GIMP_PATH_EDITOR_H__ */
