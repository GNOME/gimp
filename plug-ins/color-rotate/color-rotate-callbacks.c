/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 *
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

/*---------------------------------------------------------------------------
 * Change log:
 *
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *---------------------------------------------------------------------------*/

#include "config.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "color-rotate.h"
#include "color-rotate-utils.h"
#include "color-rotate-draw.h"
#include "color-rotate-dialog.h"
#include "color-rotate-callbacks.h"
#include "color-rotate-stock.h"

#include "images/color-rotate-stock-pixbufs.h"


/* Misc functions */

float
rcm_units_factor (gint units)
{
  switch (units)
  {
    case DEGREES:         return 180.0 / G_PI;
    case RADIANS:         return 1.0;
    case RADIANS_OVER_PI: return 1.0 / G_PI;
    default:              return -1;
  }
}

const gchar *
rcm_units_string (gint units)
{
  switch (units)
  {
    case DEGREES:         return "deg";
    case RADIANS:         return "rad";
    case RADIANS_OVER_PI: return "rad/pi";
    default:              return "(unknown)";
  }
}


/* Circle buttons */

void
rcm_360_degrees (GtkWidget *button,
		 RcmCircle *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  circle->action_flag = DO_NOTHING;
  gtk_widget_queue_draw (circle->preview);
  circle->angle->beta = circle->angle->alpha-circle->angle->cw_ccw * 0.001;
  color_rotate_draw_arrows (circle->preview->window,
                            style->black_gc,
                            circle->angle);
  circle->action_flag = VIRGIN;
  rcm_render_preview (Current.Bna->after);
}

void
rcm_cw_ccw (GtkWidget *button,
	    RcmCircle *circle)
{
  circle->angle->cw_ccw *= -1;

  g_object_set (button,
                "label",
                (circle->angle->cw_ccw>0) ?
                STOCK_COLOR_ROTATE_SWITCH_CLOCKWISE :
                STOCK_COLOR_ROTATE_SWITCH_COUNTERCLOCKWISE,
                "use_stock", TRUE,
                NULL);

  rcm_a_to_b (button, circle);
}

void
rcm_a_to_b (GtkWidget *button,
	    RcmCircle *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  circle->action_flag = DO_NOTHING;
  gtk_widget_queue_draw (circle->preview);

  SWAP (circle->angle->alpha, circle->angle->beta);

  color_rotate_draw_arrows (circle->preview->window,
                            style->black_gc,
                            circle->angle);

  circle->action_flag = VIRGIN;
  rcm_render_preview (Current.Bna->after);
}


/* Misc: units buttons */

static void
rcm_spinbutton_to_degrees (GtkWidget *button,
			   float      value,
			   GtkWidget *label)
{
  GtkAdjustment *adj;

  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (button));
  adj->value = value * rcm_units_factor (Current.Units);
  adj->upper = 360.0;
  adj->step_increment = 0.01;
  adj->page_increment = 1.0;
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (button), 2);

  gtk_label_set_text (GTK_LABEL (label), rcm_units_string (Current.Units));
}

void
rcm_switch_to_degrees (GtkWidget *button,
		       gpointer  *value)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      Current.Units = DEGREES;

      rcm_spinbutton_to_degrees (Current.From->alpha_entry,
                                 Current.From->angle->alpha,
                                 Current.From->alpha_units_label);

      rcm_spinbutton_to_degrees (Current.From->beta_entry,
                                 Current.From->angle->beta,
                                 Current.From->beta_units_label);

      rcm_spinbutton_to_degrees (Current.To->alpha_entry,
                                 Current.To->angle->alpha,
                                 Current.To->alpha_units_label);

      rcm_spinbutton_to_degrees (Current.To->beta_entry,
                                 Current.To->angle->beta,
                                 Current.To->beta_units_label);

      rcm_spinbutton_to_degrees (Current.Gray->hue_entry,
                                 Current.Gray->hue,
                                 Current.Gray->hue_units_label);

      Current.From->action_flag = VIRGIN;
      Current.To->action_flag   = VIRGIN;
      Current.Gray->action_flag = VIRGIN;
    }
}

static void
rcm_spinbutton_to_radians (GtkWidget *button,
			   float      value,
			   GtkWidget *label)
{
  GtkAdjustment *adj;

  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (button));
  adj->value = value * rcm_units_factor (Current.Units);
  adj->upper = TP;
  adj->step_increment = 0.0001;
  adj->page_increment = 0.001;
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (button), 4);

  gtk_label_set_text (GTK_LABEL (label), rcm_units_string (Current.Units));
}

void
rcm_switch_to_radians (GtkWidget *button,
		       gpointer  *value)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      Current.Units = RADIANS;

      rcm_spinbutton_to_radians (Current.From->alpha_entry,
                                 Current.From->angle->alpha,
                                 Current.From->alpha_units_label);

      rcm_spinbutton_to_radians (Current.From->beta_entry,
                                 Current.From->angle->beta,
                                 Current.From->beta_units_label);

      rcm_spinbutton_to_radians (Current.To->alpha_entry,
                                 Current.To->angle->alpha,
                                 Current.To->alpha_units_label);

      rcm_spinbutton_to_radians (Current.To->beta_entry,
                                 Current.To->angle->beta,
                                 Current.To->beta_units_label);

      rcm_spinbutton_to_radians (Current.Gray->hue_entry,
                                 Current.Gray->hue,
                                 Current.Gray->hue_units_label);

      Current.From->action_flag = VIRGIN;
      Current.To->action_flag   = VIRGIN;
      Current.Gray->action_flag = VIRGIN;
    }
}

static void
rcm_spinbutton_to_radians_over_PI (GtkWidget *button,
				   float      value,
				   GtkWidget *label)
{
  GtkAdjustment *adj;

  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (button));
  adj->value = value * rcm_units_factor (Current.Units);
  adj->upper = 2.0;
  adj->step_increment = 0.0001;
  adj->page_increment = 0.001;
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (button), 4);

  gtk_label_set_text (GTK_LABEL (label), rcm_units_string (Current.Units));
}

void
rcm_switch_to_radians_over_PI (GtkWidget *button,
			       gpointer  *value)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      Current.Units = RADIANS_OVER_PI;

      rcm_spinbutton_to_radians_over_PI (Current.From->alpha_entry,
                                         Current.From->angle->alpha,
                                         Current.From->alpha_units_label);

      rcm_spinbutton_to_radians_over_PI (Current.From->beta_entry,
                                         Current.From->angle->beta,
                                         Current.From->beta_units_label);

      rcm_spinbutton_to_radians_over_PI (Current.To->alpha_entry,
                                         Current.To->angle->alpha,
                                         Current.To->alpha_units_label);

      rcm_spinbutton_to_radians_over_PI (Current.To->beta_entry,
                                         Current.To->angle->beta,
                                         Current.To->beta_units_label);

      rcm_spinbutton_to_radians_over_PI (Current.Gray->hue_entry,
                                         Current.Gray->hue,
                                         Current.Gray->hue_units_label);

      Current.From->action_flag = VIRGIN;
      Current.To->action_flag   = VIRGIN;
      Current.Gray->action_flag = VIRGIN;
    }
}


/* Misc: Gray: mode buttons */

void
rcm_switch_to_gray_to (GtkWidget *button,
		       gpointer  *value)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      Current.Gray_to_from = GRAY_TO;

      rcm_render_preview (Current.Bna->after);
    }
}

void
rcm_switch_to_gray_from (GtkWidget *button,
			 gpointer  *value)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      Current.Gray_to_from = GRAY_FROM;

      rcm_render_preview (Current.Bna->after);
    }
}


/* Misc: Preview buttons */

void
rcm_preview_as_you_drag (GtkWidget *button,
			 gpointer  *value)
{
  Current.RealTime = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
}

void
rcm_combo_callback (GtkWidget *widget,
                    gpointer   data)
{
  gint value;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);

  Current.reduced = rcm_reduce_image (Current.drawable, Current.mask,
                                      MAX_PREVIEW_SIZE, value);

  gtk_widget_set_size_request (Current.Bna->before,
                               Current.reduced->width,
                               Current.reduced->height);
  gtk_widget_set_size_request (Current.Bna->after,
                               Current.reduced->width,
                               Current.reduced->height);
}


/* Circle events */

gboolean
rcm_expose_event (GtkWidget      *widget,
		  GdkEventExpose *event,
		  RcmCircle      *circle)
{
  if (circle->action_flag == VIRGIN)
    {
      GtkStyle *style = gtk_widget_get_style (widget);

      color_rotate_draw_arrows (widget->window,
                                style->black_gc,
                                circle->angle);
    }

  return TRUE;
}

gboolean
rcm_button_press_event (GtkWidget      *widget,
			GdkEventButton *event,
			RcmCircle      *circle)
{
  float  clicked_angle;
  float *alpha;
  float *beta;

  alpha  = &circle->angle->alpha;
  beta   = &circle->angle->beta;

  circle->action_flag = DRAG_START;
  clicked_angle = angle_mod_2PI (arctg (CENTER - event->y, event->x - CENTER));
  circle->prev_clicked = clicked_angle;

  if ((sqrt (SQR (event->y - CENTER) +
             SQR (event->x - CENTER)) > RADIUS * EACH_OR_BOTH) &&
      (min_prox (*alpha, *beta, clicked_angle) < G_PI / 12))
    {
      circle->mode = EACH;
      circle->target = closest (alpha, beta, clicked_angle);

      if (*(circle->target) != clicked_angle)
        {
          GtkStyle *style = gtk_widget_get_style (widget);

          *(circle->target) = clicked_angle;
          gtk_widget_queue_draw (circle->preview);
          color_rotate_draw_arrows (widget->window,
                                    style->black_gc,
                                    circle->angle);

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->alpha_entry),
                                     circle->angle->alpha *
                                     rcm_units_factor(Current.Units));

          gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->beta_entry),
                                     circle->angle->beta *
                                     rcm_units_factor(Current.Units));

          if (Current.RealTime)
            rcm_render_preview (Current.Bna->after);
        }
    }
  else
    circle->mode = BOTH;

  return TRUE;
}

gboolean
rcm_release_event (GtkWidget      *widget,
		   GdkEventButton *event,
		   RcmCircle      *circle)
{
  if (circle->action_flag == DRAGING)
    {
      GtkStyle *style = gtk_widget_get_style (widget);

      gtk_widget_queue_draw (circle->preview);
      color_rotate_draw_arrows (widget->window,
                                style->black_gc,
                                circle->angle);
    }

  circle->action_flag = VIRGIN;

  rcm_render_preview (Current.Bna->after);

  return TRUE;
}

gboolean
rcm_motion_notify_event (GtkWidget      *widget,
			 GdkEventMotion *event,
			 RcmCircle      *circle)
{
  gfloat       clicked_angle, delta;
  gfloat      *alpha, *beta;
  gint         cw_ccw;
  GdkGCValues  values;

  alpha  = &(circle->angle->alpha);
  beta   = &(circle->angle->beta);
  cw_ccw = circle->angle->cw_ccw;
  delta  = angle_mod_2PI (cw_ccw * (*beta - *alpha));

  values.function = GDK_INVERT;
  xor_gc = gdk_gc_new_with_values (Current.From->preview->window,
                                   &values,
                                   GDK_GC_FUNCTION);

  clicked_angle = angle_mod_2PI (arctg (CENTER - event->y, event->x - CENTER));

  delta = clicked_angle - circle->prev_clicked;
  circle->prev_clicked = clicked_angle;

  if (delta)
    {
      if (circle->action_flag == DRAG_START)
        {
          gtk_widget_queue_draw (circle->preview);
          circle->action_flag = DRAGING;
        }
      else
        {
          /* this should be erasing entire angle */
          color_rotate_draw_arrows (widget->window, xor_gc, circle->angle);
        }

      if (circle->mode == EACH)
        {
          *(circle->target)=clicked_angle;
        }
      else
        {
          circle->angle->alpha=angle_mod_2PI(circle->angle->alpha + delta);
          circle->angle->beta =angle_mod_2PI(circle->angle->beta  + delta);
        }

      gdk_window_process_updates (widget->window, FALSE);
      color_rotate_draw_arrows (widget->window, xor_gc, circle->angle);

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->alpha_entry),
                                 circle->angle->alpha *
                                 rcm_units_factor(Current.Units));

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->beta_entry),
                                 circle->angle->beta *
                                 rcm_units_factor(Current.Units));

      if (Current.RealTime)
        rcm_render_preview (Current.Bna->after);
    }

  gdk_event_request_motions (event);

  return TRUE;
}


/* Gray circle events */

gboolean
rcm_gray_expose_event (GtkWidget      *widget,
		       GdkEventExpose *event,
		       RcmGray        *circle)
{
  if (circle->action_flag == VIRGIN)
    {
      GtkStyle *style = gtk_widget_get_style (widget);

      color_rotate_draw_little_circle (widget->window,
                                       style->black_gc,
                                       circle->hue, circle->satur);

      color_rotate_draw_large_circle (widget->window,
                                      style->black_gc,
                                      circle->gray_sat);
    }

  return TRUE;
}

gboolean
rcm_gray_button_press_event (GtkWidget      *widget,
			     GdkEventButton *event,
			     RcmGray        *circle)
{
  GtkStyle *style = gtk_widget_get_style (widget);
  int       x, y;

  x = event->x - GRAY_CENTER - LITTLE_RADIUS;
  y = GRAY_CENTER - event->y + LITTLE_RADIUS;

  circle->action_flag = DRAG_START;
  circle->hue         = angle_mod_2PI(arctg(y, x));
  circle->satur       = sqrt (SQR (x) + SQR (y)) / GRAY_RADIUS;

  if (circle->satur > 1.0)
    circle->satur = 1;

  gtk_widget_queue_draw (circle->preview);
  color_rotate_draw_little_circle (widget->window,
                                   style->black_gc,
                                   circle->hue, circle->satur);

  color_rotate_draw_large_circle (circle->preview->window,
                                  gtk_widget_get_style (circle->preview)->black_gc,
                                  circle->gray_sat);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->hue_entry),
                             circle->hue * rcm_units_factor (Current.Units));

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->satur_entry),
                             circle->satur);

  if (Current.RealTime)
    rcm_render_preview (Current.Bna->after);

  return TRUE;
}

gboolean
rcm_gray_release_event (GtkWidget      *widget,
			GdkEventButton *event,
			RcmGray        *circle)
{
  if (circle->action_flag == DRAGING)
    {
      GtkStyle *style = gtk_widget_get_style (widget);

      color_rotate_draw_little_circle (widget->window,
                                       style->black_gc,
                                       circle->hue,
                                       circle->satur);
    }

  circle->action_flag = VIRGIN;

  rcm_render_preview (Current.Bna->after);

  return TRUE;
}

gboolean
rcm_gray_motion_notify_event (GtkWidget      *widget,
			      GdkEventMotion *event,
			      RcmGray        *circle)
{
  gint        x, y;
  GdkGCValues values;

  values.function = GDK_INVERT;
  xor_gc = gdk_gc_new_with_values (Current.From->preview->window,
				   &values,
                                   GDK_GC_FUNCTION);

  if (circle->action_flag == DRAG_START)
    {
      GtkStyle *style = gtk_widget_get_style (circle->preview);

      gtk_widget_queue_draw (circle->preview);
      color_rotate_draw_large_circle (circle->preview->window,
                                      style->black_gc,
                                      circle->gray_sat);

      circle->action_flag = DRAGING;
    }
  else
    {
      color_rotate_draw_little_circle (widget->window, xor_gc,
                                       circle->hue, circle->satur); /* erase */
    }

  x = event->x - GRAY_CENTER - LITTLE_RADIUS;
  y = GRAY_CENTER - event->y + LITTLE_RADIUS;

  circle->hue   = angle_mod_2PI (arctg (y, x));
  circle->satur = sqrt (SQR (x) + SQR (y)) / GRAY_RADIUS;

  if (circle->satur > 1.0)
    circle->satur = 1;

  color_rotate_draw_little_circle (widget->window, xor_gc,
                                   circle->hue, circle->satur);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->hue_entry),
                             circle->hue * rcm_units_factor(Current.Units));

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (circle->satur_entry),
                             circle->satur);

  if (Current.RealTime)
    rcm_render_preview (Current.Bna->after);

  gdk_event_request_motions (event);

  return TRUE;
}


/* Spinbuttons */

void
rcm_set_alpha (GtkWidget *entry,
               RcmCircle *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  if (circle->action_flag != VIRGIN)
    return;

  circle->angle->alpha = (gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry)) /
                          rcm_units_factor (Current.Units));

  gtk_widget_queue_draw (circle->preview);

  color_rotate_draw_arrows (circle->preview->window,
                            style->black_gc,
                            circle->angle);

  rcm_render_preview (Current.Bna->after);
}

void
rcm_set_beta (GtkWidget *entry,
              RcmCircle *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  if (circle->action_flag != VIRGIN)
    return;

  circle->angle->beta = (gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry)) /
                         rcm_units_factor(Current.Units));

  gtk_widget_queue_draw (circle->preview);

  color_rotate_draw_arrows (circle->preview->window,
                            style->black_gc,
                            circle->angle);

  rcm_render_preview (Current.Bna->after);
}

void
rcm_set_hue (GtkWidget *entry,
             RcmGray   *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  if (circle->action_flag != VIRGIN)
    return;

  circle->hue = (gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry)) /
                 rcm_units_factor(Current.Units));

  gtk_widget_queue_draw (circle->preview);

  color_rotate_draw_little_circle (circle->preview->window,
                                   style->black_gc,
                                   circle->hue, circle->satur);

  color_rotate_draw_large_circle (circle->preview->window,
                                  style->black_gc,
                                  circle->gray_sat);

  rcm_render_preview (Current.Bna->after);
}

void
rcm_set_satur (GtkWidget *entry,
               RcmGray   *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  if (circle->action_flag != VIRGIN)
    return;

  circle->satur = gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry));

  gtk_widget_queue_draw (circle->preview);

  color_rotate_draw_little_circle (circle->preview->window,
                                   style->black_gc,
                                   circle->hue, circle->satur);

  color_rotate_draw_large_circle (circle->preview->window,
                                  style->black_gc,
                                  circle->gray_sat);

  rcm_render_preview (Current.Bna->after);
}

void
rcm_set_gray_sat (GtkWidget *entry,
                  RcmGray   *circle)
{
  GtkStyle *style = gtk_widget_get_style (circle->preview);

  circle->gray_sat = gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry));

  gtk_widget_queue_draw (circle->preview);

  color_rotate_draw_large_circle (circle->preview->window,
                                  style->black_gc,
                                  circle->gray_sat);

  rcm_render_preview (Current.Bna->after);
}
