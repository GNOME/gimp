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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "tools/tools-types.h"

#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpstatusbar.h"

#include "gimprc.h"


/*  local function prototypes  */

static gdouble   img2real (GimpDisplay *gdisp,
                           gboolean     xdir,
                           gdouble      a);


/*  public functions  */

void
gimp_display_shell_scale_setup (GimpDisplayShell *shell)
{
  GtkRuler *hruler;
  GtkRuler *vruler;
  gfloat    sx, sy;
  gfloat    stepx, stepy;
  gint      image_width, image_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image_width  = shell->gdisp->gimage->width;
  image_height = shell->gdisp->gimage->height;

  sx    = SCALEX (shell->gdisp, image_width);
  sy    = SCALEY (shell->gdisp, image_height);
  stepx = SCALEFACTOR_X (shell->gdisp);
  stepy = SCALEFACTOR_Y (shell->gdisp);

  shell->hsbdata->value          = shell->offset_x;
  shell->hsbdata->upper          = sx;
  shell->hsbdata->page_size      = MIN (sx, shell->disp_width);
  shell->hsbdata->page_increment = shell->disp_width / 2;
  shell->hsbdata->step_increment = stepx;

  shell->vsbdata->value          = shell->offset_y;
  shell->vsbdata->upper          = sy;
  shell->vsbdata->page_size      = MIN (sy, shell->disp_height);
  shell->vsbdata->page_increment = shell->disp_height / 2;
  shell->vsbdata->step_increment = stepy;

  gtk_adjustment_changed (shell->hsbdata);
  gtk_adjustment_changed (shell->vsbdata);

  hruler = GTK_RULER (shell->hrule);
  vruler = GTK_RULER (shell->vrule);

  hruler->lower    = 0;
  hruler->upper    = img2real (shell->gdisp, TRUE,
                               FUNSCALEX (shell->gdisp, shell->disp_width));
  hruler->max_size = img2real (shell->gdisp, TRUE,
                               MAX (image_width, image_height));

  vruler->lower    = 0;
  vruler->upper    = img2real (shell->gdisp, FALSE,
                               FUNSCALEY (shell->gdisp, shell->disp_height));
  vruler->max_size = img2real (shell->gdisp, FALSE,
                               MAX (image_width, image_height));

  if (sx < shell->disp_width)
    {
      shell->disp_xoffset = (shell->disp_width - sx) / 2;

      hruler->lower -= img2real (shell->gdisp, TRUE,
                                 FUNSCALEX (shell->gdisp,
                                            (gdouble) shell->disp_xoffset));
      hruler->upper -= img2real (shell->gdisp, TRUE,
                                 FUNSCALEX (shell->gdisp,
                                            (gdouble) shell->disp_xoffset));
    }
  else
    {
      shell->disp_xoffset = 0;

      hruler->lower += img2real (shell->gdisp, TRUE,
				 FUNSCALEX (shell->gdisp,
                                            (gdouble) shell->offset_x));
      hruler->upper += img2real (shell->gdisp, TRUE,
				 FUNSCALEX (shell->gdisp,
                                            (gdouble) shell->offset_x));
    }

  if (sy < shell->disp_height)
    {
      shell->disp_yoffset = (shell->disp_height - sy) / 2;

      vruler->lower -= img2real (shell->gdisp, FALSE,
				 FUNSCALEY (shell->gdisp,
                                            (gdouble) shell->disp_yoffset));
      vruler->upper -= img2real (shell->gdisp, FALSE,
				 FUNSCALEY (shell->gdisp,
                                            (gdouble) shell->disp_yoffset));
    }
  else
    {
      shell->disp_yoffset = 0;

      vruler->lower += img2real (shell->gdisp, FALSE,
				 FUNSCALEY (shell->gdisp,
                                            (gdouble) shell->offset_y));
      vruler->upper += img2real (shell->gdisp, FALSE,
				 FUNSCALEY (shell->gdisp,
                                            (gdouble) shell->offset_y));
    }

  gtk_widget_queue_draw (GTK_WIDGET (hruler));
  gtk_widget_queue_draw (GTK_WIDGET (vruler));

  gimp_display_shell_scaled (shell);

#if 0
  g_print ("offset_x:     %d\n"
           "offset_y:     %d\n"
           "disp_width:   %d\n"
           "disp_height:  %d\n"
           "disp_xoffset: %d\n"
           "disp_yoffset: %d\n\n",
           shell->offset_x, shell->offset_y,
           shell->disp_width, shell->disp_height,
           shell->disp_xoffset, shell->disp_yoffset);
#endif
}

void
gimp_display_shell_scale_set_dot_for_dot (GimpDisplayShell *shell,
                                          gboolean          dot_for_dot)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (dot_for_dot != shell->gdisp->dot_for_dot)
    {
      /* freeze the active tool */
      tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                                   shell->gdisp);

      shell->gdisp->dot_for_dot = dot_for_dot;

      gimp_statusbar_resize_cursor (GIMP_STATUSBAR (shell->statusbar));

      gimp_display_shell_scale_resize (shell,
                                       gimprc.resize_windows_on_zoom, TRUE);

      /* re-enable the active tool */
      tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                                   shell->gdisp);
    }
}

void
gimp_display_shell_scale (GimpDisplayShell *shell,
                          GimpZoomType      zoom_type)
{
  guchar  scalesrc, scaledest;
  gdouble offset_x, offset_y;
  glong   sx, sy;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* user zoom control, so resolution versions not needed -- austin */
  scalesrc  = SCALESRC (shell->gdisp);
  scaledest = SCALEDEST (shell->gdisp);

  offset_x = shell->offset_x + (shell->disp_width / 2.0);
  offset_y = shell->offset_y + (shell->disp_height / 2.0);

  offset_x *= ((double) scalesrc / (double) scaledest);
  offset_y *= ((double) scalesrc / (double) scaledest);

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      if (scalesrc > 1)
	scalesrc--;
      else
	if (scaledest < 0x10)
	  scaledest++;
	else
	  return;
      break;

    case GIMP_ZOOM_OUT:
      if (scaledest > 1)
	scaledest--;
      else
	if (scalesrc < 0x10)
	  scalesrc++;
	else
	  return;
      break;

    default:
      scalesrc  = CLAMP (zoom_type % 100, 1, 0x10);
      scaledest = CLAMP (zoom_type / 100, 1, 0x10);
      break;
    }

  sx = (shell->gdisp->gimage->width  * scaledest) / scalesrc;
  sy = (shell->gdisp->gimage->height * scaledest) / scalesrc;

  /*  The slider value is a short, so make sure we are within its
   *  range.  If we are trying to scale past it, then stop the scale
   */
  if (sx < 0xffff && sy < 0xffff)
    {
      /*  set the offsets  */
      offset_x *= ((gdouble) scaledest / (gdouble) scalesrc);
      offset_y *= ((gdouble) scaledest / (gdouble) scalesrc);

      gimp_display_shell_scale_by_values (shell,
                                          (scaledest << 8) + scalesrc,
                                          (offset_x - (shell->disp_width  / 2)),
                                          (offset_y - (shell->disp_height / 2)),
                                          gimprc.resize_windows_on_zoom);
    }
}

void
gimp_display_shell_scale_fit (GimpDisplayShell *shell)
{
  gint    image_width;
  gint    image_height;
  gdouble zoom_x;
  gdouble zoom_y;
  gdouble zoom_factor;
  gdouble zoom_delta;
  gdouble min_zoom_delta = G_MAXFLOAT;
  gint    scalesrc       = 1;
  gint    scaledest      = 1;
  gint    i;
  gint    best_i         = 0x10;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image_width  = shell->gdisp->gimage->width;
  image_height = shell->gdisp->gimage->height;

  if (! shell->gdisp->dot_for_dot)
    {
      image_width  = ROUND (image_width *
                            gimprc.monitor_xres /
                            shell->gdisp->gimage->xresolution);
      image_height = ROUND (image_height *
                            gimprc.monitor_yres /
                            shell->gdisp->gimage->yresolution);
    }

  zoom_x = (gdouble) shell->disp_width  / (gdouble) image_width;
  zoom_y = (gdouble) shell->disp_height / (gdouble) image_height;

  if ((gdouble) image_height * zoom_x <= (gdouble) shell->disp_height)
    {
      zoom_factor = zoom_x;
    }
  else
    {
      zoom_factor = zoom_y;
    }

  if (zoom_factor < 1.0)
    {
      for (i = 0x10; i > 0; i--)
        {
          scalesrc  = i;
          scaledest = floor ((gdouble) scalesrc * zoom_factor);

          if (scaledest < 0x1)
            {
              scaledest = 0x1;
            }

          zoom_delta = ABS ((gdouble) scaledest / (gdouble) scalesrc -
                            zoom_factor);

          if (zoom_delta <= min_zoom_delta)
            {
              min_zoom_delta = zoom_delta;
              best_i = i;
            }
        }

      scalesrc  = best_i;
      scaledest = floor ((gdouble) scalesrc * zoom_factor);

      if (scaledest < 0x1)
        {
          scaledest = 0x1;
        }
    }
  else
    {
      for (i = 0x10; i > 0; i--)
        {
          scaledest = i;
          scalesrc  = ceil ((gdouble) scaledest / zoom_factor);

          if (scalesrc < 0x1)
            {
              scalesrc = 0x1;
            }

          zoom_delta = ABS ((gdouble) scaledest / (gdouble) scalesrc -
                            zoom_factor);

          if (zoom_delta <= min_zoom_delta)
            {
              min_zoom_delta = zoom_delta;
              best_i = i;
            }
        }

      scaledest = best_i;
      scalesrc  = ceil ((gdouble) scaledest / zoom_factor);

      if (scalesrc < 0x1)
        {
          scalesrc = 0x1;
        }
    }

  gimp_display_shell_scale_by_values (shell,
                                      (scaledest << 8) + scalesrc,
                                      0,
                                      0,
                                      FALSE);
}

void
gimp_display_shell_scale_by_values (GimpDisplayShell *shell,
                                    gint              scale,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gboolean          resize_window)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* freeze the active tool */
  tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                               shell->gdisp);

  shell->gdisp->scale = scale;
  shell->offset_x     = offset_x;
  shell->offset_y     = offset_y;

  gimp_display_shell_scale_resize (shell, resize_window, TRUE);

  /* re-enable the active tool */
  tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                               shell->gdisp);
}

void
gimp_display_shell_scale_shrink_wrap (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_resize (shell, TRUE, TRUE);
}

void
gimp_display_shell_scale_resize (GimpDisplayShell *shell,
                                 gboolean          resize_window,
                                 gboolean          redisplay)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* freeze the active tool */
  tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                               shell->gdisp);

  if (resize_window)
    gimp_display_shell_shrink_wrap (shell);

  gimp_display_shell_scroll_clamp_offsets (shell);
  gimp_display_shell_scale_setup (shell);

  if (resize_window || redisplay)
    {
      gimp_display_shell_expose_full (shell);

      /* title may have changed if it includes the zoom ratio */
      shell->title_dirty = TRUE;

      gdisplays_flush ();
    }

  /* re-enable the active tool */
  tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                               shell->gdisp);
}


/*  private functions  */

/* scale image coord to realworld units (cm, inches, pixels)
 *
 * 27/Feb/1999 I tried inlining this, but the result was slightly
 * slower (poorer cache locality, probably) -- austin
 */
static gdouble
img2real (GimpDisplay *gdisp,
	  gboolean     xdir,
	  gdouble      a)
{
  gdouble res;

  if (gdisp->dot_for_dot)
    return a;

  if (xdir)
    res = gdisp->gimage->xresolution;
  else
    res = gdisp->gimage->yresolution;

  return a * gimp_unit_get_factor (gdisp->gimage->unit) / res;
}
