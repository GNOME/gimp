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
#include "core/gimpunit.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-title.h"

#include "gimp-intl.h"


typedef struct _ScaleDialogData ScaleDialogData;

struct _ScaleDialogData
{
  GimpDisplayShell *shell;
  GtkObject        *scale_adj;
  GtkObject        *num_adj;
  GtkObject        *denom_adj;
};


/*  local function prototypes  */

static void gimp_display_shell_scale_dialog_response (GtkWidget        *widget,
                                                      gint              response_id,
                                                      ScaleDialogData  *dialog);
static void update_zoom_values                       (GtkAdjustment    *adj,
                                                      ScaleDialogData  *dialog);
static gdouble img2real                              (GimpDisplayShell *shell,
                                                      gboolean          xdir,
                                                      gdouble           a);


/*  public functions  */

gdouble
gimp_display_shell_scale_zoom_step (GimpZoomType zoom_type,
                                    gdouble      scale)
{
  gint    i, n_presets;
  gdouble new_scale = 1.0;

  /* This table is constructed to have fractions, that approximate
   * sqrt(2)^k. This gives a smooth feeling regardless of the starting
   * zoom level.
   *
   * Zooming in/out always jumps to a zoom step from the list above.
   * However, we try to guarantee a certain size of the step, to
   * avoid silly jumps from 101% to 100%.
   * The factor 1.1 is chosen a bit arbitrary, but feels better
   * than the geometric median of the zoom steps (2^(1/4)).
   */

#define ZOOM_MIN_STEP 1.1

  gdouble presets[] = {
    1.0 / 256, 1.0 / 180, 1.0 / 128, 1.0 / 90,
    1.0 / 64,  1.0 / 45,  1.0 / 32,  1.0 / 23,
    1.0 / 16,  1.0 / 11,  1.0 / 8,   2.0 / 11,
    1.0 / 4,   1.0 / 3,   1.0 / 2,   2.0 / 3,
      1.0,
               3.0 / 2,      2.0,      3.0,
      4.0,    11.0 / 2,      8.0,     11.0,
      16.0,     23.0,       32.0,     45.0,
      64.0,     90.0,      128.0,    180.0,
      256.0,
  };

  n_presets = G_N_ELEMENTS (presets);

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      scale *= ZOOM_MIN_STEP;

      new_scale = presets[n_presets-1];

      for (i = n_presets - 1; i >= 0 && presets[i] > scale; i--)
        new_scale = presets[i];

      break;

    case GIMP_ZOOM_OUT:
      scale /= ZOOM_MIN_STEP;

      new_scale = presets[0];

      for (i = 0; i < n_presets && presets[i] < scale; i++)
        new_scale = presets[i];

      break;

    case GIMP_ZOOM_TO:
      new_scale = scale;
      break;
    }

  return CLAMP (new_scale, 1.0/256.0, 256.0);

#undef ZOOM_MIN_STEP
}

void
gimp_display_shell_scale_get_fraction (gdouble  zoom_factor,
                                       gint    *numerator,
                                       gint    *denominator)
{
  gint     p0, p1, p2;
  gint     q0, q1, q2;
  gdouble  remainder, next_cf;
  gboolean swapped = FALSE;

  g_return_if_fail (numerator != NULL && denominator != NULL);

  /* make sure that zooming behaves symmetrically */
  if (zoom_factor < 1.0)
    {
      zoom_factor = 1.0 / zoom_factor;
      swapped = TRUE;
    }

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

      /* Numerator and Denominator are limited by 256 */
      /* also absurd ratios like 170:171 are excluded */
      if (p2 > 256 || q2 > 256 || (p2 > 1 && q2 > 1 && p2 * q2 > 200))
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

  if (zoom_factor > 256.0)
    {
      p1 = 256;
      q1 = 1;
    }
  else if (zoom_factor < 1.0 / 256.0)
    {
      p1 = 1;
      q1 = 256;
    }

  if (swapped)
    {
      *numerator = q1;
      *denominator = p1;
    }
  else
    {
      *numerator = p1;
      *denominator = q1;
    }
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

  if (! shell->gdisp)
    return;

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

      gimp_display_shell_scale_resize (shell,
                                       GIMP_DISPLAY_CONFIG (gimp->config)->resize_windows_on_zoom,
                                       TRUE);

      /* re-enable the active tool */
      gimp_display_shell_resume (shell);
    }
}

void
gimp_display_shell_scale (GimpDisplayShell *shell,
                          GimpZoomType      zoom_type,
                          gdouble           new_scale)
{
  GimpDisplayConfig *config;
  gdouble            offset_x, offset_y;
  gdouble            scale;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  scale = shell->scale;

  if (! shell->gdisp)
    return;

  offset_x = shell->offset_x + (shell->disp_width  / 2.0);
  offset_y = shell->offset_y + (shell->disp_height / 2.0);

  offset_x /= scale;
  offset_y /= scale;

  if (zoom_type == GIMP_ZOOM_TO)
    scale = new_scale;
  else
    scale = gimp_display_shell_scale_zoom_step (zoom_type, scale);

  offset_x *= scale;
  offset_y *= scale;

  config = GIMP_DISPLAY_CONFIG (shell->gdisp->gimage->gimp->config);

  gimp_display_shell_scale_by_values (shell, scale,
                                      offset_x - (shell->disp_width  / 2),
                                      offset_y - (shell->disp_height / 2),
                                      config->resize_windows_on_zoom);
}

void
gimp_display_shell_scale_fit_in (GimpDisplayShell *shell)
{
  GimpImage *gimage;
  gint       image_width;
  gint       image_height;
  gdouble    zoom_factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  image_width  = gimage->width;
  image_height = gimage->height;

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width *
                            shell->monitor_xres / gimage->xresolution);
      image_height = ROUND (image_height *
                            shell->monitor_yres / gimage->yresolution);
    }

  zoom_factor = MIN ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale (shell, GIMP_ZOOM_TO, zoom_factor);
}

void
gimp_display_shell_scale_fit_to (GimpDisplayShell *shell)
{
  GimpImage *gimage;
  gint       image_width;
  gint       image_height;
  gdouble    zoom_factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  image_width  = gimage->width;
  image_height = gimage->height;

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width *
                            shell->monitor_xres / gimage->xresolution);
      image_height = ROUND (image_height *
                            shell->monitor_yres / gimage->yresolution);
    }

  zoom_factor = MAX ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale (shell, GIMP_ZOOM_TO, zoom_factor);
}

void
gimp_display_shell_scale_by_values (GimpDisplayShell *shell,
                                    gdouble           scale,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gboolean          resize_window)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /*  Abort early if the values are all setup already. We don't
   *  want to inadvertently resize the window (bug #164281).
   */
  if (shell->scale    == scale    &&
      shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

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
  GtkWidget       *table;
  GtkWidget       *spin;
  GtkWidget       *label;
  gint             num, denom, row;

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

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell->scale_dialog)->vbox),
                     table);
  gtk_widget_show (table);

  row = 0;

  hbox = gtk_hbox_new (FALSE, 6);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Zoom Ratio:"), 0.0, 0.5,
                             hbox, 1, FALSE);

  if (fabs (shell->other_scale) <= 0.0001)
    shell->other_scale = shell->scale;   /* other_scale not yet initialized */

  gimp_display_shell_scale_get_fraction (fabs (shell->other_scale),
                                         &num, &denom);

  spin = gimp_spin_button_new (&data->num_adj,
                               num, 1, 256,
                               1, 8, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (":");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spin = gimp_spin_button_new (&data->denom_adj,
                               denom, 1, 256,
                               1, 8, 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  hbox = gtk_hbox_new (FALSE, 6);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Zoom:"), 0.0, 0.5,
                             hbox, 1, FALSE);

  spin = gimp_spin_button_new (&data->scale_adj,
                               fabs (shell->other_scale) * 100,
                               100.0 / 256.0, 25600.0,
                               10, 50, 0, 1, 2);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new ("%");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_signal_connect (data->scale_adj, "value-changed",
                    G_CALLBACK (update_zoom_values), data);
  g_signal_connect (data->num_adj, "value-changed",
                    G_CALLBACK (update_zoom_values), data);
  g_signal_connect (data->denom_adj, "value-changed",
                    G_CALLBACK (update_zoom_values), data);

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
      gdouble scale;

      scale = gtk_adjustment_get_value (GTK_ADJUSTMENT (dialog->scale_adj));

      gimp_display_shell_scale (dialog->shell, GIMP_ZOOM_TO, scale / 100.0);
    }
  else
    {
      /*  need to emit "scaled" to get the menu updated  */
      gimp_display_shell_scaled (dialog->shell);
    }

  dialog->shell->other_scale = - fabs (dialog->shell->other_scale);

  gtk_widget_destroy (dialog->shell->scale_dialog);
}


static void
update_zoom_values (GtkAdjustment   *adj,
                    ScaleDialogData *dialog)
{
  gint num, denom;
  gdouble scale;

  g_signal_handlers_block_by_func (GTK_ADJUSTMENT (dialog->scale_adj),
                                   G_CALLBACK (update_zoom_values),
                                   dialog);

  g_signal_handlers_block_by_func (GTK_ADJUSTMENT (dialog->num_adj),
                                   G_CALLBACK (update_zoom_values),
                                   dialog);

  g_signal_handlers_block_by_func (GTK_ADJUSTMENT (dialog->denom_adj),
                                   G_CALLBACK (update_zoom_values),
                                   dialog);

  if (GTK_OBJECT (adj) == dialog->scale_adj)
    {
      scale = gtk_adjustment_get_value (GTK_ADJUSTMENT (dialog->scale_adj));

      gimp_display_shell_scale_get_fraction (scale / 100.0, &num, &denom);

      gtk_adjustment_set_value (GTK_ADJUSTMENT (dialog->num_adj), num);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (dialog->denom_adj), denom);
    }
  else   /* fraction adjustments */
    {
      scale = (gtk_adjustment_get_value (GTK_ADJUSTMENT (dialog->num_adj)) /
               gtk_adjustment_get_value (GTK_ADJUSTMENT (dialog->denom_adj)));
      gtk_adjustment_set_value (GTK_ADJUSTMENT (dialog->scale_adj),
                                scale * 100);
    }

  g_signal_handlers_unblock_by_func (GTK_ADJUSTMENT (dialog->scale_adj),
                                     G_CALLBACK (update_zoom_values),
                                     dialog);

  g_signal_handlers_unblock_by_func (GTK_ADJUSTMENT (dialog->num_adj),
                                     G_CALLBACK (update_zoom_values),
                                     dialog);

  g_signal_handlers_unblock_by_func (GTK_ADJUSTMENT (dialog->denom_adj),
                                     G_CALLBACK (update_zoom_values),
                                     dialog);
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
  GimpImage *image = shell->gdisp->gimage;
  gdouble    res;

  if (shell->unit == GIMP_UNIT_PIXEL)
    return len;

  if (xdir)
    res = image->xresolution;
  else
    res = image->yresolution;

  return len * _gimp_unit_get_factor (image->gimp, shell->unit) / res;
}
