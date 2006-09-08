/* The GIMP -- an image manipulation program
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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "libgimpmath/gimpmatrix.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppattern.h"
#include "core/gimppickable.h"

#include "gimpperspectiveclone.h"
#include "gimpperspectivecloneoptions.h"

#include "gimp-intl.h"


#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define MIN4(a,b,c,d) MIN(MIN(a,b),MIN(c,d))
#define MAX4(a,b,c,d) MAX(MAX(a,b),MAX(c,d))


enum
{
  PROP_0,
  PROP_SRC_DRAWABLE,
  PROP_SRC_X,
  PROP_SRC_Y
};


static void   gimp_perspective_clone_set_property     (GObject          *object,
                                                       guint             property_id,
                                                       const GValue     *value,
                                                       GParamSpec       *pspec);
static void   gimp_perspective_clone_get_property     (GObject          *object,
                                                       guint             property_id,
                                                       GValue           *value,
                                                       GParamSpec       *pspec);

static void   gimp_perspective_clone_paint            (GimpPaintCore    *paint_core,
                                                       GimpDrawable     *drawable,
                                                       GimpPaintOptions *paint_options,
                                                       GimpPaintState    paint_state,
                                                       guint32           time);
static void   gimp_perspective_clone_motion           (GimpPaintCore    *paint_core,
                                                       GimpDrawable     *drawable,
                                                       GimpPaintOptions *paint_options);

static void   gimp_perspective_clone_line_image       (GimpImage        *dest,
                                                       GimpImage        *src,
                                                       GimpDrawable     *d_drawable,
                                                       GimpPickable     *s_pickable,
                                                       guchar           *s,
                                                       guchar           *d,
                                                       gint              src_bytes,
                                                       gint              dest_bytes,
                                                       gint              width);
static void   gimp_perspective_clone_line_pattern     (GimpImage        *dest,
                                                       GimpDrawable     *drawable,
                                                       GimpPattern      *pattern,
                                                       guchar           *d,
                                                       gint              x,
                                                       gint              y,
                                                       gint              bytes,
                                                       gint              width);

static void   gimp_perspective_clone_set_src_drawable (GimpPerspectiveClone    *clone,
                                                       GimpDrawable            *drawable);


G_DEFINE_TYPE (GimpPerspectiveClone, gimp_perspective_clone,
               GIMP_TYPE_BRUSH_CORE)


void
gimp_perspective_clone_register (Gimp                      *gimp,
                                 GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PERSPECTIVE_CLONE,
                GIMP_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                "gimp-perspective-clone",
                _("PerspectiveClone"),
                "gimp-tool-perspective-clone");
}

static void
gimp_perspective_clone_class_init (GimpPerspectiveCloneClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->set_property               = gimp_perspective_clone_set_property;
  object_class->get_property               = gimp_perspective_clone_get_property;

  paint_core_class->paint                  = gimp_perspective_clone_paint;

  brush_core_class->handles_changing_brush = TRUE;

  g_object_class_install_property (object_class, PROP_SRC_DRAWABLE,
                                   g_param_spec_object ("src-drawable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_X,
                                   g_param_spec_double ("src-x", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_Y,
                                   g_param_spec_double ("src-y", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_perspective_clone_init (GimpPerspectiveClone *clone)
{
  clone->set_source   = FALSE;

  clone->src_drawable = NULL;
  clone->src_x        = 0.0;
  clone->src_y        = 0.0;

  clone->orig_src_x   = 0.0;
  clone->orig_src_y   = 0.0;

  clone->dest_x       = 0.0;    /* coords where the stroke starts */
  clone->dest_y       = 0.0;

  clone->src_x_fv     = 0.0;    /* source coords in front_view perspective */
  clone->src_y_fv     = 0.0;

  clone->dest_x_fv    = 0.0;    /* destination coords in front_view perspective */
  clone->dest_y_fv    = 0.0;

  clone->offset_x     = 0.0;    /* offset from the source to the stroke that's being painted */
  clone->offset_y     = 0.0;
  clone->first_stroke = TRUE;

  gimp_matrix3_identity (&clone->transform);
  gimp_matrix3_identity (&clone->transform_inv);
}

static void
gimp_perspective_clone_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpPerspectiveClone *clone = GIMP_PERSPECTIVE_CLONE (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      gimp_perspective_clone_set_src_drawable (clone, g_value_get_object (value));
      break;
    case PROP_SRC_X:
      clone->src_x = g_value_get_double (value);
      break;
    case PROP_SRC_Y:
      clone->src_y = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_perspective_clone_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpPerspectiveClone *clone = GIMP_PERSPECTIVE_CLONE (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      g_value_set_object (value, clone->src_drawable);
      break;
    case PROP_SRC_X:
      g_value_set_int (value, clone->src_x);
      break;
    case PROP_SRC_Y:
      g_value_set_int (value, clone->src_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_perspective_clone_paint (GimpPaintCore    *paint_core,
                              GimpDrawable     *drawable,
                              GimpPaintOptions *paint_options,
                              GimpPaintState    paint_state,
                              guint32           time)
{
  GimpPerspectiveClone        *clone   = GIMP_PERSPECTIVE_CLONE (paint_core);
  GimpPerspectiveCloneOptions *options = GIMP_PERSPECTIVE_CLONE_OPTIONS (paint_options);
  GimpContext                 *context = GIMP_CONTEXT (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (clone->set_source)
        {
          gimp_perspective_clone_set_src_drawable (clone, drawable);

          clone->src_x = paint_core->cur_coords.x;
          clone->src_y = paint_core->cur_coords.y;

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        clone->src_x, clone->src_y,
                                        &clone->src_x_fv, &clone->src_y_fv);

          clone->first_stroke = TRUE;
        }
      else if (options->align_mode == GIMP_SOURCE_ALIGN_NO)
        {
          clone->orig_src_x = clone->src_x;
          clone->orig_src_y = clone->src_y;

          clone->first_stroke = TRUE;
        }

      if (options->clone_type == GIMP_PATTERN_CLONE)
        if (! gimp_context_get_pattern (context))
          g_message (_("No patterns available for this operation."));
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (clone->set_source)
        {
          /*  If the control key is down, move the src target and return */

          clone->src_x = paint_core->cur_coords.x;      /* coords where the stroke starts */
          clone->src_y = paint_core->cur_coords.y;

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        clone->src_x, clone->src_y,
                                        &clone->src_x_fv, &clone->src_y_fv);

          clone->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint dest_x;
          gint dest_y;

          dest_x = paint_core->cur_coords.x;
          dest_y = paint_core->cur_coords.y;

          if (options->align_mode == GIMP_SOURCE_ALIGN_REGISTERED)
            {
              clone->offset_x = 0;
              clone->offset_y = 0;
            }
          else if (options->align_mode == GIMP_SOURCE_ALIGN_FIXED)
            {
              clone->offset_x = clone->src_x - dest_x;
              clone->offset_y = clone->src_y - dest_y;
            }
          else if (clone->first_stroke)
            {
              clone->offset_x = clone->src_x - dest_x;
              clone->offset_y = clone->src_y - dest_y;

              clone->dest_x = dest_x;       /* cooords where start the destination stroke */
              clone->dest_y = dest_y;

              /* get destination coordinates in front view perspective */
              gimp_matrix3_transform_point (&clone->transform_inv,
                                            clone->dest_x, clone->dest_y,
                                            &clone->dest_x_fv, &clone->dest_y_fv);

              clone->first_stroke = FALSE;
            }

          gimp_perspective_clone_motion (paint_core, drawable, paint_options);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      break;

    default:
      break;
    }

  g_object_notify (G_OBJECT (clone), "src-x");
  g_object_notify (G_OBJECT (clone), "src-y");
}

static void
gimp_perspective_clone_motion (GimpPaintCore    *paint_core,
                               GimpDrawable     *drawable,
                               GimpPaintOptions *paint_options)
{
  GimpPerspectiveClone        *clone        = GIMP_PERSPECTIVE_CLONE (paint_core);
  GimpPerspectiveCloneOptions *options      = GIMP_PERSPECTIVE_CLONE_OPTIONS (paint_options);
  GimpContext                 *context      = GIMP_CONTEXT (paint_options);
  GimpImage                   *image;
  GimpImage                   *src_image    = NULL;
  GimpPickable                *src_pickable = NULL;
  guchar                      *s;
  guchar                      *d;
  TempBuf                     *area;
  gpointer                    pr = NULL;
  gint                        y;
  gint                        x1d, y1d, x2d, y2d;                      /* Coordinates of the destination area to paint */
  gdouble                     x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s;  /* Coordinates of the boundary box to copy pixels to the tempbuf and after apply perspective transform */
  gint                        itemp_x, itemp_y;
  gint                        xmin, ymin, xmax, ymax;
  TileManager                 *src_tiles;
  PixelRegion                 srcPR, destPR, auxPR, auxPR2;
  GimpPattern                 *pattern = NULL;
  gdouble                     opacity;
  gint                        offset_x;
  gint                        offset_y;

  guchar                      *src_data;
  guchar                      *dest_data;
  gint                        i, j;
  gdouble                     temp_x, temp_y;


  image = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity = gimp_paint_options_get_fade (paint_options, image,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  /*  make local copies because we change them  */
  offset_x = clone->offset_x;
  offset_y = clone->offset_y;

  /*  Make sure we still have a source if we are doing image cloning */
  if (options->clone_type == GIMP_IMAGE_CLONE)
    {
      if (! clone->src_drawable)
        return;

      src_pickable = GIMP_PICKABLE (clone->src_drawable);
      src_image    = gimp_pickable_get_image (src_pickable);

      if (options->sample_merged)
        {
          gint off_x, off_y;

          src_pickable = GIMP_PICKABLE (src_image->projection);

          /* To the offset of the layer add the offset of the layer respect the entire image*/
          gimp_item_offsets (GIMP_ITEM (clone->src_drawable), &off_x, &off_y);

          // May I delete this offset, which is it function???
          offset_x += off_x;
          offset_y += off_y;
        }

      gimp_pickable_flush (src_pickable);
    }

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  switch (options->clone_type)
    {
      TempBuf *orig, *temp_buf;

    case GIMP_IMAGE_CLONE:
      /*  Set the paint area to transparent  */
      temp_buf_data_clear (area);

      src_tiles = gimp_pickable_get_tiles (src_pickable);

      /* Destination coordinates that will be painted */
      x1d = area->x;
      y1d = area->y;
      x2d = area->x + area->width;
      y2d = area->y + area->height;

      /* Boundary box for source pixels to copy: Convert all the
       * vertex of the box to paint in destination area to its
       * correspondent in source area bearing in mind perspective
       */
      gimp_perspective_clone_get_source_point (clone, x1d, y1d, &x1s, &y1s);
      gimp_perspective_clone_get_source_point (clone, x1d, y2d, &x2s, &y2s);
      gimp_perspective_clone_get_source_point (clone, x2d, y1d, &x3s, &y3s);
      gimp_perspective_clone_get_source_point (clone, x2d, y2d, &x4s, &y4s);

      xmin = floor (MIN4 (x1s, x2s, x3s, x4s));
      ymin = floor (MIN4 (y1s, y2s, y3s, y4s));
      xmax = ceil  (MAX4 (x1s, x2s, x3s, x4s));
      ymax = ceil  (MAX4 (y1s, y2s, y3s, y4s));

      xmin = CLAMP (xmin - 1, 0, tile_manager_width  (src_tiles));
      ymin = CLAMP (ymin - 1, 0, tile_manager_height (src_tiles));
      xmax = CLAMP (xmax + 1, 0, tile_manager_width  (src_tiles));
      ymax = CLAMP (ymax + 1, 0, tile_manager_height (src_tiles));

      /* if the source area is completely out of the image */
      if (!(xmax - xmin) || !(ymax - ymin))
        return;

      /*  If the source image is different from the destination,
       *  then we should copy straight from the destination image
       *  to the canvas.
       *  Otherwise, we need a call to get_orig_image to make sure
       *  we get a copy of the unblemished (offset) image
       */
      if ((  options->sample_merged && (src_image           != image)) ||
          (! options->sample_merged && (clone->src_drawable != drawable)))
        {
          /*  get the original image  */
          pixel_region_init (&auxPR, src_tiles,
                             xmin, ymin, xmax - xmin, ymax - ymin,
                             FALSE);
          orig = temp_buf_new (xmax - xmin, ymax - ymin, auxPR.bytes,
                               0, 0, NULL);

          pixel_region_init_temp_buf (&auxPR2, orig,
                                      0, 0, xmax - xmin, ymax - ymin);
          copy_region (&auxPR, &auxPR2);
        }
      else
        {
          /*  get the original image  */
          if (options->sample_merged)
            orig = gimp_paint_core_get_orig_proj (paint_core,
                                                  src_pickable,
                                                  xmin, ymin, xmax, ymax);
          else
            orig = gimp_paint_core_get_orig_image (paint_core,
                                                   GIMP_DRAWABLE (src_pickable),
                                                   xmin, ymin, xmax, ymax);
        }

      /* note: orig is a TempBuf where are all the pixels that I need to copy */
      /* copy from orig to temp_buf, this buffer has the size of the destination area */
      /* Also allocate memory for alpha channel */
      temp_buf = temp_buf_new (x2d-x1d, y2d-y1d, orig->bytes+1, 0, 0, NULL);

      src_data  = temp_buf_data (orig);
      dest_data = temp_buf_data (temp_buf);

      for (i = x1d; i < x2d; i++)
        {
          for (j = y1d; j < y2d; j++)
            {
              guchar *dest_pixel;

              gimp_perspective_clone_get_source_point (clone,
                                                       i, j,
                                                       &temp_x, &temp_y);

              itemp_x = (gint) temp_x;
              itemp_y = (gint) temp_y;

              /* Points to the dest pixel in temp_buf*/
              dest_pixel = dest_data + (gint)((j-y1d) * temp_buf->width + (i-x1d)) * temp_buf->bytes;

              /* Check if the source pixel is inside the image*/
              if (itemp_x > 0 &&
                  itemp_x < (tile_manager_width (src_tiles)-1) &&
                  itemp_y > 0 &&
                  itemp_y < (tile_manager_height (src_tiles)-1))
                {
                  guchar  *src_pixel;
                  guchar  *color1 = g_alloca (temp_buf->bytes - 1);
                  guchar  *color2 = g_alloca (temp_buf->bytes - 1);
                  guchar  *color3 = g_alloca (temp_buf->bytes - 1);
                  guchar  *color4 = g_alloca (temp_buf->bytes - 1);
                  gdouble  dx, dy;
                  gint     k;

                  dx = temp_x - itemp_x;
                  dy = temp_y - itemp_y;

                  /* linear interpolation */
                  /* (i,j)*((1-dx)*(1-dy) + (i+1,j)*(dx*(1-dy)) + (i,j+1)*((1-dx)*dy) + (i+1,j+1)*(dx*dy) */
                  src_pixel  = src_data + (gint)((itemp_y-ymin) * orig->width + (itemp_x-xmin)) * orig->bytes;
                  for (k = 0 ; k < temp_buf->bytes-1; k++) color1[k] = *src_pixel++;

                  src_pixel  = src_data + (gint)((itemp_y-ymin) * orig->width + (itemp_x+1-xmin)) * orig->bytes;
                  for (k = 0 ; k < temp_buf->bytes-1; k++) color2[k] = *src_pixel++;

                  src_pixel  = src_data + (gint)((itemp_y+1-ymin) * orig->width + (itemp_x-xmin)) * orig->bytes;
                  for (k = 0 ; k < temp_buf->bytes-1; k++) color3[k] = *src_pixel++;

                  src_pixel  = src_data + (gint)((itemp_y+1-ymin) * orig->width + (itemp_x+1-xmin)) * orig->bytes;
                  for (k = 0 ; k < temp_buf->bytes-1; k++) color4[k] = *src_pixel++;

                  /* copy the pixel interpolated to temp_buf */
                  for (k = 0 ; k < temp_buf->bytes-1; k++)
                    *dest_pixel++ = color1[k]*((1-dx)*(1-dy)) + color2[k]*(dx*(1-dy)) + color3[k]*((1-dx)*dy) + color4[k]*(dx*dy);

                  /* If the pixel is inside the image set the alpha channel visible */
                  *dest_pixel = 255;
                }
              else
                {
                  dest_pixel[temp_buf->bytes-1] = 0; /* Pixels with source out-of-image are transparent */
                }
            }
        }

      pixel_region_init_temp_buf (&srcPR, temp_buf,
                                  0, 0, x2d - x1d, y2d - y1d);

      /*  configure the destination  */
      pixel_region_init_temp_buf (&destPR, area, 0, 0, srcPR.w, srcPR.h);

      pr = pixel_regions_register (2, &srcPR, &destPR);
      break;

    case GIMP_PATTERN_CLONE:
      pattern = gimp_context_get_pattern (context);

      if (!pattern)
        return;

      /*  configure the destination  */
      pixel_region_init_temp_buf (&destPR, area,
                                  0, 0, area->width, area->height);

      pr = pixel_regions_register (1, &destPR);
      break;
    }

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      s = srcPR.data;
      d = destPR.data;

      for (y = 0; y < destPR.h; y++)
        {
          switch (options->clone_type)
            {
            case GIMP_IMAGE_CLONE:
              gimp_perspective_clone_line_image (image, src_image,
                                                 drawable, src_pickable,
                                                 s, d,
                                                 srcPR.bytes, destPR.bytes, destPR.w);
              s += srcPR.rowstride;
              break;

            case GIMP_PATTERN_CLONE:
              gimp_perspective_clone_line_pattern (image, drawable,
                                                   pattern, d,
                                                   area->x     + offset_x,
                                                   area->y + y + offset_y,
                                                   destPR.bytes, destPR.w);
              break;
            }

          d += destPR.rowstride;
        }
    }

  if (paint_options->pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_brush_core_paste_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),

                                /* In fixed mode, paint incremental so the
                                 * individual brushes are properly applied
                                 * on top of each other.
                                 * Otherwise the stuff we paint is seamless
                                 * and we don't need intermediate masking.
                                 */
                                options->align_mode == GIMP_SOURCE_ALIGN_FIXED ?
                                GIMP_PAINT_INCREMENTAL : GIMP_PAINT_CONSTANT);
}

static void
gimp_perspective_clone_line_image (GimpImage    *dest,
                                   GimpImage    *src,
                                   GimpDrawable *d_drawable,
                                   GimpPickable *s_pickable,
                                   guchar       *s,
                                   guchar       *d,
                                   gint          src_bytes,
                                   gint          dest_bytes,
                                   gint          width)
{
  guchar rgba[MAX_CHANNELS];
  gint   alpha;

  alpha = dest_bytes - 1;

  while (width--)
    {
      gimp_image_get_color (src, gimp_pickable_get_image_type (s_pickable),
                            s, rgba);

      gimp_image_transform_color (dest, d_drawable, d, GIMP_RGB, rgba);

      d[alpha] = s[src_bytes-1]; /* This line is diferent that in gimpclone !! */

      s += src_bytes;
      d += dest_bytes;
    }
}

static void
gimp_perspective_clone_line_pattern (GimpImage    *dest,
                                     GimpDrawable *drawable,
                                     GimpPattern  *pattern,
                                     guchar       *d,
                                     gint          x,
                                     gint          y,
                                     gint          bytes,
                                     gint          width)
{
  guchar            *pat, *p;
  GimpImageBaseType  color_type;
  gint               alpha;
  gint               pat_bytes;
  gint               i;

  pat_bytes = pattern->mask->bytes;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pattern->mask->width;
  while (y < 0)
    y += pattern->mask->height;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_data (pattern->mask) +
    (y % pattern->mask->height) * pattern->mask->width * pat_bytes;

  color_type = (pat_bytes == 3 ||
                pat_bytes == 4) ? GIMP_RGB : GIMP_GRAY;

  alpha = bytes - 1;

  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->mask->width) * pat_bytes;

      gimp_image_transform_color (dest, drawable, d, color_type, p);

      if (pat_bytes == 2 || pat_bytes == 4)
        d[alpha] = p[pat_bytes - 1];
      else
        d[alpha] = OPAQUE_OPACITY;

      d += bytes;
    }
}

static void
gimp_perspective_clone_src_drawable_removed (GimpDrawable         *drawable,
                                             GimpPerspectiveClone *clone)
{
  if (drawable == clone->src_drawable)
    {
      clone->src_drawable = NULL;
    }

  g_signal_handlers_disconnect_by_func (drawable,
                                        gimp_perspective_clone_src_drawable_removed,
                                        clone);
}

static void
gimp_perspective_clone_set_src_drawable (GimpPerspectiveClone *clone,
                                         GimpDrawable         *drawable)
{
  if (clone->src_drawable == drawable)
    return;

  if (clone->src_drawable)
    g_signal_handlers_disconnect_by_func (clone->src_drawable,
                                          gimp_perspective_clone_src_drawable_removed,
                                          clone);

  clone->src_drawable = drawable;

  if (clone->src_drawable)
    g_signal_connect (clone->src_drawable, "removed",
                      G_CALLBACK (gimp_perspective_clone_src_drawable_removed),
                      clone);

  g_object_notify (G_OBJECT (clone), "src-drawable");
}

void
gimp_perspective_clone_get_source_point (GimpPerspectiveClone *perspective_clone,
                                         gdouble               x,
                                         gdouble               y,
                                         gdouble              *newx,
                                         gdouble              *newy)
{
  gdouble temp_x, temp_y;
  gdouble offset_x_fv, offset_y_fv;

  gimp_matrix3_transform_point (&perspective_clone->transform_inv,
                                x, y, &temp_x, &temp_y);

  /* Get the offset of each pixel in destination area from the
   * destination pixel in front view perspective
   */
  offset_x_fv = temp_x - perspective_clone->dest_x_fv;
  offset_y_fv = temp_y - perspective_clone->dest_y_fv;

  /* Get the source pixel in front view perspective */
  temp_x = offset_x_fv + perspective_clone->src_x_fv;
  temp_y = offset_y_fv + perspective_clone->src_y_fv;

  /* Convert the source pixel to perspective view */
  gimp_matrix3_transform_point (&perspective_clone->transform,
                                temp_x, temp_y, newx, newy);
}
