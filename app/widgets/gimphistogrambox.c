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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/gimphistogram.h"

#include "gimpcolorbar.h"
#include "gimphistogrambox.h"
#include "gimphistogramview.h"

#include "gimp-intl.h"


/*  #define DEBUG_VIEW  */

#define GRADIENT_HEIGHT  12
#define CONTROL_HEIGHT    8

#define HISTOGRAM_EVENT_MASK  (GDK_BUTTON_PRESS_MASK   | \
                               GDK_BUTTON_RELEASE_MASK | \
                               GDK_BUTTON_MOTION_MASK)


/*  local function prototypes  */

static void   gimp_histogram_box_low_adj_update  (GtkAdjustment     *adj,
                                                  GimpHistogramBox  *box);
static void   gimp_histogram_box_high_adj_update (GtkAdjustment     *adj,
                                                  GimpHistogramBox  *box);
static void   gimp_histogram_box_histogram_range (GimpHistogramView *view,
                                                  gint               start,
                                                  gint               end,
                                                  GimpHistogramBox  *box);
static void   gimp_histogram_box_channel_notify  (GimpHistogramView *view,
                                                  GParamSpec        *pspec,
                                                  GimpColorBar      *bar);
static void   gimp_histogram_box_border_notify   (GimpHistogramView *view,
                                                  GParamSpec        *pspec,
                                                  GimpColorBar      *bar);

static gboolean gimp_histogram_slider_area_event (GtkWidget         *widget,
                                                  GdkEvent          *event,
                                                  GimpHistogramBox  *box);
static gboolean gimp_histogram_slider_area_expose (GtkWidget        *widget,
                                                  GdkEvent          *event,
                                                  GimpHistogramBox  *box);

static void   gimp_histogram_draw_slider         (GtkWidget         *widget,
                                                  GdkGC             *border_gc,
                                                  GdkGC             *fill_gc,
                                                  gint               xpos);


G_DEFINE_TYPE (GimpHistogramBox, gimp_histogram_box, GTK_TYPE_VBOX)


static void
gimp_histogram_box_class_init (GimpHistogramBoxClass *klass)
{
}

static void
gimp_histogram_box_init (GimpHistogramBox *box)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkWidget *frame;
  GtkWidget *view;
  GtkWidget *bar;
  GtkWidget *slider_area;

  gtk_box_set_spacing (GTK_BOX (box), 2);

  /*  The histogram  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  view = gimp_histogram_view_new (TRUE);
  gtk_container_add (GTK_CONTAINER (frame), view);
  gtk_widget_show (view);

  g_signal_connect (view, "range-changed",
                    G_CALLBACK (gimp_histogram_box_histogram_range),
                    box);

  box->view = GIMP_HISTOGRAM_VIEW (view);

  /*  The gradient below the histogram */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box->slider_area = slider_area = gtk_event_box_new ();
  gtk_widget_set_size_request (slider_area, -1,
                               GRADIENT_HEIGHT + CONTROL_HEIGHT);
  gtk_widget_add_events (slider_area, HISTOGRAM_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (frame), slider_area);
  gtk_widget_show (slider_area);

  g_signal_connect (slider_area, "event",
                    G_CALLBACK (gimp_histogram_slider_area_event),
                    box);
  g_signal_connect_after (slider_area, "expose-event",
                          G_CALLBACK (gimp_histogram_slider_area_expose),
                          box);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (slider_area), vbox);
  gtk_widget_show (vbox);

  bar = g_object_new (GIMP_TYPE_COLOR_BAR,
                      "histogram-channel", box->view->channel,
                      "xpad",              box->view->border_width,
                      "ypad",              box->view->border_width,
                      NULL);

  gtk_widget_set_size_request (bar,
                               -1,
                               GRADIENT_HEIGHT + 2 * box->view->border_width);
  gtk_box_pack_start (GTK_BOX (vbox), bar, FALSE, FALSE, 0);
  gtk_widget_show (bar);

  g_signal_connect (view, "notify::histogram-channel",
                    G_CALLBACK (gimp_histogram_box_channel_notify),
                    bar);
  g_signal_connect (view, "notify::border-width",
                    G_CALLBACK (gimp_histogram_box_border_notify),
                    bar);

  /*  The range selection */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low spinbutton  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     0.0, 0.0, 255.0, 1.0, 16.0, 0.0,
                                     1.0, 0);
  box->low_adj = GTK_ADJUSTMENT (adjustment);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_histogram_box_low_adj_update),
                    box);

  /*  high spinbutton  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     255.0, 0.0, 255.0, 1.0, 16.0, 0.0,
                                     1.0, 0);
  box->high_adj = GTK_ADJUSTMENT (adjustment);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_histogram_box_high_adj_update),
                    box);

#ifdef DEBUG_VIEW
  spinbutton = gimp_prop_spin_button_new (G_OBJECT (box->view), "border-width",
                                          1, 5, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  spinbutton = gimp_prop_spin_button_new (G_OBJECT (box->view), "subdivisions",
                                          1, 5, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);
#endif
}

static void
gimp_histogram_box_low_adj_update (GtkAdjustment    *adjustment,
                                   GimpHistogramBox *box)
{
  if ((gdouble) box->view->start == adjustment->value)
    return;

  box->high_adj->lower = adjustment->value;
  gtk_adjustment_changed (box->high_adj);

  gimp_histogram_view_set_range (box->view,
                                 adjustment->value, box->view->end);
  gtk_widget_queue_draw (box->slider_area);
}

static void
gimp_histogram_box_high_adj_update (GtkAdjustment    *adjustment,
                                    GimpHistogramBox *box)
{
  if ((gdouble) box->view->end == adjustment->value)
    return;

  box->low_adj->upper = adjustment->value;
  gtk_adjustment_changed (box->low_adj);

  gimp_histogram_view_set_range (box->view,
                                 box->view->start, adjustment->value);
  gtk_widget_queue_draw (box->slider_area);
}

static void
gimp_histogram_box_histogram_range (GimpHistogramView *view,
                                    gint               start,
                                    gint               end,
                                    GimpHistogramBox  *box)
{
  box->high_adj->lower = start;
  box->low_adj->upper  = end;
  gtk_adjustment_changed (box->high_adj);
  gtk_adjustment_changed (box->low_adj);

  gtk_adjustment_set_value (box->low_adj,  start);
  gtk_adjustment_set_value (box->high_adj, end);
  gtk_widget_queue_draw (box->slider_area);
}

static void
gimp_histogram_box_channel_notify (GimpHistogramView *view,
                                   GParamSpec        *pspec,
                                   GimpColorBar      *bar)
{
  gimp_color_bar_set_channel (bar, view->channel);
}

static void
gimp_histogram_box_border_notify (GimpHistogramView *view,
                                  GParamSpec        *pspec,
                                  GimpColorBar      *bar)
{
  g_object_set (bar,
                "xpad", view->border_width,
                "ypad", view->border_width,
                NULL);

  gtk_widget_set_size_request (GTK_WIDGET (bar),
                               -1, GRADIENT_HEIGHT + 2 * view->border_width);
}

GtkWidget *
gimp_histogram_box_new (void)
{
  return g_object_new (GIMP_TYPE_HISTOGRAM_BOX, NULL);
}

void
gimp_histogram_box_set_channel (GimpHistogramBox     *box,
                                GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_BOX (box));

  if (box->view)
    gimp_histogram_view_set_channel (box->view, channel);
}

static gboolean
gimp_histogram_slider_area_event (GtkWidget         *widget,
                                  GdkEvent          *event,
                                  GimpHistogramBox  *box)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            x, distance;
  gint            i;
  gboolean        update = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 0; i < 2; i++)
        if (fabs (bevent->x - box->slider_pos[i]) < distance)
          {
            box->active_slider = i;
            distance = fabs (bevent->x - box->slider_pos[i]);
          }

      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      switch (box->active_slider)
        {
        case 3:  /*  low output  */
          gtk_adjustment_set_value (box->low_adj, box->low_slider_val);
          break;
        case 4:  /*  high output  */
          gtk_adjustment_set_value (box->high_adj, box->high_slider_val);
          break;
        }

      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      update = TRUE;
      break;

    default:
      break;
    }

  if (update)
    {
      gdouble value;
      gint    border = box->view->border_width;
      gint    width  = widget->allocation.width - 2 * border;

      if (width < 1)
        return FALSE;

      switch (box->active_slider)
        {
        case 0:  /*  low output  */
          value = (gdouble) (x - border) / (gdouble) width * 256.0;

          gtk_adjustment_set_value (box->low_adj, CLAMP (value, 0, 255));
          break;

        case 1:  /*  high output  */
          value = (gdouble) (x - border) / (gdouble) width * 256.0;

          gtk_adjustment_set_value (box->high_adj, CLAMP (value, 0, 255));
          break;
        }

    }

  return FALSE;
}

static gboolean
gimp_histogram_slider_area_expose (GtkWidget        *widget,
                                   GdkEvent         *event,
                                   GimpHistogramBox *box)
{
  gint border = box->view->border_width;
  gint width  = widget->allocation.width - 2 * border;

  box->slider_pos[0] = border + ROUND ((gdouble) width *
                                       box->high_adj->lower / 256.0);

  box->slider_pos[1] = border + ROUND ((gdouble) width *
                                       (box->low_adj->upper + 1.0) / 256.0);

  gimp_histogram_draw_slider (widget,
                              widget->style->black_gc,
                              widget->style->black_gc,
                              box->slider_pos[0]);
  gimp_histogram_draw_slider (widget,
                              widget->style->black_gc,
                              widget->style->white_gc,
                              box->slider_pos[1]);

  return FALSE;
}

static void
gimp_histogram_draw_slider (GtkWidget *widget,
                            GdkGC     *border_gc,
                            GdkGC     *fill_gc,
                            gint       xpos)
{
  gint y;

  for (y = 0; y < CONTROL_HEIGHT; y++)
    gdk_draw_line (widget->window, fill_gc,
                   xpos - y / 2, GRADIENT_HEIGHT + y,
                   xpos + y / 2, GRADIENT_HEIGHT + y);

  gdk_draw_line (widget->window, border_gc,
                 xpos,
                 GRADIENT_HEIGHT,
                 xpos - (CONTROL_HEIGHT - 1) / 2,
                 GRADIENT_HEIGHT + CONTROL_HEIGHT - 1);

  gdk_draw_line (widget->window, border_gc,
                 xpos,
                 GRADIENT_HEIGHT,
                 xpos + (CONTROL_HEIGHT - 1) / 2,
                 GRADIENT_HEIGHT + CONTROL_HEIGHT - 1);

  gdk_draw_line (widget->window, border_gc,
                 xpos - (CONTROL_HEIGHT - 1) / 2,
                 GRADIENT_HEIGHT + CONTROL_HEIGHT - 1,
                 xpos + (CONTROL_HEIGHT - 1) / 2,
                 GRADIENT_HEIGHT + CONTROL_HEIGHT - 1);
}
