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

#include <gtk/gtk.h>

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplay-handlers.h"


/*  local function prototypes  */

static void   gimp_display_update_handler (GimpProjection *projection,
                                           gboolean        now,
                                           gint            x,
                                           gint            y,
                                           gint            w,
                                           gint            h,
                                           GimpDisplay    *gdisp);
static void   gimp_display_flush_handler  (GimpImage      *image,
                                           GimpDisplay    *gdisp);


/*  public functions  */

void
gimp_display_connect (GimpDisplay *gdisp,
                      GimpImage   *image)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (gdisp->image == NULL);

  gdisp->image    = image;
  gdisp->instance = image->instance_count;

  image->instance_count++;   /* this is obsolete */
  image->disp_count++;

#if 0
  g_print ("%s: image->ref_count before refing: %d\n",
           G_STRFUNC, G_OBJECT (gdisp->image)->ref_count);
#endif

  g_object_ref (image);

  g_signal_connect (image->projection, "update",
                    G_CALLBACK (gimp_display_update_handler),
                    gdisp);
  g_signal_connect (image, "flush",
                    G_CALLBACK (gimp_display_flush_handler),
                    gdisp);
}

void
gimp_display_disconnect (GimpDisplay *gdisp)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (gdisp->image));

  g_signal_handlers_disconnect_by_func (gdisp->image,
                                        gimp_display_flush_handler,
                                        gdisp);
  g_signal_handlers_disconnect_by_func (gdisp->image->projection,
                                        gimp_display_update_handler,
                                        gdisp);

  gdisp->image->disp_count--;

#if 0
  g_print ("%s: image->ref_count before unrefing: %d\n",
           G_STRFUNC, G_OBJECT (gdisp->image)->ref_count);
#endif

  /*  set gdisp->image to NULL before unrefing because there may be code
   *  that listens for image removals and then iterates the display list
   *  to find a valid display.
   */
  image = gdisp->image;
  gdisp->image = NULL;

  g_object_unref (image);
}


/*  private functions  */

static void
gimp_display_update_handler (GimpProjection *projection,
                             gboolean        now,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             GimpDisplay    *gdisp)
{
  gimp_display_update_area (gdisp, now, x, y, w, h);
}

static void
gimp_display_flush_handler (GimpImage   *image,
                            GimpDisplay *gdisp)
{
  gimp_display_flush (gdisp);
}
