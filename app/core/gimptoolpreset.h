/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_TOOL_PRESET_H__
#define __LIGMA_TOOL_PRESET_H__


#include "ligmadata.h"


#define LIGMA_TYPE_TOOL_PRESET            (ligma_tool_preset_get_type ())
#define LIGMA_TOOL_PRESET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_PRESET, LigmaToolPreset))
#define LIGMA_TOOL_PRESET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_PRESET, LigmaToolPresetClass))
#define LIGMA_IS_TOOL_PRESET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_PRESET))
#define LIGMA_IS_TOOL_PRESET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_PRESET))
#define LIGMA_TOOL_PRESET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_PRESET, LigmaToolPresetClass))


typedef struct _LigmaToolPresetClass LigmaToolPresetClass;

struct _LigmaToolPreset
{
  LigmaData         parent_instance;

  Ligma            *ligma;
  LigmaToolOptions *tool_options;

  gboolean         use_fg_bg;
  gboolean         use_opacity_paint_mode;
  gboolean         use_brush;
  gboolean         use_dynamics;
  gboolean         use_mybrush;
  gboolean         use_gradient;
  gboolean         use_pattern;
  gboolean         use_palette;
  gboolean         use_font;
};

struct _LigmaToolPresetClass
{
  LigmaDataClass  parent_class;
};


GType                 ligma_tool_preset_get_type      (void) G_GNUC_CONST;

LigmaData            * ligma_tool_preset_new           (LigmaContext    *context,
                                                      const gchar    *unused);

LigmaContextPropMask   ligma_tool_preset_get_prop_mask (LigmaToolPreset *preset);


#endif  /*  __LIGMA_TOOL_PRESET_H__  */
