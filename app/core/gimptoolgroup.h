/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolgroup.h
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

#include "gimptoolitem.h"


#define GIMP_TYPE_TOOL_GROUP            (gimp_tool_group_get_type ())
#define GIMP_TOOL_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_GROUP, GimpToolGroup))
#define GIMP_TOOL_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_GROUP, GimpToolGroupClass))
#define GIMP_IS_TOOL_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_GROUP))
#define GIMP_IS_TOOL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_GROUP))
#define GIMP_TOOL_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_GROUP, GimpToolGroupClass))


typedef struct _GimpToolGroupPrivate GimpToolGroupPrivate;
typedef struct _GimpToolGroupClass   GimpToolGroupClass;

struct _GimpToolGroup
{
  GimpToolItem          parent_instance;

  GimpToolGroupPrivate *priv;
};

struct _GimpToolGroupClass
{
  GimpToolItemClass  parent_class;

  /*  signals  */
  void (* active_tool_changed) (GimpToolGroup *tool_group);
};


GType           gimp_tool_group_get_type             (void) G_GNUC_CONST;

GimpToolGroup * gimp_tool_group_new                  (void);

void            gimp_tool_group_set_active_tool      (GimpToolGroup *tool_group,
                                                      const gchar   *tool_name);
const gchar   * gimp_tool_group_get_active_tool      (GimpToolGroup *tool_group);

void            gimp_tool_group_set_active_tool_info (GimpToolGroup *tool_group,
                                                      GimpToolInfo  *tool_info);
GimpToolInfo  * gimp_tool_group_get_active_tool_info (GimpToolGroup *tool_group);
