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

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-unit.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-title.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


typedef struct _ScaleDialogData ScaleDialogData;

struct _ScaleDialogData
{
  GimpDisplayShell *shell;
  GtkObject        *src_adj;
  GtkObject        *dest_adj;
};


/*  local function prototypes  */

static void gimp_display_shell_scale_dialog_response (GtkWidget        *widget,
                                                      gint              response_id,
                                                      ScaleDialogData  *dialog);
static gdouble img2real                              (GimpDisplayShell *shell,
                                                      gboolean          xdir,
                                                      gdouble           a);


/*  public functions  */

void
gimp_display_shell_scale_zoom_fraction (GimpZoomType  zoom_type,
                                        gint         *scalesrc,
                                        gint         *scaledest)
{
  gdouble ratio;

  g_return_if_fail (scalesrc != NULL);
  g_return_if_fail (scaledest != NULL);

  ratio = (double) *scaledest/ *scalesrc;

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      ratio *= G_SQRT2;
      break;

    case GIMP_ZOOM_OUT:
      ratio /= G_SQRT2;
      break;

    default:
      *scalesrc  = CLAMP (zoom_type % 100, 1, 0xFF);
      *scaledest = CLAMP (zoom_type / 100, 1, 0xFF);
      return;
      break;
    }

  /* set scalesrc and scaledest to a fraction close to ratio */
  gimp_display_shell_scale_calc_fraction (ratio, scalesrc, scaledest);
}

void
gimp_display_shell_scale_calc_fraction (gdouble  zoom_factor,
                                        gint    *scalesrc,
                                        gint    *scaledest)
{
  gint    p0, p1, p2;
  gint    q0, q1, q2;
  gdouble remainder, next_cf;

  g_return_if_fail (scalesrc != NULL);
  g_return_if_fail (scaledest != NULL);

  /* calculate the continued fraction for the desired zoom factor */

  p0 = 1;
  q0 = 0;
  p1 = floor (zoom_factor);
  q1 = 1;

  remainder = zoom_factor - p1;

  while (fabs (remainder) >= 0.0001 &&
         fabs (((gdouble) p1 / q1) - zoom_factor) > 0.0001)
    {
      remainder = 1.0 / remainder;

      next_cf = floor (remainder);

      p2 = next_cf * p1 + p0;
      q2 = next_cf * q1 + q0;

      /* Numerator and Denominator are limited by 255 */
      if (p2 > 255 || q2 > 255)
        break;

      /* remember the last two fractions */
      p0 = p1;
      p1 = p2;
      q0 = q1;
      q1 = q2;

      remainder = remainder - next_cf;
    }

  zoom_factor = (gdouble) p1 / q1;

  /* hard upper and lower bounds for zoom ratio */

  if (zoom_factor > 255.0)
    {
      p1 = 255;
      q1 = 1;
    }
  else if (zoom_factor < 1.0 / 255.0)
    {
      p1 = 1;
      q1 = 255;
    }

  *scalesrc = q1;
  *scaledest = p1;
}

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

  sx    = SCALEX (shell, image_width);
  sy    = SCALEY (shell, image_height);
  stepx = SCALEFACTOR_X (shell);
  stepy = SCALEFACTOR_Y (shell);

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
  hruler->upper    = img2real (shell, TRUE,
                               FUNSCALEX (shell, shell->disp_width));
  hruler->max_size = img2real (shell, TRUE,
                               MAX (image_width, image_height));

  vruler->lower    = 0;
  vruler->upper    = img2real (shell, FALSE,
                               FUNSCALEY (shell, shell->disp_height));
  vruler->max_size = img2real (shell, FALSE,
                               MAX (image_width, image_height));

  if (sx < shell->disp_width)
    {
      shell->disp_xoffset = (shell->disp_width - sx) / 2;

      hruler->lower -= img2real (shell, TRUE,
                                 FUNSCALEX (shell,
                                            (gdouble) shell->disp_xoffset));
      hruler->upper -= img2real (shell, TRUE,
                                 FUNSCALEX (shell,
                                            (gdouble) shell->disp_xoffset));
    }
  else
    {
      shell->disp_xoffset = 0;

      hruler->lower += img2real (shell, TRUE,
				 FUNSCALEX (shell,
                                            (gdouble) shell->offset_x));
      hruler->upper += img2real (shell, TRUE,
				 FUNSCALEX (shell,
                                            (gdouble) shell->offset_x));
    }

  if (sy < shell->disp_height)
    {
      shell->disp_yoffset = (shell->disp_height - sy) / 2;

      vruler->lower -= img2real (shell, FALSE,
				 FUNSCALEY (shell,
                                            (gdouble) shell->disp_yoffset));
      vruler->upper -= img2real (shell, FALSE,
				 FUNSCALEY (shell,
                                            (gdouble) shell->disp_yoffset));
    }
  else
    {
      shell->disp_yoffset = 0;

      vruler->lower += img2real (shell, FALSE,
				 FUNSCALEY (shell,
                                            (gdouble) shell->offset_y));
      vruler->upper += img2real (shell, FALSE,
				 FUNSCALEY (shell,
                                            (gdouble) shell->offset_y));
    }

  gtk_widget_queue_draw (GTK_WIDGET (hruler));
  gtk_widget_queue_draw (GTK_WIDGET (vruler));

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

  if (dot_for_dot != shell->dot_for_dot)
    {
      Gimp *gimp = shell->gdisp->gimage->gimp;

      /* freeze the active tool */
      gimp_display_shell_pause (shell);

      shell->dot_for_dot = dot_for_dot;

      gimp_statusbar_resize_cursor (GIMP_STATUSBAR (shell->statusbar));

      gimp_display_shell_scale_resize (shell,
				       GIMP_DISPLAY_CONFIG (gimp->config)->resize_windows_on_zoom,
				       TRUE);

      /* re-enable the active tool */
      gimp_display_shell_resume (shell);
    }
}

void
gimp_display_shell_scale (GimpDisplayShell *shell,
                          GimpZoomType      zoom_type)
{
  GimpDisplayConfig *config;
  gint               scalesrc, scaledest;
  gdouble            offset_x, offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* user zoom control, so resolution versions not needed -- austin */
  scalesrc  = SCALESRC (shell);
  scaledest = SCALEDEST (shell);

  offset_x = shell->offset_x + (shell->disp_width  / 2.0);
  offset_y = shell->offset_y + (shell->disp_height / 2.0);

  offset_x *= ((gdouble) scalesrc / (gdouble) scaledest);
  offset_y *= ((gdouble) scalesrc / (gdouble) scaledest);

  gimp_display_shell_scale_zoom_fraction (zoom_type, &scalesrc, &scaledest);

  offset_x *= ((gdouble) scaledest / (gdouble) scalesrc);
  offset_y *= ((gdouble) scaledest / (gdouble) scalesrc);

  config = GIMP_DISPLAY_CONFIG (shell->gdisp->gimage->gimp->config);

  gimp_display_shell_scale_by_values (shell,
                                      (scaledest << 8) + scalesrc,
                                      (offset_x - (shell->disp_width  / 2)),
                                      (offset_y - (shell->disp_height / 2)),
                                      config->resize_windows_on_zoom);
}

void
gimp_display_shell_scale_fit (GimpDisplayShell *shell)
{
  GimpImage *gimage;
  gint       image_width;
  gint       image_height;
  gdouble    zoom_factor;
  gint       scalesrc;
  gint       scaledest;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  image_width  = gimage->width;
  image_height = gimage->height;

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width *
                            shell->monitor_xres / gimage->xresolution);
      image_height = ROUND (image_height *
                            shell->monitor_xres / gimage->yresolution);
    }

  zoom_factor = MIN ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale_calc_fraction (zoom_factor,
                                          &scalesrc, &scaledest);

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
  Gimp *gimp;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp = shell->gdisp->gimage->gimp;

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  shell->scale    = scale;
  shell->offset_x = offset_x;
  shell->offset_y = offset_y;

  gimp_display_shell_scale_resize (shell, resize_window, TRUE);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
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
  Gimp *gimp;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp = shell->gdisp->gimage->gimp;

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  if (resize_window)
    gimp_display_shell_shrink_wrap (shell);

  gimp_display_shell_scroll_clamp_offsets (shell);
  gimp_display_shell_scale_setup (shell);
  gimp_display_shell_scaled (shell);

  if (resize_window || redisplay)
    gimp_display_shell_expose_full (shell);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

void
gimp_display_shell_scale_dialog (GimpDisplayShell *shell)
{
  ScaleDialogData *data;
  GtkWidget       *hbox;
  GtkWidget       *spin;
  GtkWidget       *label;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->scale_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->scale_dialog));
      return;
    }

  data = g_new (ScaleDialogData, 1);
  data->shell = shell;

  shell->scale_dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (shell->gdisp->gimage),
                              _("Zoom Ratio"), "display_scale",
                              GTK_STOCK_ZOOM_100,
                              _("Select Zoom Ratio"),
                              GTK_WIDGET (shell),
                              gimp_standard_help_func,
                              GIMP_HELP_VIEW_ZOOM_OTHER,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) g_free, data);
  g_object_add_weak_pointer (G_OBJECT (shell->scale_dialog),
                             (gpointer *) &shell->scale_dialog);

  gtk_window_set_transient_for (GTK_WINDOW (shell->scale_dialog),
                                GTK_WINDOW (shell));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (shell->scale_dialog), TRUE);

  g_signal_connect (shell->scale_dialog, "response",
                    G_CALLBACK (gimp_display_shell_scale_dialog_response),
                    data);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell->scale_dialog)->vbox),
                     hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Zoom Ratio:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if ((shell->other_scale & 0xFFFF) == 0)
    shell->other_scale = shell->scale;

  spin = gimp_spin_button_new (&data->dest_adj,
                               (shell->other_scale & 0xFF00) >> 8, 1, 0xFF,
                               1, 8, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (":");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spin = gimp_spin_button_new (&data->src_adj,
                               (shell->other_scale & 0xFF), 1, 0xFF,
                               1, 8, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  gtk_widget_show (shell->scale_dialog);
}


/*  private functions  */

static void
gimp_display_shell_scale_dialog_response (GtkWidget       *widget,
                                          gint             response_id,
                                          ScaleDialogData *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gint scale_src;
      gint scale_dest;

      scale_src  = gtk_adjustment_get_value (GTK_ADJUSTMENT (dialog->src_adj));
      scale_dest = gtk_adjustment_get_value (GTK_ADJUSTMENT (dialog->dest_adj));

      gimp_display_shell_scale (dialog->shell, scale_dest * 100 + scale_src);
    }
  else
    {
      /*  need to emit "scaled" to get the menu updated  */
      gimp_display_shell_scaled (dialog->shell);
    }

  dialog->shell->other_scale |= (1 << 30);

  gtk_widget_destroy (dialog->shell->scale_dialog);
}


/* scale image coord to realworld units (cm, inches, pixels)
 *
 * 27/Feb/1999 I tried inlining this, but the result was slightly
 * slower (poorer cache locality, probably) -- austin
 */
static gdouble
img2real (GimpDisplayShell *shell,
	  gboolean          xdir,
	  gdouble           len)
{
  GimpImage *gimage;
  gdouble    res;

  if (shell->dot_for_dot)
    return len;

  gimage = shell->gdisp->gimage;

  if (xdir)
    res = gimage->xresolution;
  else
    res = gimage->yresolution;

  return len * gimp_image_unit_get_factor (gimage) / res;
}
