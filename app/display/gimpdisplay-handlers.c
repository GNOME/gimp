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

static void   gimp_display_update_handler           (GimpImage   *gimage,
                                                     gint         x,
                                                     gint         y,
                                                     gint         w,
                                                     gint         h,
                                                     GimpDisplay *gdisp);
static void   gimp_display_mode_changed_handler     (GimpImage   *gimage,
                                                     GimpDisplay *gdisp);
static void   gimp_display_colormap_changed_handler (GimpImage   *gimage,
                                                     gint         ncol,
                                                     GimpDisplay *gdisp);
static void   gimp_display_size_changed_handler     (GimpImage   *gimage,
                                                     GimpDisplay *gdisp);
static void   gimp_display_flush_handler            (GimpImage   *gimage,
                                                     GimpDisplay *gdisp);


/*  public functions  */

void
gimp_display_connect (GimpDisplay *gdisp,
                      GimpImage   *gimage)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (gdisp->gimage == NULL);

  gdisp->monitor_xres = GIMP_DISPLAY_CONFIG (gimage->gimp->config)->monitor_xres;
  gdisp->monitor_yres = GIMP_DISPLAY_CONFIG (gimage->gimp->config)->monitor_yres;

  gdisp->gimage   = gimage;
  gdisp->instance = gimage->instance_count;

  gimage->instance_count++;   /* this is obsolete */
  gimage->disp_count++;

#if 0
  g_print ("%s: gimage->ref_count before refing: %d\n",
           G_GNUC_FUNCTION, G_OBJECT (gdisp->gimage)->ref_count);
#endif

  g_object_ref (G_OBJECT (gimage));

  g_signal_connect (G_OBJECT (gimage), "update",
                    G_CALLBACK (gimp_display_update_handler),
                    gdisp);
  g_signal_connect (G_OBJECT (gimage), "mode_changed",
                    G_CALLBACK (gimp_display_mode_changed_handler),
                    gdisp);
  g_signal_connect (G_OBJECT (gimage), "colormap_changed",
                    G_CALLBACK (gimp_display_colormap_changed_handler),
                    gdisp);
  g_signal_connect (G_OBJECT (gimage), "size_changed",
                    G_CALLBACK (gimp_display_size_changed_handler),
                    gdisp);
  g_signal_connect (G_OBJECT (gimage), "flush",
                    G_CALLBACK (gimp_display_flush_handler),
                    gdisp);
}

void
gimp_display_disconnect (GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (gdisp->gimage));

  g_signal_handlers_disconnect_by_func (G_OBJECT (gdisp->gimage),
                                        gimp_display_flush_handler,
                                        gdisp);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gdisp->gimage),
                                        gimp_display_size_changed_handler,
                                        gdisp);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gdisp->gimage),
                                        gimp_display_colormap_changed_handler,
                                        gdisp);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gdisp->gimage),
                                        gimp_display_mode_changed_handler,
                                        gdisp);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gdisp->gimage),
                                        gimp_display_update_handler,
                                        gdisp);

  gdisp->gimage->disp_count--;

#if 0
  g_print ("%s: gimage->ref_count before unrefing: %d\n",
           G_GNUC_FUNCTION, G_OBJECT (gdisp->gimage)->ref_count);
#endif

  g_object_unref (G_OBJECT (gdisp->gimage));
  gdisp->gimage = NULL;
}


/*  private functions  */

static void
gimp_display_update_handler (GimpImage   *gimage,
                             gint         x,
                             gint         y,
                             gint         w,
                             gint         h,
                             GimpDisplay *gdisp)
{
  gimp_display_add_update_area (gdisp, x, y, w, h);
}

static void
gimp_display_mode_changed_handler (GimpImage   *gimage,
                                   GimpDisplay *gdisp)
{
  gimp_display_add_update_area (gdisp,
                                0, 0,
                                gdisp->gimage->width,
                                gdisp->gimage->height);
}

static void
gimp_display_colormap_changed_handler (GimpImage   *gimage,
                                       gint         ncol,
                                       GimpDisplay *gdisp)
{
  if (gimp_image_base_type (gdisp->gimage) == GIMP_INDEXED)
    {
      gimp_display_add_update_area (gdisp,
                                    0, 0,
                                    gdisp->gimage->width,
                                    gdisp->gimage->height);
    }
}

static void
gimp_display_size_changed_handler (GimpImage   *gimage,
                                   GimpDisplay *gdisp)
{
  gimp_display_add_update_area (gdisp,
                                0, 0,
                                gdisp->gimage->width,
                                gdisp->gimage->height);
}

static void
gimp_display_flush_handler (GimpImage   *gimage,
                            GimpDisplay *gdisp)
{
  gimp_display_flush (gdisp);
}
