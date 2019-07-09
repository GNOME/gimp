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


gint32      input_drawable_id;
gint32      output_drawable_id;
GeglBuffer *source_buffer;
GeglBuffer *dest_buffer;

gint32      box_drawable_ids[6];
GeglBuffer *box_buffers[6];

gint32      cylinder_drawable_ids[2];
GeglBuffer *cylinder_buffers[2];

guchar          *preview_rgb_data = NULL;
gint             preview_rgb_stride;
cairo_surface_t *preview_surface = NULL;

glong    maxcounter, old_depth, max_depth;
gint     width, height, image_id;
GimpRGB  background;

gint border_x, border_y, border_w, border_h;

/******************/
/* Implementation */
/******************/

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

static GimpRGB
peek_box_image (gint image,
                gint x,
                gint y)
{
  GimpRGB color;

  gegl_buffer_sample (box_buffers[image], x, y, NULL,
                      &color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! babl_format_has_alpha (gegl_buffer_get_format (box_buffers[image])))
    color.a = 1.0;

  return color;
}

static GimpRGB
peek_cylinder_image (gint image,
                     gint x,
                     gint y)
{
  GimpRGB color;

  gegl_buffer_sample (cylinder_buffers[image], x, y, NULL,
                      &color, babl_format ("R'G'B'A double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! babl_format_has_alpha (gegl_buffer_get_format (cylinder_buffers[image])))
    color.a = 1.0;

  return color;
}

void
poke (gint      x,
      gint      y,
      GimpRGB  *color,
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

GimpRGB
get_image_color (gdouble  u,
                 gdouble  v,
                 gint    *inside)
{
  gint    x1, y1, x2, y2;
  GimpRGB p[4];

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

      p[0] = peek (x1, y1);
      p[1] = peek (x2, y1);
      p[2] = peek (x1, y2);
      p[3] = peek (x2, y2);

      return gimp_bilinear_rgba (u * width, v * height, p);
    }

  if (checkbounds (x1, y1) == FALSE)
    {
      *inside =FALSE;

      return background;
    }

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (checkbounds (x2, y2) == FALSE)
   {
     *inside = TRUE;

     return peek (x1, y1);
   }

  *inside=TRUE;

  p[0] = peek (x1, y1);
  p[1] = peek (x2, y1);
  p[2] = peek (x1, y2);
  p[3] = peek (x2, y2);

  return gimp_bilinear_rgba (u * width, v * height, p);
}

GimpRGB
get_box_image_color (gint    image,
                     gdouble u,
                     gdouble v)
{
  gint    w, h;
  gint    x1, y1, x2, y2;
  GimpRGB p[4];

  w = gegl_buffer_get_width  (box_buffers[image]);
  h = gegl_buffer_get_height (box_buffers[image]);

  x1 = (gint) ((u * (gdouble) w));
  y1 = (gint) ((v * (gdouble) h));

  if (checkbounds_box_image (image, x1, y1) == FALSE)
    return background;

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (checkbounds_box_image (image, x2, y2) == FALSE)
    return peek_box_image (image, x1,y1);

  p[0] = peek_box_image (image, x1, y1);
  p[1] = peek_box_image (image, x2, y1);
  p[2] = peek_box_image (image, x1, y2);
  p[3] = peek_box_image (image, x2, y2);

  return gimp_bilinear_rgba (u * w, v * h, p);
}

GimpRGB
get_cylinder_image_color (gint    image,
                          gdouble u,
                          gdouble v)
{
  gint    w, h;
  gint    x1, y1, x2, y2;
  GimpRGB p[4];

  w = gegl_buffer_get_width  (cylinder_buffers[image]);
  h = gegl_buffer_get_height (cylinder_buffers[image]);

  x1 = (gint) ((u * (gdouble) w));
  y1 = (gint) ((v * (gdouble) h));

  if (checkbounds_cylinder_image (image, x1, y1) == FALSE)
    return background;

  x2 = (x1 + 1);
  y2 = (y1 + 1);

  if (checkbounds_cylinder_image (image, x2, y2) == FALSE)
    return peek_cylinder_image (image, x1,y1);

  p[0] = peek_cylinder_image (image, x1, y1);
  p[1] = peek_cylinder_image (image, x2, y1);
  p[2] = peek_cylinder_image (image, x1, y2);
  p[3] = peek_cylinder_image (image, x2, y2);

  return gimp_bilinear_rgba (u * w, v * h, p);
}

/****************************************/
/* Allocate memory for temporary images */
/****************************************/

gint
image_setup (gint32 drawable_id,
             gint   interactive)
{
  input_drawable_id  = drawable_id;
  output_drawable_id = drawable_id;

  if (! gimp_drawable_mask_intersect (drawable_id, &border_x, &border_y,
                                      &border_w, &border_h))
    return FALSE;

  width  = gimp_drawable_width  (input_drawable_id);
  height = gimp_drawable_height (input_drawable_id);

  source_buffer = gimp_drawable_get_buffer (input_drawable_id);

  maxcounter = (glong) width * (glong) height;

  if (mapvals.transparent_background == TRUE)
    {
      gimp_rgba_set (&background, 0.0, 0.0, 0.0, 0.0);
    }
  else
    {
      gimp_context_get_background (&background);
      gimp_rgb_set_alpha (&background, 1.0);
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
