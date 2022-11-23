/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_IMAGE_EDITOR_H__
#define __LIGMA_IMAGE_EDITOR_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_IMAGE_EDITOR            (ligma_image_editor_get_type ())
#define LIGMA_IMAGE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_EDITOR, LigmaImageEditor))
#define LIGMA_IMAGE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE_EDITOR, LigmaImageEditorClass))
#define LIGMA_IS_IMAGE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_EDITOR))
#define LIGMA_IS_IMAGE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE_EDITOR))
#define LIGMA_IMAGE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE_EDITOR, LigmaImageEditorClass))


typedef struct _LigmaImageEditorClass LigmaImageEditorClass;

struct _LigmaImageEditor
{
  LigmaEditor   parent_instance;

  LigmaContext *context;
  LigmaImage   *image;
};

struct _LigmaImageEditorClass
{
  LigmaEditorClass  parent_class;

  /*  virtual function  */
  void (* set_image) (LigmaImageEditor *editor,
                      LigmaImage       *image);
};


GType       ligma_image_editor_get_type  (void) G_GNUC_CONST;

void        ligma_image_editor_set_image (LigmaImageEditor *editor,
                                         LigmaImage       *image);
LigmaImage * ligma_image_editor_get_image (LigmaImageEditor *editor);


#endif /* __LIGMA_IMAGE_EDITOR_H__ */
