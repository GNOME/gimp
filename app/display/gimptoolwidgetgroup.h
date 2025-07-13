/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolwidgetgroup.h
 * Copyright (C) 2018 Ell
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


#define GIMP_TYPE_TOOL_WIDGET_GROUP            (gimp_tool_widget_group_get_type ())
#define GIMP_TOOL_WIDGET_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_WIDGET_GROUP, GimpToolWidgetGroup))
#define GIMP_TOOL_WIDGET_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_WIDGET_GROUP, GimpToolWidgetGroupClass))
#define GIMP_IS_TOOL_WIDGET_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_WIDGET_GROUP))
#define GIMP_IS_TOOL_WIDGET_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_WIDGET_GROUP))
#define GIMP_TOOL_WIDGET_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_WIDGET_GROUP, GimpToolWidgetGroupClass))


typedef struct _GimpToolWidgetGroupPrivate GimpToolWidgetGroupPrivate;
typedef struct _GimpToolWidgetGroupClass   GimpToolWidgetGroupClass;

struct _GimpToolWidgetGroup
{
  GimpToolWidget              parent_instance;

  GimpToolWidgetGroupPrivate *priv;
};

struct _GimpToolWidgetGroupClass
{
  GimpToolWidgetClass  parent_class;
};


GType            gimp_tool_widget_group_get_type         (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_widget_group_new              (GimpDisplayShell    *shell);

GimpContainer  * gimp_tool_widget_group_get_children     (GimpToolWidgetGroup *group);

GimpToolWidget * gimp_tool_widget_group_get_focus_widget (GimpToolWidgetGroup *group);

void             gimp_tool_widget_group_set_auto_raise   (GimpToolWidgetGroup *group,
                                                          gboolean             auto_raise);
gboolean         gimp_tool_widget_group_get_auto_raise   (GimpToolWidgetGroup *group);
