/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmawarpoptions.h
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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

#ifndef __LIGMA_WARP_OPTIONS_H__
#define __LIGMA_WARP_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_WARP_OPTIONS            (ligma_warp_options_get_type ())
#define LIGMA_WARP_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_WARP_OPTIONS, LigmaWarpOptions))
#define LIGMA_WARP_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_WARP_OPTIONS, LigmaWarpOptionsClass))
#define LIGMA_IS_WARP_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_WARP_OPTIONS))
#define LIGMA_IS_WARP_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_WARP_OPTIONS))
#define LIGMA_WARP_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_WARP_OPTIONS, LigmaWarpOptionsClass))


typedef struct _LigmaWarpOptions      LigmaWarpOptions;
typedef struct _LigmaWarpOptionsClass LigmaWarpOptionsClass;

struct _LigmaWarpOptions
{
  LigmaToolOptions        parent_instance;

  LigmaWarpBehavior       behavior;
  gdouble                effect_size;
  gdouble                effect_hardness;
  gdouble                effect_strength;
  gdouble                stroke_spacing;
  LigmaInterpolationType  interpolation;
  GeglAbyssPolicy        abyss_policy;
  gboolean               high_quality_preview;
  gboolean               real_time_preview;

  gboolean               stroke_during_motion;
  gboolean               stroke_periodically;
  gdouble                stroke_periodically_rate;

  gint                   n_animation_frames;

  /*  options gui  */
  GtkWidget             *behavior_combo;
  GtkWidget             *stroke_frame;
  GtkWidget             *animate_button;
};

struct _LigmaWarpOptionsClass
{
  LigmaToolOptionsClass  parent_class;
};


GType       ligma_warp_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_warp_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_WARP_OPTIONS_H__  */
