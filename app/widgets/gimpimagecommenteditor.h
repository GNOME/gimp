/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaImageCommentEditor
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_IMAGE_COMMENT_EDITOR_H__
#define __LIGMA_IMAGE_COMMENT_EDITOR_H__


#include "ligmaimageparasiteview.h"


#define LIGMA_TYPE_IMAGE_COMMENT_EDITOR            (ligma_image_comment_editor_get_type ())
#define LIGMA_IMAGE_COMMENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_COMMENT_EDITOR, LigmaImageCommentEditor))
#define LIGMA_IMAGE_COMMENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE_COMMENT_EDITOR, LigmaImageCommentEditorClass))
#define LIGMA_IS_IMAGE_COMMENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_COMMENT_EDITOR))
#define LIGMA_IS_IMAGE_COMMENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE_COMMENT_EDITOR))
#define LIGMA_IMAGE_COMMENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE_COMMENT_EDITOR, LigmaImageCommentEditorClass))


typedef struct _LigmaImageCommentEditorClass LigmaImageCommentEditorClass;

struct _LigmaImageCommentEditor
{
  LigmaImageParasiteView  parent_instance;

  GtkTextBuffer         *buffer;
  gboolean               recoursing;
};

struct _LigmaImageCommentEditorClass
{
  LigmaImageParasiteViewClass  parent_class;
};


GType       ligma_image_comment_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_image_comment_editor_new      (LigmaImage *image);


#endif /*  __LIGMA_IMAGE_COMMENT_EDITOR_H__  */
