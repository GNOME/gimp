/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolline.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "gimptoolwidget.h"


/* in the context of GimpToolLine, "handle" is a collective term for either an
 * endpoint or a slider.  a handle value may be either a (nonnegative) slider
 * index, or one of the values below:
 */
#define GIMP_TOOL_LINE_HANDLE_NONE  (-3)
#define GIMP_TOOL_LINE_HANDLE_START (-2)
#define GIMP_TOOL_LINE_HANDLE_END   (-1)

#define GIMP_TOOL_LINE_HANDLE_IS_SLIDER(handle) ((handle) >= 0)


#define GIMP_TYPE_TOOL_LINE            (gimp_tool_line_get_type ())
#define GIMP_TOOL_LINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_LINE, GimpToolLine))
#define GIMP_TOOL_LINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_LINE, GimpToolLineClass))
#define GIMP_IS_TOOL_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_LINE))
#define GIMP_IS_TOOL_LINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_LINE))
#define GIMP_TOOL_LINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_LINE, GimpToolLineClass))


typedef struct _GimpToolLine        GimpToolLine;
typedef struct _GimpToolLinePrivate GimpToolLinePrivate;
typedef struct _GimpToolLineClass   GimpToolLineClass;

struct _GimpToolLine
{
  GimpToolWidget       parent_instance;

  GimpToolLinePrivate *private;
};

struct _GimpToolLineClass
{
  GimpToolWidgetClass  parent_class;

  /*  signals  */
  gboolean (* can_add_slider)           (GimpToolLine        *line,
                                         gdouble              value);
  gint     (* add_slider)               (GimpToolLine        *line,
                                         gdouble              value);
  void     (* prepare_to_remove_slider) (GimpToolLine        *line,
                                         gint                 slider,
                                         gboolean             remove);
  void     (* remove_slider)            (GimpToolLine        *line,
                                         gint                 slider);
  void     (* selection_changed)        (GimpToolLine        *line);
  gboolean (* handle_clicked)           (GimpToolLine        *line,
                                         gint                 handle,
                                         GdkModifierType      state,
                                         GimpButtonPressType  press_type);
};


GType                        gimp_tool_line_get_type      (void) G_GNUC_CONST;

GimpToolWidget             * gimp_tool_line_new           (GimpDisplayShell           *shell,
                                                           gdouble                     x1,
                                                           gdouble                     y1,
                                                           gdouble                     x2,
                                                           gdouble                     y2);

void                         gimp_tool_line_set_sliders   (GimpToolLine               *line,
                                                           const GimpControllerSlider *sliders,
                                                           gint                        n_sliders);
const GimpControllerSlider * gimp_tool_line_get_sliders   (GimpToolLine               *line,
                                                           gint                       *n_sliders);

void                         gimp_tool_line_set_selection (GimpToolLine               *line,
                                                           gint                        handle);
gint                         gimp_tool_line_get_selection (GimpToolLine               *line);
