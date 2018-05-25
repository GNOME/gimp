/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpblobeditor.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  __GIMP_BLOB_EDITOR_H__
#define  __GIMP_BLOB_EDITOR_H__


#define GIMP_TYPE_BLOB_EDITOR            (gimp_blob_editor_get_type ())
#define GIMP_BLOB_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BLOB_EDITOR, GimpBlobEditor))
#define GIMP_BLOB_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BLOB_EDITOR, GimpBlobEditorClass))
#define GIMP_IS_BLOB_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BLOB_EDITOR))
#define GIMP_IS_BLOB_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BLOB_EDITOR))
#define GIMP_BLOB_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BLOB_EDITOR, GimpBlobEditorClass))


typedef struct _GimpBlobEditorClass GimpBlobEditorClass;

struct _GimpBlobEditor
{
  GtkDrawingArea       parent_instance;

  GimpInkBlobType      type;
  gdouble              aspect;
  gdouble              angle;

  /*<  private  >*/
  gboolean             in_handle;
  gboolean             active;
};

struct _GimpBlobEditorClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       gimp_blob_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_blob_editor_new      (GimpInkBlobType  type,
                                       gdouble          aspect,
                                       gdouble          angle);


#endif  /*  __GIMP_BLOB_EDITOR_H__  */
