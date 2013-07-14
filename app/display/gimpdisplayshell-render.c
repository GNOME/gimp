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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "base/tile-manager.h"
#include "base/tile.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scroll.h"


#define GIMP_DISPLAY_ZOOM_FAST     (1 << 0) /* use the fastest possible code
                                               path trading quality for speed
                                             */
#define GIMP_DISPLAY_ZOOM_PIXEL_AA (1 << 1) /* provide AA edges when zooming in
                                               on the actual pixels (in current
                                               code only enables it between
                                               100% and 200% zoom)
                                             */


typedef struct _RenderInfo  RenderInfo;

typedef void (* RenderFunc) (RenderInfo *info);

struct _RenderInfo
{
  TileManager  *src_tiles;
  const guchar *src;
  gboolean      src_is_premult;
  guchar       *dest;
  gint          x, y;
  gint          w, h;
  gdouble       scalex;       /* scale from (pre-scaled) src to dest       */
  gdouble       scaley;
  gdouble       full_scalex;  /* actual display scale factor               */
  gdouble       full_scaley;
  gint          src_x;
  gint          src_y;
  gint          dest_bpl;

  gint          zoom_quality;

  /* Bresenham helpers */
  gint          x_dest_inc;   /* amount to increment for each dest. pixel  */
  gint          x_src_dec;    /* amount to decrement for each source pixel */
  gint64        dx_start;     /* pixel fraction for first pixel            */

  gint          y_dest_inc;
  gint          y_src_dec;
  gint64        dy_start;

  gint          footprint_x;
  gint          footprint_y;
  gint          footshift_x;
  gint          footshift_y;

  gint64        dy;
};


static guchar tile_buf[GIMP_DISPLAY_RENDER_BUF_WIDTH * MAX_CHANNELS];


static void  gimp_display_shell_render_info_init (RenderInfo       *info,
                                                  GimpDisplayShell *shell,
                                                  gint              x,
                                                  gint              y,
                                                  gint              w,
                                                  gint              h,
                                                  cairo_surface_t  *dest,
                                                  TileManager      *tiles,
                                                  gint              level,
                                                  gboolean          is_premult);

/*  Render Image functions  */

static void           render_image_alpha         (RenderInfo       *info);
static void           render_image_gray_a        (RenderInfo       *info);
static void           render_image_rgb_a         (RenderInfo       *info);

static const guchar * render_image_tile_fault    (RenderInfo       *info);


/*****************************************************************/
/*  This function is the core of the display -- it offsets and   */
/*  scales the image according to the current parameters in the  */
/*  display object.  It handles RGBA and GRAYA projection tiles  */
/*  and renders them to an ARGB32 cairo surface.                 */
/*****************************************************************/

void
gimp_display_shell_render (GimpDisplayShell *shell,
                           cairo_t          *cr,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h)
{
  GimpProjection *projection;
  GimpImage      *image;
  TileManager    *tiles;
  RenderInfo      info;
  GimpImageType   type;
  gint            level;
  gboolean        premult;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (w > 0 && h > 0);

  image = gimp_display_get_image (shell->display);
  projection = gimp_image_get_projection (image);

  /* setup RenderInfo for rendering a GimpProjection level. */
  level = gimp_projection_get_level (projection,
                                     shell->scale_x, shell->scale_y);

  tiles = gimp_projection_get_tiles_at_level (projection, level, &premult);

  gimp_display_shell_render_info_init (&info,
                                       shell, x, y, w, h,
                                       shell->render_surface,
                                       tiles, level, premult);

  /* Currently, only RGBA and GRAYA projection types are used. */
  type = gimp_pickable_get_image_type (GIMP_PICKABLE (projection));

  switch (type)
    {
    case GIMP_RGBA_IMAGE:
      render_image_rgb_a (&info);
      break;
    case GIMP_GRAYA_IMAGE:
      render_image_gray_a (&info);
      break;
    default:
      g_warning ("%s: unsupported projection type (%d)", G_STRFUNC, type);
      g_assert_not_reached ();
    }

  /*  apply filters to the rendered projection  */
  if (shell->filter_stack)
    {
      cairo_surface_t *sub = shell->render_surface;

      if (w != GIMP_DISPLAY_RENDER_BUF_WIDTH ||
          h != GIMP_DISPLAY_RENDER_BUF_HEIGHT)
        sub = cairo_image_surface_create_for_data (cairo_image_surface_get_data (sub),
                                                   CAIRO_FORMAT_ARGB32, w, h,
                                                   GIMP_DISPLAY_RENDER_BUF_WIDTH * 4);

      gimp_color_display_stack_convert_surface (shell->filter_stack, sub);

      if (sub != shell->render_surface)
        cairo_surface_destroy (sub);
    }

  cairo_surface_mark_dirty_rectangle (shell->render_surface, 0, 0, w, h);

  if (shell->mask)
    {
      if (! shell->mask_surface)
        {
          shell->mask_surface =
            cairo_image_surface_create (CAIRO_FORMAT_A8,
                                        GIMP_DISPLAY_RENDER_BUF_WIDTH,
                                        GIMP_DISPLAY_RENDER_BUF_HEIGHT);
        }

      tiles = gimp_drawable_get_tiles (shell->mask);

      /* The mask does not (yet) have an image pyramid, use 0 as level, */
      gimp_display_shell_render_info_init (&info,
                                           shell, x, y, w, h,
                                           shell->mask_surface,
                                           tiles, 0, FALSE);

      render_image_alpha (&info);

      cairo_surface_mark_dirty (shell->mask_surface);
    }

  /*  put it to the screen  */
  {
    gint disp_xoffset, disp_yoffset;

    cairo_save (cr);

    gimp_display_shell_scroll_get_disp_offset (shell,
                                               &disp_xoffset, &disp_yoffset);

    cairo_rectangle (cr, x + disp_xoffset, y + disp_yoffset, w, h);
    cairo_clip (cr);

    cairo_set_source_surface (cr, shell->render_surface,
                              x + disp_xoffset, y + disp_yoffset);
    cairo_paint (cr);

    if (shell->mask)
      {
        gimp_cairo_set_source_rgba (cr, &shell->mask_color);
        cairo_mask_surface (cr, shell->mask_surface,
                            x + disp_xoffset, y + disp_yoffset);
      }

    cairo_restore (cr);
  }
}

/*  render a GRAY tile to an A8 cairo surface  */
static void
render_image_alpha (RenderInfo *info)
{
  gint y, ye;
  gint x, xe;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  info->dy = info->dy_start;
  info->src = render_image_tile_fault (info);

  while (TRUE)
    {
      const guchar *src  = info->src;
      guchar       *dest = info->dest;

      for (x = info->x; x < xe; x++, src++, dest++)
        {
          *dest = 255 - *src;
        }

      if (++y == ye)
        break;

      info->dest  += info->dest_bpl;

      info->dy    += info->y_dest_inc;
      info->src_y += info->dy / info->y_src_dec;
      info->dy     = info->dy % info->y_src_dec;

      info->src = render_image_tile_fault (info);
    }
}

/*  render a GRAYA tile to an ARGB32 cairo surface  */
static void
render_image_gray_a (RenderInfo *info)
{
  gint y, ye;
  gint x, xe;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  info->dy = info->dy_start;
  info->src = render_image_tile_fault (info);

  while (TRUE)
    {
      const guchar *src  = info->src;
      guint32      *dest = (guint32 *) info->dest;

      for (x = info->x; x < xe; x++, src += 2, dest++)
        {
          /*  data in src is premultiplied already  */
          *dest = (src[1] << 24) | (src[0] << 16) | (src[0] << 8) | src[0];
        }

      if (++y == ye)
        break;

      info->dest  += info->dest_bpl;

      info->dy    += info->y_dest_inc;
      info->src_y += info->dy / info->y_src_dec;
      info->dy     = info->dy % info->y_src_dec;

      info->src = render_image_tile_fault (info);
    }
}

/*  render an RGBA tile to an ARGB32 cairo surface  */
static void
render_image_rgb_a (RenderInfo *info)
{
  gint y, ye;
  gint x, xe;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  info->dy = info->dy_start;
  info->src = render_image_tile_fault (info);

  while (TRUE)
    {
      const guchar *src  = info->src;
      guint32      *dest = (guint32 *) info->dest;

      for (x = info->x; x < xe; x++, src += 4, dest++)
        {
          /*  data in src is premultiplied already  */
          *dest = (src[3] << 24) | (src[0] << 16) | (src[1] << 8) | src[2];
        }

      if (++y == ye)
        break;

      info->dest  += info->dest_bpl;

      info->dy    += info->y_dest_inc;
      info->src_y += info->dy / info->y_src_dec;
      info->dy     = info->dy % info->y_src_dec;

      if (info->src_y >= 0)
        info->src = render_image_tile_fault (info);
    }
}

static void
gimp_display_shell_render_info_init (RenderInfo       *info,
                                     GimpDisplayShell *shell,
                                     gint              x,
                                     gint              y,
                                     gint              w,
                                     gint              h,
                                     cairo_surface_t  *dest,
                                     TileManager      *tiles,
                                     gint              level,
                                     gboolean          is_premult)
{
  gint offset_x;
  gint offset_y;

  gimp_display_shell_scroll_get_render_start_offset (shell,
						     &offset_x, &offset_y);

  info->x = x + offset_x;
  info->y = y + offset_y;
  info->w = w;
  info->h = h;

  /* This function must be called before switching from drawing
   * on the surface with cairo to drawing on it directly
   */
  cairo_surface_flush (dest);

  info->dest        = cairo_image_surface_get_data (dest);
  info->dest_bpl    = cairo_image_surface_get_stride (dest);

  info->src_tiles      = tiles;
  info->src_is_premult = is_premult;

  info->scalex      = shell->scale_x * (1 << level);
  info->scaley      = shell->scale_y * (1 << level);

  info->full_scalex = shell->scale_x;
  info->full_scaley = shell->scale_y;

  /* use Bresenham like stepping */
  info->x_dest_inc  = shell->x_dest_inc >> level;
  info->x_src_dec   = shell->x_src_dec;

  info->dx_start    = ((gint64) info->x_dest_inc * info->x
                       + info->x_dest_inc / 2);

  info->src_x       = info->dx_start / info->x_src_dec;
  info->dx_start    = info->dx_start % info->x_src_dec;

  /* same for y */
  info->y_dest_inc  = shell->y_dest_inc >> level;
  info->y_src_dec   = shell->y_src_dec;

  info->dy_start    = ((gint64) info->y_dest_inc * info->y
                       + info->y_dest_inc / 2);

  info->src_y       = info->dy_start / info->y_src_dec;
  info->dy_start    = info->dy_start % info->y_src_dec;

  /* make sure that the footprint is in the range 256..512 */
  info->footprint_x = info->x_src_dec;
  info->footshift_x = 0;
  while (info->footprint_x > 512)
    {
      info->footprint_x >>= 1;
      info->footshift_x --;
    }
  while (info->footprint_x < 256)
    {
      info->footprint_x <<= 1;
      info->footshift_x ++;
    }

  info->footprint_y = info->y_src_dec;
  info->footshift_y = 0;
  while (info->footprint_y > 512)
    {
      info->footprint_y >>= 1;
      info->footshift_y --;
    }
  while (info->footprint_y < 256)
    {
      info->footprint_y <<= 1;
      info->footshift_y ++;
    }

  switch (shell->display->config->zoom_quality)
    {
    case GIMP_ZOOM_QUALITY_LOW:
      info->zoom_quality = GIMP_DISPLAY_ZOOM_FAST;
      break;

    case GIMP_ZOOM_QUALITY_HIGH:
      info->zoom_quality = GIMP_DISPLAY_ZOOM_PIXEL_AA;
      break;
    }
}

/* This version assumes that the src data is already pre-multiplied. */
static inline void
box_filter (const guint    left_weight,
            const guint    center_weight,
            const guint    right_weight,
            const guint    top_weight,
            const guint    middle_weight,
            const guint    bottom_weight,
            const guchar **src,   /* the 9 surrounding source pixels */
            guchar        *dest,
            const gint     bpp)
{
  const guint sum = ((left_weight + center_weight + right_weight) *
                     (top_weight + middle_weight + bottom_weight));
  gint i;

  for (i = 0; i < bpp; i++)
    {
      dest[i] = ( left_weight   * ((src[0][i] * top_weight) +
                                   (src[3][i] * middle_weight) +
                                   (src[6][i] * bottom_weight))
                + center_weight * ((src[1][i] * top_weight) +
                                   (src[4][i] * middle_weight) +
                                   (src[7][i] * bottom_weight))
                + right_weight  * ((src[2][i] * top_weight) +
                                   (src[5][i] * middle_weight) +
                                   (src[8][i] * bottom_weight))) / sum;
    }
}

/* This version assumes that the src data is not pre-multipled.
 * It creates pre-multiplied output though.
 */
static inline void
box_filter_premult (const guint    left_weight,
                    const guint    center_weight,
                    const guint    right_weight,
                    const guint    top_weight,
                    const guint    middle_weight,
                    const guint    bottom_weight,
                    const guchar **src,   /* the 9 surrounding source pixels */
                    guchar        *dest,
                    const gint     bpp)
{
  const guint sum = ((left_weight + center_weight + right_weight) *
                     (top_weight + middle_weight + bottom_weight)) >> 4;

  switch (bpp)
    {
      case 4:
#define ALPHA 3
        {
          const guint factors[9] =
            {
              (src[1][ALPHA] * top_weight)    >> 4,
              (src[4][ALPHA] * middle_weight) >> 4,
              (src[7][ALPHA] * bottom_weight) >> 4,
              (src[2][ALPHA] * top_weight)    >> 4,
              (src[5][ALPHA] * middle_weight) >> 4,
              (src[8][ALPHA] * bottom_weight) >> 4,
              (src[0][ALPHA] * top_weight)    >> 4,
              (src[3][ALPHA] * middle_weight) >> 4,
              (src[6][ALPHA] * bottom_weight) >> 4
            };

          const guint a =
            (center_weight * (factors[0] + factors[1] + factors[2]) +
             right_weight  * (factors[3] + factors[4] + factors[5]) +
             left_weight   * (factors[6] + factors[7] + factors[8]));

          guint i;

          for (i = 0; i < ALPHA; i++)
            {
              dest[i] =  (center_weight * (factors[0] * src[1][i] +
                                           factors[1] * src[4][i] +
                                           factors[2] * src[7][i]) +

                          right_weight  * (factors[3] * src[2][i] +
                                           factors[4] * src[5][i] +
                                           factors[5] * src[8][i]) +

                          left_weight   * (factors[6] * src[0][i] +
                                           factors[7] * src[3][i] +
                                           factors[8] * src[6][i]) + ((255 * sum) >> 1)) / (255 * sum);
            }

          dest[ALPHA] = (a + (sum >> 1)) / sum;
        }
#undef ALPHA
        break;

      case 2:
#define ALPHA 1

        /* NOTE: this is a copy and paste of the code above, ALPHA changes
         *       the behavior in all needed ways.
         */
        {
          const guint factors[9] =
            {
              (src[1][ALPHA] * top_weight)    >> 4,
              (src[4][ALPHA] * middle_weight) >> 4,
              (src[7][ALPHA] * bottom_weight) >> 4,
              (src[2][ALPHA] * top_weight)    >> 4,
              (src[5][ALPHA] * middle_weight) >> 4,
              (src[8][ALPHA] * bottom_weight) >> 4,
              (src[0][ALPHA] * top_weight)    >> 4,
              (src[3][ALPHA] * middle_weight) >> 4,
              (src[6][ALPHA] * bottom_weight) >> 4
            };

          const guint a =
            (center_weight * (factors[0] + factors[1] + factors[2]) +
             right_weight  * (factors[3] + factors[4] + factors[5]) +
             left_weight   * (factors[6] + factors[7] + factors[8]));

          guint i;

          for (i = 0; i < ALPHA; i++)
            {
              dest[i] =  (center_weight * (factors[0] * src[1][i] +
                                           factors[1] * src[4][i] +
                                           factors[2] * src[7][i]) +

                          right_weight  * (factors[3] * src[2][i] +
                                           factors[4] * src[5][i] +
                                           factors[5] * src[8][i]) +

                          left_weight   * (factors[6] * src[0][i] +
                                           factors[7] * src[3][i] +
                                           factors[8] * src[6][i]) + ((255 * sum) >> 1)) / (sum * 255);
            }

          dest[ALPHA] = (a + (sum >> 1)) / sum;
        }
#undef ALPHA
        break;

      default:
        g_warning ("bpp=%i not implemented as box filter", bpp);
        break;
    }
}


/* fast paths */
static const guchar * render_image_tile_fault_one_row  (RenderInfo *info);
static const guchar * render_image_tile_fault_nearest  (RenderInfo *info);

/*  012 <- this is the order of the numbered source tiles / pixels.
 *  345    for the 3x3 neighbourhoods.
 *  678
 */

/* Function to render a horizontal line of view data.  The data
 * returned from this function has the alpha channel pre-multiplied.
 */
static const guchar *
render_image_tile_fault (RenderInfo *info)
{
  Tile         *tile[9];
  const guchar *src[9];

  guchar       *dest;
  gint          width;
  gint          tilex0;   /* the current x-tile indice used for the middle
                             sample pair*/
  gint          tilex1;   /* the current x-tile indice used for the right
                             sample pair */
  gint          tilexL;   /* the current x-tile indice used for the left
                             sample pair */
  gint          bpp;
  gint          dx;
  gint          src_x;
  gint          skipped;

  guint         left_weight;
  guint         center_weight;
  guint         right_weight;

  guint         top_weight;
  guint         middle_weight;
  guint         bottom_weight;

  guint         source_width;
  guint         source_height;

  source_width  = tile_manager_width (info->src_tiles);
  source_height = tile_manager_height (info->src_tiles);

  /* dispatch to fast path functions on special conditions */
  if ((info->zoom_quality & GIMP_DISPLAY_ZOOM_FAST)

      /* use nearest neighbour for exact levels */
      || (info->scalex == 1.0 &&
          info->scaley == 1.0)

      /* or when we're larger than 1.0 and not using any AA */
      || (info->full_scalex > 1.0 &&
          info->full_scaley > 1.0 &&
          (! (info->zoom_quality & GIMP_DISPLAY_ZOOM_PIXEL_AA)))

      /* or at any point when both scale factors are greater or equal to 200% */
      || (info->full_scalex >= 2.0 &&
          info->full_scaley >= 2.0 )

      /* or when we're scaling a 1bpp texture, this code-path seems to be
       * invoked when interacting with SIOX which uses a palletized drawable
       */
      || (tile_manager_bpp (info->src_tiles)==1)
      )
    {
      return render_image_tile_fault_nearest (info);
    }
  else if (((info->src_y)     & ~(TILE_WIDTH -1)) ==
           ((info->src_y + 1) & ~(TILE_WIDTH -1)) &&
           ((info->src_y)     & ~(TILE_WIDTH -1)) ==
           ((info->src_y - 1) & ~(TILE_WIDTH -1)) &&
           (info->src_y + 1 < source_height)
          )
    {
      /* all the tiles needed are in a single row, use a tile iterator
       * optimized for this case. */
      return render_image_tile_fault_one_row (info);
    }

  top_weight    = MAX (info->footprint_y / 2,
                       info->footshift_y > 0 ? info->dy << info->footshift_y
                                             : info->dy >> -info->footshift_y)
                  - (info->footshift_y > 0 ? info->dy << info->footshift_y
                                           : info->dy >> -info->footshift_y);

  bottom_weight = MAX (info->footprint_y / 2,
                       info->footshift_y > 0 ? info->dy << info->footshift_y
                                             : info->dy >> -info->footshift_y)
                  - info->footprint_y / 2;

  middle_weight = info->footprint_y - top_weight - bottom_weight;

  tile[4] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x, info->src_y,
                                   TRUE, FALSE);
  tile[7] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x, info->src_y + 1,
                                   TRUE, FALSE);
  tile[1] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x, info->src_y - 1,
                                   TRUE, FALSE);

  tile[5] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x + 1, info->src_y,
                                   TRUE, FALSE);
  tile[8] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x + 1, info->src_y + 1,
                                   TRUE, FALSE);
  tile[2] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x + 1, info->src_y - 1,
                                   TRUE, FALSE);

  tile[3] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x - 1, info->src_y,
                                   TRUE, FALSE);
  tile[6] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x - 1, info->src_y + 1,
                                   TRUE, FALSE);
  tile[0] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x - 1, info->src_y - 1,
                                   TRUE, FALSE);

  g_return_val_if_fail (tile[4] != NULL, tile_buf);

  src[4] = tile_data_pointer (tile[4], info->src_x, info->src_y);

  if (tile[5])
    {
      src[5] = tile_data_pointer (tile[5], info->src_x + 1, info->src_y);
    }
  else
    {
      src[5] = src[4];  /* reusing existing pixel data */
    }

  if (tile[7])
    {
      src[7] = tile_data_pointer (tile[7], info->src_x, info->src_y + 1);
    }
  else
    {
      src[7] = src[4];  /* reusing existing pixel data */
    }

  if (tile[1])
    {
      src[1] = tile_data_pointer (tile[1], info->src_x, info->src_y - 1);
    }
  else
    {
      src[1] = src[4];  /* reusing existing pixel data */
    }

  if (tile[8])
    {
      src[8] = tile_data_pointer (tile[8], info->src_x + 1, info->src_y + 1);
    }
  else
    {
      src[8] = src[5];  /* reusing existing pixel data */
    }

  if (tile[0])
    {
      src[0] = tile_data_pointer (tile[0], info->src_x - 1, info->src_y - 1);
    }
  else
    {
      src[0] = src[1];  /* reusing existing pixel data */
    }

  if (tile[2])
    {
      src[2] = tile_data_pointer (tile[2], info->src_x + 1, info->src_y - 1);
    }
  else
    {
      src[2] = src[4];  /* reusing existing pixel data */
    }

  if (tile[3])
    {
      src[3] = tile_data_pointer (tile[3], info->src_x - 1, info->src_y);
    }
  else
    {
      src[3] = src[4];  /* reusing existing pixel data */
    }

  if (tile[6])
    {
      src[6] = tile_data_pointer (tile[6], info->src_x - 1, info->src_y + 1);
    }
  else
    {
      src[6] = src[7];  /* reusing existing pixel data */
    }

  bpp    = tile_manager_bpp (info->src_tiles);
  dest   = tile_buf;

  dx     = info->dx_start;
  src_x  = info->src_x;

  width  = info->w;

  tilex0 = info->src_x / TILE_WIDTH;
  tilex1 = (info->src_x + 1) / TILE_WIDTH;
  tilexL = (info->src_x - 1) / TILE_WIDTH;

  do
    {
      /* we're dealing with unsigneds here, be extra careful */
      left_weight  = MAX (info->footprint_x / 2,
                          info->footshift_x > 0 ? dx << info->footshift_x
                                                : dx >> -info->footshift_x)
                     - (info->footshift_x > 0 ? dx << info->footshift_x
                                              : dx >> -info->footshift_x);

      right_weight = MAX (info->footprint_x / 2,
                          info->footshift_x > 0 ? dx << info->footshift_x
                                                : dx >> -info->footshift_x)
                     - info->footprint_x / 2;

      center_weight = info->footprint_x - left_weight - right_weight;

      if (src_x + 1 >= source_width)
        {
           src[2] = src[1];
           src[5] = src[4];
           src[8] = src[7];
        }

      if (info->src_y + 1 >= source_height)
        {
           src[6] = src[3];
           src[7] = src[4];
           src[8] = src[5];
        }

      if (info->src_is_premult)
        box_filter (left_weight, center_weight, right_weight,
                    top_weight, middle_weight, bottom_weight,
                    src, dest, bpp);
      else
        box_filter_premult (left_weight, center_weight, right_weight,
                            top_weight, middle_weight, bottom_weight,
                            src, dest, bpp);

      dest += bpp;

      dx += info->x_dest_inc;
      skipped = dx / info->x_src_dec;

      if (skipped)
        {
          dx -= skipped * info->x_src_dec;

          /* if we changed integer source pixel coordinates in the source
           * buffer, make sure the src pointers (and their backing tiles) are
           * correct
           */

          if (src[0] != src[1])
            {
              src[0] += skipped * bpp;
            }
          else
            {
              tilexL = -1;  /* this forces a refetch of the left most source
                               samples */
            }

          if (src[3] != src[4])
            {
              src[3] += skipped * bpp;
            }
          else
            {
              tilexL = -1;  /* this forces a refetch of the left most source
                               samples */
            }

          if (src[6] != src[7])
            {
              src[6] += skipped * bpp;
            }
          else
            {
              tilexL = -1;  /* this forces a refetch of the left most source
                               samples */
            }

          src[1] += skipped * bpp;
          src[4] += skipped * bpp;
          src[7] += skipped * bpp;

          src[5] += skipped * bpp;
          src[8] += skipped * bpp;
          src[2] += skipped * bpp;


          src_x += skipped;

          if ((src_x / TILE_WIDTH) != tilex0)
            {
              tile_release (tile[4], FALSE);

              if (tile[7])
                tile_release (tile[7], FALSE);
              if (tile[1])
                tile_release (tile[1], FALSE);

              tilex0 += 1;

              tile[4] = tile_manager_get_tile (info->src_tiles,
                                               src_x, info->src_y,
                                               TRUE, FALSE);
              tile[7] = tile_manager_get_tile (info->src_tiles,
                                               src_x, info->src_y + 1,
                                               TRUE, FALSE);
              tile[1] = tile_manager_get_tile (info->src_tiles,
                                               src_x, info->src_y - 1,
                                               TRUE, FALSE);
              if (! tile[4])
                goto done;

              src[4] = tile_data_pointer (tile[4], src_x, info->src_y);

              if (! tile[7])
                {
                  src[7] = src[4];
                }
              else
                {
                  src[7] = tile_data_pointer (tile[7],
                                              src_x, info->src_y + 1);
                }

              if (! tile[1])
                {
                  src[1] = src[4];
                }
              else
                {
                  src[1] = tile_data_pointer (tile[1],
                                              src_x, info->src_y - 1);
                }
            }

          if (((src_x + 1) / TILE_WIDTH) != tilex1)
            {
              if (tile[5])
                tile_release (tile[5], FALSE);
              if (tile[8])
                tile_release (tile[8], FALSE);
              if (tile[2])
                tile_release (tile[2], FALSE);

              tilex1 += 1;

              tile[5] = tile_manager_get_tile (info->src_tiles,
                                               src_x + 1, info->src_y,
                                               TRUE, FALSE);
              tile[8] = tile_manager_get_tile (info->src_tiles,
                                               src_x + 1, info->src_y + 1,
                                               TRUE, FALSE);
              tile[2] = tile_manager_get_tile (info->src_tiles,
                                               src_x + 1, info->src_y - 1,
                                               TRUE, FALSE);

              if (! tile[5])
                {
                  src[5] = src[4];
                }
              else
                {
                  src[5] = tile_data_pointer (tile[5],
                                              src_x + 1, info->src_y);
                }

              if (! tile[8])
                {
                  src[8] = src[7];
                }
              else
                {
                  src[8] = tile_data_pointer (tile[8],
                                              src_x + 1, info->src_y + 1);
                }

              if (! tile[2])
                {
                  src[2] = src[1];
                }
              else
                {
                  src[2] = tile_data_pointer (tile[2],
                                              src_x + 1, info->src_y - 1);
                }
            }

          if (((src_x - 1) / TILE_WIDTH) != tilexL)
            {
              if (tile[0])
                tile_release (tile[0], FALSE);
              if (tile[3])
                tile_release (tile[3], FALSE);
              if (tile[6])
                tile_release (tile[6], FALSE);

              tilexL += 1;

              tile[0] = tile_manager_get_tile (info->src_tiles,
                                               src_x - 1, info->src_y - 1,
                                               TRUE, FALSE);
              tile[3] = tile_manager_get_tile (info->src_tiles,
                                               src_x - 1, info->src_y,
                                               TRUE, FALSE);
              tile[6] = tile_manager_get_tile (info->src_tiles,
                                               src_x - 1, info->src_y + 1,
                                               TRUE, FALSE);

              if (! tile[3])
                {
                  src[3] = src[4];
                }
              else
                {
                  src[3] = tile_data_pointer (tile[3],
                                              src_x - 1, info->src_y);
                }

              if (! tile[6])
                {
                  src[6] = src[7];
                }
              else
                {
                  src[6] = tile_data_pointer (tile[6],
                                              src_x - 1, info->src_y + 1);
                }

              if (! tile[0])
                {
                  src[0] = src[1];
                }
              else
                {
                  src[0] = tile_data_pointer (tile[0],
                                              src_x - 1, info->src_y - 1);
                }
            }

        }
    }
  while (--width);

done:
  for (dx = 0; dx < 9; dx++)
    if (tile[dx])
      tile_release (tile[dx], FALSE);

  return tile_buf;
}

static const guchar *
render_image_tile_fault_one_row (RenderInfo *info)
{
  /* NOTE: there are some additional overhead that can be factored out
   * in the tile administration of this fast path
   */

  Tile         *tile[3];

  const guchar *src[9];

  guchar       *dest;
  gint          width;
  gint          tilex0;   /* the current x-tile indice used for the middle
                             sample pair*/
  gint          tilex1;   /* the current x-tile indice used for the right
                             sample pair */
  gint          tilexL;   /* the current x-tile indice used for the left
                             sample pair */
  gint          bpp;
  gint          dx;
  gint          src_x;
  gint          skipped;

  guint         left_weight;
  guint         center_weight;
  guint         right_weight;

  guint         top_weight;
  guint         middle_weight;
  guint         bottom_weight;

  guint         source_width;

  source_width = tile_manager_width (info->src_tiles);

  top_weight    = MAX (info->footprint_y / 2,
                       info->footshift_y > 0 ? info->dy << info->footshift_y
                                             : info->dy >> -info->footshift_y)
                  - (info->footshift_y > 0 ? info->dy << info->footshift_y
                                           : info->dy >> -info->footshift_y);

  bottom_weight = MAX (info->footprint_y / 2,
                       info->footshift_y > 0 ? info->dy << info->footshift_y
                                             : info->dy >> -info->footshift_y)
                  - info->footprint_y / 2;

  middle_weight = info->footprint_y - top_weight - bottom_weight;

  tile[0] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x, info->src_y, TRUE, FALSE);

  tile[1] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x + 1, info->src_y, TRUE, FALSE);

  tile[2] = tile_manager_get_tile (info->src_tiles,
                                   info->src_x - 1, info->src_y, TRUE, FALSE);

  g_return_val_if_fail (tile[0] != NULL, tile_buf);

  src[4] = tile_data_pointer (tile[0], info->src_x, info->src_y);
  src[7] = tile_data_pointer (tile[0], info->src_x, info->src_y + 1);

  if (tile[1])
    {
      src[5] = tile_data_pointer (tile[1], info->src_x + 1, info->src_y);
      src[8] = tile_data_pointer (tile[1], info->src_x + 1, info->src_y + 1);
    }
  else
    {
      src[5] = src[4];  /* reusing existing pixel data */
      src[8] = src[5];  /* reusing existing pixel data */
    }

  if (tile[0])
    {
      src[1] = tile_data_pointer (tile[0], info->src_x, info->src_y - 1);
    }
  else
    {
      src[1] = src[4];  /* reusing existing pixel data */
    }

  if (tile[2])
    {
      src[0] = tile_data_pointer (tile[2], info->src_x - 1, info->src_y - 1);
    }
  else
    {
      src[0] = src[1];  /* reusing existing pixel data */
    }

  if (tile[1])
    {
      src[2] = tile_data_pointer (tile[1], info->src_x + 1, info->src_y - 1);
    }
  else
    {
      src[2] = src[4];  /* reusing existing pixel data */
    }

  if (tile[2])
    {
      src[3] = tile_data_pointer (tile[2], info->src_x - 1, info->src_y);
      src[6] = tile_data_pointer (tile[2], info->src_x - 1, info->src_y + 1);
    }
  else
    {
      src[3] = src[4];  /* reusing existing pixel data */
      src[6] = src[7];  /* reusing existing pixel data */
    }

  bpp    = tile_manager_bpp (info->src_tiles);
  dest   = tile_buf;

  dx     = info->dx_start;
  src_x  = info->src_x;

  width  = info->w;

  tilex0 = info->src_x / TILE_WIDTH;
  tilex1 = (info->src_x + 1) / TILE_WIDTH;
  tilexL = (info->src_x - 1) / TILE_WIDTH;

  do
    {
      left_weight  = MAX (info->footprint_x / 2,
                          info->footshift_x > 0 ? dx << info->footshift_x
                                                : dx >> -info->footshift_x)
                     - (info->footshift_x > 0 ? dx << info->footshift_x
                                              : dx >> -info->footshift_x);

      right_weight = MAX (info->footprint_x / 2,
                          info->footshift_x > 0 ? dx << info->footshift_x
                                                : dx >> -info->footshift_x)
                     - info->footprint_x / 2;

      center_weight = info->footprint_x - left_weight - right_weight;

      if (src_x + 1 >= source_width)
        {
          src[2] = src[1];
          src[5] = src[4];
          src[8] = src[7];
        }

      if (info->src_is_premult)
        box_filter (left_weight, center_weight, right_weight,
                    top_weight, middle_weight, bottom_weight,
                    src, dest, bpp);
      else
        box_filter_premult (left_weight, center_weight, right_weight,
                            top_weight, middle_weight, bottom_weight,
                            src, dest, bpp);

      dest += bpp;

      dx += info->x_dest_inc;
      skipped = dx / info->x_src_dec;

      if (skipped)
        {
          dx -= skipped * info->x_src_dec;

          /* if we changed integer source pixel coordinates in the source
           * buffer, make sure the src pointers (and their backing tiles) are
           * correct
           */

          if (src[0] != src[1])
            {
              src[0] += skipped * bpp;
            }
          else
            {
              tilexL = -1;  /* this forces a refetch of the left most source
                               samples */
            }

          if (src[3] != src[4])
            {
              src[3] += skipped * bpp;
            }
          else
            {
              tilexL = -1;  /* this forces a refetch of the left most source
                               samples */
            }

          if (src[6] != src[7])
            {
              src[6] += skipped * bpp;
            }
          else
            {
              tilexL = -1;  /* this forces a refetch of the left most source
                               samples */
            }

          src[1] += skipped * bpp;
          src[4] += skipped * bpp;
          src[7] += skipped * bpp;

          src[5] += skipped * bpp;
          src[8] += skipped * bpp;
          src[2] += skipped * bpp;


          src_x += skipped;

          if ((src_x / TILE_WIDTH) != tilex0)
            {
              tile_release (tile[0], FALSE);

              tilex0 += 1;

              tile[0] = tile_manager_get_tile (info->src_tiles,
                                               src_x, info->src_y, TRUE, FALSE);
              if (! tile[0])
                goto done;

              src[4] = tile_data_pointer (tile[0], src_x, info->src_y);
              src[7] = tile_data_pointer (tile[0], src_x, info->src_y + 1);
              src[1] = tile_data_pointer (tile[0], src_x, info->src_y - 1);
            }

          if (((src_x + 1) / TILE_WIDTH) != tilex1)
            {
              if (tile[1])
                tile_release (tile[1], FALSE);

              tilex1 += 1;

              tile[1] = tile_manager_get_tile (info->src_tiles,
                                               src_x + 1, info->src_y,
                                               TRUE, FALSE);

              if (! tile[1])
                {
                  src[5] = src[4];
                  src[8] = src[7];
                  src[2] = src[1];
                }
              else
                {
                  src[5] = tile_data_pointer (tile[1],
                                              src_x + 1, info->src_y);
                  src[8] = tile_data_pointer (tile[1],
                                              src_x + 1, info->src_y + 1);
                  src[2] = tile_data_pointer (tile[1],
                                              src_x + 1, info->src_y - 1);
                }
            }

          if (((src_x - 1) / TILE_WIDTH) != tilexL)
            {
              if (tile[2])
                tile_release (tile[2], FALSE);

              tilexL += 1;

              tile[2] = tile_manager_get_tile (info->src_tiles,
                                               src_x - 1, info->src_y,
                                               TRUE, FALSE);

              if (! tile[2])
                {
                  src[3] = src[4];
                  src[6] = src[7];
                  src[0] = src[1];
                }
              else
                {
                  src[3] = tile_data_pointer (tile[2],
                                              src_x - 1, info->src_y);
                  src[6] = tile_data_pointer (tile[2],
                                              src_x - 1, info->src_y + 1);
                  src[0] = tile_data_pointer (tile[2],
                                              src_x - 1, info->src_y - 1);
                }
            }
        }
    }
  while (--width);

done:
  for (dx = 0; dx < 3; dx++)
    if (tile[dx])
      tile_release (tile[dx], FALSE);

  return tile_buf;
}

/* function to render a horizontal line of view data */
static const guchar *
render_image_tile_fault_nearest (RenderInfo *info)
{
  Tile         *tile;
  const guchar *src;
  guchar       *d;
  gint          width;
  gint          tilex;
  gint          bpp;
  gint          src_x;
  gint64        dx;

  tile = tile_manager_get_tile (info->src_tiles,
                                info->src_x, info->src_y, TRUE, FALSE);

  g_return_val_if_fail (tile != NULL, tile_buf);

  src = tile_data_pointer (tile, info->src_x, info->src_y);

  bpp   = tile_manager_bpp (info->src_tiles);

  dx    = info->dx_start;

  width = info->w;

  src_x = info->src_x;
  tilex = info->src_x / TILE_WIDTH;

  d     = tile_buf;

  do
    {
      const guchar *s = src;
      gint          skipped;

      if (info->src_is_premult)
        {
          switch (bpp)
            {
            case 4:
              *d++ = *s++;
            case 3:
              *d++ = *s++;
            case 2:
              *d++ = *s++;
            case 1:
              *d++ = *s++;
            }
        }
      else  /* pre-multiply */
        {
          switch (bpp)
            {
            case 4:
              d[0] = (s[0] * (s[3] + 1)) >> 8;
              d[1] = (s[1] * (s[3] + 1)) >> 8;
              d[2] = (s[2] * (s[3] + 1)) >> 8;
              d[3] = s[3];

              d += 4;
              s += 4;
              break;

            case 3:
              *d++ = *s++;
              *d++ = *s++;
              *d++ = *s++;
              break;

            case 2:
              d[0] = (s[0] * (s[1] + 1)) >> 8;
              d[1] = s[1];

              d += 2;
              s += 2;
              break;

            case 1:
              *d++ = *s++;
              break;
            }
        }

      dx += info->x_dest_inc;
      skipped = dx / info->x_src_dec;

      if (skipped)
        {
          src += skipped * bpp;
          src_x += skipped;
          dx -= skipped * info->x_src_dec;

          if ((src_x / TILE_WIDTH) != tilex)
            {
              tile_release (tile, FALSE);
              tilex += 1;

              tile = tile_manager_get_tile (info->src_tiles,
                                            src_x, info->src_y, TRUE, FALSE);
              if (! tile)
                return tile_buf;

              src = tile_data_pointer (tile, src_x, info->src_y);
            }
        }
    }
  while (--width);

  tile_release (tile, FALSE);

  return tile_buf;
}
