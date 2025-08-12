/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolbutton.h
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


#define GIMP_TYPE_TOOL_BUTTON            (gimp_tool_button_get_type ())
#define GIMP_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_BUTTON, GimpToolButton))
#define GIMP_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_BUTTON, GimpToolButtonClass))
#define GIMP_IS_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_TOOL_BUTTON))
#define GIMP_IS_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_BUTTON))
#define GIMP_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_BUTTON, GimpToolButtonClass))


typedef struct _GimpToolButtonPrivate GimpToolButtonPrivate;
typedef struct _GimpToolButtonClass   GimpToolButtonClass;

struct _GimpToolButton
{
  GtkToggleToolButton    parent_instance;

  GimpToolButtonPrivate *priv;
};

struct _GimpToolButtonClass
{
  GtkToggleToolButtonClass  parent_class;
};


GType          gimp_tool_button_get_type      (void) G_GNUC_CONST;

GtkToolItem  * gimp_tool_button_new           (GimpToolbox    *toolbox,
                                               GimpToolItem   *tool_item);

GimpToolbox  * gimp_tool_button_get_toolbox   (GimpToolButton *tool_button);

void           gimp_tool_button_set_tool_item (GimpToolButton *tool_button,
                                               GimpToolItem   *tool_item);
GimpToolItem * gimp_tool_button_get_tool_item (GimpToolButton *tool_button);

GimpToolInfo * gimp_tool_button_get_tool_info (GimpToolButton *tool_button);
