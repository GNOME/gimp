/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmablobeditor.h
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

#ifndef  __LIGMA_BLOB_EDITOR_H__
#define  __LIGMA_BLOB_EDITOR_H__


#define LIGMA_TYPE_BLOB_EDITOR            (ligma_blob_editor_get_type ())
#define LIGMA_BLOB_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BLOB_EDITOR, LigmaBlobEditor))
#define LIGMA_BLOB_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BLOB_EDITOR, LigmaBlobEditorClass))
#define LIGMA_IS_BLOB_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BLOB_EDITOR))
#define LIGMA_IS_BLOB_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BLOB_EDITOR))
#define LIGMA_BLOB_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BLOB_EDITOR, LigmaBlobEditorClass))


typedef struct _LigmaBlobEditorClass LigmaBlobEditorClass;

struct _LigmaBlobEditor
{
  GtkDrawingArea       parent_instance;

  LigmaInkBlobType      type;
  gdouble              aspect;
  gdouble              angle;

  /*<  private  >*/
  gboolean             in_handle;
  gboolean             active;
};

struct _LigmaBlobEditorClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       ligma_blob_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_blob_editor_new      (LigmaInkBlobType  type,
                                       gdouble          aspect,
                                       gdouble          angle);


#endif  /*  __LIGMA_BLOB_EDITOR_H__  */
