/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppatheditor.h
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_PATH_EDITOR_H__
#define __GIMP_PATH_EDITOR_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_PATH_EDITOR            (gimp_path_editor_get_type ())
#define GIMP_PATH_EDITOR(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_PATH_EDITOR, GimpPathEditor))
#define GIMP_PATH_EDITOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATH_EDITOR, GimpPathEditorClass))
#define GIMP_IS_PATH_EDITOR(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_PATH_EDITOR))
#define GIMP_IS_PATH_EDITOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATH_EDITOR))

typedef struct _GimpPathEditor       GimpPathEditor;
typedef struct _GimpPathEditorClass  GimpPathEditorClass;

struct _GimpPathEditor
{
  GtkVBox    vbox;

  GtkWidget *upper_hbox;

  GtkWidget *new_button;
  GtkWidget *up_button;
  GtkWidget *down_button;
  GtkWidget *delete_button;

  GtkWidget *file_selection;

  GtkWidget *dir_list;

  GtkWidget *selected_item;
  gint       number_of_items;
};

struct _GimpPathEditorClass
{
  GtkVBoxClass parent_class;

  void (* path_changed) (GimpPathEditor *gpe);
};

/* For information look into the C source or the html documentation */

GtkType     gimp_path_editor_get_type (void);

GtkWidget * gimp_path_editor_new      (gchar          *filesel_title,
				       gchar          *path);

gchar     * gimp_path_editor_get_path (GimpPathEditor *gpe);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PATH_EDITOR_H__ */
