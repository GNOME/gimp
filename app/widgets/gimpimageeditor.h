/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_IMAGE_EDITOR_H__
#define __GIMP_IMAGE_EDITOR_H__


#include "gimpeditor.h"


#define GIMP_TYPE_IMAGE_EDITOR            (gimp_image_editor_get_type ())
#define GIMP_IMAGE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_EDITOR, GimpImageEditor))
#define GIMP_IMAGE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_EDITOR, GimpImageEditorClass))
#define GIMP_IS_IMAGE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_EDITOR))
#define GIMP_IS_IMAGE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_EDITOR))
#define GIMP_IMAGE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_EDITOR, GimpImageEditorClass))


typedef struct _GimpImageEditorClass GimpImageEditorClass;

struct _GimpImageEditor
{
  GimpEditor   parent_instance;

  GimpContext *context;
  GimpImage   *image;
};

struct _GimpImageEditorClass
{
  GimpEditorClass  parent_class;

  /*  virtual function  */
  void (* set_image) (GimpImageEditor *editor,
                      GimpImage       *image);
};


GType       gimp_image_editor_get_type  (void) G_GNUC_CONST;

void        gimp_image_editor_set_image (GimpImageEditor *editor,
                                         GimpImage       *image);
GimpImage * gimp_image_editor_get_image (GimpImageEditor *editor);


#endif /* __GIMP_IMAGE_EDITOR_H__ */
