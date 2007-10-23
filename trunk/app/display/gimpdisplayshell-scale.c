/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpmath/gimpmath.h"
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


#define SCALE_TIMEOUT 1

#define SCALE_EPSILON 0.0001

#define SCALE_EQUALS(a,b) (fabs ((a) - (b)) < SCALE_EPSILON)


typedef struct
{
  GimpDisplayShell *shell;
  GimpZoomModel    *model;
  GtkObject        *scale_adj;
  GtkObject        *num_adj;
  GtkObject        *denom_adj;
} ScaleDialogData;


/*  local function prototypes  */

static void gimp_display_shell_scale_dialog_response (GtkWidget        *widget,
                                                      gint              response_id,
                                                      ScaleDialogData  *dialog);
static void gimp_display_shell_scale_dialog_free     (ScaleDialogData  *dialog);

static void    update_zoom_values                    (GtkAdjustment    *adj,
                                                      ScaleDialogData  *dialog);
static gdouble img2real                              (GimpDisplayShell *shell,
                                                      gboolean          xdir,
                                                      gdouble           a);


/*  public functions  */

/**
 * gimp_display_shell_scale_setup:
 * @shell:        the #GimpDisplayShell
 *
 * Prepares the display for drawing the image at current scale and offset.
 * This preparation involves, for example, setting up scrollbars and rulers.
 **/
void
gimp_display_shell_scale_setup (GimpDisplayShell *shell)
{
  GtkRuler *hruler;
  GtkRuler *vruler;
  gfloat    sx, sy;
  gint      image_width, image_height;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display)
    return;

  image_width  = shell->display->image->width;
  image_height = shell->display->image->height;

  sx = SCALEX (shell, image_width);
  sy = SCALEY (shell, image_height);

  shell->hsbdata->value          = shell->offset_x;
  shell->hsbdata->upper          = sx;
  shell->hsbdata->page_size      = MIN (sx, shell->disp_width);
  shell->hsbdata->page_increment = shell->disp_width / 2;
  shell->hsbdata->step_increment = shell->scale_x;

  shell->vsbdata->value          = shell->offset_y;
  shell->vsbdata->upper          = sy;
  shell->vsbdata->page_size      = MIN (sy, shell->disp_height);
  shell->vsbdata->page_increment = shell->disp_height / 2;
  shell->vsbdata->step_increment = shell->scale_y;

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
  g_printerr ("offset_x:     %d\n"
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

/**
 * gimp_display_shell_scale_revert:
 * @shell:     the #GimpDisplayShell
 *
 * Reverts the display to the previously used scale. If no previous scale exist
 * then the call does nothing.
 *
 * Return value: %TRUE if the scale was reverted, otherwise %FALSE.
 **/
gboolean
gimp_display_shell_scale_revert (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  /* don't bother if no scale has been set */
  if (shell->last_scale < SCALE_EPSILON)
    return FALSE;

  shell->last_scale_time = 0;

  gimp_display_shell_scale_by_values (shell,
                                      shell->last_scale,
                                      shell->last_offset_x,
                                      shell->last_offset_y,
                                      FALSE);   /* don't resize the window */

  return TRUE;
}

/**
 * gimp_display_shell_scale_can_revert:
 * @shell:     the #GimpDisplayShell
 *
 * Return value: %TRUE if a previous display scale exists, otherwise %FALSE.
 **/
gboolean
gimp_display_shell_scale_can_revert (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);

  return (shell->last_scale > SCALE_EPSILON);
}

/**
 * gimp_display_shell_scale_set_dot_for_dot:
 * @shell:        the #GimpDisplayShell
 * @dot_for_dot:  whether "Dot for Dot" should be enabled
 *
 * If @dot_for_dot is set to %TRUE then the "Dot for Dot" mode (where image and
 * screen pixels are of the same size) is activated. Dually, the mode is
 * disabled if @dot_for_dot is %FALSE.
 **/
void
gimp_display_shell_scale_set_dot_for_dot (GimpDisplayShell *shell,
                                          gboolean          dot_for_dot)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (dot_for_dot != shell->dot_for_dot)
    {
      Gimp *gimp = shell->display->image->gimp;

      /* freeze the active tool */
      gimp_display_shell_pause (shell);

      shell->dot_for_dot = dot_for_dot;

      gimp_display_shell_scale_changed (shell);

      gimp_display_shell_scale_resize (shell,
                                       GIMP_DISPLAY_CONFIG (gimp->config)->resize_windows_on_zoom,
                                       TRUE);

      /* re-enable the active tool */
      gimp_display_shell_resume (shell);
    }
}

/**
 * gimp_display_shell_scale:
 * @shell:     the #GimpDisplayShell
 * @zoom_type: whether to zoom in, our or to a specific scale
 * @scale:     ignored unless @zoom_type == %GIMP_ZOOM_TO
 *
 * This function calls gimp_display_shell_scale_to(). It tries to be
 * smart whether to use the position of the mouse pointer or the
 * center of the display as coordinates.
 **/
void
gimp_display_shell_scale (GimpDisplayShell *shell,
                          GimpZoomType      zoom_type,
                          gdouble           new_scale)
{
  GdkEvent *event;
  gint      x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->canvas != NULL);

  if (zoom_type == GIMP_ZOOM_TO &&
      SCALE_EQUALS (new_scale, gimp_zoom_model_get_factor (shell->zoom)))
    return;

  x = shell->disp_width  / 2;
  y = shell->disp_height / 2;

  /*  Center on the mouse position instead of the display center,
   *  if one of the following conditions is fulfilled:
   *
   *   (1) there's no current event (the action was triggered by an
   *       input controller)
   *   (2) the event originates from the canvas (a scroll event)
   *   (3) the event originates from the shell (a key press event)
   *
   *  Basically the only situation where we don't want to center on
   *  mouse position is if the action is being called from a menu.
   */

  event = gtk_get_current_event ();

  if (! event ||
      gtk_get_event_widget (event) == shell->canvas ||
      gtk_get_event_widget (event) == GTK_WIDGET (shell))
    {
      gtk_widget_get_pointer (shell->canvas, &x, &y);
    }

  gimp_display_shell_scale_to (shell, zoom_type, new_scale, x, y);
}

/**
 * gimp_display_shell_scale_to:
 * @shell:     the #GimpDisplayShell
 * @zoom_type: whether to zoom in, out or to a specified scale
 * @scale:     ignored unless @zoom_type == %GIMP_ZOOM_TO
 * @x:         x screen coordinate
 * @y:         y screen coordinate
 *
 * This function changes the scale (zoom ratio) of the display shell.
 * It either zooms in / out one step (%GIMP_ZOOM_IN / %GIMP_ZOOM_OUT)
 * or sets the scale to the zoom ratio passed as @scale (%GIMP_ZOOM_TO).
 *
 * The display offsets are adjusted so that the point specified by @x
 * and @y doesn't change it's position on screen (if possible). You
 * would typically pass either the display center or the mouse
 * position here.
 **/
void
gimp_display_shell_scale_to (GimpDisplayShell *shell,
                             GimpZoomType      zoom_type,
                             gdouble           scale,
                             gdouble           x,
                             gdouble           y)
{
  GimpDisplayConfig *config;
  gdouble            current;
  gdouble            offset_x;
  gdouble            offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->display)
    return;

  current = gimp_zoom_model_get_factor (shell->zoom);

  offset_x = shell->offset_x + x;
  offset_y = shell->offset_y + y;

  offset_x /= current;
  offset_y /= current;

  if (zoom_type != GIMP_ZOOM_TO)
    scale = gimp_zoom_model_zoom_step (zoom_type, current);

  offset_x *= scale;
  offset_y *= scale;

  config = GIMP_DISPLAY_CONFIG (shell->display->image->gimp->config);

  gimp_display_shell_scale_by_values (shell, scale,
                                      offset_x - x, offset_y - y,
                                      config->resize_windows_on_zoom);
}

/**
 * gimp_display_shell_scale_fit_in:
 * @shell:     the #GimpDisplayShell
 *
 * Sets the scale such that the entire image precisely fits in the display
 * area.
 **/
void
gimp_display_shell_scale_fit_in (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;
  gdouble    zoom_factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  image_width  = image->width;
  image_height = image->height;

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width *
                            shell->monitor_xres / image->xresolution);
      image_height = ROUND (image_height *
                            shell->monitor_yres / image->yresolution);
    }

  zoom_factor = MIN ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale (shell, GIMP_ZOOM_TO, zoom_factor);
}

/**
 * gimp_display_shell_scale_fit_to:
 * @shell:     the #GimpDisplayShell
 *
 * Sets the scale such that the entire display area is precisely filled by the
 * image.
 **/
void
gimp_display_shell_scale_fit_to (GimpDisplayShell *shell)
{
  GimpImage *image;
  gint       image_width;
  gint       image_height;
  gdouble    zoom_factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  image_width  = image->width;
  image_height = image->height;

  if (! shell->dot_for_dot)
    {
      image_width  = ROUND (image_width *
                            shell->monitor_xres / image->xresolution);
      image_height = ROUND (image_height *
                            shell->monitor_yres / image->yresolution);
    }

  zoom_factor = MAX ((gdouble) shell->disp_width  / (gdouble) image_width,
                     (gdouble) shell->disp_height / (gdouble) image_height);

  gimp_display_shell_scale (shell, GIMP_ZOOM_TO, zoom_factor);
}

/**
 * gimp_display_shell_scale_by_values:
 * @shell:          the #GimpDisplayShell
 * @scale:          the new scale
 * @offset_x:       the new X offset
 * @offset_y:       the new Y offset
 * @resize_window:  whether the display window should be resized
 *
 * Directly sets the image scale and image offsets used by the display. If
 * @resize_window is %TRUE then the display window is resized to better
 * accomodate the image, see gimp_display_shell_shrink_wrap().
 **/
void
gimp_display_shell_scale_by_values (GimpDisplayShell *shell,
                                    gdouble           scale,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gboolean          resize_window)
{
  guint now;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /*  Abort early if the values are all setup already. We don't
   *  want to inadvertently resize the window (bug #164281).
   */
  if (SCALE_EQUALS (gimp_zoom_model_get_factor (shell->zoom), scale) &&
      shell->offset_x == offset_x &&
      shell->offset_y == offset_y)
    return;

  /* remember the current scale and offsets to allow reverting the scaling */

  now = time (NULL);

  if (now - shell->last_scale_time > SCALE_TIMEOUT)
    {
      shell->last_scale    = gimp_zoom_model_get_factor (shell->zoom);
      shell->last_offset_x = shell->offset_x;
      shell->last_offset_y = shell->offset_y;
    }

  shell->last_scale_time = now;

  /* freeze the active tool */
  gimp_display_shell_pause (shell);

  gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

  shell->offset_x = offset_x;
  shell->offset_y = offset_y;

  gimp_display_shell_scale_resize (shell, resize_window, TRUE);

  /* re-enable the active tool */
  gimp_display_shell_resume (shell);
}

/**
 * gimp_display_shell_scale_shrink_wrap:
 * @shell:          the #GimpDisplayShell
 *
 * Convenience function with the same functionality as
 * gimp_display_shell_scale_resize(@shell, TRUE, TRUE).
 **/
void
gimp_display_shell_scale_shrink_wrap (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_scale_resize (shell, TRUE, TRUE);
}

/**
 * gimp_display_shell_scale_resize:
 * @shell:          the #GimpDisplayShell
 * @resize_window:  whether the display window should be resized
 * @redisplay:      whether the display window should be redrawn
 *
 * Function commonly called after a change in display scale to make the changes
 * visible to the user. If @resize_window is %TRUE then the display window is
 * resized to accomodate the display image as per
 * gimp_display_shell_shrink_wrap().
 **/
void
gimp_display_shell_scale_resize (GimpDisplayShell *shell,
                                 gboolean          resize_window,
                                 gboolean          redisplay)
{
  Gimp *gimp;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp = shell->display->image->gimp;

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

/**
 * gimp_display_shell_scale_dialog:
 * @shell:          the #GimpDisplayShell
 *
 * Constructs and displays a dialog allowing the user to enter a custom display
 * scale.
 **/
void
gimp_display_shell_scale_dialog (GimpDisplayShell *shell)
{
  ScaleDialogData *data;
  GimpImage       *image;
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

  if (SCALE_EQUALS (shell->other_scale, 0.0))
    {
      /* other_scale not yet initialized */
      shell->other_scale = gimp_zoom_model_get_factor (shell->zoom);
    }

  image = shell->display->image;

  data = g_slice_new (ScaleDialogData);

  data->shell = shell;
  data->model = g_object_new (GIMP_TYPE_ZOOM_MODEL,
                              "value", fabs (shell->other_scale),
                              NULL);

  shell->scale_dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (image),
                              gimp_get_user_context (image->gimp),
                              _("Zoom Ratio"), "display_scale",
                              GTK_STOCK_ZOOM_100,
                              _("Select Zoom Ratio"),
                              GTK_WIDGET (shell),
                              gimp_standard_help_func,
                              GIMP_HELP_VIEW_ZOOM_OTHER,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (shell->scale_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) gimp_display_shell_scale_dialog_free, data);
  g_object_weak_ref (G_OBJECT (shell->scale_dialog),
                     (GWeakNotify) g_object_unref, data->model);

  g_object_add_weak_pointer (G_OBJECT (shell->scale_dialog),
                             (gpointer) &shell->scale_dialog);

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
                             _("Zoom ratio:"), 0.0, 0.5,
                             hbox, 1, FALSE);

  gimp_zoom_model_get_fraction (data->model, &num, &denom);

  spin = gimp_spin_button_new (&data->num_adj,
                               num, 1, 256,
                               1, 8, 1, 1, 0);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  label = gtk_label_new (":");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spin = gimp_spin_button_new (&data->denom_adj,
                               denom, 1, 256,
                               1, 8, 1, 1, 0);
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
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
  gtk_entry_set_activates_default (GTK_ENTRY (spin), TRUE);
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
gimp_display_shell_scale_dialog_free (ScaleDialogData *dialog)
{
  g_slice_free (ScaleDialogData, dialog);
}

static void
update_zoom_values (GtkAdjustment   *adj,
                    ScaleDialogData *dialog)
{
  gint    num, denom;
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

      gimp_zoom_model_zoom (dialog->model, GIMP_ZOOM_TO, scale / 100.0);
      gimp_zoom_model_get_fraction (dialog->model, &num, &denom);

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
  GimpImage *image = shell->display->image;
  gdouble    res;

  if (shell->unit == GIMP_UNIT_PIXEL)
    return len;

  if (xdir)
    res = image->xresolution;
  else
    res = image->yresolution;

  return len * _gimp_unit_get_factor (image->gimp, shell->unit) / res;
}
