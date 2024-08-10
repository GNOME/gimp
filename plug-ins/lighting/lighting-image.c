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


GimpDrawable *input_drawable;
GimpDrawable *output_drawable;
GeglBuffer   *source_buffer;
GeglBuffer   *dest_buffer;

GimpDrawable *bump_drawable;
GeglBuffer   *bump_buffer;
const Babl   *bump_format;

GimpDrawable *env_drawable;
GeglBuffer   *env_buffer;

guchar          *preview_rgb_data = NULL;
gint             preview_rgb_stride;
cairo_surface_t *preview_surface = NULL;

glong   maxcounter;
gint    width, height;
gint    env_width, env_height;
GimpRGB background;

gint border_x1, border_y1, border_x2, border_y2;

guchar sinemap[256], spheremap[256], logmap[256];

/******************/
/* Implementation */
/******************/

guchar
peek_map (GeglBuffer *buffer,
          const Babl *format,
	  gint        x,
	  gint        y)
{
  guchar data[4];
  guchar ret_val;

  gegl_buffer_sample (buffer, x, y, NULL, data, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (babl_format_get_bytes_per_pixel (format))
    {
      ret_val = data[0];
    }
  else
    {
      ret_val = (guchar)((float)((data[0] + data[1] + data[2])/3.0));
    }

  return ret_val;
}

GimpRGB
peek (gint x,
      gint y)
{
  GimpRGB color;

  gegl_buffer_sample (source_buffer, x, y, NULL,
                      &color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! babl_format_has_alpha (gegl_buffer_get_format (source_buffer)))
    color.a = 1.0;

  return color;
}

GimpRGB
peek_env_map (gint x,
	      gint y)
{
  GimpRGB color;

  if (x < 0)
    x = 0;
  else if (x >= env_width)
    x = env_width - 1;
  if (y < 0)
    y = 0;
  else if (y >= env_height)
    y = env_height - 1;

  gegl_buffer_sample (env_buffer, x, y, NULL,
                      &color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  color.a = 1.0;

  return color;
}

void
poke (gint    x,
      gint    y,
      GimpRGB *color)
{
  if (x < 0)
    x = 0;
  else if (x >= width)
    x = width - 1;
  if (y < 0)
    y = 0;
  else if (y >= height)
    y = height - 1;

  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x, y, 1, 1), 0,
                   babl_format ("R'G'B'A double"), color,
                   GEGL_AUTO_ROWSTRIDE);
}

gint
check_bounds (gint x,
	      gint y)
{
  if (x < border_x1 ||
      y < border_y1 ||
      x >= border_x2 ||
      y >= border_y2)
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
get_map_value (GeglBuffer *buffer,
               const Babl *format,
	       gdouble     u,
	       gdouble     v,
	       gint       *inside)
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
      return (gdouble) peek_map (buffer, format, x1, y1);
    }

  *inside = TRUE;
  p[0] = (gdouble) peek_map (buffer, format, x1, y1);
  p[1] = (gdouble) peek_map (buffer, format, x2, y1);
  p[2] = (gdouble) peek_map (buffer, format, x1, y2);
  p[3] = (gdouble) peek_map (buffer, format, x2, y2);

  return gimp_bilinear (u, v, p);
}

static void
compute_maps (void)
{
  gint    x;
  gdouble val, c, d;

  /* Compute Sine, Log and Spherical transfer function maps */
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
  gint	   w, h;
  gboolean ret;

  compute_maps ();

  /* Get some useful info on the input drawable */
  /* ========================================== */

  input_drawable  = drawable;
  output_drawable = drawable;

  ret = gimp_drawable_mask_intersect (drawable,
                                      &border_x1, &border_y1, &w, &h);

  border_x2 = border_x1 + w;
  border_y2 = border_y1 + h;

  if (! ret)
    return FALSE;

  width  = gimp_drawable_get_width  (input_drawable);
  height = gimp_drawable_get_height (input_drawable);

  source_buffer = gimp_drawable_get_buffer (input_drawable);

  maxcounter = (glong) width * (glong) height;

  if (interactive)
    {
      preview_rgb_stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24,
                                                          PREVIEW_WIDTH);
      preview_rgb_data = g_new0 (guchar, preview_rgb_stride * PREVIEW_HEIGHT);
      preview_surface = cairo_image_surface_create_for_data (preview_rgb_data,
                                                             CAIRO_FORMAT_RGB24,
                                                             PREVIEW_WIDTH,
                                                             PREVIEW_HEIGHT,
                                                             preview_rgb_stride);
    }

  return TRUE;
}

void
bumpmap_setup (GimpDrawable *bumpmap)
{
  if (bumpmap)
    {
      if (! bump_buffer)
        {
          bump_buffer = gimp_drawable_get_buffer (bumpmap);
        }

      if (gimp_drawable_is_rgb (bumpmap))
        bump_format = babl_format ("R'aG'aB'aA u8");
      else
        bump_format = babl_format ("Y'aA u8"); /* FIXME */
    }
}

void
envmap_setup (GimpDrawable *envmap)
{
  if (envmap && ! env_buffer)
    {
      env_width  = gimp_drawable_get_width  (envmap);
      env_height = gimp_drawable_get_height (envmap);

      env_buffer = gimp_drawable_get_buffer (envmap);
    }
}
