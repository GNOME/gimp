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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "base/tile-manager.h"
#include "base/tile.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpprojection.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-render.h"


typedef struct _RenderInfo  RenderInfo;

typedef void (* RenderFunc) (RenderInfo *info);

struct _RenderInfo
{
  GimpDisplayShell *shell;
  TileManager      *src_tiles;
  guint            *alpha;
  guchar           *scale;
  guchar           *src;
  guchar           *dest;
  gint              x, y;
  gint              w, h;
  gdouble           scalex;
  gdouble           scaley;
  gint              src_x, src_y;
  gint              src_bpp;
  gint              dest_bpp;
  gint              dest_bpl;
  gint              dest_width;
  gint              byte_order;
};


static void   render_setup_notify (gpointer    config,
                                   GParamSpec *param_spec,
                                   Gimp       *gimp);


/*  accelerate transparency of image scaling  */
guchar *render_check_buf         = NULL;
guchar *render_empty_buf         = NULL;
guchar *render_white_buf         = NULL;
guchar *render_temp_buf          = NULL;

guchar *render_blend_dark_check  = NULL;
guchar *render_blend_light_check = NULL;
guchar *render_blend_white       = NULL;


static guchar *tile_buf           = NULL;
static guint   tile_shift         = 0;
static guint   check_mod          = 0;
static guint   check_shift        = 0;


void
render_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_connect (gimp->config, "notify::transparency-size",
                    G_CALLBACK (render_setup_notify),
                    gimp);
  g_signal_connect (gimp->config, "notify::transparency-type",
                    G_CALLBACK (render_setup_notify),
                    gimp);

  render_setup_notify (gimp->config, NULL, gimp);
}

void
render_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        render_setup_notify,
                                        gimp);

  if (tile_buf)
    {
      g_free (tile_buf);
      tile_buf = NULL;
    }

  if (render_blend_dark_check)
    {
      g_free (render_blend_dark_check);
      render_blend_dark_check = NULL;
    }

  if (render_blend_light_check)
    {
      g_free (render_blend_light_check);
      render_blend_light_check = NULL;
    }

  if (render_blend_white)
    {
      g_free (render_blend_white);
      render_blend_white = NULL;
    }

  if (render_check_buf)
    {
      g_free (render_check_buf);
      render_check_buf = NULL;
    }

  if (render_empty_buf)
    {
      g_free (render_empty_buf);
      render_empty_buf = NULL;
    }

  if (render_white_buf)
    {
      g_free (render_white_buf);
      render_white_buf = NULL;
    }

  if (render_temp_buf)
    {
      g_free (render_temp_buf);
      render_temp_buf = NULL;
    }
}


static void
render_setup_notify (gpointer    config,
                     GParamSpec *param_spec,
                     Gimp       *gimp)
{
  GimpCheckType check_type;
  GimpCheckSize check_size;
  guchar        light, dark;
  gint          i, j;

  g_object_get (config,
                "transparency-type", &check_type,
                "transparency-size", &check_size,
                NULL);

  /*  based on the tile size, determine the tile shift amount
   *  (assume here that tile_height and tile_width are equal)
   */
  tile_shift = 0;
  while ((1 << tile_shift) < TILE_WIDTH)
    tile_shift++;

  /*  allocate a buffer for arranging information from a row of tiles  */
  if (! tile_buf)
    tile_buf = g_new (guchar,
                      GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH * MAX_CHANNELS);

  if (! render_blend_dark_check)
    render_blend_dark_check = g_new (guchar, 65536);
  if (! render_blend_light_check)
    render_blend_light_check = g_new (guchar, 65536);
  if (! render_blend_white)
    render_blend_white = g_new (guchar, 65536);

  gimp_checks_get_shades (check_type, &light, &dark);

  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
      {
	render_blend_dark_check [(i << 8) + j] =
	  (guchar) ((j * i + dark * (255 - i)) / 255);
	render_blend_light_check [(i << 8) + j] =
          (guchar) ((j * i + light * (255 - i)) / 255);
	render_blend_white [(i << 8) + j] =
          (guchar) ((j * i + 255 * (255 - i)) / 255);
      }

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

  g_free (render_check_buf);
  g_free (render_empty_buf);
  g_free (render_white_buf);
  g_free (render_temp_buf);

#define BUF_SIZE (MAX (GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH, \
                       GIMP_VIEWABLE_MAX_PREVIEW_SIZE) + 4)

  render_check_buf = g_new  (guchar, BUF_SIZE * 3);
  render_empty_buf = g_new0 (guchar, BUF_SIZE * 3);
  render_white_buf = g_new  (guchar, BUF_SIZE * 3);
  render_temp_buf  = g_new  (guchar, BUF_SIZE * 3);

  /*  calculate check buffer for previews  */

  memset (render_white_buf, 255, BUF_SIZE * 3);

  for (i = 0; i < BUF_SIZE; i++)
    {
      if (i & 0x4)
        {
          render_check_buf[i * 3 + 0] = render_blend_dark_check[0];
          render_check_buf[i * 3 + 1] = render_blend_dark_check[0];
          render_check_buf[i * 3 + 2] = render_blend_dark_check[0];
        }
      else
        {
          render_check_buf[i * 3 + 0] = render_blend_light_check[0];
          render_check_buf[i * 3 + 1] = render_blend_light_check[0];
          render_check_buf[i * 3 + 2] = render_blend_light_check[0];
        }
    }

#undef BUF_SIZE
}


/*  Render Image functions  */

static void     render_image_rgb                (RenderInfo       *info);
static void     render_image_rgb_a              (RenderInfo       *info);
static void     render_image_gray               (RenderInfo       *info);
static void     render_image_gray_a             (RenderInfo       *info);
static void     render_image_indexed            (RenderInfo       *info);
static void     render_image_indexed_a          (RenderInfo       *info);

static void     render_image_init_info          (RenderInfo       *info,
						 GimpDisplayShell *shell,
						 gint              x,
						 gint              y,
						 gint              w,
						 gint              h);
static guint  * render_image_init_alpha         (gint              mult);
static guchar * render_image_accelerate_scaling (gint              width,
						 gint              start,
						 gdouble           scalex);
static guchar * render_image_tile_fault         (RenderInfo       *info);


static RenderFunc render_funcs[6] =
{
  render_image_rgb,
  render_image_rgb_a,
  render_image_gray,
  render_image_gray_a,
  render_image_indexed,
  render_image_indexed_a,
};


static void  gimp_display_shell_render_highlight (GimpDisplayShell *shell,
                                                  gint              x,
                                                  gint              y,
                                                  gint              w,
                                                  gint              h,
                                                  GdkRectangle     *highlight);


/*****************************************************************/
/*  This function is the core of the display--it offsets and     */
/*  scales the image according to the current parameters in the  */
/*  gdisp object.  It handles color, grayscale, 8, 15, 16, 24,   */
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
  RenderInfo     info;
  GimpImageType  image_type;

  render_image_init_info (&info, shell, x, y, w, h);

  image_type = gimp_projection_get_image_type (shell->gdisp->gimage->projection);

  if ((image_type < GIMP_RGB_IMAGE) || (image_type > GIMP_INDEXEDA_IMAGE))
    {
      g_message ("unknown gimage projection type: %d", image_type);
      return;
    }

  if ((info.dest_bpp < 1) || (info.dest_bpp > 4))
    {
      g_message ("unsupported destination bytes per pixel: %d", info.dest_bpp);
      return;
    }

  /* Currently, only RGBA and GRAYA projection types are used - the rest
   * are in case of future need.  -- austin, 28th Nov 1998.
   */
  if (image_type != GIMP_RGBA_IMAGE && image_type != GIMP_GRAYA_IMAGE)
    g_warning ("using untested projection type %d", image_type);

  (* render_funcs[image_type]) (&info);

  /*  apply filters to the rendered projection  */
  if (shell->filter_stack)
    gimp_color_display_stack_convert (shell->filter_stack,
                                      shell->render_buf,
                                      w, h,
                                      3,
                                      3 * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH);

  /*  dim pixels outside the highlighted rectangle  */
  if (highlight)
    gimp_display_shell_render_highlight (shell, x, y, w, h, highlight);

  /*  put it to the screen  */
  gimp_canvas_draw_rgb (GIMP_CANVAS (shell->canvas), GIMP_CANVAS_STYLE_RENDER,
                        x + shell->disp_xoffset, y + shell->disp_yoffset,
                        w, h,
                        shell->render_buf,
                        3 * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH,
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

          buf += 3 * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH;
        }

      for ( ; y < rect.y + rect.height; y++)
        {
          for (x = 0; x < rect.x; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          for (x += rect.width; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH;
        }

      for ( ; y < h; y++)
        {
          for (x = 0; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH;
        }
    }
  else
    {
      for (y = 0; y < h; y++)
        {
          for (x = 0; x < w; x++)
            GIMP_DISPLAY_SHELL_DIM_PIXEL (buf, x)

          buf += 3 * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH;
        }
    }
}


/*************************/
/*  8 Bit functions      */
/*************************/

static void
render_image_indexed (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guchar *cmap;
  gulong  val;
  gint    byte_order;
  gint    y, ye;
  gint    x, xe;
  gint    initial;

  cmap = gimp_image_get_colormap (info->shell->gdisp->gimage);

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  initial = TRUE;
  byte_order = info->byte_order;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      gint error = RINT (floor ((y + 1) / info->scaley)
                         - floor (y / info->scaley));

      if (!initial && (error == 0))
	{
	  memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
	}
      else
	{
	  src = info->src;
	  dest = info->dest;

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      val = src[INDEXED_PIX] * 3;
	      src += 1;

	      dest[0] = cmap[val+0];
	      dest[1] = cmap[val+1];
	      dest[2] = cmap[val+2];
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (error >= 1)
	{
	  info->src_y += error;
	  info->src = render_image_tile_fault (info);
	  initial = TRUE;
	}
    }
}

static void
render_image_indexed_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint  *alpha;
  guchar *cmap;
  gulong  r, g, b;
  gulong  val;
  guint   a;
  gint    dark_light;
  gint    byte_order;
  gint    y, ye;
  gint    x, xe;
  gint    initial;

  cmap = gimp_image_get_colormap (info->shell->gdisp->gimage);
  alpha = info->alpha;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  initial = TRUE;
  byte_order = info->byte_order;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      gint error = RINT (floor ((y + 1) / info->scaley)
                         - floor (y / info->scaley));

      if (!initial && (error == 0) && (y & check_mod))
	{
	  memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
	}
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_I_PIX]];
	      val = src[INDEXED_PIX] * 3;
	      src += 2;

	      if (dark_light & 0x1)
		{
		  r = render_blend_dark_check[(a | cmap[val + 0])];
		  g = render_blend_dark_check[(a | cmap[val + 1])];
		  b = render_blend_dark_check[(a | cmap[val + 2])];
		}
	      else
		{
		  r = render_blend_light_check[(a | cmap[val + 0])];
		  g = render_blend_light_check[(a | cmap[val + 1])];
		  b = render_blend_light_check[(a | cmap[val + 2])];
		}

		dest[0] = r;
		dest[1] = g;
		dest[2] = b;
		dest += 3;

		if (((x + 1) & check_mod) == 0)
		  dark_light += 1;
	      }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (error >= 1)
	{
	  info->src_y += error;
	  info->src = render_image_tile_fault (info);
	  initial = TRUE;
	}
    }
}

static void
render_image_gray (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  gulong  val;
  gint    byte_order;
  gint    y, ye;
  gint    x, xe;
  gint    initial;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  initial = TRUE;
  byte_order = info->byte_order;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      gint error = RINT (floor ((y + 1) / info->scaley)
                         - floor (y / info->scaley));

      if (!initial && (error == 0))
	{
	  memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
	}
      else
	{
	  src = info->src;
	  dest = info->dest;

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      val = src[GRAY_PIX];
	      src += 1;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (error >= 1)
	{
	  info->src_y += error;
	  info->src = render_image_tile_fault (info);
	  initial = TRUE;
	}
    }
}

static void
render_image_gray_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint  *alpha;
  gulong  val;
  guint   a;
  gint    dark_light;
  gint    byte_order;
  gint    y, ye;
  gint    x, xe;
  gint    initial;

  alpha = info->alpha;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  initial = TRUE;
  byte_order = info->byte_order;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      gint error = RINT (floor ((y + 1) / info->scaley)
                         - floor (y / info->scaley));

      if (!initial && (error == 0) && (y & check_mod))
	{
	  memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
	}
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_G_PIX]];
	      if (dark_light & 0x1)
		val = render_blend_dark_check[(a | src[GRAY_PIX])];
	      else
		val = render_blend_light_check[(a | src[GRAY_PIX])];
	      src += 2;

	      dest[0] = val;
	      dest[1] = val;
	      dest[2] = val;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (error >= 1)
	{
	  info->src_y += error;
	  info->src = render_image_tile_fault (info);
	  initial = TRUE;
	}
    }
}

static void
render_image_rgb (RenderInfo *info)
{
  gint    byte_order;
  gint    y, ye;
  gint    xe;
  gint    initial;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  initial = TRUE;
  byte_order = info->byte_order;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      gint error = RINT (floor ((y + 1) / info->scaley)
                         - floor (y / info->scaley));

      if (!initial && (error == 0))
	{
	  memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
	}
      else
	{
	  g_return_if_fail (info->src != NULL);

          memcpy (info->dest, info->src, 3 * info->w);
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (error >= 1)
	{
	  info->src_y += error;
	  info->src = render_image_tile_fault (info);
	  initial = TRUE;
	}
    }
}

static void
render_image_rgb_a (RenderInfo *info)
{
  guchar *src;
  guchar *dest;
  guint  *alpha;
  gulong  r, g, b;
  guint   a;
  gint    dark_light;
  gint    byte_order;
  gint    y, ye;
  gint    x, xe;
  gint    initial;

  alpha = info->alpha;

  y  = info->y;
  ye = info->y + info->h;
  xe = info->x + info->w;

  initial = TRUE;
  byte_order = info->byte_order;
  info->src = render_image_tile_fault (info);

  for (; y < ye; y++)
    {
      gint error = RINT (floor ((y + 1) / info->scaley)
                         - floor (y / info->scaley));

      if (!initial && (error == 0) && (y & check_mod))
	{
	  memcpy (info->dest, info->dest - info->dest_bpl, info->dest_width);
	}
      else
	{
	  src = info->src;
	  dest = info->dest;

	  dark_light = (y >> check_shift) + (info->x >> check_shift);

	  g_return_if_fail (src != NULL);

	  for (x = info->x; x < xe; x++)
	    {
	      a = alpha[src[ALPHA_PIX]];
	      if (dark_light & 0x1)
		{
		  r = render_blend_dark_check[(a | src[RED_PIX])];
		  g = render_blend_dark_check[(a | src[GREEN_PIX])];
		  b = render_blend_dark_check[(a | src[BLUE_PIX])];
		}
	      else
		{
		  r = render_blend_light_check[(a | src[RED_PIX])];
		  g = render_blend_light_check[(a | src[GREEN_PIX])];
		  b = render_blend_light_check[(a | src[BLUE_PIX])];
		}

	      src += 4;

	      dest[0] = r;
	      dest[1] = g;
	      dest[2] = b;
	      dest += 3;

	      if (((x + 1) & check_mod) == 0)
		dark_light += 1;
	    }
	}

      info->dest += info->dest_bpl;

      initial = FALSE;

      if (error >= 1)
	{
	  info->src_y += error;
	  info->src = render_image_tile_fault (info);
	  initial = TRUE;
	}
    }
}

static void
render_image_init_info (RenderInfo       *info,
			GimpDisplayShell *shell,
			gint              x,
			gint              y,
			gint              w,
			gint              h)
{
  GimpImage *gimage = shell->gdisp->gimage;

  info->shell      = shell;
  info->src_tiles  = gimp_projection_get_tiles (gimage->projection);
  info->x          = x + shell->offset_x;
  info->y          = y + shell->offset_y;
  info->w          = w;
  info->h          = h;
  info->scalex     = SCALEFACTOR_X (shell);
  info->scaley     = SCALEFACTOR_Y (shell);
  info->src_x      = (gdouble) info->x / info->scalex;
  info->src_y      = (gdouble) info->y / info->scaley;
  info->src_bpp    = gimp_projection_get_bytes (gimage->projection);
  info->dest       = shell->render_buf;
  info->dest_bpp   = 3;
  info->dest_bpl   = info->dest_bpp * GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH;
  info->dest_width = info->w * info->dest_bpp;
  info->byte_order = GDK_MSB_FIRST;
  info->scale      = render_image_accelerate_scaling (w,
                                                      info->x, info->scalex);
  info->alpha      = NULL;

  if (GIMP_IMAGE_TYPE_HAS_ALPHA (gimp_projection_get_image_type (gimage->projection)))
    {
      info->alpha =
	render_image_init_alpha (gimp_projection_get_opacity (gimage->projection) * 255.999);
    }
}

static guint *
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

static guchar *
render_image_accelerate_scaling (gint    width,
				 gint    start,
				 gdouble scalex)
{
  static guchar *scale = NULL;

  gint  i;

  if (! scale)
    scale = g_new (guchar, GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH + 1);

  for (i = 0; i <= width; i++)
    {
      scale[i] = RINT (floor ((start + 1) / scalex) - floor (start / scalex));
      start++;
    }

  return scale;
}

static guchar *
render_image_tile_fault (RenderInfo *info)
{
  Tile   *tile;
  guchar *data;
  guchar *dest;
  guchar *scale;
  gint    width;
  gint    tilex;
  gint    step;
  gint    bpp = info->src_bpp;
  gint    x, b;

  tile = tile_manager_get_tile (info->src_tiles,
				info->src_x, info->src_y,
				TRUE, FALSE);
  if (!tile)
    return NULL;

  data = tile_data_pointer (tile,
			    info->src_x % TILE_WIDTH,
			    info->src_y % TILE_HEIGHT);
  scale = info->scale;
  dest  = tile_buf;

  x     = info->src_x;
  width = info->w;

  tilex = info->src_x / TILE_WIDTH;

  while (width--)
    {
      for (b = 0; b < bpp; b++)
	*dest++ = data[b];

      step = *scale++;
      if (step != 0)
	{
	  x += step;
	  data += step * bpp;

	  if ((x >> tile_shift) != tilex)
	    {
	      tile_release (tile, FALSE);
	      tilex += 1;

	      tile = tile_manager_get_tile (info->src_tiles,
                                            x, info->src_y,
                                            TRUE, FALSE);

	      if (!tile)
		return tile_buf;

	      data = tile_data_pointer (tile,
                                        x % TILE_WIDTH,
                                        info->src_y % TILE_HEIGHT);
	    }
	}
    }

  tile_release (tile, FALSE);

  return tile_buf;
}
