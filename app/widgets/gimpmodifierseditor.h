/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodifierseditor.h
 * Copyright (C) 2022 Jehan
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

#ifndef __GIMP_MODIFIERS_EDITOR_H__
#define __GIMP_MODIFIERS_EDITOR_H__


#define GIMP_TYPE_MODIFIERS_EDITOR            (gimp_modifiers_editor_get_type ())
#define GIMP_MODIFIERS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MODIFIERS_EDITOR, GimpModifiersEditor))
#define GIMP_MODIFIERS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MODIFIERS_EDITOR, GimpModifiersEditorClass))
#define GIMP_IS_MODIFIERS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MODIFIERS_EDITOR))
#define GIMP_IS_MODIFIERS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MODIFIERS_EDITOR))
#define GIMP_MODIFIERS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MODIFIERS_EDITOR, GimpModifiersEditorClass))


typedef struct _GimpModifiersEditorPrivate GimpModifiersEditorPrivate;
typedef struct _GimpModifiersEditorClass   GimpModifiersEditorClass;

struct _GimpModifiersEditor
{
  GimpFrame                  parent_instance;

  GimpModifiersEditorPrivate *priv;
};

struct _GimpModifiersEditorClass
{
  GimpFrameClass             parent_class;
};


GType          gimp_modifiers_editor_get_type (void) G_GNUC_CONST;

GtkWidget    * gimp_modifiers_editor_new      (GimpModifiersManager *manager,
                                               Gimp                 *gimp);

void           gimp_modifiers_editor_clear    (GimpModifiersEditor  *editor);


#endif /* __GIMP_MODIFIERS_EDITOR_H__ */
