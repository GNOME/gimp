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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "paint-types.h"

#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmadynamics.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmapattern.h"
#include "core/ligmapickable.h"
#include "core/ligmasymmetry.h"

#include "ligmaclone.h"
#include "ligmacloneoptions.h"

#include "ligma-intl.h"


static gboolean   ligma_clone_start      (LigmaPaintCore     *paint_core,
                                         GList             *drawables,
                                         LigmaPaintOptions  *paint_options,
                                         const LigmaCoords  *coords,
                                         GError           **error);

static void       ligma_clone_motion     (LigmaSourceCore    *source_core,
                                         LigmaDrawable      *drawable,
                                         LigmaPaintOptions  *paint_options,
                                         const LigmaCoords  *coords,
                                         GeglNode          *op,
                                         gdouble            opacity,
                                         LigmaPickable      *src_pickable,
                                         GeglBuffer        *src_buffer,
                                         GeglRectangle     *src_rect,
                                         gint               src_offset_x,
                                         gint               src_offset_y,
                                         GeglBuffer        *paint_buffer,
                                         gint               paint_buffer_x,
                                         gint               paint_buffer_y,
                                         gint               paint_area_offset_x,
                                         gint               paint_area_offset_y,
                                         gint               paint_area_width,
                                         gint               paint_area_height);

static gboolean   ligma_clone_use_source (LigmaSourceCore    *source_core,
                                         LigmaSourceOptions *options);


G_DEFINE_TYPE (LigmaClone, ligma_clone, LIGMA_TYPE_SOURCE_CORE)

#define parent_class ligma_clone_parent_class


void
ligma_clone_register (Ligma                      *ligma,
                     LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_CLONE,
                LIGMA_TYPE_CLONE_OPTIONS,
                "ligma-clone",
                _("Clone"),
                "ligma-tool-clone");
}

static void
ligma_clone_class_init (LigmaCloneClass *klass)
{
  LigmaPaintCoreClass  *paint_core_class  = LIGMA_PAINT_CORE_CLASS (klass);
  LigmaSourceCoreClass *source_core_class = LIGMA_SOURCE_CORE_CLASS (klass);

  paint_core_class->start       = ligma_clone_start;

  source_core_class->use_source = ligma_clone_use_source;
  source_core_class->motion     = ligma_clone_motion;
}

static void
ligma_clone_init (LigmaClone *clone)
{
}

static gboolean
ligma_clone_start (LigmaPaintCore     *paint_core,
                  GList             *drawables,
                  LigmaPaintOptions  *paint_options,
                  const LigmaCoords  *coords,
                  GError           **error)
{
  LigmaCloneOptions *options = LIGMA_CLONE_OPTIONS (paint_options);

  if (! LIGMA_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawables,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (options->clone_type == LIGMA_CLONE_PATTERN)
    {
      if (! ligma_context_get_pattern (LIGMA_CONTEXT (options)))
        {
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("No patterns available for use with this tool."));
          return FALSE;
        }
    }

  return TRUE;
}

static void
ligma_clone_motion (LigmaSourceCore   *source_core,
                   LigmaDrawable     *drawable,
                   LigmaPaintOptions *paint_options,
                   const LigmaCoords *coords,
                   GeglNode         *op,
                   gdouble           opacity,
                   LigmaPickable     *src_pickable,
                   GeglBuffer       *src_buffer,
                   GeglRectangle    *src_rect,
                   gint              src_offset_x,
                   gint              src_offset_y,
                   GeglBuffer       *paint_buffer,
                   gint              paint_buffer_x,
                   gint              paint_buffer_y,
                   gint              paint_area_offset_x,
                   gint              paint_area_offset_y,
                   gint              paint_area_width,
                   gint              paint_area_height)
{
  LigmaPaintCore     *paint_core     = LIGMA_PAINT_CORE (source_core);
  LigmaBrushCore     *brush_core     = LIGMA_BRUSH_CORE (source_core);
  LigmaCloneOptions  *options        = LIGMA_CLONE_OPTIONS (paint_options);
  LigmaSourceOptions *source_options = LIGMA_SOURCE_OPTIONS (paint_options);
  LigmaContext       *context        = LIGMA_CONTEXT (paint_options);
  LigmaDynamics      *dynamics       = brush_core->dynamics;
  LigmaImage         *image          = ligma_item_get_image (LIGMA_ITEM (drawable));
  gdouble            fade_point;
  gdouble            force;

  if (ligma_source_core_use_source (source_core, source_options))
    {
      if (! op)
        {
          ligma_gegl_buffer_copy (src_buffer,
                                 GEGL_RECTANGLE (src_rect->x,
                                                 src_rect->y,
                                                 paint_area_width,
                                                 paint_area_height),
                                 GEGL_ABYSS_NONE,
                                 paint_buffer,
                                 GEGL_RECTANGLE (paint_area_offset_x,
                                                 paint_area_offset_y,
                                                 0, 0));
        }
      else
        {
          ligma_gegl_apply_operation (src_buffer, NULL, NULL, op,
                                     paint_buffer,
                                     GEGL_RECTANGLE (paint_area_offset_x,
                                                     paint_area_offset_y,
                                                     paint_area_width,
                                                     paint_area_height),
                                     FALSE);
        }
    }
  else if (options->clone_type == LIGMA_CLONE_PATTERN)
    {
      LigmaPattern *pattern    = ligma_context_get_pattern (context);
      GeglBuffer  *src_buffer = ligma_pattern_create_buffer (pattern);

      src_offset_x += gegl_buffer_get_width (src_buffer) / 2;
      src_offset_y += gegl_buffer_get_height (src_buffer) / 2;

      gegl_buffer_set_pattern (paint_buffer,
                               GEGL_RECTANGLE (paint_area_offset_x,
                                               paint_area_offset_y,
                                               paint_area_width,
                                               paint_area_height),
                               src_buffer,
                               - paint_buffer_x - src_offset_x,
                               - paint_buffer_y - src_offset_y);

      g_object_unref (src_buffer);
    }
  else
    {
      g_return_if_reached ();
    }

  fade_point = ligma_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  if (ligma_dynamics_is_output_enabled (dynamics, LIGMA_DYNAMICS_OUTPUT_FORCE))
    force = ligma_dynamics_get_linear_value (dynamics,
                                            LIGMA_DYNAMICS_OUTPUT_FORCE,
                                            coords,
                                            paint_options,
                                            fade_point);
  else
    force = paint_options->brush_force;

  ligma_brush_core_paste_canvas (LIGMA_BRUSH_CORE (paint_core), drawable,
                                coords,
                                MIN (opacity, LIGMA_OPACITY_OPAQUE),
                                ligma_context_get_opacity (context),
                                ligma_context_get_paint_mode (context),
                                ligma_paint_options_get_brush_mode (paint_options),
                                force,

                                /* In fixed mode, paint incremental so the
                                 * individual brushes are properly applied
                                 * on top of each other.
                                 * Otherwise the stuff we paint is seamless
                                 * and we don't need intermediate masking.
                                 */
                                source_options->align_mode ==
                                LIGMA_SOURCE_ALIGN_FIXED ?
                                LIGMA_PAINT_INCREMENTAL : LIGMA_PAINT_CONSTANT);
}

static gboolean
ligma_clone_use_source (LigmaSourceCore    *source_core,
                       LigmaSourceOptions *options)
{
  return LIGMA_CLONE_OPTIONS (options)->clone_type == LIGMA_CLONE_IMAGE;
}
