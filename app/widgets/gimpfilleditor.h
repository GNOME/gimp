/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfilleditor.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_FILL_EDITOR_H__
#define __GIMP_FILL_EDITOR_H__


#define GIMP_TYPE_FILL_EDITOR            (gimp_fill_editor_get_type ())
#define GIMP_FILL_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILL_EDITOR, GimpFillEditor))
#define GIMP_FILL_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILL_EDITOR, GimpFillEditorClass))
#define GIMP_IS_FILL_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILL_EDITOR))
#define GIMP_IS_FILL_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILL_EDITOR))
#define GIMP_FILL_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILL_EDITOR, GimpFillEditorClass))


typedef struct _GimpFillEditorClass GimpFillEditorClass;

struct _GimpFillEditor
{
  GtkBox           parent_instance;

  GimpFillOptions *options;
  gboolean         edit_context;
  gboolean         use_custom_style; /* For solid color and pattern only */
};

struct _GimpFillEditorClass
{
  GtkBoxClass      parent_class;
};


GType       gimp_fill_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_fill_editor_new      (GimpFillOptions *options,
                                       gboolean         edit_context,
                                       gboolean         use_custom_style);


#endif /* __GIMP_FILL_EDITOR_H__ */
