/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppattern.h"
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"

#include "gimpclone.h"
#include "gimpcloneoptions.h"

#include "gimp-intl.h"


static gboolean   gimp_clone_start      (GimpPaintCore     *paint_core,
                                         GList             *drawables,
                                         GimpPaintOptions  *paint_options,
                                         const GimpCoords  *coords,
                                         GError           **error);

static void       gimp_clone_motion     (GimpSourceCore    *source_core,
                                         GimpDrawable      *drawable,
                                         GimpPaintOptions  *paint_options,
                                         const GimpCoords  *coords,
                                         GeglNode          *op,
                                         gdouble            opacity,
                                         GimpPickable      *src_pickable,
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

static gboolean   gimp_clone_use_source (GimpSourceCore    *source_core,
                                         GimpSourceOptions *options);


G_DEFINE_TYPE (GimpClone, gimp_clone, GIMP_TYPE_SOURCE_CORE)

#define parent_class gimp_clone_parent_class


void
gimp_clone_register (Gimp                      *gimp,
                     GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_CLONE,
                GIMP_TYPE_CLONE_OPTIONS,
                "gimp-clone",
                _("Clone"),
                "gimp-tool-clone");
}

static void
gimp_clone_class_init (GimpCloneClass *klass)
{
  GimpPaintCoreClass  *paint_core_class  = GIMP_PAINT_CORE_CLASS (klass);
  GimpSourceCoreClass *source_core_class = GIMP_SOURCE_CORE_CLASS (klass);

  paint_core_class->start       = gimp_clone_start;

  source_core_class->use_source = gimp_clone_use_source;
  source_core_class->motion     = gimp_clone_motion;
}

static void
gimp_clone_init (GimpClone *clone)
{
}

static gboolean
gimp_clone_start (GimpPaintCore     *paint_core,
                  GList             *drawables,
                  GimpPaintOptions  *paint_options,
                  const GimpCoords  *coords,
                  GError           **error)
{
  GimpCloneOptions *options = GIMP_CLONE_OPTIONS (paint_options);

  if (! GIMP_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawables,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (options->clone_type == GIMP_CLONE_PATTERN)
    {
      if (! gimp_context_get_pattern (GIMP_CONTEXT (options)))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("No patterns available for use with this tool."));
          return FALSE;
        }
    }

  return TRUE;
}

static void
gimp_clone_motion (GimpSourceCore   *source_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   const GimpCoords *coords,
                   GeglNode         *op,
                   gdouble           opacity,
                   GimpPickable     *src_pickable,
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
  GimpPaintCore     *paint_core     = GIMP_PAINT_CORE (source_core);
  GimpBrushCore     *brush_core     = GIMP_BRUSH_CORE (source_core);
  GimpCloneOptions  *options        = GIMP_CLONE_OPTIONS (paint_options);
  GimpSourceOptions *source_options = GIMP_SOURCE_OPTIONS (paint_options);
  GimpContext       *context        = GIMP_CONTEXT (paint_options);
  GimpDynamics      *dynamics       = brush_core->dynamics;
  GimpImage         *image          = gimp_item_get_image (GIMP_ITEM (drawable));
  gdouble            fade_point;
  gdouble            force;

  if (gimp_source_core_use_source (source_core, source_options))
    {
      if (! op)
        {
          gimp_gegl_buffer_copy (src_buffer,
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
          gimp_gegl_apply_operation (src_buffer, NULL, NULL, op,
                                     paint_buffer,
                                     GEGL_RECTANGLE (paint_area_offset_x,
                                                     paint_area_offset_y,
                                                     paint_area_width,
                                                     paint_area_height),
                                     FALSE);
        }
    }
  else if (options->clone_type == GIMP_CLONE_PATTERN)
    {
      GimpPattern *pattern    = gimp_context_get_pattern (context);
      GeglBuffer  *src_buffer = gimp_pattern_create_buffer (pattern);

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

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  if (gimp_dynamics_is_output_enabled (dynamics, GIMP_DYNAMICS_OUTPUT_FORCE))
    force = gimp_dynamics_get_linear_value (dynamics,
                                            GIMP_DYNAMICS_OUTPUT_FORCE,
                                            coords,
                                            paint_options,
                                            fade_point);
  else
    force = paint_options->brush_force;

  gimp_brush_core_paste_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                coords,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),
                                force,

                                /* In fixed mode, paint incremental so the
                                 * individual brushes are properly applied
                                 * on top of each other.
                                 * Otherwise the stuff we paint is seamless
                                 * and we don't need intermediate masking.
                                 */
                                source_options->align_mode ==
                                GIMP_SOURCE_ALIGN_FIXED ?
                                GIMP_PAINT_INCREMENTAL : GIMP_PAINT_CONSTANT);
}

static gboolean
gimp_clone_use_source (GimpSourceCore    *source_core,
                       GimpSourceOptions *options)
{
  return GIMP_CLONE_OPTIONS (options)->clone_type == GIMP_CLONE_IMAGE;
}
