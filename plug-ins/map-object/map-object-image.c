/*********************************************************/
/* Image manipulation routines. Calls mapobject_shade.c  */
/* functions to compute the shading of the image at each */
/* pixel. These routines are used by the functions in    */
/* mapobject_preview.c and mapobject_apply.c             */
/*********************************************************/

#include "config.h"

#include <string.h>

#include <sys/types.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "map-object-main.h"
#include "map-object-preview.h"
#include "map-object-shade.h"
#include "map-object-ui.h"
#include "map-object-image.h"


GimpImage    *image;

GimpDrawable *input_drawable;
GimpDrawable *output_drawable;
GeglBuffer   *source_buffer;
GeglBuffer   *dest_buffer;

GimpDrawable *box_drawables[6];
GeglBuffer   *box_buffers[6];

GimpDrawable *cylinder_drawables[2];
GeglBuffer   *cylinder_buffers[2];

guchar          *preview_rgb_data = NULL;
gint             preview_rgb_stride;
cairo_surface_t *preview_surface = NULL;

glong    maxcounter, old_depth, max_depth;
gint     width, height;
gdouble  background[4];

gint border_x, border_y, border_w, border_h;


void  peek_box_image  (gint     image,
                       gint     x,
                       gint     y,
                       gdouble *color);


/******************/
/* Implementation */
/******************/

void
peek (gint     x,
      gint     y,
      gdouble *color)
{
  gegl_buffer_sample (source_buffer, x, y, NULL,
                      color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! babl_format_has_alpha (gegl_buffer_get_format (source_buffer)))
    color[3] = 1.0;
}

void
peek_box_image (gint     image,
                gint     x,
                gint     y,
                gdouble *color)
{
  gegl_buffer_sample (box_buffers[image], x, y, NULL,
                      color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! babl_format_has_alpha (gegl_buffer_get_format (box_buffers[image])))
    color[3] = 1.0;
}

static void
peek_cylinder_image (gint     image,
                     gint     x,
                     gint     y,
                     gdouble *color)
{
  gegl_buffer_sample (cylinder_buffers[image], x, y, NULL,
                      color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! babl_format_has_alpha (gegl_buffer_get_format (cylinder_buffers[image])))
    color[3] = 1.0;
}

void
poke (gint      x,
      gint      y,
      gdouble  *color,
      gpointer  user_data)
{
  gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x, y, 1, 1), 0,
                   babl_format ("R'G'B'A double"), color,
                   GEGL_AUTO_ROWSTRIDE);
}

gint
checkbounds (gint x,
             gint y)
{
  if (x < border_x ||
      y < border_y ||
      x >= border_x + border_w ||
      y >= border_y + border_h)
    return FALSE;
  else
    return TRUE;
}

static gint
checkbounds_box_image (gint image,
                       gint x,
                       gint y)
{
  gint w, h;

  w = gegl_buffer_get_width  (box_buffers[image]);
  h = gegl_buffer_get_height (box_buffers[image]);

  if (x < 0 || y < 0 || x >= w || y >= h)
    return FALSE ;
  else
    return TRUE ;
}

static gint
checkbounds_cylinder_image (gint image,
                            gint x,
                            gint y)
{
  gint w, h;

  w = gegl_buffer_get_width  (cylinder_buffers[image]);
  h = gegl_buffer_get_height (cylinder_buffers[image]);

  if (x < 0 || y < 0 || x >= w || y >= h)
    return FALSE;
  else
    return TRUE;
}

GimpVector3
int_to_pos (gint x,
            gint y)
{
  GimpVector3 pos;

  pos.x = (gdouble) x / (gdouble) width;
  pos.y = (gdouble) y / (gdouble) height;
  pos.z = 0.0;

  return pos;
}

void
pos_to_int (gdouble  x,
            gdouble  y,
            gint    *scr_x,
            gint    *scr_y)
{
  *scr_x = (gint) ((x * (gdouble) width));
  *scr_y = (gint) ((y * (gdouble) height));
}

/**********************************************/
/* Compute the image color at pos (u,v) using */
/* Quartics bilinear interpolation stuff.     */
/**********************************************/

void
get_image_color (gdouble  u,
                 gdouble  v,
                 gint    *inside,
                 gdouble *color)
{
  gint     x1;
  gint     y1;
  gint     x2;
  gint     y2;
  gdouble  p[4];
  gdouble  pixels[16];

  pos_to_int (u, v, &x1, &y1);

  if (mapvals.tiled == TRUE)
    {
      *inside = TRUE;

      if (x1 < 0) x1 = (width-1) - (-x1 % width);
      else        x1 = x1 % width;

      if (y1 < 0) y1 = (height-1) - (-y1 % height);
      else        y1 = y1 % height;

      x2 = (x1 + 1) % width;
      y2 = (y1 + 1) % height;

      peek (x1, y1, p);
      for (gint i = 0; i < 4; i++)
        pixels[i] = p[i];
      peek (x2, y1, p);
      for (gint i = 0; i < 4; i++)
        pixels[i + 4] = p[i];
      peek (x1, y2, p);
      for (gint i = 0; i < 4; i++)
        pixels[i + 8] = p[i];
      peek (x2, y2, p);
      for (gint i = 0; i < 4; i++)
        pixels[i + 12] = p[i];

      gimp_bilinear_rgb (u * width, v * height, pixels, TRUE, color);

      return;
    }

  if (checkbounds (x1, y1) == FALSE)
    {
      *inside =FALSE;

      for (gint i = 0; i < 4; i++)
        color[i] = background[i];

      return;
    }

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (checkbounds (x2, y2) == FALSE)
   {
     *inside = TRUE;

     return peek (x1, y1, color);
   }

  *inside = TRUE;

  peek (x1, y1, p);
  for (gint i = 0; i < 4; i++)
    pixels[i] = p[i];
  peek (x2, y1, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 4] = p[i];
  peek (x1, y2, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 8] = p[i];
  peek (x2, y2, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 12] = p[i];

  gimp_bilinear_rgb (u * width, v * height, pixels, TRUE, color);
}

void
get_box_image_color (gint     image,
                     gdouble  u,
                     gdouble  v,
                     gdouble *color)
{
  gint     w;
  gint     h;
  gint     x1;
  gint     y1;
  gint     x2;
  gint     y2;
  gdouble  p[4];
  gdouble  pixels[16];

  w = gegl_buffer_get_width  (box_buffers[image]);
  h = gegl_buffer_get_height (box_buffers[image]);

  x1 = (gint) ((u * (gdouble) w));
  y1 = (gint) ((v * (gdouble) h));

  if (checkbounds_box_image (image, x1, y1) == FALSE)
    {
      for (gint i = 0; i < 4; i++)
        color[i] = background[i];

      return;
    }

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (checkbounds_box_image (image, x2, y2) == FALSE)
    return peek_box_image (image, x1, y1, color);

  peek_box_image (image, x1, y1, p);
  for (gint i = 0; i < 4; i++)
    pixels[i] = p[i];
  peek_box_image (image, x2, y1, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 4] = p[i];
  peek_box_image (image, x1, y2, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 8] = p[i];
  peek_box_image (image, x2, y2, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 12] = p[i];

  gimp_bilinear_rgb (u * w, v * w, pixels, TRUE, color);
}

void
get_cylinder_image_color (gint     image,
                          gdouble  u,
                          gdouble  v,
                          gdouble *color)
{
  gint     w;
  gint     h;
  gint     x1;
  gint     y1;
  gint     x2;
  gint     y2;
  gdouble  p[4];
  gdouble  pixels[16];

  w = gegl_buffer_get_width  (cylinder_buffers[image]);
  h = gegl_buffer_get_height (cylinder_buffers[image]);

  x1 = (gint) ((u * (gdouble) w));
  y1 = (gint) ((v * (gdouble) h));

  if (checkbounds_cylinder_image (image, x1, y1) == FALSE)
    {
      for (gint i = 0; i < 4; i++)
        color[i] = background[i];

      return;
    }

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (checkbounds_cylinder_image (image, x2, y2) == FALSE)
    return peek_cylinder_image (image, x1, y1, color);

  peek_cylinder_image (image, x1, y1, p);
  for (gint i = 0; i < 4; i++)
    pixels[i] = p[i];
  peek_cylinder_image (image, x2, y1, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 4] = p[i];
  peek_cylinder_image (image, x1, y2, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 8] = p[i];
  peek_cylinder_image (image, x2, y2, p);
  for (gint i = 0; i < 4; i++)
    pixels[i + 12] = p[i];

  gimp_bilinear_rgb (u * w, v * h, pixels, TRUE, color);
}

/****************************************/
/* Allocate memory for temporary images */
/****************************************/

gint
image_setup (GimpDrawable        *drawable,
             gint                 interactive,
             GimpProcedureConfig *config)
{
  gboolean transparent_background;

  input_drawable  = drawable;
  output_drawable = drawable;

  g_object_get (config,
                "transparent_background", &transparent_background,
                NULL);

  if (! gimp_drawable_mask_intersect (drawable, &border_x, &border_y,
                                      &border_w, &border_h))
    return FALSE;

  width  = gimp_drawable_get_width  (input_drawable);
  height = gimp_drawable_get_height (input_drawable);

  source_buffer = gimp_drawable_get_buffer (input_drawable);

  maxcounter = (glong) width * (glong) height;

  if (transparent_background == TRUE)
    {
      for (gint i = 0; i < 4; i++)
        background[i] = 0;
    }
  else
    {
      GeglColor *gegl_color;

      gegl_color = gimp_context_get_background ();
      gimp_color_set_alpha (gegl_color, 1.0);
      gegl_color_get_rgba_with_space (gegl_color, &background[0], &background[1],
                                      &background[2], &background[3], NULL);

      g_object_unref (gegl_color);
    }

  if (interactive == TRUE)
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
