/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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
#include "gimage.h"
#include "gimpdnd.h"
#include "gimprc.h"

#define GRAD_CHECK_SIZE_SM 4

#define GRAD_CHECK_DARK  (1.0 / 3.0)
#define GRAD_CHECK_LIGHT (2.0 / 3.0)

/* Good idea for size to be <= small preview size in LCP dialog.
 * that way we get good cache hits.
 */

#define DRAG_IMAGE_SZ    32 

void
gimp_dnd_set_drawable_preview_icon (GtkWidget      *widget,
				    GdkDragContext *context,
				    GimpDrawable   *drawable,
				    GdkGC          *gc)
{
  GdkPixmap *drag_pixmap;
  GImage    *gimage;
  TempBuf   *tmpbuf;
  gint       bpp;
  gint       x, y;
  guchar    *src;
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  guchar    *p0, *p1, *even, *odd;
  gint       width;
  gint       height;
  gdouble    ratio;
  gint       offx, offy;

  if (! preview_size)
    return;

  gimage = gimp_drawable_gimage (drawable);

  if (gimage->width > gimage->height)
    ratio = (gdouble) DRAG_IMAGE_SZ / (gdouble) gimage->width;
  else
    ratio = (gdouble) DRAG_IMAGE_SZ / (gdouble) gimage->height;

  width =  (gint) (ratio * gimage->width);
  height = (gint) (ratio * gimage->height);

  if (width < 1) 
    width = 1;
  if (height < 1)
    height = 1;

  gimp_drawable_offsets (drawable, &offx, &offy);

  offx = (int) (ratio * offx);
  offy = (int) (ratio * offy);

  drag_pixmap = gdk_pixmap_new (widget->window,
				width+2, height+2, -1);

  gdk_draw_rectangle (drag_pixmap, 
		      /*  Is this always valid???  */
		      widget->style->bg_gc[GTK_STATE_NORMAL],
		      TRUE,
		      0, 0,
		      width + 2,
		      height + 2);

  gdk_draw_rectangle (drag_pixmap, 
		      gc,
		      FALSE,
		      0, 0,
		      width + 1,
		      height + 1);

  /*  readjust for actual layer size  */
  width  = (gint) (ratio * gimp_drawable_width  (drawable));
  height = (gint) (ratio * gimp_drawable_height (drawable));

  if (width < 1) 
    width = 1;
  if (height < 1)
    height = 1;

  if (GIMP_IS_LAYER (drawable))
    {
      tmpbuf = layer_preview (GIMP_LAYER (drawable), width, height);
    }
  else if (GIMP_IS_LAYER_MASK (drawable))
    {
      tmpbuf =
	layer_mask_preview (layer_mask_get_layer (GIMP_LAYER_MASK (drawable)),
			    width, height);
    }
  else if (GIMP_IS_CHANNEL (drawable))
    {
      tmpbuf = channel_preview (GIMP_CHANNEL (drawable), width, height);
    }
  else
    {
      gdk_pixmap_unref (drag_pixmap);
      return;
    }

  bpp = tmpbuf->bytes;

  /*  Draw the thumbnail with checks  */
  src = temp_buf_data (tmpbuf);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  
  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;

      for (x = 0; x < width; x++)
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble) src[x*4+0]) / 255.0;
	      g = ((gdouble) src[x*4+1]) / 255.0;
	      b = ((gdouble) src[x*4+2]) / 255.0;
	      a = ((gdouble) src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble) src[x*3+0]) / 255.0;
	      g = ((gdouble) src[x*3+1]) / 255.0;
	      b = ((gdouble) src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble) src[x*bpp+0]) / 255.0;
	      g = b = r;
	      if (bpp == 2)
		a = ((gdouble) src[x*bpp+1]) / 255.0;
	      else
		a = 1.0;
	    }

	  if ((x / GRAD_CHECK_SIZE_SM) & 1)
	    {
	      c0 = GRAD_CHECK_LIGHT;
	      c1 = GRAD_CHECK_DARK;
	    }
	  else
	    {
	      c0 = GRAD_CHECK_DARK;
	      c1 = GRAD_CHECK_LIGHT;
	    }

	  *p0++ = (c0 + (r - c0) * a) * 255.0;
	  *p0++ = (c0 + (g - c0) * a) * 255.0;
	  *p0++ = (c0 + (b - c0) * a) * 255.0;

	  *p1++ = (c1 + (r - c1) * a) * 255.0;
	  *p1++ = (c1 + (g - c1) * a) * 255.0;
	  *p1++ = (c1 + (b - c1) * a) * 255.0;

	}
      
      if ((y / GRAD_CHECK_SIZE_SM) & 1)
	{
	  gdk_draw_rgb_image (drag_pixmap, gc,
			      1+offx, y+1+offy,
			      width,
			      1,
			      GDK_RGB_DITHER_NORMAL,
			      (guchar *) odd,
			      3);
	}
      else
	{
	  gdk_draw_rgb_image (drag_pixmap, gc,
			      1+offx, y+1+offy,
			      width,
			      1,
			      GDK_RGB_DITHER_NORMAL,
			      (guchar *) even,
			      3);
	}
      src += width * bpp;
    }

  g_free (even);
  g_free (odd);

  gtk_drag_set_icon_pixmap (context,
			    gtk_widget_get_colormap (widget),
			    drag_pixmap,
			    NULL,
			    -8, -8);

  gdk_pixmap_unref (drag_pixmap);
}
