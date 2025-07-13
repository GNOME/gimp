/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstrokeeditor.h
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#pragma once

#include "gimpfilleditor.h"


#define GIMP_TYPE_STROKE_EDITOR            (gimp_stroke_editor_get_type ())
#define GIMP_STROKE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STROKE_EDITOR, GimpStrokeEditor))
#define GIMP_STROKE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STROKE_EDITOR, GimpStrokeEditorClass))
#define GIMP_IS_STROKE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STROKE_EDITOR))
#define GIMP_IS_STROKE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STROKE_EDITOR))
#define GIMP_STROKE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STROKE_EDITOR, GimpStrokeEditorClass))


typedef struct _GimpStrokeEditorClass GimpStrokeEditorClass;

struct _GimpStrokeEditor
{
  GimpFillEditor  parent_instance;

  gdouble         resolution;
};

struct _GimpStrokeEditorClass
{
  GimpFillEditorClass  parent_class;
};


GType       gimp_stroke_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_stroke_editor_new      (GimpStrokeOptions *options,
                                         gdouble            resolution,
                                         gboolean           edit_context,
                                         gboolean           use_custom_style);
