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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-apply-operation.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimp-utils.h"

#include "gimpperspectiveclone.h"
#include "gimpperspectivecloneoptions.h"

#include "gimp-intl.h"


static gboolean     gimp_perspective_clone_start      (GimpPaintCore    *paint_core,
                                                       GimpDrawable     *drawable,
                                                       GimpPaintOptions *paint_options,
                                                       const GimpCoords *coords,
                                                       GError          **error);
static void         gimp_perspective_clone_paint      (GimpPaintCore    *paint_core,
                                                       GimpDrawable     *drawable,
                                                       GimpPaintOptions *paint_options,
                                                       const GimpCoords *coords,
                                                       GimpPaintState    paint_state,
                                                       guint32           time);

static GeglBuffer * gimp_perspective_clone_get_source (GimpSourceCore   *source_core,
                                                       GimpDrawable     *drawable,
                                                       GimpPaintOptions *paint_options,
                                                       GimpPickable     *src_pickable,
                                                       gint              src_offset_x,
                                                       gint              src_offset_y,
                                                       TempBuf          *paint_area,
                                                       gint             *paint_area_offset_x,
                                                       gint             *paint_area_offset_y,
                                                       gint             *paint_area_width,
                                                       gint             *paint_area_height,
                                                       GeglRectangle    *src_rect);

static void         gimp_perspective_clone_get_matrix (GimpPerspectiveClone *clone,
                                                       GimpMatrix3          *matrix);


G_DEFINE_TYPE (GimpPerspectiveClone, gimp_perspective_clone,
               GIMP_TYPE_CLONE)

#define parent_class gimp_perspective_clone_parent_class


void
gimp_perspective_clone_register (Gimp                      *gimp,
                                 GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PERSPECTIVE_CLONE,
                GIMP_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                "gimp-perspective-clone",
                _("Perspective Clone"),
                "gimp-tool-perspective-clone");
}

static void
gimp_perspective_clone_class_init (GimpPerspectiveCloneClass *klass)
{
  GimpPaintCoreClass  *paint_core_class  = GIMP_PAINT_CORE_CLASS (klass);
  GimpSourceCoreClass *source_core_class = GIMP_SOURCE_CORE_CLASS (klass);

  paint_core_class->start       = gimp_perspective_clone_start;
  paint_core_class->paint       = gimp_perspective_clone_paint;

  source_core_class->get_source = gimp_perspective_clone_get_source;
}

static void
gimp_perspective_clone_init (GimpPerspectiveClone *clone)
{
  clone->src_x_fv  = 0.0;    /* source coords in front_view perspective */
  clone->src_y_fv  = 0.0;

  clone->dest_x_fv = 0.0;    /* destination coords in front_view perspective */
  clone->dest_y_fv = 0.0;

  gimp_matrix3_identity (&clone->transform);
  gimp_matrix3_identity (&clone->transform_inv);
}

static gboolean
gimp_perspective_clone_start (GimpPaintCore     *paint_core,
                              GimpDrawable      *drawable,
                              GimpPaintOptions  *paint_options,
                              const GimpCoords  *coords,
                              GError           **error)
{
  GimpSourceCore *source_core = GIMP_SOURCE_CORE (paint_core);

  if (! GIMP_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawable,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (! source_core->set_source && gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Perspective Clone does not operate on "
			     "indexed layers."));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_perspective_clone_paint (GimpPaintCore    *paint_core,
                              GimpDrawable     *drawable,
                              GimpPaintOptions *paint_options,
                              const GimpCoords *coords,
                              GimpPaintState    paint_state,
                              guint32           time)
{
  GimpSourceCore       *source_core = GIMP_SOURCE_CORE (paint_core);
  GimpPerspectiveClone *clone       = GIMP_PERSPECTIVE_CLONE (paint_core);
  GimpSourceOptions    *options     = GIMP_SOURCE_OPTIONS (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (source_core->set_source)
        {
          g_object_set (source_core, "src-drawable", drawable, NULL);

          source_core->src_x = coords->x;
          source_core->src_y = coords->y;

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        source_core->src_x,
                                        source_core->src_y,
                                        &clone->src_x_fv,
                                        &clone->src_y_fv);

          source_core->first_stroke = TRUE;
        }
      else
        {
          GimpPickable *src_pickable;
          GimpImage    *src_image;
          GimpImage    *dest_image;
          GeglBuffer   *orig_buffer;

          if (options->align_mode == GIMP_SOURCE_ALIGN_NO)
            {
              source_core->orig_src_x = source_core->src_x;
              source_core->orig_src_y = source_core->src_y;

              source_core->first_stroke = TRUE;
            }

          /*  If the source image is different from the destination,
           *  then we should copy straight from the source image
           *  to the canvas.
           *  Otherwise, we need a call to get_orig_image to make sure
           *  we get a copy of the unblemished (offset) image
           */
          src_pickable = GIMP_PICKABLE (source_core->src_drawable);
          src_image    = gimp_pickable_get_image (src_pickable);

          if (options->sample_merged)
            src_pickable = GIMP_PICKABLE (gimp_image_get_projection (src_image));

          dest_image = gimp_item_get_image (GIMP_ITEM (drawable));

          if ((options->sample_merged &&
               (src_image != dest_image)) ||
              (! options->sample_merged &&
               (source_core->src_drawable != drawable)))
            {
              orig_buffer = gimp_pickable_get_buffer (src_pickable);
            }
          else
            {
              if (options->sample_merged)
                orig_buffer = gimp_paint_core_get_orig_proj (paint_core);
              else
                orig_buffer = gimp_paint_core_get_orig_image (paint_core);
            }

          clone->node = gegl_node_new ();

          g_object_set (clone->node,
                        "dont-cache", TRUE,
                        NULL);

          clone->src_node =
            gegl_node_new_child (clone->node,
                                 "operation", "gegl:buffer-source",
                                 "buffer",    orig_buffer,
                                  NULL);

          clone->transform_node =
            gegl_node_new_child (clone->node,
                                 "operation",  "gegl:transform",
                                 "filter",     gimp_interpolation_to_gegl_filter (GIMP_INTERPOLATION_LINEAR),
                                 "hard-edges", TRUE,
                                 NULL);

          clone->dest_node =
            gegl_node_new_child (clone->node,
                                 "operation", "gegl:write-buffer",
                                 NULL);

          gegl_node_link_many (clone->src_node,
                               clone->transform_node,
                               clone->dest_node,
                               NULL);

          clone->processor = gegl_node_new_processor (clone->dest_node, NULL);
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (source_core->set_source)
        {
          /*  If the control key is down, move the src target and return */

          source_core->src_x = coords->x;
          source_core->src_y = coords->y;

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        source_core->src_x,
                                        source_core->src_y,
                                        &clone->src_x_fv,
                                        &clone->src_y_fv);

          source_core->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint dest_x;
          gint dest_y;

          dest_x = coords->x;
          dest_y = coords->y;

          if (options->align_mode == GIMP_SOURCE_ALIGN_REGISTERED)
            {
              source_core->offset_x = 0;
              source_core->offset_y = 0;
            }
          else if (options->align_mode == GIMP_SOURCE_ALIGN_FIXED)
            {
              source_core->offset_x = source_core->src_x - dest_x;
              source_core->offset_y = source_core->src_y - dest_y;
            }
          else if (source_core->first_stroke)
            {
              source_core->offset_x = source_core->src_x - dest_x;
              source_core->offset_y = source_core->src_y - dest_y;

              /* get destination coordinates in front view perspective */
              gimp_matrix3_transform_point (&clone->transform_inv,
                                            dest_x,
                                            dest_y,
                                            &clone->dest_x_fv,
                                            &clone->dest_y_fv);

              source_core->first_stroke = FALSE;
            }

          gimp_source_core_motion (source_core, drawable, paint_options, coords);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (clone->node)
        {
          g_object_unref (clone->node);
          g_object_unref (clone->processor);
          clone->node = NULL;
          clone->src_node = NULL;
          clone->transform_node = NULL;
          clone->dest_node = NULL;
          clone->processor = NULL;
        }
      break;

    default:
      break;
    }

  g_object_notify (G_OBJECT (clone), "src-x");
  g_object_notify (G_OBJECT (clone), "src-y");
}

static GeglBuffer *
gimp_perspective_clone_get_source (GimpSourceCore   *source_core,
                                   GimpDrawable     *drawable,
                                   GimpPaintOptions *paint_options,
                                   GimpPickable     *src_pickable,
                                   gint              src_offset_x,
                                   gint              src_offset_y,
                                   TempBuf          *paint_area,
                                   gint             *paint_area_offset_x,
                                   gint             *paint_area_offset_y,
                                   gint             *paint_area_width,
                                   gint             *paint_area_height,
                                   GeglRectangle    *src_rect)
{
  GimpPerspectiveClone *clone = GIMP_PERSPECTIVE_CLONE (source_core);
  GeglBuffer           *src_buffer;
  GeglBuffer           *dest_buffer;
  const Babl           *src_format_alpha;
  gint                  x1d, y1d, x2d, y2d;
  gdouble               x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s;
  gint                  xmin, ymin, xmax, ymax;
  GimpMatrix3           matrix;
  gchar                *matrix_string;
  GimpMatrix3           gegl_matrix;

  src_buffer       = gimp_pickable_get_buffer (src_pickable);
  src_format_alpha = gimp_pickable_get_format_with_alpha (src_pickable);

  /* Destination coordinates that will be painted */
  x1d = paint_area->x;
  y1d = paint_area->y;
  x2d = paint_area->x + paint_area->width;
  y2d = paint_area->y + paint_area->height;

  /* Boundary box for source pixels to copy: Convert all the vertex of
   * the box to paint in destination area to its correspondent in
   * source area bearing in mind perspective
   */
  gimp_perspective_clone_get_source_point (clone, x1d, y1d, &x1s, &y1s);
  gimp_perspective_clone_get_source_point (clone, x1d, y2d, &x2s, &y2s);
  gimp_perspective_clone_get_source_point (clone, x2d, y1d, &x3s, &y3s);
  gimp_perspective_clone_get_source_point (clone, x2d, y2d, &x4s, &y4s);

  xmin = floor (MIN4 (x1s, x2s, x3s, x4s));
  ymin = floor (MIN4 (y1s, y2s, y3s, y4s));
  xmax = ceil  (MAX4 (x1s, x2s, x3s, x4s));
  ymax = ceil  (MAX4 (y1s, y2s, y3s, y4s));

  if (! gimp_rectangle_intersect (xmin, ymin,
                                  xmax - xmin, ymax - ymin,
                                  0, 0,
                                  gegl_buffer_get_width  (src_buffer),
                                  gegl_buffer_get_height (src_buffer),
                                  NULL, NULL, NULL, NULL))
    {
      /* if the source area is completely out of the image */
      return FALSE;
    }

  dest_buffer = gegl_buffer_new (GIMP_GEGL_RECT (0, 0, x2d - x1d, y2d - y1d),
                                 src_format_alpha);
  gegl_buffer_clear (dest_buffer, NULL);

  gimp_perspective_clone_get_matrix (clone, &matrix);

  gimp_matrix3_identity (&gegl_matrix);
  gimp_matrix3_mult (&matrix, &gegl_matrix);
  gimp_matrix3_translate (&gegl_matrix, -x1d, -y1d);

  matrix_string = gegl_matrix3_to_string ((GeglMatrix3 *) &gegl_matrix);
  gegl_node_set (clone->transform_node,
                 "transform", matrix_string,
                 NULL);
  g_free (matrix_string);

  gegl_node_set (clone->dest_node,
                 "buffer", dest_buffer,
                 NULL);

  gegl_processor_set_rectangle (clone->processor,
                                GIMP_GEGL_RECT (0, 0,
                                                x2d - x1d, y2d - y1d));
  while (gegl_processor_work (clone->processor, NULL));

  *src_rect = *GIMP_GEGL_RECT (0, 0, x2d - x1d, y2d - y1d);

  return dest_buffer;
}


/*  public functions  */

void
gimp_perspective_clone_set_transform (GimpPerspectiveClone *clone,
                                      GimpMatrix3          *transform)
{
  g_return_if_fail (GIMP_IS_PERSPECTIVE_CLONE (clone));
  g_return_if_fail (transform != NULL);

  clone->transform = *transform;

  clone->transform_inv = clone->transform;
  gimp_matrix3_invert (&clone->transform_inv);

#if 0
  g_printerr ("%f\t%f\t%f\n%f\t%f\t%f\n%f\t%f\t%f\n\n",
              clone->transform.coeff[0][0],
              clone->transform.coeff[0][1],
              clone->transform.coeff[0][2],
              clone->transform.coeff[1][0],
              clone->transform.coeff[1][1],
              clone->transform.coeff[1][2],
              clone->transform.coeff[2][0],
              clone->transform.coeff[2][1],
              clone->transform.coeff[2][2]);
#endif
}

void
gimp_perspective_clone_get_source_point (GimpPerspectiveClone *clone,
                                         gdouble               x,
                                         gdouble               y,
                                         gdouble              *newx,
                                         gdouble              *newy)
{
  gdouble temp_x, temp_y;

  g_return_if_fail (GIMP_IS_PERSPECTIVE_CLONE (clone));
  g_return_if_fail (newx != NULL);
  g_return_if_fail (newy != NULL);

  gimp_matrix3_transform_point (&clone->transform_inv,
                                x, y, &temp_x, &temp_y);

#if 0
  /* Get the offset of each pixel in destination area from the
   * destination pixel in front view perspective
   */
  offset_x_fv = temp_x - clone->dest_x_fv;
  offset_y_fv = temp_y - clone->dest_y_fv;

  /* Get the source pixel in front view perspective */
  temp_x = offset_x_fv + clone->src_x_fv;
  temp_y = offset_y_fv + clone->src_y_fv;
#endif

  temp_x += clone->src_x_fv - clone->dest_x_fv;
  temp_y += clone->src_y_fv - clone->dest_y_fv;

  /* Convert the source pixel to perspective view */
  gimp_matrix3_transform_point (&clone->transform,
                                temp_x, temp_y, newx, newy);
}


/*  private functions  */

static void
gimp_perspective_clone_get_matrix (GimpPerspectiveClone *clone,
                                   GimpMatrix3          *matrix)
{
  GimpMatrix3 temp;

  gimp_matrix3_identity (&temp);
  gimp_matrix3_translate (&temp,
                          clone->dest_x_fv - clone->src_x_fv,
                          clone->dest_y_fv - clone->src_y_fv);

  *matrix = clone->transform_inv;
  gimp_matrix3_mult (&temp, matrix);
  gimp_matrix3_mult (&clone->transform, matrix);
}
