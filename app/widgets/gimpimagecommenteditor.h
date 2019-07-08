/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImageCommentEditor
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_IMAGE_COMMENT_EDITOR_H__
#define __GIMP_IMAGE_COMMENT_EDITOR_H__


#include "gimpimageparasiteview.h"


#define GIMP_TYPE_IMAGE_COMMENT_EDITOR            (gimp_image_comment_editor_get_type ())
#define GIMP_IMAGE_COMMENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_COMMENT_EDITOR, GimpImageCommentEditor))
#define GIMP_IMAGE_COMMENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_COMMENT_EDITOR, GimpImageCommentEditorClass))
#define GIMP_IS_IMAGE_COMMENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_COMMENT_EDITOR))
#define GIMP_IS_IMAGE_COMMENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_COMMENT_EDITOR))
#define GIMP_IMAGE_COMMENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_COMMENT_EDITOR, GimpImageCommentEditorClass))


typedef struct _GimpImageCommentEditorClass GimpImageCommentEditorClass;

struct _GimpImageCommentEditor
{
  GimpImageParasiteView  parent_instance;

  GtkTextBuffer         *buffer;
  gboolean               recoursing;
};

struct _GimpImageCommentEditorClass
{
  GimpImageParasiteViewClass  parent_class;
};


GType       gimp_image_comment_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_image_comment_editor_new      (GimpImage *image);


#endif /*  __GIMP_IMAGE_COMMENT_EDITOR_H__  */
