#include "shadow.h"
#include <math.h>

#define BLUR_RADIUS 5
#define SHADOW_OFFSET (BLUR_RADIUS * 4 / 5)
#define SHADOW_OPACITY 0.75

typedef struct {
  int size;
  double *data;
} ConvFilter;

static double
gaussian (double x, double y, double r)
{
    return ((1 / (2 * M_PI * r)) *
	    exp ((- (x * x + y * y)) / (2 * r * r)));
}

static ConvFilter *
create_blur_filter (int radius)
{
  ConvFilter *filter;
  int x, y;
  double sum;
  
  filter = g_new0 (ConvFilter, 1);
  filter->size = radius * 2 + 1;
  filter->data = g_new (double, filter->size * filter->size);

  sum = 0.0;
  
  for (y = 0 ; y < filter->size; y++)
    {
      for (x = 0 ; x < filter->size; x++)
	{
	  sum += filter->data[y * filter->size + x] = gaussian (x - (filter->size >> 1),
								y - (filter->size >> 1),
								radius);
	}
    }

  for (y = 0; y < filter->size; y++)
    {
      for (x = 0; x < filter->size; x++)
	{
	  filter->data[y * filter->size + x] /= sum;
	}
    }

  return filter;
  
}

static GdkPixbuf *
create_shadow (GdkPixbuf *src)
{
  int x, y, i, j;
  int width, height;
  GdkPixbuf *dest;
  static ConvFilter *filter = NULL;
  int src_rowstride, dest_rowstride;
  int src_bpp, dest_bpp;
  
  guchar *src_pixels, *dest_pixels;

  if (!filter)
    filter = create_blur_filter (BLUR_RADIUS);
  
  width = gdk_pixbuf_get_width (src) + BLUR_RADIUS * 2 + SHADOW_OFFSET;
  height = gdk_pixbuf_get_height (src) + BLUR_RADIUS * 2 + SHADOW_OFFSET;

  dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			 gdk_pixbuf_get_has_alpha (src),
			 gdk_pixbuf_get_bits_per_sample (src),
			 width, height);
  gdk_pixbuf_fill (dest, 0);  
  src_pixels = gdk_pixbuf_get_pixels (src);
  src_rowstride = gdk_pixbuf_get_rowstride (src);
  src_bpp = gdk_pixbuf_get_has_alpha (src) ? 4 : 3;
  
  dest_pixels = gdk_pixbuf_get_pixels (dest);
  dest_rowstride = gdk_pixbuf_get_rowstride (dest);
  dest_bpp = gdk_pixbuf_get_has_alpha (dest) ? 4 : 3;
  
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
	{
	  int sumr = 0, sumg = 0, sumb = 0, suma = 0;

	  for (i = 0; i < filter->size; i++)
	    {
	      for (j = 0; j < filter->size; j++)
		{
		  int src_x, src_y;

		  src_y = -(BLUR_RADIUS + SHADOW_OFFSET) + y - (filter->size >> 1) + i;
		  src_x = -(BLUR_RADIUS + SHADOW_OFFSET) + x - (filter->size >> 1) + j;

		  if (src_y < 0 || src_y > gdk_pixbuf_get_height (src) ||
		      src_x < 0 || src_x > gdk_pixbuf_get_width (src))
		    continue;

		  sumr += src_pixels [src_y * src_rowstride +
				      src_x * src_bpp + 0] *
		    filter->data [i * filter->size + j];
		  sumg += src_pixels [src_y * src_rowstride +
				      src_x * src_bpp + 1] * 
		    filter->data [i * filter->size + j];

		  sumb += src_pixels [src_y * src_rowstride +
				      src_x * src_bpp + 2] * 
		    filter->data [i * filter->size + j];
		  
		  if (src_bpp == 4)
		    suma += src_pixels [src_y * src_rowstride +
					src_x * src_bpp + 3] *
		    filter->data [i * filter->size + j];

		    
		}
	    }

	  if (dest_bpp == 4)
	    dest_pixels [y * dest_rowstride +
			 x * dest_bpp + 3] = suma * SHADOW_OPACITY;

	}
    }
  
  return dest;
}

GdkPixbuf *
create_shadowed_pixbuf (GdkPixbuf *src)
{
  GdkPixbuf *dest;
  
  dest = create_shadow (src);

  gdk_pixbuf_composite (src, dest,
			BLUR_RADIUS, BLUR_RADIUS,
			gdk_pixbuf_get_width (src),
			gdk_pixbuf_get_height (src),
			BLUR_RADIUS, BLUR_RADIUS, 1.0, 1.0,
			GDK_INTERP_NEAREST, 255);
  return dest;
}
