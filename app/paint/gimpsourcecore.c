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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"

#include "gimpsourcecore.h"
#include "gimpsourceoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
};


static gboolean gimp_source_core_start           (GimpPaintCore     *paint_core,
                                                  GList             *drawables,
                                                  GimpPaintOptions  *paint_options,
                                                  const GimpCoords  *coords,
                                                  GError           **error);
static void     gimp_source_core_paint           (GimpPaintCore     *paint_core,
                                                  GList             *drawables,
                                                  GimpPaintOptions  *paint_options,
                                                  GimpSymmetry      *sym,
                                                  GimpPaintState     paint_state,
                                                  guint32            time);

#if 0
static void     gimp_source_core_motion          (GimpSourceCore    *source_core,
                                                  GimpDrawable      *drawable,
                                                  GimpPaintOptions  *paint_options,
                                                  gboolean           self_drawable,
                                                  GimpSymmetry      *sym);
#endif

static gboolean gimp_source_core_real_use_source (GimpSourceCore    *source_core,
                                                  GimpSourceOptions *options);
static GeglBuffer *
                gimp_source_core_real_get_source (GimpSourceCore    *source_core,
                                                  GimpDrawable      *drawable,
                                                  GimpPaintOptions  *paint_options,
                                                  gboolean           self_drawable,
                                                  GimpPickable      *src_pickable,
                                                  gint               src_offset_x,
                                                  gint               src_offset_y,
                                                  GeglBuffer        *paint_buffer,
                                                  gint               paint_buffer_x,
                                                  gint               paint_buffer_y,
                                                  gint              *paint_area_offset_x,
                                                  gint              *paint_area_offset_y,
                                                  gint              *paint_area_width,
                                                  gint              *paint_area_height,
                                                  GeglRectangle     *src_rect);


G_DEFINE_TYPE (GimpSourceCore, gimp_source_core, GIMP_TYPE_BRUSH_CORE)

#define parent_class gimp_source_core_parent_class


static void
gimp_source_core_class_init (GimpSourceCoreClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  paint_core_class->start                  = gimp_source_core_start;
  paint_core_class->paint                  = gimp_source_core_paint;

  brush_core_class->handles_changing_brush = TRUE;

  klass->use_source                        = gimp_source_core_real_use_source;
  klass->get_source                        = gimp_source_core_real_get_source;
  klass->motion                            = NULL;
}

static void
gimp_source_core_init (GimpSourceCore *source_core)
{
  source_core->set_source    = FALSE;

  source_core->orig_src_x    = 0;
  source_core->orig_src_y    = 0;

  source_core->offset_x      = 0;
  source_core->offset_y      = 0;
  source_core->first_stroke  = TRUE;
}

static gboolean
gimp_source_core_start (GimpPaintCore     *paint_core,
                        GList             *drawables,
                        GimpPaintOptions  *paint_options,
                        const GimpCoords  *coords,
                        GError           **error)
{
  GimpSourceCore    *source_core = GIMP_SOURCE_CORE (paint_core);
  GimpSourceOptions *options     = GIMP_SOURCE_OPTIONS (paint_options);

  if (! GIMP_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawables,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  paint_core->use_saved_proj = FALSE;

  if (! source_core->set_source &&
      gimp_source_core_use_source (source_core, options))
    {
      if (! options->src_drawables)
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("Set a source image first."));
          return FALSE;
        }
      else if (options->align_mode == GIMP_SOURCE_ALIGN_REGISTERED &&
               g_list_length (drawables) > 1)
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("\"Registered\" alignment cannot paint on multiple drawables."));
          return FALSE;
        }

      if (options->sample_merged         &&
          g_list_length (drawables) == 1 &&
          gimp_item_get_image (GIMP_ITEM (options->src_drawables->data)) ==
          gimp_item_get_image (GIMP_ITEM (drawables->data)))
        {
          paint_core->use_saved_proj = TRUE;
        }
    }

  return TRUE;
}

static void
gimp_source_core_paint (GimpPaintCore    *paint_core,
                        GList            *drawables,
                        GimpPaintOptions *paint_options,
                        GimpSymmetry     *sym,
                        GimpPaintState    paint_state,
                        guint32           time)
{
  GimpSourceCore    *source_core = GIMP_SOURCE_CORE (paint_core);
  GimpSourceOptions *options     = GIMP_SOURCE_OPTIONS (paint_options);
  const GimpCoords  *coords;

  /* The source is based on the original stroke */
  coords = gimp_symmetry_get_origin (sym);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (source_core->set_source)
        {
          g_object_set (options,
                        "src-drawables", drawables,
                        "src-x",         (gint) floor (coords->x),
                        "src-y",         (gint) floor (coords->y),
                        NULL);

          /* FIXME(?): subpixel source sampling */

          source_core->first_stroke = TRUE;
        }
      else if (options->align_mode == GIMP_SOURCE_ALIGN_NO)
        {
          g_object_get (options,
                        "src-x", &source_core->orig_src_x,
                        "src-y", &source_core->orig_src_y,
                        NULL);

          source_core->first_stroke = TRUE;
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (source_core->set_source)
        {
          /*  If the control key is down, move the src target and return */

          g_object_set (options,
                        "src-drawables", drawables,
                        "src-x",         (gint) floor (coords->x),
                        "src-y",         (gint) floor (coords->y),
                        NULL);

          source_core->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint src_x;
          gint src_y;

          gint dest_x;
          gint dest_y;

          dest_x = floor (coords->x);
          dest_y = floor (coords->y);

          g_object_get (options,
                        "src-x", &src_x,
                        "src-y", &src_y,
                        NULL);

          if (options->align_mode == GIMP_SOURCE_ALIGN_REGISTERED)
            {
              source_core->offset_x = 0;
              source_core->offset_y = 0;
            }
          else if (options->align_mode == GIMP_SOURCE_ALIGN_FIXED)
            {
              source_core->offset_x = src_x - dest_x;
              source_core->offset_y = src_y - dest_y;
            }
          else if (source_core->first_stroke)
            {
              source_core->offset_x = src_x - dest_x;
              source_core->offset_y = src_y - dest_y;

              source_core->first_stroke = FALSE;
            }

          g_object_set (options,
                        "src-x", dest_x + source_core->offset_x,
                        "src-y", dest_y + source_core->offset_y,
                        NULL);


          for (GList *iter = drawables; iter; iter = iter->next)
            gimp_source_core_motion (source_core, iter->data,
                                     paint_options,
                                     (g_list_length (drawables) > 1),
                                     sym);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (options->align_mode == GIMP_SOURCE_ALIGN_NO &&
          ! source_core->first_stroke)
        {
          g_object_set (options,
                        "src-x", source_core->orig_src_x,
                        "src-y", source_core->orig_src_y,
                        NULL);

        }
      break;

    default:
      break;
    }
}

void
gimp_source_core_motion (GimpSourceCore   *source_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options,
                         gboolean          self_drawable,
                         GimpSymmetry     *sym)

{
  GimpPaintCore     *paint_core   = GIMP_PAINT_CORE (source_core);
  GimpBrushCore     *brush_core   = GIMP_BRUSH_CORE (source_core);
  GimpSourceOptions *options      = GIMP_SOURCE_OPTIONS (paint_options);
  GimpDynamics      *dynamics     = GIMP_BRUSH_CORE (paint_core)->dynamics;
  GimpImage         *image        = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpPickable      *src_pickable = NULL;
  GeglBuffer        *src_buffer   = NULL;
  GeglRectangle      src_rect;
  gint               base_src_offset_x;
  gint               base_src_offset_y;
  gint               src_offset_x;
  gint               src_offset_y;
  GeglBuffer        *paint_buffer;
  gint               paint_buffer_x;
  gint               paint_buffer_y;
  gint               paint_area_offset_x;
  gint               paint_area_offset_y;
  gint               paint_area_width;
  gint               paint_area_height;
  gdouble            fade_point;
  gdouble            opacity;
  GimpLayerMode      paint_mode;
  GeglNode          *op;
  GimpCoords         origin;
  GimpCoords         coords;
  gint               src_off_x = 0;
  gint               src_off_y = 0;
  gint               src_x, src_y;
  gint               off_x, off_y;
  gint               n_strokes;
  gint               i;

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  origin = *(gimp_symmetry_get_origin (sym));

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  coords    = origin;
  coords.x -= off_x;
  coords.y -= off_y;
  gimp_symmetry_set_origin (sym, drawable, &coords);
  paint_core->sym = sym;

  /* Some settings are based on the original stroke. */
  opacity = gimp_dynamics_get_linear_value (dynamics,
                                            GIMP_DYNAMICS_OUTPUT_OPACITY,
                                            &origin,
                                            paint_options,
                                            fade_point);
  if (opacity == 0.0)
    return;

  base_src_offset_x = source_core->offset_x;
  base_src_offset_y = source_core->offset_y;

  if (gimp_source_core_use_source (source_core, options))
    {
      if (self_drawable)
        {
          src_pickable = GIMP_PICKABLE (drawable);
        }
      else if (options->sample_merged)
        {
          GimpImage *src_image;

          src_image = gimp_pickable_get_image (options->src_drawables->data);

          if (! gimp_paint_core_get_show_all (paint_core))
            {
              src_pickable = GIMP_PICKABLE (src_image);
            }
          else
            {
              src_pickable = GIMP_PICKABLE (gimp_image_get_projection (src_image));
            }
        }
      else
        {
          src_pickable = options->src_pickable;
        }

      if (GIMP_IS_ITEM (src_pickable))
        gimp_item_get_offset (GIMP_ITEM (src_pickable),
                              &src_off_x, &src_off_y);
    }

  g_object_get (options,
                "src-x", &src_x,
                "src-y", &src_y,
                NULL);

  gimp_brush_core_eval_transform_dynamics (brush_core,
                                           image,
                                           paint_options,
                                           &origin);

  paint_mode = gimp_context_get_paint_mode (GIMP_CONTEXT (paint_options));

  n_strokes  = gimp_symmetry_get_size (sym);
  for (i = 0; i < n_strokes; i++)
    {
      coords    = *(gimp_symmetry_get_coords (sym, i));

      gimp_brush_core_eval_transform_symmetry (brush_core, sym, i);

      paint_buffer = gimp_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       paint_mode,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       NULL, NULL);
      if (! paint_buffer)
        continue;

      paint_area_offset_x = 0;
      paint_area_offset_y = 0;
      paint_area_width    = gegl_buffer_get_width  (paint_buffer);
      paint_area_height   = gegl_buffer_get_height (paint_buffer);

      src_offset_x = base_src_offset_x;
      src_offset_y = base_src_offset_y;
      if (gimp_source_core_use_source (source_core, options))
        {
          /* When using a source, use the same for every stroke. */
          src_offset_x += floor (origin.x) - floor (coords.x) - src_off_x;
          src_offset_y += floor (origin.y) - floor (coords.y) - src_off_y;
          src_buffer =
            GIMP_SOURCE_CORE_GET_CLASS (source_core)->get_source (source_core,
                                                                  drawable,
                                                                  paint_options,
                                                                  self_drawable,
                                                                  src_pickable,
                                                                  src_offset_x,
                                                                  src_offset_y,
                                                                  paint_buffer,
                                                                  paint_buffer_x,
                                                                  paint_buffer_y,
                                                                  &paint_area_offset_x,
                                                                  &paint_area_offset_y,
                                                                  &paint_area_width,
                                                                  &paint_area_height,
                                                                  &src_rect);

          if (! src_buffer)
            continue;
        }

      /*  Set the paint buffer to transparent  */
      gegl_buffer_clear (paint_buffer, NULL);

      op = gimp_symmetry_get_operation (sym, i);

      if (op)
        {
          GeglNode *node;
          GeglNode *input;
          GeglNode *translate_before;
          GeglNode *translate_after;
          GeglNode *output;

          node = gegl_node_new ();

          input = gegl_node_get_input_proxy (node, "input");

          translate_before = gegl_node_new_child (
            node,
            "operation", "gegl:translate",
            "x",         -((gdouble) src_x + 0.5),
            "y",         -((gdouble) src_y + 0.5),
            NULL);

          gegl_node_add_child (node, op);

          translate_after = gegl_node_new_child (
            node,
            "operation", "gegl:translate",
            "x",         ((gdouble) src_x + 0.5) +
                         (paint_area_offset_x - src_rect.x),
            "y",         ((gdouble) src_y + 0.5) +
                         (paint_area_offset_y - src_rect.y),
            NULL);

          output = gegl_node_get_output_proxy (node, "output");

          gegl_node_link_many (input,
                               translate_before,
                               op,
                               translate_after,
                               output,
                               NULL);

          g_object_unref (op);

          op = node;
        }

      GIMP_SOURCE_CORE_GET_CLASS (source_core)->motion (source_core,
                                                        drawable,
                                                        paint_options,
                                                        &coords,
                                                        op,
                                                        opacity,
                                                        src_pickable,
                                                        src_buffer,
                                                        &src_rect,
                                                        src_offset_x,
                                                        src_offset_y,
                                                        paint_buffer,
                                                        paint_buffer_x,
                                                        paint_buffer_y,
                                                        paint_area_offset_x,
                                                        paint_area_offset_y,
                                                        paint_area_width,
                                                        paint_area_height);

      g_clear_object (&op);

      g_clear_object (&src_buffer);
    }
}

gboolean
gimp_source_core_use_source (GimpSourceCore    *source_core,
                             GimpSourceOptions *options)
{
  return GIMP_SOURCE_CORE_GET_CLASS (source_core)->use_source (source_core,
                                                               options);
}

static gboolean
gimp_source_core_real_use_source (GimpSourceCore    *source_core,
                                  GimpSourceOptions *options)
{
  return TRUE;
}

static GeglBuffer *
gimp_source_core_real_get_source (GimpSourceCore   *source_core,
                                  GimpDrawable     *drawable,
                                  GimpPaintOptions *paint_options,
                                  gboolean          self_drawable,
                                  GimpPickable     *src_pickable,
                                  gint              src_offset_x,
                                  gint              src_offset_y,
                                  GeglBuffer       *paint_buffer,
                                  gint              paint_buffer_x,
                                  gint              paint_buffer_y,
                                  gint             *paint_area_offset_x,
                                  gint             *paint_area_offset_y,
                                  gint             *paint_area_width,
                                  gint             *paint_area_height,
                                  GeglRectangle    *src_rect)
{
  GimpSourceOptions *options    = GIMP_SOURCE_OPTIONS (paint_options);
  GimpImage         *image      = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpImage         *src_image  = gimp_pickable_get_image (src_pickable);
  GeglBuffer        *src_buffer = gimp_pickable_get_buffer (src_pickable);
  GeglBuffer        *dest_buffer;
  gboolean           sample_merged;
  gint               x, y;
  gint               width, height;

  /* As a special case, we bypass sample merged value when we request
   * each drawable to be its own source.
   */
  sample_merged = options->sample_merged && (! self_drawable);

  if (! gimp_rectangle_intersect (paint_buffer_x + src_offset_x,
                                  paint_buffer_y + src_offset_y,
                                  gegl_buffer_get_width  (paint_buffer),
                                  gegl_buffer_get_height (paint_buffer),
                                  gegl_buffer_get_x      (src_buffer),
                                  gegl_buffer_get_y      (src_buffer),
                                  gegl_buffer_get_width  (src_buffer),
                                  gegl_buffer_get_height (src_buffer),
                                  &x, &y,
                                  &width, &height))
    {
      return FALSE;
    }

  /*  If the source image is different from the destination,
   *  then we should copy straight from the source image
   *  to the canvas.
   *  Otherwise, we need a call to get_orig_image to make sure
   *  we get a copy of the unblemished (offset) image
   */
  if ((  sample_merged && (src_image                 != image)) ||
      (! sample_merged && (g_list_length (options->src_drawables) != 1 ||
                           options->src_drawables->data != drawable)))
    {
      dest_buffer = src_buffer;
    }
  else
    {
      /*  get the original image  */
      if (sample_merged)
        dest_buffer = gimp_paint_core_get_orig_proj (GIMP_PAINT_CORE (source_core));
      else
        dest_buffer = gimp_paint_core_get_orig_image (GIMP_PAINT_CORE (source_core),
                                                      drawable);
    }

  *paint_area_offset_x = x - (paint_buffer_x + src_offset_x);
  *paint_area_offset_y = y - (paint_buffer_y + src_offset_y);
  *paint_area_width    = width;
  *paint_area_height   = height;

  *src_rect = *GEGL_RECTANGLE (x, y, width, height);

  return g_object_ref (dest_buffer);
}
