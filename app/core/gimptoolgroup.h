/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolgroup.h
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

#ifndef __LIGMA_TOOL_GROUP_H__
#define __LIGMA_TOOL_GROUP_H__


#include "ligmatoolitem.h"


#define LIGMA_TYPE_TOOL_GROUP            (ligma_tool_group_get_type ())
#define LIGMA_TOOL_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_GROUP, LigmaToolGroup))
#define LIGMA_TOOL_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_GROUP, LigmaToolGroupClass))
#define LIGMA_IS_TOOL_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_GROUP))
#define LIGMA_IS_TOOL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_GROUP))
#define LIGMA_TOOL_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_GROUP, LigmaToolGroupClass))


typedef struct _LigmaToolGroupPrivate LigmaToolGroupPrivate;
typedef struct _LigmaToolGroupClass   LigmaToolGroupClass;

struct _LigmaToolGroup
{
  LigmaToolItem          parent_instance;

  LigmaToolGroupPrivate *priv;
};

struct _LigmaToolGroupClass
{
  LigmaToolItemClass  parent_class;

  /*  signals  */
  void (* active_tool_changed) (LigmaToolGroup *tool_group);
};


GType           ligma_tool_group_get_type             (void) G_GNUC_CONST;

LigmaToolGroup * ligma_tool_group_new                  (void);

void            ligma_tool_group_set_active_tool      (LigmaToolGroup *tool_group,
                                                      const gchar   *tool_name);
const gchar   * ligma_tool_group_get_active_tool      (LigmaToolGroup *tool_group);

void            ligma_tool_group_set_active_tool_info (LigmaToolGroup *tool_group,
                                                      LigmaToolInfo  *tool_info);
LigmaToolInfo  * ligma_tool_group_get_active_tool_info (LigmaToolGroup *tool_group);


#endif  /*  __LIGMA_TOOL_GROUP_H__  */
