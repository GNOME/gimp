/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolfocus.h
 * Copyright (C) 2020 Ell
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


#define GIMP_TYPE_TOOL_FOCUS            (gimp_tool_focus_get_type ())
#define GIMP_TOOL_FOCUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_FOCUS, GimpToolFocus))
#define GIMP_TOOL_FOCUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_FOCUS, GimpToolFocusClass))
#define GIMP_IS_TOOL_FOCUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_FOCUS))
#define GIMP_IS_TOOL_FOCUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_FOCUS))
#define GIMP_TOOL_FOCUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_FOCUS, GimpToolFocusClass))


typedef struct _GimpToolFocus        GimpToolFocus;
typedef struct _GimpToolFocusPrivate GimpToolFocusPrivate;
typedef struct _GimpToolFocusClass   GimpToolFocusClass;

struct _GimpToolFocus
{
  GimpToolWidget        parent_instance;

  GimpToolFocusPrivate *priv;
};

struct _GimpToolFocusClass
{
  GimpToolWidgetClass  parent_class;
};


GType            gimp_tool_focus_get_type (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_focus_new      (GimpDisplayShell *shell);
