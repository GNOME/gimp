/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolwidgetgroup.h
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

#ifndef __LIGMA_TOOL_WIDGET_GROUP_H__
#define __LIGMA_TOOL_WIDGET_GROUP_H__


#include "ligmatoolwidget.h"


#define LIGMA_TYPE_TOOL_WIDGET_GROUP            (ligma_tool_widget_group_get_type ())
#define LIGMA_TOOL_WIDGET_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_WIDGET_GROUP, LigmaToolWidgetGroup))
#define LIGMA_TOOL_WIDGET_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_WIDGET_GROUP, LigmaToolWidgetGroupClass))
#define LIGMA_IS_TOOL_WIDGET_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_WIDGET_GROUP))
#define LIGMA_IS_TOOL_WIDGET_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_WIDGET_GROUP))
#define LIGMA_TOOL_WIDGET_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_WIDGET_GROUP, LigmaToolWidgetGroupClass))


typedef struct _LigmaToolWidgetGroupPrivate LigmaToolWidgetGroupPrivate;
typedef struct _LigmaToolWidgetGroupClass   LigmaToolWidgetGroupClass;

struct _LigmaToolWidgetGroup
{
  LigmaToolWidget              parent_instance;

  LigmaToolWidgetGroupPrivate *priv;
};

struct _LigmaToolWidgetGroupClass
{
  LigmaToolWidgetClass  parent_class;
};


GType            ligma_tool_widget_group_get_type         (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_widget_group_new              (LigmaDisplayShell    *shell);

LigmaContainer  * ligma_tool_widget_group_get_children     (LigmaToolWidgetGroup *group);

LigmaToolWidget * ligma_tool_widget_group_get_focus_widget (LigmaToolWidgetGroup *group);

void             ligma_tool_widget_group_set_auto_raise   (LigmaToolWidgetGroup *group,
                                                          gboolean             auto_raise);
gboolean         ligma_tool_widget_group_get_auto_raise   (LigmaToolWidgetGroup *group);


#endif /* __LIGMA_TOOL_WIDGET_GROUP_H__ */
