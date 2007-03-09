/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"

#include "gimpinkoptions.h"
#include "gimpink.h"
#include "gimpink-blob.h"
#include "gimpinkundo.h"

#include "gimp-intl.h"


#define SUBSAMPLE 8


/*  local function prototypes  */

static void      gimp_ink_finalize       (GObject          *object);

static void      gimp_ink_paint          (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options,
                                          GimpPaintState    paint_state,
                                          guint32           time);
static TempBuf * gimp_ink_get_paint_area (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options);
static GimpUndo* gimp_ink_push_undo      (GimpPaintCore    *core,
                                          GimpImage        *image,
                                          const gchar      *undo_desc);

static void      gimp_ink_motion         (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options,
                                          guint32           time);

static Blob    * ink_pen_ellipse         (GimpInkOptions   *options,
                                          gdouble           x_center,
                                          gdouble           y_center,
                                          gdouble           pressure,
                                          gdouble           xtilt,
                                          gdouble           ytilt,
                                          gdouble           velocity);

static void      time_smoother_add       (GimpInk          *ink,
                                          guint32           value);
static guint32   time_smoother_result    (GimpInk          *ink);
static void      time_smoother_init      (GimpInk          *ink,
                                          guint32           initval);

static void      dist_smoother_add       (GimpInk          *ink,
                                          gdouble           value);
static gdouble   dist_smoother_result    (GimpInk          *ink);
static void      dist_smoother_init      (GimpInk          *ink,
                                          gdouble           initval);

static void      render_blob             (Blob             *blob,
                                          PixelRegion      *dest);


G_DEFINE_TYPE (GimpInk, gimp_ink, GIMP_TYPE_PAINT_CORE)

#define parent_class gimp_ink_parent_class


void
gimp_ink_register (Gimp                      *gimp,
                   GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_INK,
                GIMP_TYPE_INK_OPTIONS,
                "gimp-ink",
                _("Ink"),
                "gimp-tool-ink");
}

static void
gimp_ink_class_init (GimpInkClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  object_class->finalize           = gimp_ink_finalize;

  paint_core_class->paint          = gimp_ink_paint;
  paint_core_class->get_paint_area = gimp_ink_get_paint_area;
  paint_core_class->push_undo      = gimp_ink_push_undo;
}

static void
gimp_ink_init (GimpInk *ink)
{
}

static void
gimp_ink_finalize (GObject *object)
{
  GimpInk *ink = GIMP_INK (object);

  if (ink->start_blob)
    {
      g_free (ink->start_blob);
      ink->start_blob = NULL;
    }

  if (ink->last_blob)
    {
      g_free (ink->last_blob);
      ink->last_blob = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_ink_paint (GimpPaintCore    *paint_core,
                GimpDrawable     *drawable,
                GimpPaintOptions *paint_options,
                GimpPaintState    paint_state,
                guint32           time)
{
  GimpInk *ink = GIMP_INK (paint_core);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (paint_core->cur_coords.x == paint_core->last_coords.x &&
          paint_core->cur_coords.y == paint_core->last_coords.y)
        {
          /*  start with new blobs if we're not interpolating  */

          if (ink->start_blob)
            {
              g_free (ink->start_blob);
              ink->start_blob = NULL;
            }

          if (ink->last_blob)
            {
              g_free (ink->last_blob);
              ink->last_blob = NULL;
            }
        }
      else if (ink->last_blob)
        {
          /*  save the start blob of the line for undo otherwise  */

          if (ink->start_blob)
            g_free (ink->start_blob);

          ink->start_blob = blob_duplicate (ink->last_blob);
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      gimp_ink_motion (paint_core, drawable, paint_options, time);
      break;

    case GIMP_PAINT_STATE_FINISH:
      break;
    }
}

static TempBuf *
gimp_ink_get_paint_area (GimpPaintCore    *paint_core,
                         GimpDrawable     *drawable,
                         GimpPaintOptions *paint_options)
{
  GimpInk *ink  = GIMP_INK (paint_core);
  gint     x, y;
  gint     width, height;
  gint     dwidth, dheight;
  gint     x1, y1, x2, y2;
  gint     bytes;

  bytes = gimp_drawable_bytes_with_alpha (drawable);

  blob_bounds (ink->cur_blob, &x, &y, &width, &height);

  dwidth  = gimp_item_width  (GIMP_ITEM (drawable));
  dheight = gimp_item_height (GIMP_ITEM (drawable));

  x1 = CLAMP (x / SUBSAMPLE - 1,            0, dwidth);
  y1 = CLAMP (y / SUBSAMPLE - 1,            0, dheight);
  x2 = CLAMP ((x + width)  / SUBSAMPLE + 2, 0, dwidth);
  y2 = CLAMP ((y + height) / SUBSAMPLE + 2, 0, dheight);

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    paint_core->canvas_buf = temp_buf_resize (paint_core->canvas_buf, bytes,
                                              x1, y1,
                                              (x2 - x1), (y2 - y1));
  else
    return NULL;

  return paint_core->canvas_buf;
}

static GimpUndo *
gimp_ink_push_undo (GimpPaintCore *core,
                    GimpImage     *image,
                    const gchar   *undo_desc)
{
  return gimp_image_undo_push (image, GIMP_TYPE_INK_UNDO,
                               GIMP_UNDO_INK, undo_desc,
                               0,
                               "paint-core", core,
                               NULL);
}

static void
gimp_ink_motion (GimpPaintCore    *paint_core,
                 GimpDrawable     *drawable,
                 GimpPaintOptions *paint_options,
                 guint32           time)
{
  GimpInk        *ink     = GIMP_INK (paint_core);
  GimpInkOptions *options = GIMP_INK_OPTIONS (paint_options);
  GimpContext    *context = GIMP_CONTEXT (paint_options);
  GimpImage      *image;
  Blob           *blob_union = NULL;
  Blob           *blob_to_render;
  TempBuf        *area;
  guchar          col[MAX_CHANNELS];
  PixelRegion     blob_maskPR;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! ink->last_blob)
    {
      ink->last_blob = ink_pen_ellipse (options,
                                        paint_core->cur_coords.x,
                                        paint_core->cur_coords.y,
                                        paint_core->cur_coords.pressure,
                                        paint_core->cur_coords.xtilt,
                                        paint_core->cur_coords.ytilt,
                                        10.0);

      if (ink->start_blob)
        g_free (ink->start_blob);

      ink->start_blob = blob_duplicate (ink->last_blob);

      time_smoother_init (ink, time);
      ink->last_time = time;

      dist_smoother_init (ink, 0.0);
      ink->init_velocity = TRUE;

      blob_to_render = ink->last_blob;
    }
  else
    {
      Blob    *blob;
      gdouble  dist;
      gdouble  velocity;
      guint32  lasttime = ink->last_time;
      guint32  thistime;

      time_smoother_add (ink, time);
      thistime = ink->last_time = time_smoother_result (ink);

      /* The time resolution on X-based GDK motion events is bloody
       * awful, hence the use of the smoothing function.  Sadly this
       * also means that there is always the chance of having an
       * indeterminite velocity since this event and the previous
       * several may still appear to issue at the same
       * instant. -ADM
       */
      if (thistime == lasttime)
        thistime = lasttime + 1;

      dist = sqrt ((paint_core->last_coords.x - paint_core->cur_coords.x) *
                   (paint_core->last_coords.x - paint_core->cur_coords.x) +
                   (paint_core->last_coords.y - paint_core->cur_coords.y) *
                   (paint_core->last_coords.y - paint_core->cur_coords.y));

      if (ink->init_velocity)
        {
          dist_smoother_init (ink, dist);
          ink->init_velocity = FALSE;
        }
      else
        {
          dist_smoother_add (ink, dist);
          dist = dist_smoother_result (ink);
        }

      velocity = 10.0 * sqrt ((dist) / (gdouble) (thistime - lasttime));

      blob = ink_pen_ellipse (options,
                              paint_core->cur_coords.x,
                              paint_core->cur_coords.y,
                              paint_core->cur_coords.pressure,
                              paint_core->cur_coords.xtilt,
                              paint_core->cur_coords.ytilt,
                              velocity);

      blob_union = blob_convex_union (ink->last_blob, blob);
      g_free (ink->last_blob);
      ink->last_blob = blob;

      blob_to_render = blob_union;
    }

  /* Get the buffer */
  ink->cur_blob = blob_to_render;
  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  ink->cur_blob = NULL;

  if (! area)
    return;

  gimp_image_get_foreground (image, context, gimp_drawable_type (drawable),
                             col);

  /*  set the alpha channel  */
  col[paint_core->canvas_buf->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (paint_core->canvas_buf), col,
                area->width * area->height, area->bytes);

  gimp_paint_core_validate_canvas_tiles (paint_core,
                                         paint_core->canvas_buf->x,
                                         paint_core->canvas_buf->y,
                                         paint_core->canvas_buf->width,
                                         paint_core->canvas_buf->height);

  /*  draw the blob directly to the canvas_tiles  */
  pixel_region_init (&blob_maskPR, paint_core->canvas_tiles,
                     paint_core->canvas_buf->x,
                     paint_core->canvas_buf->y,
                     paint_core->canvas_buf->width,
                     paint_core->canvas_buf->height,
                     TRUE);

  render_blob (blob_to_render, &blob_maskPR);

  /*  draw the canvas_buf using the just rendered canvas_tiles as mask */
  pixel_region_init (&blob_maskPR, paint_core->canvas_tiles,
                     paint_core->canvas_buf->x,
                     paint_core->canvas_buf->y,
                     paint_core->canvas_buf->width,
                     paint_core->canvas_buf->height,
                     FALSE);

  gimp_paint_core_paste (paint_core, &blob_maskPR, drawable,
                         GIMP_OPACITY_OPAQUE,
                         gimp_context_get_opacity (context),
                         gimp_context_get_paint_mode (context),
                         GIMP_PAINT_CONSTANT);

  if (blob_union)
    g_free (blob_union);
}

static Blob *
ink_pen_ellipse (GimpInkOptions *options,
                 gdouble         x_center,
                 gdouble         y_center,
                 gdouble         pressure,
                 gdouble         xtilt,
                 gdouble         ytilt,
                 gdouble         velocity)
{
  BlobFunc blob_function;
  gdouble  size;
  gdouble  tsin, tcos;
  gdouble  aspect, radmin;
  gdouble  x,y;
  gdouble  tscale;
  gdouble  tscale_c;
  gdouble  tscale_s;

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
  tscale_c = tscale * cos (gimp_deg_to_rad (options->tilt_angle));
  tscale_s = tscale * sin (gimp_deg_to_rad (options->tilt_angle));

  x = (options->blob_aspect * cos (options->blob_angle) +
       xtilt * tscale_c - ytilt * tscale_s);
  y = (options->blob_aspect * sin (options->blob_angle) +
       ytilt * tscale_c + xtilt * tscale_s);

#ifdef VERBOSE
  g_printerr ("angle %g aspect %g; %g %g; %g %g\n",
              options->blob_angle, options->blob_aspect,
              tscale_c, tscale_s, x, y);
#endif

  aspect = sqrt (x * x + y * y);

  if (aspect != 0)
    {
      tcos = x / aspect;
      tsin = y / aspect;
    }
  else
    {
      tsin = sin (options->blob_angle);
      tcos = cos (options->blob_angle);
    }

  aspect = CLAMP (aspect, 1.0, 10.0);

  radmin = MAX (1.0, SUBSAMPLE * size / aspect);

  switch (options->blob_type)
    {
    case GIMP_INK_BLOB_TYPE_ELLIPSE:
      blob_function = blob_ellipse;
      break;

    case GIMP_INK_BLOB_TYPE_SQUARE:
      blob_function = blob_square;
      break;

    case GIMP_INK_BLOB_TYPE_DIAMOND:
      blob_function = blob_diamond;
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


static void
time_smoother_init (GimpInk *ink,
                    guint32  initval)
{
  gint i;

  ink->ts_index = 0;

  for (i = 0; i < TIME_SMOOTHER_BUFFER; i++)
    ink->ts_buffer[i] = initval;
}

static guint32
time_smoother_result (GimpInk *ink)
{
  gint    i;
  guint64 result = 0;

  for (i = 0; i < TIME_SMOOTHER_BUFFER; i++)
    result += ink->ts_buffer[i];

  return (result / (guint64) TIME_SMOOTHER_BUFFER);
}

static void
time_smoother_add (GimpInk *ink,
                   guint32  value)
{
  guint64 long_value = (guint64) value;

  /*  handle wrap-around of time values  */
  if (long_value < ink->ts_buffer[ink->ts_index])
    long_value += (guint64) + G_MAXUINT32;

  ink->ts_buffer[ink->ts_index++] = long_value;

  ink->ts_buffer[ink->ts_index++] = value;

  if (ink->ts_index == TIME_SMOOTHER_BUFFER)
    ink->ts_index = 0;
}


static void
dist_smoother_init (GimpInk *ink,
                    gdouble  initval)
{
  gint i;

  ink->dt_index = 0;

  for (i = 0; i < DIST_SMOOTHER_BUFFER; i++)
    ink->dt_buffer[i] = initval;
}

static gdouble
dist_smoother_result (GimpInk *ink)
{
  gint    i;
  gdouble result = 0.0;

  for (i = 0; i < DIST_SMOOTHER_BUFFER; i++)
    result += ink->dt_buffer[i];

  return (result / (gdouble) DIST_SMOOTHER_BUFFER);
}

static void
dist_smoother_add (GimpInk *ink,
                   gdouble  value)
{
  ink->dt_buffer[ink->dt_index++] = value;

  if (ink->dt_index == DIST_SMOOTHER_BUFFER)
    ink->dt_index = 0;
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

enum { ROW_START, ROW_STOP };

/* The insertion sort here, for SUBSAMPLE = 8, tends to beat out
 * qsort() by 4x with CFLAGS=-O2, 2x with CFLAGS=-g
 */
static void
insert_sort (gint *data,
             gint  n)
{
  gint i, j, k;
  gint tmp1, tmp2;

  for (i = 2; i < 2 * n; i += 2)
    {
      tmp1 = data[i];
      tmp2 = data[i + 1];
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
fill_run (guchar *dest,
          guchar  alpha,
          gint    w)
{
  if (alpha == 255)
    {
      memset (dest, 255, w);
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
render_blob_line (Blob   *blob,
                  guchar *dest,
                  gint    x,
                  gint    y,
                  gint    width)
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
        fill_run (dest + last_x, (255 * current) / SUBSAMPLE, cur_x - last_x);

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

      dest[cur_x] = MAX (dest[cur_x], (pixel * 255) / (SUBSAMPLE * SUBSAMPLE));

      last_x = cur_x + 1;
    }

  if (current != 0)
    fill_run (dest + last_x, (255 * current)/ SUBSAMPLE, width - last_x);
}

static void
render_blob (Blob        *blob,
             PixelRegion *dest)
{
  gint      i;
  gint      h;
  guchar   *s;
  gpointer  pr;

  for (pr = pixel_regions_register (1, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      h = dest->h;
      s = dest->data;

      for (i=0; i<h; i++)
        {
          render_blob_line (blob, s,
                            dest->x, dest->y + i, dest->w);
          s += dest->rowstride;
        }
    }
}
