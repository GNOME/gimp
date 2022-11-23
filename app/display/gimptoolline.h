/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolline.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_LINE_H__
#define __LIGMA_TOOL_LINE_H__


#include "ligmatoolwidget.h"


/* in the context of LigmaToolLine, "handle" is a collective term for either an
 * endpoint or a slider.  a handle value may be either a (nonnegative) slider
 * index, or one of the values below:
 */
#define LIGMA_TOOL_LINE_HANDLE_NONE  (-3)
#define LIGMA_TOOL_LINE_HANDLE_START (-2)
#define LIGMA_TOOL_LINE_HANDLE_END   (-1)

#define LIGMA_TOOL_LINE_HANDLE_IS_SLIDER(handle) ((handle) >= 0)


#define LIGMA_TYPE_TOOL_LINE            (ligma_tool_line_get_type ())
#define LIGMA_TOOL_LINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_LINE, LigmaToolLine))
#define LIGMA_TOOL_LINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_LINE, LigmaToolLineClass))
#define LIGMA_IS_TOOL_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_LINE))
#define LIGMA_IS_TOOL_LINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_LINE))
#define LIGMA_TOOL_LINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_LINE, LigmaToolLineClass))


typedef struct _LigmaToolLine        LigmaToolLine;
typedef struct _LigmaToolLinePrivate LigmaToolLinePrivate;
typedef struct _LigmaToolLineClass   LigmaToolLineClass;

struct _LigmaToolLine
{
  LigmaToolWidget       parent_instance;

  LigmaToolLinePrivate *private;
};

struct _LigmaToolLineClass
{
  LigmaToolWidgetClass  parent_class;

  /*  signals  */
  gboolean (* can_add_slider)           (LigmaToolLine        *line,
                                         gdouble              value);
  gint     (* add_slider)               (LigmaToolLine        *line,
                                         gdouble              value);
  void     (* prepare_to_remove_slider) (LigmaToolLine        *line,
                                         gint                 slider,
                                         gboolean             remove);
  void     (* remove_slider)            (LigmaToolLine        *line,
                                         gint                 slider);
  void     (* selection_changed)        (LigmaToolLine        *line);
  gboolean (* handle_clicked)           (LigmaToolLine        *line,
                                         gint                 handle,
                                         GdkModifierType      state,
                                         LigmaButtonPressType  press_type);
};


GType                        ligma_tool_line_get_type      (void) G_GNUC_CONST;

LigmaToolWidget             * ligma_tool_line_new           (LigmaDisplayShell           *shell,
                                                           gdouble                     x1,
                                                           gdouble                     y1,
                                                           gdouble                     x2,
                                                           gdouble                     y2);

void                         ligma_tool_line_set_sliders   (LigmaToolLine               *line,
                                                           const LigmaControllerSlider *sliders,
                                                           gint                        n_sliders);
const LigmaControllerSlider * ligma_tool_line_get_sliders   (LigmaToolLine               *line,
                                                           gint                       *n_sliders);

void                         ligma_tool_line_set_selection (LigmaToolLine               *line,
                                                           gint                        handle);
gint                         ligma_tool_line_get_selection (LigmaToolLine               *line);


#endif /* __LIGMA_TOOL_LINE_H__ */
