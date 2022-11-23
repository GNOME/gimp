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

#ifndef __LIGMA_LEVELS_TOOL_H__
#define __LIGMA_LEVELS_TOOL_H__


#include "ligmafiltertool.h"


#define LIGMA_TYPE_LEVELS_TOOL            (ligma_levels_tool_get_type ())
#define LIGMA_LEVELS_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LEVELS_TOOL, LigmaLevelsTool))
#define LIGMA_LEVELS_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LEVELS_TOOL, LigmaLevelsToolClass))
#define LIGMA_IS_LEVELS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LEVELS_TOOL))
#define LIGMA_IS_LEVELS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LEVELS_TOOL))
#define LIGMA_LEVELS_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LEVELS_TOOL, LigmaLevelsToolClass))


typedef struct _LigmaLevelsTool      LigmaLevelsTool;
typedef struct _LigmaLevelsToolClass LigmaLevelsToolClass;

struct _LigmaLevelsTool
{
  LigmaFilterTool  parent_instance;

  /* dialog */
  LigmaHistogram  *histogram;
  LigmaAsync      *histogram_async;

  GtkWidget      *channel_menu;

  GtkWidget      *histogram_view;

  GtkWidget      *input_bar;
  GtkWidget      *low_input_spinbutton;
  GtkWidget      *high_input_spinbutton;
  GtkWidget      *low_output_spinbutton;
  GtkWidget      *high_output_spinbutton;
  GtkAdjustment  *low_input;
  GtkAdjustment  *gamma;
  GtkAdjustment  *gamma_linear;
  GtkAdjustment  *high_input;

  GtkWidget      *output_bar;

  /* export dialog */
  gboolean        export_old_format;
};

struct _LigmaLevelsToolClass
{
  LigmaFilterToolClass  parent_class;
};


void    ligma_levels_tool_register (LigmaToolRegisterCallback  callback,
                                   gpointer                  data);

GType   ligma_levels_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __LIGMA_LEVELS_TOOL_H__  */
