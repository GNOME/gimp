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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include "appenv.h"
#include "colormaps.h"
#include "gimage.h"
#include "gximage.h"
#include "errors.h"

typedef struct _GXImage  GXImage;

struct _GXImage
{
  long width, height;		/*  width and height of ximage structure    */

  GdkVisual *visual;		/*  visual appropriate to our depth         */
  GdkGC *gc;			/*  graphics context                        */

  GdkImage *image;		/*  private data  */
};


/*  The static gximages for drawing to windows  */
static GXImage *gximage = NULL;

#define QUANTUM   32

/*  STATIC functions  */

static GXImage *
create_gximage (GdkVisual *visual, int width, int height)
{
  GXImage * gximage;

  gximage = (GXImage *) g_malloc (sizeof (GXImage));

  gximage->visual = visual;
  gximage->gc = NULL;

  gximage->image = gdk_image_new (GDK_IMAGE_FASTEST, visual, width, height);

  return gximage;
}

static void
delete_gximage (GXImage *gximage)
{
  gdk_image_destroy (gximage->image);
  if (gximage->gc)
    gdk_gc_destroy (gximage->gc);
  g_free (gximage);
}

/****************************************************************/


/*  Function definitions  */

void
gximage_init ()
{
  gximage = create_gximage (g_visual, GXIMAGE_WIDTH, GXIMAGE_HEIGHT);
}

void
gximage_free ()
{
  delete_gximage (gximage);
}

guchar*
gximage_get_data ()
{
  return gximage->image->mem;
}

int
gximage_get_bpp ()
{
  return gximage->image->bpp;
}

int
gximage_get_bpl ()
{
  return gximage->image->bpl;
}

int
gximage_get_byte_order ()
{
  return gximage->image->byte_order;
}

void
gximage_put (GdkWindow *win, int x, int y, int w, int h)
{
  /*  create the GC if it doesn't yet exist  */
  if (!gximage->gc)
    {
      gximage->gc = gdk_gc_new (win);
      gdk_gc_set_exposures (gximage->gc, TRUE);
    }

  gdk_draw_image (win, gximage->gc, gximage->image, 0, 0, x, y, w, h);

  /*  sync the draw image to make sure it has been displayed before continuing  */
  gdk_flush ();
}
