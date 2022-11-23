/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_TOOL_INFO_H__
#define __LIGMA_TOOL_INFO_H__


#include "ligmatoolitem.h"


#define LIGMA_TYPE_TOOL_INFO            (ligma_tool_info_get_type ())
#define LIGMA_TOOL_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_INFO, LigmaToolInfo))
#define LIGMA_TOOL_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_INFO, LigmaToolInfoClass))
#define LIGMA_IS_TOOL_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_INFO))
#define LIGMA_IS_TOOL_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_INFO))
#define LIGMA_TOOL_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_INFO, LigmaToolInfoClass))


typedef struct _LigmaToolInfoClass LigmaToolInfoClass;

struct _LigmaToolInfo
{
  LigmaToolItem         parent_instance;

  Ligma                *ligma;

  GType                tool_type;
  GType                tool_options_type;
  LigmaContextPropMask  context_props;

  gchar               *label;
  gchar               *tooltip;

  gchar               *menu_label;
  gchar               *menu_accel;

  gchar               *help_domain;
  gchar               *help_id;

  gboolean             hidden;
  gboolean             experimental;

  LigmaToolOptions     *tool_options;
  LigmaPaintInfo       *paint_info;

  LigmaContainer       *presets;
};

struct _LigmaToolInfoClass
{
  LigmaToolItemClass  parent_class;
};


GType          ligma_tool_info_get_type         (void) G_GNUC_CONST;

LigmaToolInfo * ligma_tool_info_new              (Ligma                *ligma,
                                                GType                tool_type,
                                                GType                tool_options_type,
                                                LigmaContextPropMask  context_props,
                                                const gchar         *identifier,
                                                const gchar         *label,
                                                const gchar         *tooltip,
                                                const gchar         *menu_label,
                                                const gchar         *menu_accel,
                                                const gchar         *help_domain,
                                                const gchar         *help_id,
                                                const gchar         *paint_core_name,
                                                const gchar         *icon_name);

void           ligma_tool_info_set_standard     (Ligma                *ligma,
                                                LigmaToolInfo        *tool_info);
LigmaToolInfo * ligma_tool_info_get_standard     (Ligma                *ligma);

gchar        * ligma_tool_info_get_action_name (LigmaToolInfo         *tool_info);

GFile        * ligma_tool_info_get_options_file (LigmaToolInfo        *tool_info,
                                                const gchar         *suffix);


#endif  /*  __LIGMA_TOOL_INFO_H__  */
