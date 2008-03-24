/*************************************/
/* GIMP image manipulation routines. */
/*************************************/

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "lighting-main.h"
#include "lighting-image.h"
#include "lighting-preview.h"
#include "lighting-ui.h"


GimpDrawable *input_drawable,*output_drawable;
GimpPixelRgn  source_region, dest_region;

GimpDrawable *bump_drawable = NULL;
GimpPixelRgn  bump_region;

GimpDrawable *env_drawable = NULL;
GimpPixelRgn  env_region;

guchar   *preview_rgb_data = NULL;

glong  maxcounter;
gint   imgtype, width, height, env_width, env_height, in_channels, out_channels;
GimpRGB background;

gint border_x1, border_y1, border_x2, border_y2;

guchar sinemap[256], spheremap[256], logmap[256];

/******************/
/* Implementation */
/******************/

guchar
peek_map (GimpPixelRgn *region,
	  gint       x,
	  gint       y)
{
  guchar data[4];
  guchar ret_val;


  gimp_pixel_rgn_get_pixel (region, data, x, y);

  if (region->bpp == 1)
  {
    ret_val = data[0];
  } else
  {
    ret_val = (guchar)((float)((data[0] + data[1] + data[2])/3.0));
  }

  return ret_val;
}

GimpRGB
peek (gint x,
      gint y)
{
  guchar data[4];
  GimpRGB color;

  gimp_pixel_rgn_get_pixel (&source_region,data, x, y);

  color.r = (gdouble) (data[0]) / 255.0;
  color.g = (gdouble) (data[1]) / 255.0;
  color.b = (gdouble) (data[2]) / 255.0;

  if (input_drawable->bpp == 4)
    {
      if (in_channels == 4)
        color.a = (gdouble) (data[3]) / 255.0;
      else
        color.a = 1.0;
    }
  else
    {
      color.a = 1.0;
    }

  return color;
}

GimpRGB
peek_env_map (gint x,
	      gint y)
{
  guchar data[4];
  GimpRGB color;

  if (x < 0)
    x = 0;
  else if (x >= env_width)
    x = env_width - 1;
  if (y < 0)
    y = 0;
  else if (y >= env_height)
    y = env_height - 1;

  gimp_pixel_rgn_get_pixel (&env_region, data, x, y);

  color.r = (gdouble) (data[0]) / 255.0;
  color.g = (gdouble) (data[1]) / 255.0;
  color.b = (gdouble) (data[2]) / 255.0;
  color.a = 1.0;

  return color;
}

void
poke (gint    x,
      gint    y,
      GimpRGB *color)
{
  static guchar data[4];

  if (x < 0)
    x = 0;
  else if (x >= width)
    x = width - 1;
  if (y < 0)
    y = 0;
  else if (y >= height)
    y = height - 1;

  data[0] = (guchar) (color->r * 255.0);
  data[1] = (guchar) (color->g * 255.0);
  data[2] = (guchar) (color->b * 255.0);
  data[3] = (guchar) (color->a * 255.0);

  gimp_pixel_rgn_set_pixel (&dest_region, data, x, y);
}

gint
check_bounds (gint x,
	      gint y)
{
  if (x < border_x1 || y < border_y1 || x >= border_x2 || y >= border_y2)
    return FALSE;
  else
    return TRUE;
}

GimpVector3
int_to_pos (gint x,
	    gint y)
{
  GimpVector3 pos;

  if (width >= height)
    {
      pos.x = (gdouble) x / (gdouble) width;
      pos.y = (gdouble) y / (gdouble) width;

      pos.y += 0.5 * (1.0 - (gdouble) height / (gdouble) width);
    }
  else
    {
      pos.x = (gdouble) x / (gdouble) height;
      pos.y = (gdouble) y / (gdouble) height;

      pos.x += 0.5 * (1.0 - (gdouble) width / (gdouble) height);
    }

  pos.z = 0.0;
  return pos;
}

GimpVector3
int_to_posf (gdouble x,
	     gdouble y)
{
  GimpVector3 pos;

  if (width >= height)
    {
      pos.x = x / (gdouble) width;
      pos.y = y / (gdouble) width;

      pos.y += 0.5 * (1.0 - (gdouble) height / (gdouble) width);
    }
  else
    {
      pos.x = x / (gdouble) height;
      pos.y = y / (gdouble) height;

      pos.x += 0.5 * (1.0 - (gdouble) width / (gdouble) height);
    }

  pos.z = 0.0;
  return pos;
}

void
pos_to_int (gdouble  x,
	    gdouble  y,
	    gint    *scr_x,
	    gint    *scr_y)
{
  if (width >= height)
    {
      y -= 0.5 * (1.0 - (gdouble) height / (gdouble) width);
      *scr_x = RINT ((x * (gdouble) width));
      *scr_y = RINT ((y * (gdouble) width));
    }
  else
    {
      x -= 0.5 * (1.0 - (gdouble) width / (gdouble) height);

      *scr_x = RINT ((x * (gdouble) height));
      *scr_y = RINT ((y  *(gdouble) height));
    }
}

void
pos_to_float (gdouble  x,
	      gdouble  y,
	      gdouble *xf,
	      gdouble *yf)
{
  if (width >= height)
    {
      y -= 0.5 * (1.0 - (gdouble) height / (gdouble) width);

      *xf = x * (gdouble) (width-1);
      *yf = y * (gdouble) (width-1);
    }
  else
    {
      x -= 0.5 * (1.0 - (gdouble) width / (gdouble) height);

      *xf = x * (gdouble) (height-1);
      *yf = y * (gdouble) (height-1);
    }
}

/**********************************************/
/* Compute the image color at pos (u,v) using */
/* Quartics bilinear interpolation stuff.     */
/**********************************************/

GimpRGB
get_image_color (gdouble  u,
		 gdouble  v,
		 gint    *inside)
{
  gint    x1, y1, x2, y2;
  GimpRGB p[4];

  x1 = RINT (u);
  y1 = RINT (v);

  if (check_bounds (x1, y1) == FALSE)
    {
      *inside = FALSE;
      return background;
    }

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (check_bounds (x2, y2) == FALSE)
    {
      *inside = TRUE;
      return peek (x1, y1);
    }

  *inside = TRUE;
  p[0] = peek (x1, y1);
  p[1] = peek (x2, y1);
  p[2] = peek (x1, y2);
  p[3] = peek (x2, y2);

  return gimp_bilinear_rgba (u, v, p);
}

gdouble
get_map_value (GimpPixelRgn *region,
	       gdouble    u,
	       gdouble    v,
	       gint      *inside)
{
  gint    x1, y1, x2, y2;
  gdouble p[4];

  x1 = RINT (u);
  y1 = RINT (v);

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (check_bounds (x2, y2) == FALSE)
    {
      *inside = TRUE;
      return (gdouble) peek_map (region, x1, y1);
    }

  *inside = TRUE;
  p[0] = (gdouble) peek_map (region, x1, y1);
  p[1] = (gdouble) peek_map (region, x2, y1);
  p[2] = (gdouble) peek_map (region, x1, y2);
  p[3] = (gdouble) peek_map (region, x2, y2);

  return gimp_bilinear (u, v, p);
}

static void
compute_maps (void)
{
  gint    x;
  gdouble val, c, d;

  /* Compute Sine, Log ans Spherical transfer function maps */
  /* ====================================================== */

  c = 1.0  / 255.0;
  d = 1.15 * 255.0;

  for (x = 0; x < 256; x++)
    {
      sinemap[x] = (guchar) (255.0 * (0.5 * (sin ((G_PI * c * (gdouble) x) -
						  0.5 * G_PI) +
					     1.0)));
      spheremap[x] = (guchar) (255.0 * (sqrt (sin (G_PI * (gdouble) x /
						   512.0))));
      val = (d * exp (-1.0 / (8.0 * c * ((gdouble) x + 5.0))));

      if (val > 255.0)
        val = 255.0;
      logmap[x] = (guchar) val;
    }
}

/****************************************/
/* Allocate memory for temporary images */
/****************************************/

gint
image_setup (GimpDrawable *drawable,
	     gint          interactive)
{
  compute_maps ();

  /* Get some useful info on the input drawable */
  /* ========================================== */

  input_drawable  = drawable;
  output_drawable = drawable;

  gimp_drawable_mask_bounds (drawable->drawable_id,
			     &border_x1, &border_y1, &border_x2, &border_y2);

  width  = input_drawable->width;
  height = input_drawable->height;

  gimp_pixel_rgn_init (&source_region, input_drawable,
		       0, 0, width, height, FALSE, FALSE);

  maxcounter = (glong) width * (glong) height;

  /* Assume at least RGB */
  /* =================== */

  in_channels = 3;
  if (gimp_drawable_has_alpha (input_drawable->drawable_id) == TRUE)
    in_channels++;

  if (interactive)
    {
      preview_rgb_data = g_new0 (guchar, PREVIEW_WIDTH * PREVIEW_HEIGHT * 3);
    }

  return TRUE;
}
