/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsamplepointeditor.h
 * Copyright (C) 2005-2016 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_SAMPLE_POINT_EDITOR_H__
#define __GIMP_SAMPLE_POINT_EDITOR_H__


#include "gimpimageeditor.h"


#define GIMP_TYPE_SAMPLE_POINT_EDITOR            (gimp_sample_point_editor_get_type ())
#define GIMP_SAMPLE_POINT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SAMPLE_POINT_EDITOR, GimpSamplePointEditor))
#define GIMP_SAMPLE_POINT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SAMPLE_POINT_EDITOR, GimpSamplePointEditorClass))
#define GIMP_IS_SAMPLE_POINT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SAMPLE_POINT_EDITOR))
#define GIMP_IS_SAMPLE_POINT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SAMPLE_POINT_EDITOR))
#define GIMP_SAMPLE_POINT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SAMPLE_POINT_EDITOR, GimpSamplePointEditorClass))


typedef struct _GimpSamplePointEditorClass GimpSamplePointEditorClass;

struct _GimpSamplePointEditor
{
  GimpImageEditor  parent_instance;

  GtkWidget       *empty_icon;
  GtkWidget       *empty_label;

  GtkWidget       *grid;
  GtkWidget      **color_frames;
  gint             n_color_frames;

  guint            dirty_idle_id;

  gboolean         sample_merged;
};

struct _GimpSamplePointEditorClass
{
  GimpImageEditorClass  parent_class;
};


GType       gimp_sample_point_editor_get_type          (void) G_GNUC_CONST;

GtkWidget * gimp_sample_point_editor_new               (GimpMenuFactory *menu_factory);

void        gimp_sample_point_editor_set_sample_merged (GimpSamplePointEditor *editor,
                                                        gboolean               sample_merged);
gboolean    gimp_sample_point_editor_get_sample_merged (GimpSamplePointEditor *editor);


#endif /* __GIMP_SAMPLE_POINT_EDITOR_H__ */
