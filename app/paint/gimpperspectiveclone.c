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
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "gegl/gimp-gegl-nodes.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimppattern.h"
#include "core/gimppickable.h"
#include "core/gimpsymmetry.h"

#include "gimpperspectiveclone.h"
#include "gimpperspectivecloneoptions.h"

#include "gimp-intl.h"


static void         gimp_perspective_clone_paint      (GimpPaintCore     *paint_core,
                                                       GList             *drawables,
                                                       GimpPaintOptions  *paint_options,
                                                       GimpSymmetry      *sym,
                                                       GimpPaintState     paint_state,
                                                       guint32            time);

static gboolean     gimp_perspective_clone_use_source (GimpSourceCore    *source_core,
                                                       GimpSourceOptions *options);
static GeglBuffer * gimp_perspective_clone_get_source (GimpSourceCore    *source_core,
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

  paint_core_class->paint       = gimp_perspective_clone_paint;

  source_core_class->use_source = gimp_perspective_clone_use_source;
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

static void
gimp_perspective_clone_paint (GimpPaintCore    *paint_core,
                              GList            *drawables,
                              GimpPaintOptions *paint_options,
                              GimpSymmetry     *sym,
                              GimpPaintState    paint_state,
                              guint32           time)
{
  GimpSourceCore       *source_core   = GIMP_SOURCE_CORE (paint_core);
  GimpPerspectiveClone *clone         = GIMP_PERSPECTIVE_CLONE (paint_core);
  GimpContext          *context       = GIMP_CONTEXT (paint_options);
  GimpCloneOptions     *clone_options = GIMP_CLONE_OPTIONS (paint_options);
  GimpSourceOptions    *options       = GIMP_SOURCE_OPTIONS (paint_options);
  const GimpCoords     *coords;
  gboolean              sample_merged;

  /* The source is based on the original stroke */
  coords        = gimp_symmetry_get_origin (sym);

  sample_merged = options->sample_merged && (g_list_length (drawables) > 1);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (source_core->set_source)
        {
          gint src_x = floor (coords->x);
          gint src_y = floor (coords->y);

          g_object_set (options,
                        "src-drawables", drawables,
                        "src-x",         src_x,
                        "src-y",         src_y,
                        NULL);

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        src_x,
                                        src_y,
                                        &clone->src_x_fv,
                                        &clone->src_y_fv);

          source_core->first_stroke = TRUE;
        }
      else
        {
          GimpImage  *del_image   = NULL;
          GeglBuffer *orig_buffer = NULL;
          GeglNode   *tile        = NULL;

          if (options->align_mode == GIMP_SOURCE_ALIGN_NO)
            {
              g_object_get (options,
                            "src-x", &source_core->orig_src_x,
                            "src-y", &source_core->orig_src_y,
                            NULL);

              source_core->first_stroke = TRUE;
            }

          clone->node = gegl_node_new ();

          g_object_set (clone->node,
                        "cache-policy", GEGL_CACHE_POLICY_NEVER,
                        NULL);

          switch (clone_options->clone_type)
            {
            case GIMP_CLONE_IMAGE:
              {
                GimpPickable *src_pickable = NULL;
                GimpImage    *src_image;
                GimpImage    *dest_image;

                /*  If the source image is different from the
                 *  destination, then we should copy straight from the
                 *  source image to the canvas.
                 *  Otherwise, we need a call to get_orig_image to make sure
                 *  we get a copy of the unblemished (offset) image
                 */
                src_image = gimp_pickable_get_image (options->src_drawables->data);

                if (sample_merged)
                  src_pickable = GIMP_PICKABLE (src_image);

                dest_image = gimp_item_get_image (GIMP_ITEM (drawables->data));

                if ((sample_merged &&
                     (src_image != dest_image)) ||
                    (! sample_merged                                 &&
                     g_list_length (options->src_drawables) == 1 &&
                     g_list_length (drawables) == 1                  &&
                     (options->src_drawables != drawables->data)))
                  {
                    if (! sample_merged)
                      src_pickable = GIMP_PICKABLE (options->src_drawables->data);

                    orig_buffer = gimp_pickable_get_buffer (src_pickable);
                  }
                else
                  {
                    if (sample_merged && paint_core->use_saved_proj)
                      orig_buffer = gimp_paint_core_get_orig_proj (paint_core);
                    else
                      orig_buffer = gimp_paint_core_get_orig_image (paint_core, drawables->data);
                  }

                if (! orig_buffer)
                  {
                    /* A composited image of the drawables */
                    del_image = gimp_image_new_from_drawables (src_image->gimp, drawables,
                                                               FALSE, FALSE);
                    gimp_container_remove (src_image->gimp->images, GIMP_OBJECT (del_image));

                    src_pickable = GIMP_PICKABLE (del_image);
                    gimp_pickable_flush (src_pickable);

                    orig_buffer = gimp_pickable_get_buffer (src_pickable);
                  }
              }
              break;

            case GIMP_CLONE_PATTERN:
              {
                GimpPattern *pattern = gimp_context_get_pattern (context);

                orig_buffer = gimp_pattern_create_buffer (pattern);

                tile = gegl_node_new_child (clone->node,
                                            "operation", "gegl:tile",
                                            NULL);
                clone->crop = gegl_node_new_child (clone->node,
                                                   "operation", "gegl:crop",
                                                   NULL);
              }
              break;
            }

          clone->src_node = gegl_node_new_child (clone->node,
                                                 "operation", "gegl:buffer-source",
                                                 "buffer",    orig_buffer,
                                                 NULL);

          clone->transform_node =
            gegl_node_new_child (clone->node,
                                 "operation", "gegl:transform",
                                 "sampler",    GIMP_INTERPOLATION_LINEAR,
                                 NULL);

          clone->dest_node =
            gegl_node_new_child (clone->node,
                                 "operation", "gegl:write-buffer",
                                 NULL);

          if (tile)
            {
              gegl_node_link_many (clone->src_node,
                                   tile,
                                   clone->crop,
                                   clone->transform_node,
                                   clone->dest_node,
                                   NULL);

              g_object_unref (orig_buffer);
            }
          else
            {
              gegl_node_link_many (clone->src_node,
                                   clone->transform_node,
                                   clone->dest_node,
                                   NULL);
            }

          g_clear_object (&del_image);
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (source_core->set_source)
        {
          gint src_x = floor (coords->x);
          gint src_y = floor (coords->y);

          /*  If the control key is down, move the src target and return */

          g_object_set (options,
                        "src-drawables", drawables,
                        "src-x",         src_x,
                        "src-y",         src_y,
                        NULL);

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        src_x,
                                        src_y,
                                        &clone->src_x_fv,
                                        &clone->src_y_fv);

          source_core->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint src_x;
          gint src_y;

          gint dest_x;
          gint dest_y;
          gint n_strokes;
          gint i;

          g_object_get (options,
                        "src-x", &src_x,
                        "src-y", &src_y,
                        NULL);

          n_strokes = gimp_symmetry_get_size (sym);
          for (i = 0; i < n_strokes; i++)
            {
              coords = gimp_symmetry_get_coords (sym, i);

              dest_x = floor (coords->x);
              dest_y = floor (coords->y);

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

                  /* get destination coordinates in front view perspective */
                  gimp_matrix3_transform_point (&clone->transform_inv,
                                                dest_x,
                                                dest_y,
                                                &clone->dest_x_fv,
                                                &clone->dest_y_fv);

                  source_core->first_stroke = FALSE;
                }
            }

          for (GList *iter = drawables; iter; iter = iter->next)
            gimp_source_core_motion (source_core, iter->data, paint_options,
                                     (g_list_length (drawables) > 1), sym);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      g_clear_object (&clone->node);
      clone->crop           = NULL;
      clone->transform_node = NULL;
      clone->dest_node      = NULL;
      break;

    default:
      break;
    }
}

static gboolean
gimp_perspective_clone_use_source (GimpSourceCore    *source_core,
                                   GimpSourceOptions *options)
{
  return TRUE;
}

static GeglBuffer *
gimp_perspective_clone_get_source (GimpSourceCore   *source_core,
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
  GimpPerspectiveClone *clone         = GIMP_PERSPECTIVE_CLONE (source_core);
  GimpCloneOptions     *clone_options = GIMP_CLONE_OPTIONS (paint_options);
  GeglBuffer           *src_buffer;
  GeglBuffer           *dest_buffer;
  const Babl           *src_format_alpha;
  gint                  x1d, y1d, x2d, y2d;
  gdouble               x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s;
  gint                  xmin, ymin, xmax, ymax;
  GimpMatrix3           matrix;
  GimpMatrix3           gegl_matrix;

  if (self_drawable)
    src_pickable = GIMP_PICKABLE (drawable);

  src_buffer       = gimp_pickable_get_buffer (src_pickable);
  src_format_alpha = gimp_pickable_get_format_with_alpha (src_pickable);

  /* Destination coordinates that will be painted */
  x1d = paint_buffer_x;
  y1d = paint_buffer_y;
  x2d = paint_buffer_x + gegl_buffer_get_width  (paint_buffer);
  y2d = paint_buffer_y + gegl_buffer_get_height (paint_buffer);

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

  switch (clone_options->clone_type)
    {
    case GIMP_CLONE_IMAGE:
      if (! gimp_rectangle_intersect (xmin, ymin,
                                      xmax - xmin, ymax - ymin,
                                      gegl_buffer_get_x      (src_buffer),
                                      gegl_buffer_get_y      (src_buffer),
                                      gegl_buffer_get_width  (src_buffer),
                                      gegl_buffer_get_height (src_buffer),
                                      NULL, NULL, NULL, NULL))
        {
          /* if the source area is completely out of the image */
          return NULL;
        }
      else
        {
          gegl_node_set (clone->src_node,
                         "buffer", src_buffer,
                         NULL);
        }
      break;

    case GIMP_CLONE_PATTERN:
      gegl_node_set (clone->crop,
                     "x",      (gdouble) xmin,
                     "y",      (gdouble) ymin,
                     "width",  (gdouble) xmax - xmin,
                     "height", (gdouble) ymax - ymin,
                     NULL);
      break;
    }

  dest_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, x2d - x1d, y2d - y1d),
                                 src_format_alpha);

  gimp_perspective_clone_get_matrix (clone, &matrix);

  gimp_matrix3_identity (&gegl_matrix);
  gimp_matrix3_mult (&matrix, &gegl_matrix);
  gimp_matrix3_translate (&gegl_matrix, -x1d, -y1d);

  gimp_gegl_node_set_matrix (clone->transform_node, &gegl_matrix);

  gegl_node_set (clone->dest_node,
                 "buffer", dest_buffer,
                 NULL);

  gegl_node_blit (clone->dest_node, 1.0,
                  GEGL_RECTANGLE (0, 0, x2d - x1d, y2d - y1d),
                  NULL, NULL, 0, GEGL_BLIT_DEFAULT);

  *src_rect = *GEGL_RECTANGLE (0, 0, x2d - x1d, y2d - y1d);

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
