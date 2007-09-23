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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "base/tile-manager.h"
#include "base/tile.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpprojection.h"

#include "widgets/gimprender.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-render.h"

#define GIMP_DISPLAY_ZOOM_FAST      1 << 0  /* use the fastest possible code
                                               path trading quality for speed
                                             */
#define GIMP_DISPLAY_ZOOM_PIXEL_AA  1 << 1  /* provide AA edges when zooming in
                                               on the actual pixels (in current
                                               code only enables it between
                                               100% and 200% zoom)
                                             */

/* The default settings are debatable, and perhaps this should even somehow be
 * configurable by the user. */
static gint gimp_zoom_quality = GIMP_DISPLAY_ZOOM_PIXEL_AA;

typedef struct _RenderInfo  RenderInfo;

typedef void (* RenderFunc) (RenderInfo *info);

struct _RenderInfo
{
  GimpDisplayShell *shell;
  TileManager      *src_tiles;
  const guint      *alpha;
  const guchar     *src;
  guchar           *dest;
  gint              x, y;
  gint              w, h;
  gdouble           scalex;
  gdouble           scaley;
  gint              src_x;
  gint              src_y;
  gint              dest_bpp;
  gint              dest_bpl;
  gint              dest_width;

  /* Bresenham helpers */
  gint              x_dest_inc; /* amount to increment for each dest. pixel  */
  gint              x_src_dec;  /* amount to decrement for each source pixel */
  gint              dx_start;   /* pixel fraction for first pixel            */

  gint              y_dest_inc;
  gint              y_src_dec;
  gint              dy_start;

  gint              dy;
};

static void  gimp_display_shell_render_info_scale   (RenderInfo       *info,
                                                     GimpDisplayShell *shell,
                                                     TileManager      *src_tiles,
                                                     gdouble           scale_x,
                                                     gdouble           scale_y);

static void  gimp_display_shell_render_setup_notify (GObject          *config,
                                                     GParamSpec       *param_spec,
                                                     Gimp             *gimp);


static guchar *tile_buf    = NULL;

static guint   check_mod   = 0;
static guint   check_shift = 0;


void
gimp_display_shell_render_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (tile_buf == NULL);

  g_signal_connect (gimp->config, "notify::transparency-size",
                    G_CALLBACK (gimp_display_shell_render_setup_notify),
                    gimp);
  g_signal_connect (gimp->config, "notify::transparency-type",
                    G_CALLBACK (gimp_display_shell_render_setup_notify),
                    gimp);

  /*  allocate a buffer for arranging information from a row of tiles  */
  tile_buf = g_new (guchar, GIMP_RENDER_BUF_WIDTH * MAX_CHANNELS);

  gimp_display_shell_render_setup_notify (G_OBJECT (gimp->config), NULL, gimp);
}

void
gimp_display_shell_render_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gimp_display_shell_render_setup_notify,
                                        gimp);

  if (tile_buf)
    {
      g_free (tile_buf);
      tile_buf = NULL;
    }
}

static void
gimp_display_shell_render_setup_notify (GObject    *config,
                                        GParamSpec *param_spec,
                                        Gimp       *gimp)
{
  GimpCheckSize check_size;

  g_object_get (config,
                "transparency-size", &check_size,
                NULL);

  switch (check_size)
    {
    case GIMP_CHECK_SIZE_SMALL_CHECKS:
      check_mod   = 0x3;
      check_shift = 2;
      break;

    case GIMP_CHECK_SIZE_MEDIUM_CHECKS:
      check_mod   = 0x7;
      check_shift = 3;
      break;

    case GIMP_CHECK_SIZE_LARGE_CHECKS:
      check_mod   = 0xf;
      check_shift = 4;
      break;
    }
}


/*  Render Image functions  */

static void           render_image_rgb_a        (RenderInfo *info);
static void           render_image_gray_a       (RenderInfo *info);

static const guint  * render_image_init_alpha   (gint        mult);

static const guchar * render_image_tile_fault   (RenderInfo *info);


static void  gimp_display_shell_render_highlight (GimpDisplayShell *shell,
                                                  gint              x,
                                                  gint              y,
                                                  gint              w,
                                                  gint              h,
                                                  GdkRectangle     *highlight);
static void  gimp_display_shell_render_mask      (GimpDisplayShell *shell,
                                                  RenderInfo       *info);


/*****************************************************************/
/*  This function is the core of the display--it offsets and     */
/*  scales the image according to the current parameters in the  */
/*  display object.  It handles color, grayscale, 8, 15, 16, 24  */
/*  & 32 bit output depths.                                      */
/*****************************************************************/

void
gimp_display_shell_render (GimpDisplayShell *shell,
                           gint              x,
                           gint              y,
                           gint              w,
                           gint              h,
                           GdkRectangle     *highlight)
{
  GimpProjection *projection;
  RenderInfo      info;
  GimpImageType   type;

  g_return_if_fail (w > 0 && h > 0);

  projection = shell->display->image->projection;

  /* Initialize RenderInfo with values that don't change during the
   * call of this function.
   */
  info.shell      = shell;

  info.x          = x + shell->offset_x;
  info.y          = y + shell->offset_y;
  info.w          = w;
  info.h          = h;

  info.dest_bpp   = 3;
  info.dest_bpl   = info.dest_bpp * GIMP_RENDER_BUF_WIDTH;
  info.dest_width = info.dest_bpp * info.w;

  if (GIMP_IMAGE_TYPE_HAS_ALPHA (gimp_projection_get_image_type (projection)))
    {
      gdouble opacity = gimp_projection_get_opacity (projection);

      info.alpha = render_image_init_alpha (opacity * 255.999);
    }

  /* Setup RenderInfo for rendering a GimpProjection level. */
  {
    TileManager *src_tiles;
    gint         level;

    level = gimp_projection_get_level (projection,
                                       shell->scale_x,
                                       shell->scale_y);

    src_tiles = gimp_projection_get_tiles_at_level (projection, level);

    gimp_display_shell_render_info_scale (&info,
                                          shell,
                                          src_tiles,
                                          shell->scale_x * (1 << level),
                                          shell->scale_y * (1 << level));
  }

  /* Currently, only RGBA and GRAYA projection types are used - the rest
   * are in case of future need.  -- austin, 28th Nov 1998.
   *
   * I retired them before they reach the age of 9 years unused...
   *                              -- simon,  23rd Sep 2007.
   */
  type = gimp_projection_get_image_type (projection);

  if (G_UNLIKELY (type != GIMP_RGBA_IMAGE && type != GIMP_GRAYA_IMAGE))
    g_warning ("using untested projection type %d", type);

  switch (type)
    {
      case GIMP_RGBA_IMAGE:
        render_image_rgb_a (&info);
        break;
      case GIMP_GRAYA_IMAGE:
        render_image_gray_a (&info);
        break;
      default:
        g_printerr ("gimp_display_shell_render: unsupported projection type\n");
        g_assert_not_reached ();
    }

  /*  apply filters to the rendered projection  */
  if (shell->filter_stack)
    gimp_color_display_stack_convert (shell->filter_stack,
                                      shell->render_buf,
                                      w, h,
                                      3,
                                      3 * GIMP_RENDER_BUF_WIDTH);

  /*  dim pixels outside the highlighted rectangle  */
  if (highlight)
    {
      gimp_display_shell_render_highlight (shell, x, y, w, h, highlight);
    }
  else if (shell->mask)
    {
      TileManager *src_tiles = gimp_drawable_get_tiles (shell->mask);

      /* The mask does not (yet) have an image pyramid, use the base scale of
       * the shell.
       */
      gimp_display_shell_render_info_scale (&info,
                                            shell,
                                            src_tiles,
                                            shell->scale_x,
                                            shell->scale_y);

      gimp_display_shell_render_mask (shell, &info);
    }

  /*  put it to the screen  */
  gimp_canvas_draw_rgb (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_RENDER,
                        x + shell->disp_xoffset, y + shell->disp_yoffset,
                        w, h,
                        shell->render_buf,
                        3 * GIMP_RENDER_BUF_WIDTH,
                        shell->offset_x, shell->offset_y);
}


#define GIMP_DISPLAY_SHELL_DIM_PIXEL(buf,x) \
{ \
  buf[3 * (x) + 0] >>= 1; \
  buf[3 * (x) + 1] >>= 1; \
  buf[3 * (x) + 2] >>= 1; \
}

/*  This function highlights the given area by dimming all pixels outside. */

static void
gimp_display_shell_render_highlight (GimpDisplayShell *shell,
                                     gint              x,
                                     gint              y,
                                     gint              w,
                                     gint              h,
                                     GdkRectangle     *highlight)
{
  guchar       *buf  = shell->render_buf;
  GdkRectangle  rect;

  rect.x      = shell->offset_x + x;
  rect.y      = shell->offset_y + y;
  rect.width  = w;
  rect.height = h;

  if (gdk_rectangle_intersect (highlight, &rect, &rect))
    {
      rect.x -= shell->offset_x + x;
      rect.y -= shell->offset_y + y;

      for (y = 0; y < rect.y; y++)
        {
          for (x = 0; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_RENDER_BUF_WIDTH;
        }

      for ( ; y < rect.y + rect.height; y++)
        {
          for (x = 0; x < rect.x; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          for (x += rect.width; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_RENDER_BUF_WIDTH;
        }

      for ( ; y < h; y++)
        {
          for (x = 0; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_RENDER_BUF_WIDTH;
        }
    }
  else
    {
      for (y = 0; y < h; y++)
        {
          for (x = 0; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_RENDER_BUF_WIDTH;
        }
    }
}

static void
gimp_display_shell_render_mask (GimpDisplayShell *shell,
                                RenderInfo       *info)
{
  gint      y, ye;
  gint      x, xe;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  info->dy = info->dy_start;
  info->src = render_image_tile_fault (info);

  while (TRUE)
    {
      const guchar *src  = info->src;
      guchar       *dest = info->dest;

      switch (shell->mask_color)
        {
        case GIMP_RED_CHANNEL:
          for (x = info->x; x < xe; x++, src++, dest += 3)
            {
              if (*src & 0x80)
                continue;

              dest[1] = dest[1] >> 2;
              dest[2] = dest[2] >> 2;
            }
          break;

        case GIMP_GREEN_CHANNEL:
          for (x = info->x; x < xe; x++, src++, dest += 3)
            {
              if (*src & 0x80)
                continue;

              dest[0] = dest[0] >> 2;
              dest[2] = dest[2] >> 2;
            }
          break;

        case GIMP_BLUE_CHANNEL:
          for (x = info->x; x < xe; x++, src++, dest += 3)
            {
              if (*src & 0x80)
                continue;

              dest[0] = dest[0] >> 2;
              dest[1] = dest[1] >> 2;
            }
          break;

        default:
          break;
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


/*************************/
/*  8 Bit functions      */
/*************************/

static void
render_image_gray_a (RenderInfo *info)
{
  const guint *alpha = info->alpha;
  gint         y, ye;
  gint         x, xe;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  info->dy = info->dy_start;
  info->src = render_image_tile_fault (info);

  while (TRUE)
    {
      const guchar *src  = info->src;
      guchar       *dest = info->dest;
      guint         dark_light;

      dark_light = (y >> check_shift) + (info->x >> check_shift);

      for (x = info->x; x < xe; x++)
        {
          guint a = alpha[src[ALPHA_G_PIX]];
          guint val;

          if (dark_light & 0x1)
            val = gimp_render_blend_dark_check[(a | src[GRAY_PIX])];
          else
            val = gimp_render_blend_light_check[(a | src[GRAY_PIX])];

          src += 2;

          dest[0] = val;
          dest[1] = val;
          dest[2] = val;
          dest += 3;

          if (((x + 1) & check_mod) == 0)
            dark_light += 1;
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

static void
render_image_rgb_a (RenderInfo *info)
{
  const guint *alpha = info->alpha;
  gint         y, ye;
  gint         x, xe;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  info->dy = info->dy_start;
  info->src = render_image_tile_fault (info);

  while (TRUE)
    {
      const guchar *src  = info->src;
      guchar       *dest = info->dest;
      guint         dark_light;

      dark_light = (y >> check_shift) + (info->x >> check_shift);

      for (x = info->x; x < xe; x++)
        {
          guint r, g, b, a = alpha[src[ALPHA_PIX]];

          if (dark_light & 0x1)
            {
              r = gimp_render_blend_dark_check[(a | src[RED_PIX])];
              g = gimp_render_blend_dark_check[(a | src[GREEN_PIX])];
              b = gimp_render_blend_dark_check[(a | src[BLUE_PIX])];
            }
          else
            {
              r = gimp_render_blend_light_check[(a | src[RED_PIX])];
              g = gimp_render_blend_light_check[(a | src[GREEN_PIX])];
              b = gimp_render_blend_light_check[(a | src[BLUE_PIX])];
            }

          src += 4;

          dest[0] = r;
          dest[1] = g;
          dest[2] = b;
          dest += 3;

          if (((x + 1) & check_mod) == 0)
            dark_light += 1;
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

static void
gimp_display_shell_render_info_scale (RenderInfo       *info,
                                      GimpDisplayShell *shell,
                                      TileManager      *src_tiles,
                                      gdouble           scale_x,
                                      gdouble           scale_y)
{
  info->src_tiles = src_tiles;

  /* We must reset info->dest because this member is modified in render
   * functions.
   */
  info->dest     = shell->render_buf;

  info->scalex   = scale_x;
  info->scaley   = scale_y;

  info->src_y    = (gdouble) info->y / info->scaley;

  /* use Bresenham like stepping */
  info->x_dest_inc = tile_manager_width (src_tiles);
  info->x_src_dec  = ceil (tile_manager_width (src_tiles) * info->scalex);

  info->dx_start   = info->x_dest_inc * info->x + info->x_dest_inc/2;
  info->src_x      = info->dx_start / info->x_src_dec;
  info->dx_start   = info->dx_start % info->x_src_dec;

  /* same for y */
  info->y_dest_inc = tile_manager_height (src_tiles);
  info->y_src_dec  = ceil (tile_manager_height (src_tiles) * info->scaley);

  info->dy_start   = info->y_dest_inc * info->y + info->y_dest_inc/2;
  info->src_y      = info->dy_start / info->y_src_dec;
  info->dy_start   = info->dy_start % info->y_src_dec;
}

static const guint *
render_image_init_alpha (gint mult)
{
  static guint *alpha_mult = NULL;
  static gint   alpha_val  = -1;

  gint i;

  if (alpha_val != mult)
    {
      if (!alpha_mult)
        alpha_mult = g_new (guint, 256);

      alpha_val = mult;
      for (i = 0; i < 256; i++)
        alpha_mult[i] = ((mult * i) / 255) << 8;
    }

  return alpha_mult;
}

static inline void
box_filter (guint          left_weight,
            guint          center_weight,
            guint          right_weight,
            guint          top_weight,
            guint          middle_weight,
            guint          bottom_weight,
            guint          sum,
            const guchar **src,   /* the 9 surrounding source pixels */
            guchar        *dest,
            gint           bpp)
{
  switch (bpp)
    {
      gint i;

      guint a;
      case 4:
#define ALPHA 3
        {
          const guint factors[9] =
            {
              (src[1][ALPHA] * top_weight)    >> 8,
              (src[4][ALPHA] * middle_weight) >> 8,
              (src[7][ALPHA] * bottom_weight) >> 8,
              (src[2][ALPHA] * top_weight)    >> 8,
              (src[5][ALPHA] * middle_weight) >> 8,
              (src[8][ALPHA] * bottom_weight) >> 8,
              (src[0][ALPHA] * top_weight)    >> 8,
              (src[3][ALPHA] * middle_weight) >> 8,
              (src[6][ALPHA] * bottom_weight) >> 8
            };

          a = (center_weight * (factors[0] + factors[1] + factors[2]) +
               right_weight  * (factors[3] + factors[4] + factors[5]) +
               left_weight   * (factors[6] + factors[7] + factors[8]));

          dest[ALPHA] = a / sum;

          for (i = 0; i <= ALPHA; i++)
            {
              if (a)
                {
                  dest[i] = ((center_weight * (factors[0] * src[1][i] +
                                               factors[1] * src[4][i] +
                                               factors[2] * src[7][i]) +

                              right_weight  * (factors[3] * src[2][i] +
                                               factors[4] * src[5][i] +
                                               factors[5] * src[8][i]) +

                              left_weight   * (factors[6] * src[0][i] +
                                               factors[7] * src[3][i] +
                                               factors[8] * src[6][i])
                             ) / a) & 0xff;
                }
            }
        }
#undef ALPHA
        break;
      case 2:
#define ALPHA 1

        /* NOTE: this is a copy and paste of the code above, the ALPHA changes
         * the behavior in all needed ways. */
        {
          const guint factors[9] =
            {
              (src[1][ALPHA] * top_weight)    >> 8,
              (src[4][ALPHA] * middle_weight) >> 8,
              (src[7][ALPHA] * bottom_weight) >> 8,
              (src[2][ALPHA] * top_weight)    >> 8,
              (src[5][ALPHA] * middle_weight) >> 8,
              (src[8][ALPHA] * bottom_weight) >> 8,
              (src[0][ALPHA] * top_weight)    >> 8,
              (src[3][ALPHA] * middle_weight) >> 8,
              (src[6][ALPHA] * bottom_weight) >> 8
            };

          a = (center_weight * (factors[0] + factors[1] + factors[2]) +
               right_weight  * (factors[3] + factors[4] + factors[5]) +
               left_weight   * (factors[6] + factors[7] + factors[8]));

          dest[ALPHA] = a / sum;

          for (i = 0; i <= ALPHA; i++)
            {
              if (a)
                {
                  dest[i] = ((center_weight * (factors[0] * src[1][i] +
                                               factors[1] * src[4][i] +
                                               factors[2] * src[7][i]) +

                              right_weight  * (factors[3] * src[2][i] +
                                               factors[4] * src[5][i] +
                                               factors[5] * src[8][i]) +

                              left_weight   * (factors[6] * src[0][i] +
                                               factors[7] * src[3][i] +
                                               factors[8] * src[6][i])
                              ) / a) & 0xff;
                }
            }
        }
#undef ALPHA
        break;
      default:
        g_warning ("bpp=%i not implemented as box filter", bpp);
    }
}

/* fast paths */
static const guchar * render_image_tile_fault_one_row  (RenderInfo *info);
static const guchar * render_image_tile_fault_nearest  (RenderInfo *info);

/*  012 <- this is the order of the numbered source tiles / pixels.
 *  345    for the 3x3 neighbourhoods.
 *  678
 */

/* function to render a horizontal line of view data */
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

  guint         footprint_x;
  guint         footprint_y;
  guint         foosum;

  guint         left_weight;
  guint         center_weight;
  guint         right_weight;

  guint         top_weight;
  guint         middle_weight;
  guint         bottom_weight;

  guint         source_width;
  guint         source_height;

  source_width = tile_manager_width (info->src_tiles);
  source_height = tile_manager_height (info->src_tiles);

  /* dispatch to fast path functions on special conditions */
  if ((gimp_zoom_quality & GIMP_DISPLAY_ZOOM_FAST)

      /* use nearest neighbour for exact levels */
      || (info->scalex == 1.0 &&
          info->scaley == 1.0)

      /* or when we're larger than 1.0 and not using any AA */
      || (info->shell->scale_x > 1.0 &&
          info->shell->scale_y > 1.0 &&
          (!(gimp_zoom_quality & GIMP_DISPLAY_ZOOM_PIXEL_AA)))

      /* or at any point when both scale factors are greater or equal to 200% */
      || (info->shell->scale_x >= 2.0 &&
          info->shell->scale_y >= 2.0 )

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

  footprint_y = info->y_src_dec;
  footprint_x = info->x_src_dec;

  foosum = footprint_x * footprint_y;

  top_weight    = MAX (footprint_y / 2, info->dy) - info->dy;
  bottom_weight = MAX (info->dy, footprint_y / 2) - footprint_y / 2;
  middle_weight = footprint_y - top_weight - bottom_weight;

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
      left_weight  = MAX (footprint_x / 2, dx) - dx;
      right_weight = MAX (dx, footprint_x / 2) - footprint_x / 2;
      center_weight = footprint_x - left_weight - right_weight;

      if (src_x + 1 >= source_width)
        {
           src[2]=src[1];
           src[5]=src[4];
           src[8]=src[7];
        }
      if (info->src_y + 1 >= source_height)
        {
           src[6]=src[3];
           src[7]=src[4];
           src[8]=src[5];
        }
      box_filter (left_weight, center_weight, right_weight,
                  top_weight, middle_weight, bottom_weight, foosum,
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
              tilexL =- 1;  /* this forces a refetch of the left most source
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

/* function to render a horizontal line of view data */
static const guchar *
render_image_tile_fault_nearest (RenderInfo *info)
{
  Tile         *tile;
  const guchar *src;
  guchar       *dest;
  gint          width;
  gint          tilex;
  gint          bpp;
  gint          src_x;
  gint          dx;

  tile = tile_manager_get_tile (info->src_tiles,
                                info->src_x, info->src_y, TRUE, FALSE);

  g_return_val_if_fail (tile != NULL, tile_buf);

  src = tile_data_pointer (tile, info->src_x, info->src_y);

  bpp   = tile_manager_bpp (info->src_tiles);
  dest  = tile_buf;

  dx    = info->dx_start;

  width = info->w;

  src_x = info->src_x;
  tilex = info->src_x / TILE_WIDTH;

  do
    {
      const guchar *s     = src;
      gint          skipped;

      switch (bpp)
        {
        case 4:
          *dest++ = *s++;
        case 3:
          *dest++ = *s++;
        case 2:
          *dest++ = *s++;
        case 1:
          *dest++ = *s++;
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

  guint         footprint_x;
  guint         footprint_y;
  guint         foosum;

  guint         left_weight;
  guint         center_weight;
  guint         right_weight;

  guint         top_weight;
  guint         middle_weight;
  guint         bottom_weight;

  guint         source_width;

  source_width = tile_manager_width (info->src_tiles);

  footprint_y = info->y_src_dec;
  footprint_x = info->x_src_dec;
  foosum      = footprint_x * footprint_y;

  top_weight    = MAX (footprint_y / 2, info->dy) - info->dy;
  bottom_weight = MAX (info->dy, footprint_y / 2) - footprint_y / 2;
  middle_weight = footprint_y - top_weight - bottom_weight;

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
      left_weight  = MAX (footprint_x / 2, dx) - dx;
      right_weight = MAX (dx, footprint_x / 2) - footprint_x / 2;
      center_weight = footprint_x - left_weight - right_weight;

      if (src_x + 1 >= source_width)
        {
          src[2]=src[1];
          src[5]=src[4];
          src[8]=src[7];
        }
      box_filter (left_weight, center_weight, right_weight,
                  top_weight, middle_weight, bottom_weight, foosum,
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
  for (dx=0; dx<3; dx++)
    if (tile[dx])
      tile_release (tile[dx], FALSE);

  return tile_buf;
}
