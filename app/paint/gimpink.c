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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "paint-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligma-palettes.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmapickable.h"
#include "core/ligmasymmetry.h"
#include "core/ligmatempbuf.h"

#include "ligmainkoptions.h"
#include "ligmaink.h"
#include "ligmaink-blob.h"
#include "ligmainkundo.h"

#include "ligma-intl.h"


#define SUBSAMPLE 8


/*  local function prototypes  */

static void         ligma_ink_finalize         (GObject           *object);

static void         ligma_ink_paint            (LigmaPaintCore     *paint_core,
                                               GList             *drawables,
                                               LigmaPaintOptions  *paint_options,
                                               LigmaSymmetry      *sym,
                                               LigmaPaintState     paint_state,
                                               guint32            time);
static GeglBuffer * ligma_ink_get_paint_buffer (LigmaPaintCore     *paint_core,
                                               LigmaDrawable      *drawable,
                                               LigmaPaintOptions  *paint_options,
                                               LigmaLayerMode      paint_mode,
                                               const LigmaCoords  *coords,
                                               gint              *paint_buffer_x,
                                               gint              *paint_buffer_y,
                                               gint              *paint_width,
                                               gint              *paint_height);
static LigmaUndo   * ligma_ink_push_undo        (LigmaPaintCore     *core,
                                               LigmaImage         *image,
                                               const gchar       *undo_desc);

static void         ligma_ink_motion           (LigmaPaintCore     *paint_core,
                                               LigmaDrawable      *drawable,
                                               LigmaPaintOptions  *paint_options,
                                               LigmaSymmetry      *sym,
                                               guint32            time);

static LigmaBlob   * ink_pen_ellipse           (LigmaInkOptions    *options,
                                               gdouble            x_center,
                                               gdouble            y_center,
                                               gdouble            pressure,
                                               gdouble            xtilt,
                                               gdouble            ytilt,
                                               gdouble            velocity,
                                               const LigmaMatrix3 *transform);

static void         render_blob               (GeglBuffer        *buffer,
                                               GeglRectangle     *rect,
                                               LigmaBlob          *blob);


G_DEFINE_TYPE (LigmaInk, ligma_ink, LIGMA_TYPE_PAINT_CORE)

#define parent_class ligma_ink_parent_class


void
ligma_ink_register (Ligma                      *ligma,
                   LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_INK,
                LIGMA_TYPE_INK_OPTIONS,
                "ligma-ink",
                _("Ink"),
                "ligma-tool-ink");
}

static void
ligma_ink_class_init (LigmaInkClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  LigmaPaintCoreClass *paint_core_class = LIGMA_PAINT_CORE_CLASS (klass);

  object_class->finalize             = ligma_ink_finalize;

  paint_core_class->paint            = ligma_ink_paint;
  paint_core_class->get_paint_buffer = ligma_ink_get_paint_buffer;
  paint_core_class->push_undo        = ligma_ink_push_undo;
}

static void
ligma_ink_init (LigmaInk *ink)
{
}

static void
ligma_ink_finalize (GObject *object)
{
  LigmaInk *ink = LIGMA_INK (object);

  if (ink->start_blobs)
    {
      g_list_free_full (ink->start_blobs, g_free);
      ink->start_blobs = NULL;
    }

  if (ink->last_blobs)
    {
      g_list_free_full (ink->last_blobs, g_free);
      ink->last_blobs = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_ink_paint (LigmaPaintCore    *paint_core,
                GList            *drawables,
                LigmaPaintOptions *paint_options,
                LigmaSymmetry     *sym,
                LigmaPaintState    paint_state,
                guint32           time)
{
  LigmaInk    *ink = LIGMA_INK (paint_core);
  LigmaCoords *cur_coords;
  LigmaCoords  last_coords;

  g_return_if_fail (g_list_length (drawables) == 1);

  ligma_paint_core_get_last_coords (paint_core, &last_coords);
  cur_coords = ligma_symmetry_get_origin (sym);

  switch (paint_state)
    {
    case LIGMA_PAINT_STATE_INIT:
        {
          LigmaContext *context = LIGMA_CONTEXT (paint_options);
          LigmaRGB      foreground;

          ligma_symmetry_set_stateful (sym, TRUE);
          ligma_context_get_foreground (context, &foreground);
          ligma_palettes_add_color_history (context->ligma,
                                           &foreground);

          if (cur_coords->x == last_coords.x &&
              cur_coords->y == last_coords.y)
            {
              if (ink->start_blobs)
                {
                  g_list_free_full (ink->start_blobs, g_free);
                  ink->start_blobs = NULL;
                }

              if (ink->last_blobs)
                {
                  g_list_free_full (ink->last_blobs, g_free);
                  ink->last_blobs = NULL;
                }
            }
          else if (ink->last_blobs)
            {
              LigmaBlob *last_blob;
              GList    *iter;
              gint      i;

              if (ink->start_blobs)
                {
                  g_list_free_full (ink->start_blobs, g_free);
                  ink->start_blobs = NULL;
                }

              /*  save the start blobs of each stroke for undo otherwise  */
              for (iter = ink->last_blobs, i = 0; iter; iter = g_list_next (iter), i++)
                {
                  last_blob = g_list_nth_data (ink->last_blobs, i);

                  ink->start_blobs = g_list_prepend (ink->start_blobs,
                                                     ligma_blob_duplicate (last_blob));
                }
              ink->start_blobs = g_list_reverse (ink->start_blobs);
            }
        }
      break;

    case LIGMA_PAINT_STATE_MOTION:
      for (GList *iter = drawables; iter; iter = iter->next)
        ligma_ink_motion (paint_core, iter->data, paint_options, sym, time);
      break;

    case LIGMA_PAINT_STATE_FINISH:
      ligma_symmetry_set_stateful (sym, FALSE);
      break;
    }
}

static GeglBuffer *
ligma_ink_get_paint_buffer (LigmaPaintCore    *paint_core,
                           LigmaDrawable     *drawable,
                           LigmaPaintOptions *paint_options,
                           LigmaLayerMode     paint_mode,
                           const LigmaCoords *coords,
                           gint             *paint_buffer_x,
                           gint             *paint_buffer_y,
                           gint             *paint_width,
                           gint             *paint_height)
{
  LigmaInk *ink = LIGMA_INK (paint_core);
  gint     x, y;
  gint     width, height;
  gint     dwidth, dheight;
  gint     x1, y1, x2, y2;

  ligma_blob_bounds (ink->cur_blob, &x, &y, &width, &height);

  dwidth  = ligma_item_get_width  (LIGMA_ITEM (drawable));
  dheight = ligma_item_get_height (LIGMA_ITEM (drawable));

  x1 = CLAMP (x / SUBSAMPLE - 1,            0, dwidth);
  y1 = CLAMP (y / SUBSAMPLE - 1,            0, dheight);
  x2 = CLAMP ((x + width)  / SUBSAMPLE + 2, 0, dwidth);
  y2 = CLAMP ((y + height) / SUBSAMPLE + 2, 0, dheight);

  if (paint_width)
    *paint_width = width / SUBSAMPLE + 3;
  if (paint_height)
    *paint_height = height / SUBSAMPLE + 3;

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    {
      LigmaTempBuf            *temp_buf;
      const Babl             *format;
      LigmaLayerCompositeMode  composite_mode;

      composite_mode = ligma_layer_mode_get_paint_composite_mode (paint_mode);

      format = ligma_layer_mode_get_format (paint_mode,
                                           LIGMA_LAYER_COLOR_SPACE_AUTO,
                                           LIGMA_LAYER_COLOR_SPACE_AUTO,
                                           composite_mode,
                                           ligma_drawable_get_format (drawable));

      temp_buf = ligma_temp_buf_new ((x2 - x1), (y2 - y1),
                                    format);

      *paint_buffer_x = x1;
      *paint_buffer_y = y1;

      if (paint_core->paint_buffer)
        g_object_unref (paint_core->paint_buffer);

      paint_core->paint_buffer = ligma_temp_buf_create_buffer (temp_buf);

      ligma_temp_buf_unref (temp_buf);

      return paint_core->paint_buffer;
    }

  return NULL;
}

static LigmaUndo *
ligma_ink_push_undo (LigmaPaintCore *core,
                    LigmaImage     *image,
                    const gchar   *undo_desc)
{
  return ligma_image_undo_push (image, LIGMA_TYPE_INK_UNDO,
                               LIGMA_UNDO_INK, undo_desc,
                               0,
                               "paint-core", core,
                               NULL);
}

static void
ligma_ink_motion (LigmaPaintCore    *paint_core,
                 LigmaDrawable     *drawable,
                 LigmaPaintOptions *paint_options,
                 LigmaSymmetry     *sym,
                 guint32           time)
{
  LigmaInk        *ink             = LIGMA_INK (paint_core);
  LigmaInkOptions *options         = LIGMA_INK_OPTIONS (paint_options);
  LigmaContext    *context         = LIGMA_CONTEXT (paint_options);
  GList          *blob_unions     = NULL;
  GList          *blobs_to_render = NULL;
  GeglBuffer     *paint_buffer;
  gint            paint_buffer_x;
  gint            paint_buffer_y;
  LigmaLayerMode   paint_mode;
  LigmaRGB         foreground;
  GeglColor      *color;
  LigmaBlob       *last_blob;
  LigmaCoords      coords;
  gint            off_x, off_y;
  gint            n_strokes;
  gint            i;

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);
  n_strokes = ligma_symmetry_get_size (sym);

  if (ink->last_blobs &&
      g_list_length (ink->last_blobs) != n_strokes)
    {
      g_list_free_full (ink->last_blobs, g_free);
      ink->last_blobs = NULL;
    }

  if (! ink->last_blobs)
    {
      if (ink->start_blobs)
        {
          g_list_free_full (ink->start_blobs, g_free);
          ink->start_blobs = NULL;
        }

      for (i = 0; i < n_strokes; i++)
        {
          LigmaMatrix3 transform;

          coords    = *(ligma_symmetry_get_coords (sym, i));
          coords.x -= off_x;
          coords.y -= off_y;

          ligma_symmetry_get_matrix (sym, i, &transform);

          last_blob = ink_pen_ellipse (options,
                                       coords.x,
                                       coords.y,
                                       coords.pressure,
                                       coords.xtilt,
                                       coords.ytilt,
                                       100,
                                       &transform);

          ink->last_blobs = g_list_prepend (ink->last_blobs,
                                            last_blob);
          ink->start_blobs = g_list_prepend (ink->start_blobs,
                                             ligma_blob_duplicate (last_blob));
          blobs_to_render = g_list_prepend (blobs_to_render, last_blob);
        }
      ink->start_blobs = g_list_reverse (ink->start_blobs);
      ink->last_blobs = g_list_reverse (ink->last_blobs);
      blobs_to_render = g_list_reverse (blobs_to_render);
    }
  else
    {
      for (i = 0; i < n_strokes; i++)
        {
          LigmaBlob    *blob;
          LigmaBlob    *blob_union = NULL;
          LigmaMatrix3  transform;

          coords    = *(ligma_symmetry_get_coords (sym, i));
          coords.x -= off_x;
          coords.y -= off_y;

          ligma_symmetry_get_matrix (sym, i, &transform);

          blob = ink_pen_ellipse (options,
                                  coords.x,
                                  coords.y,
                                  coords.pressure,
                                  coords.xtilt,
                                  coords.ytilt,
                                  coords.velocity * 100,
                                  &transform);

          last_blob = g_list_nth_data (ink->last_blobs, i);
          blob_union = ligma_blob_convex_union (last_blob, blob);

          g_free (last_blob);
          g_list_nth (ink->last_blobs, i)->data = blob;

          blobs_to_render = g_list_prepend (blobs_to_render, blob_union);
          blob_unions = g_list_prepend (blob_unions, blob_union);
        }
      blobs_to_render = g_list_reverse (blobs_to_render);
    }

  paint_mode = ligma_context_get_paint_mode (context);

  ligma_context_get_foreground (context, &foreground);
  ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (drawable),
                                     &foreground, &foreground);
  color = ligma_gegl_color_new (&foreground, ligma_drawable_get_space (drawable));

  for (i = 0; i < n_strokes; i++)
    {
      LigmaBlob *blob_to_render = g_list_nth_data (blobs_to_render, i);

      coords    = *(ligma_symmetry_get_coords (sym, i));
      coords.x -= off_x;
      coords.y -= off_y;

      ink->cur_blob = blob_to_render;
      paint_buffer = ligma_paint_core_get_paint_buffer (paint_core, drawable,
                                                       paint_options,
                                                       paint_mode,
                                                       &coords,
                                                       &paint_buffer_x,
                                                       &paint_buffer_y,
                                                       NULL, NULL);
      ink->cur_blob = NULL;

      if (! paint_buffer)
        continue;

      gegl_buffer_set_color (paint_buffer, NULL, color);

      /*  draw the blob directly to the canvas_buffer  */
      render_blob (paint_core->canvas_buffer,
                   GEGL_RECTANGLE (paint_core->paint_buffer_x,
                                   paint_core->paint_buffer_y,
                                   gegl_buffer_get_width  (paint_core->paint_buffer),
                                   gegl_buffer_get_height (paint_core->paint_buffer)),
                   blob_to_render);

      /*  draw the paint_area using the just rendered canvas_buffer as mask */
      ligma_paint_core_paste (paint_core,
                             NULL,
                             paint_core->paint_buffer_x,
                             paint_core->paint_buffer_y,
                             drawable,
                             LIGMA_OPACITY_OPAQUE,
                             ligma_context_get_opacity (context),
                             paint_mode,
                             LIGMA_PAINT_CONSTANT);

    }

  g_object_unref (color);

  g_list_free_full (blob_unions, g_free);
  g_list_free (blobs_to_render);
}

static LigmaBlob *
ink_pen_ellipse (LigmaInkOptions    *options,
                 gdouble            x_center,
                 gdouble            y_center,
                 gdouble            pressure,
                 gdouble            xtilt,
                 gdouble            ytilt,
                 gdouble            velocity,
                 const LigmaMatrix3 *transform)
{
  LigmaBlobFunc blob_function;
  gdouble      size;
  gdouble      tsin, tcos;
  gdouble      aspect, radmin;
  gdouble      x,y;
  gdouble      tscale;
  gdouble      tscale_c;
  gdouble      tscale_s;

  /* Adjust the size depending on pressure. */

  size = options->size * (1.0 + options->size_sensitivity *
                          (2.0 * pressure - 1.0));

  /* Adjust the size further depending on pointer velocity and
   * velocity-sensitivity.  These 'magic constants' are 'feels
   * natural' tigert-approved. --ADM
   */

  if (velocity < 3.0)
    velocity = 3.0;

#ifdef VERBOSE
  g_printerr ("%g (%g) -> ", size, velocity);
#endif

  size = (options->vel_sensitivity *
          ((4.5 * size) / (1.0 + options->vel_sensitivity * (2.0 * velocity)))
          + (1.0 - options->vel_sensitivity) * size);

#ifdef VERBOSE
  g_printerr ("%g\n", (gfloat) size);
#endif

  /* Clamp resulting size to sane limits */

  if (size > options->size * (1.0 + options->size_sensitivity))
    size = options->size * (1.0 + options->size_sensitivity);

  if (size * SUBSAMPLE < 1.0)
    size = 1.0 / SUBSAMPLE;

  /* Add brush angle/aspect to tilt vectorially */

  /* I'm not happy with the way the brush widget info is combined with
   * tilt info from the brush. My personal feeling is that
   * representing both as affine transforms would make the most
   * sense. -RLL
   */

  tscale   = options->tilt_sensitivity * 10.0;
  tscale_c = tscale * cos (ligma_deg_to_rad (options->tilt_angle));
  tscale_s = tscale * sin (ligma_deg_to_rad (options->tilt_angle));

  x = (options->blob_aspect * cos (options->blob_angle) +
       xtilt * tscale_c - ytilt * tscale_s);
  y = (options->blob_aspect * sin (options->blob_angle) +
       ytilt * tscale_c + xtilt * tscale_s);

#ifdef VERBOSE
  g_printerr ("angle %g aspect %g; %g %g; %g %g\n",
              options->blob_angle, options->blob_aspect,
              tscale_c, tscale_s, x, y);
#endif

  aspect = sqrt (SQR (x) + SQR (y));

  if (aspect != 0)
    {
      tcos = x / aspect;
      tsin = y / aspect;
    }
  else
    {
      tcos = cos (options->blob_angle);
      tsin = sin (options->blob_angle);
    }

  ligma_matrix3_transform_point (transform,
                                tcos,  tsin,
                                &tcos, &tsin);

  aspect = CLAMP (aspect, 1.0, 10.0);

  radmin = MAX (1.0, SUBSAMPLE * size / aspect);

  switch (options->blob_type)
    {
    case LIGMA_INK_BLOB_TYPE_CIRCLE:
      blob_function = ligma_blob_ellipse;
      break;

    case LIGMA_INK_BLOB_TYPE_SQUARE:
      blob_function = ligma_blob_square;
      break;

    case LIGMA_INK_BLOB_TYPE_DIAMOND:
      blob_function = ligma_blob_diamond;
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }

  return (* blob_function) (x_center * SUBSAMPLE,
                            y_center * SUBSAMPLE,
                            radmin * aspect * tcos,
                            radmin * aspect * tsin,
                            -radmin * tsin,
                            radmin * tcos);
}


/*********************************/
/*  Rendering functions          */
/*********************************/

/* Some of this stuff should probably be combined with the
 * code it was copied from in paint_core.c; but I wanted
 * to learn this stuff, so I've kept it simple.
 *
 * The following only supports CONSTANT mode. Incremental
 * would, I think, interact strangely with the way we
 * do things. But it wouldn't be hard to implement at all.
 */

enum
{
  ROW_START,
  ROW_STOP
};

/* The insertion sort here, for SUBSAMPLE = 8, tends to beat out
 * qsort() by 4x with CFLAGS=-O2, 2x with CFLAGS=-g
 */
static void
insert_sort (gint *data,
             gint  n)
{
  gint i, j, k;

  for (i = 2; i < 2 * n; i += 2)
    {
      gint tmp1 = data[i];
      gint tmp2 = data[i + 1];

      j = 0;

      while (data[j] < tmp1)
        j += 2;

      for (k = i; k > j; k -= 2)
        {
          data[k]     = data[k - 2];
          data[k + 1] = data[k - 1];
        }

      data[j]     = tmp1;
      data[j + 1] = tmp2;
    }
}

static void
fill_run (gfloat *dest,
          gfloat  alpha,
          gint    w)
{
  if (alpha == 1.0)
    {
      while (w--)
        {
          *dest = 1.0;
          dest++;
        }
    }
  else
    {
      while (w--)
        {
          *dest = MAX (*dest, alpha);
          dest++;
        }
    }
}

static void
render_blob_line (LigmaBlob *blob,
                  gfloat   *dest,
                  gint      x,
                  gint      y,
                  gint      width)
{
  gint  buf[4 * SUBSAMPLE];
  gint *data    = buf;
  gint  n       = 0;
  gint  i, j;
  gint  current = 0;  /* number of filled rows at this point
                       * in the scan line
                       */
  gint last_x;

  /* Sort start and ends for all lines */

  j = y * SUBSAMPLE - blob->y;
  for (i = 0; i < SUBSAMPLE; i++)
    {
      if (j >= blob->height)
        break;

      if ((j > 0) && (blob->data[j].left <= blob->data[j].right))
        {
          data[2 * n]                     = blob->data[j].left;
          data[2 * n + 1]                 = ROW_START;
          data[2 * SUBSAMPLE + 2 * n]     = blob->data[j].right;
          data[2 * SUBSAMPLE + 2 * n + 1] = ROW_STOP;
          n++;
        }
      j++;
    }

  /*   If we have less than SUBSAMPLE rows, compress */
  if (n < SUBSAMPLE)
    {
      for (i = 0; i < 2 * n; i++)
        data[2 * n + i] = data[2 * SUBSAMPLE + i];
    }

  /*   Now count start and end separately */
  n *= 2;

  insert_sort (data, n);

  /* Discard portions outside of tile */

  while ((n > 0) && (data[0] < SUBSAMPLE*x))
    {
      if (data[1] == ROW_START)
        current++;
      else
        current--;
      data += 2;
      n--;
    }

  while ((n > 0) && (data[2*(n-1)] >= SUBSAMPLE*(x+width)))
    n--;

  /* Render the row */

  last_x = 0;
  for (i = 0; i < n;)
    {
      gint cur_x = data[2 * i] / SUBSAMPLE - x;
      gint pixel;

      /* Fill in portion leading up to this pixel */
      if (current && cur_x != last_x)
        fill_run (dest + last_x, (gfloat) current / SUBSAMPLE, cur_x - last_x);

      /* Compute the value for this pixel */
      pixel = current * SUBSAMPLE;

      while (i<n)
        {
          gint tmp_x = data[2 * i] / SUBSAMPLE;

          if (tmp_x - x != cur_x)
            break;

          if (data[2 * i + 1] == ROW_START)
            {
              current++;
              pixel += ((tmp_x + 1) * SUBSAMPLE) - data[2 * i];
            }
          else
            {
              current--;
              pixel -= ((tmp_x + 1) * SUBSAMPLE) - data[2 * i];
            }

          i++;
        }

      dest[cur_x] = MAX (dest[cur_x], (gfloat) pixel / (SUBSAMPLE * SUBSAMPLE));

      last_x = cur_x + 1;
    }

  if (current != 0)
    fill_run (dest + last_x, (gfloat) current / SUBSAMPLE, width - last_x);
}

static void
render_blob (GeglBuffer    *buffer,
             GeglRectangle *rect,
             LigmaBlob      *blob)
{
  GeglBufferIterator *iter;
  GeglRectangle      *roi;

  iter = gegl_buffer_iterator_new (buffer, rect, 0, babl_format ("Y float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);
  roi = &iter->items[0].roi;

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *d = iter->items[0].data;
      gint    h = roi->height;
      gint    y;

      for (y = 0; y < h; y++, d += roi->width * 1)
        {
          render_blob_line (blob, d, roi->x, roi->y + y, roi->width);
        }
    }
}
