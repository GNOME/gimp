/*************************************/
/* GIMP image manipulation routines. */
/*************************************/

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include <gck/gck.h>

#include "lighting_main.h"
#include "lighting_image.h"
#include "lighting_preview.h"
#include "lighting_ui.h"

GDrawable *input_drawable,*output_drawable;
GPixelRgn  source_region, dest_region;

GDrawable *bump_drawable = NULL;
GPixelRgn  bump_region;

GDrawable *env_drawable = NULL;
GPixelRgn  env_region;

guchar   *preview_rgb_data = NULL;
GdkImage *image = NULL;

glong  maxcounter;
gint   imgtype, width, height, env_width, env_height, in_channels, out_channels;
GckRGB background;

gint border_x1, border_y1, border_x2, border_y2;

guchar sinemap[256], spheremap[256], logmap[256];

/******************/
/* Implementation */
/******************/

guchar
peek_map (GPixelRgn *region,
	  gint       x,
	  gint       y)
{
  guchar data;

  gimp_pixel_rgn_get_pixel (region, &data, x, y);

  return data;
}

GckRGB
peek (gint x,
      gint y)
{
  guchar data[4];
  GckRGB color;

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
    color.a = 1.0;

  return color;
}

GckRGB
peek_env_map (gint x,
	      gint y)
{
  guchar data[4];
  GckRGB color;

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
      GckRGB *color)
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
	    gint *scr_y)
{
  if (width >= height)
    {
      y -= 0.5 * (1.0 - (gdouble) height / (gdouble) width);
      *scr_x = (gint) ((x * (gdouble) width) + 0.5);
      *scr_y = (gint) ((y * (gdouble) width) + 0.5);
    }
  else
    {
      x -= 0.5 * (1.0 - (gdouble) width / (gdouble) height);

      *scr_x = (gint) ((x * (gdouble) height) + 0.5);
      *scr_y = (gint) ((y  *(gdouble) height) + 0.5);
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

      *xf = x * (gdouble) width;
      *yf = y * (gdouble) width;
    }
  else
    {
      x -= 0.5 * (1.0 - (gdouble) width / (gdouble) height);

      *xf = x * (gdouble) height;
      *yf = y * (gdouble) height;
    }
}

/**********************************************/
/* Compute the image color at pos (u,v) using */
/* Quartics bilinear interpolation stuff.     */
/**********************************************/

GckRGB
get_image_color (gdouble  u,
		 gdouble  v,
		 gint    *inside)
{
  gint   x1, y1, x2, y2;
  GckRGB p[4];
 
  x1 = (gint) (u + 0.5);
  y1 = (gint) (v + 0.5);

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

  return gck_bilinear_rgba (u, v, p);
}

gdouble
get_map_value (GPixelRgn *region,
	       gdouble    u,
	       gdouble    v,
	       gint      *inside)
{
  gint    x1, y1, x2, y2;
  gdouble p[4];
 
  x1 = (gint) (u + 0.5);
  y1 = (gint) (v + 0.5);

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

  return gck_bilinear (u, v, p);
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
image_setup (GDrawable *drawable,
	     gint       interactive)
{
  glong numbytes;

  compute_maps ();

  /* Get some useful info on the input drawable */
  /* ========================================== */

  input_drawable = drawable;
  output_drawable = drawable;

  gimp_drawable_mask_bounds (drawable->id,
			     &border_x1, &border_y1, &border_x2, &border_y2);

  width = input_drawable->width;
  height = input_drawable->height;

  gimp_pixel_rgn_init (&source_region, input_drawable,
		       0, 0, width, height, FALSE, FALSE);

  maxcounter = (glong) width * (glong) height;

  /* Assume at least RGB */
  /* =================== */

  in_channels = 3;
  if (gimp_drawable_has_alpha (input_drawable->id) == TRUE)
    in_channels++;

  if (interactive == TRUE)
    {
      /* Allocate memory for temp. images */
      /* ================================ */

      image = gdk_image_new (GDK_IMAGE_FASTEST, visinfo->visual,
			     PREVIEW_WIDTH, PREVIEW_HEIGHT);

      if (image == NULL)
        return FALSE;

      numbytes = (glong) PREVIEW_WIDTH * (glong) PREVIEW_HEIGHT * 3;
      preview_rgb_data = g_new0 (guchar, numbytes);

      /* Convert from raw RGB to GdkImage */
      /* ================================ */

      gck_rgb_to_gdkimage (visinfo, preview_rgb_data, image,
			   PREVIEW_WIDTH, PREVIEW_HEIGHT);
    }

  return TRUE;
}
