/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmahistogram.h"

#include "ligmacolorbar.h"
#include "ligmahandlebar.h"
#include "ligmahistogrambox.h"
#include "ligmahistogramview.h"

#include "ligma-intl.h"


/*  #define DEBUG_VIEW  */

#define GRADIENT_HEIGHT  12
#define CONTROL_HEIGHT   10


/*  local function prototypes  */

static void   ligma_histogram_box_low_adj_update  (GtkAdjustment     *adj,
                                                  LigmaHistogramBox  *box);
static void   ligma_histogram_box_high_adj_update (GtkAdjustment     *adj,
                                                  LigmaHistogramBox  *box);
static void   ligma_histogram_box_histogram_range (LigmaHistogramView *view,
                                                  gint               start,
                                                  gint               end,
                                                  LigmaHistogramBox  *box);
static void   ligma_histogram_box_channel_notify  (LigmaHistogramView *view,
                                                  GParamSpec        *pspec,
                                                  LigmaHistogramBox  *box);
static void   ligma_histogram_box_border_notify   (LigmaHistogramView *view,
                                                  GParamSpec        *pspec,
                                                  LigmaHistogramBox  *box);


G_DEFINE_TYPE (LigmaHistogramBox, ligma_histogram_box, GTK_TYPE_BOX)


static void
ligma_histogram_box_class_init (LigmaHistogramBoxClass *klass)
{
}

static void
ligma_histogram_box_init (LigmaHistogramBox *box)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *frame;
  GtkWidget *view;
  GtkWidget *bar;

  box->n_bins = 256;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (box), 2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  The histogram  */
  view = ligma_histogram_view_new (TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  g_signal_connect (view, "range-changed",
                    G_CALLBACK (ligma_histogram_box_histogram_range),
                    box);

  box->view = LIGMA_HISTOGRAM_VIEW (view);

  /*  The gradient below the histogram */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2),
                                  LIGMA_HISTOGRAM_VIEW (view)->border_width);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  box->color_bar = bar = g_object_new (LIGMA_TYPE_COLOR_BAR,
                                       "histogram-channel", box->view->channel,
                                       NULL);
  gtk_widget_set_size_request (bar, -1, GRADIENT_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), bar, FALSE, FALSE, 0);
  gtk_widget_show (bar);

  g_signal_connect (view, "notify::histogram-channel",
                    G_CALLBACK (ligma_histogram_box_channel_notify),
                    box);
  g_signal_connect (view, "notify::border-width",
                    G_CALLBACK (ligma_histogram_box_border_notify),
                    box);

  box->slider_bar = bar = g_object_new (LIGMA_TYPE_HANDLE_BAR, NULL);
  gtk_widget_set_size_request (bar, -1, CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), bar, FALSE, FALSE, 0);
  gtk_widget_show (bar);

  ligma_handle_bar_connect_events (LIGMA_HANDLE_BAR (box->slider_bar),
                                  box->color_bar);

  /*  The range selection */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low spinbutton  */
  box->low_adj = gtk_adjustment_new (0.0, 0.0, 255.0, 1.0, 16.0, 0.0);
  box->low_spinbutton = ligma_spin_button_new (box->low_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (box->low_spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), box->low_spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (box->low_spinbutton);

  g_signal_connect (box->low_adj, "value-changed",
                    G_CALLBACK (ligma_histogram_box_low_adj_update),
                    box);

  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (bar), 0, box->low_adj);

  /*  high spinbutton  */
  box->high_adj = gtk_adjustment_new (255.0, 0.0, 255.0, 1.0, 16.0, 0.0);
  box->high_spinbutton = ligma_spin_button_new (box->high_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (box->high_spinbutton), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), box->high_spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (box->high_spinbutton);

  g_signal_connect (box->high_adj, "value-changed",
                    G_CALLBACK (ligma_histogram_box_high_adj_update),
                    box);

  ligma_handle_bar_set_adjustment (LIGMA_HANDLE_BAR (bar), 2, box->high_adj);

#ifdef DEBUG_VIEW
  spinbutton = ligma_prop_spin_button_new (G_OBJECT (box->view), "border-width",
                                          1, 5, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  spinbutton = ligma_prop_spin_button_new (G_OBJECT (box->view), "subdivisions",
                                          1, 5, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
#endif
}

static void
ligma_histogram_box_low_adj_update (GtkAdjustment    *adjustment,
                                   LigmaHistogramBox *box)
{
  gdouble value = gtk_adjustment_get_value (adjustment);

  gtk_adjustment_set_lower (box->high_adj, value);

  if (box->n_bins != 256)
    value *= box->n_bins - 1;

  value = ROUND (value);

  if (box->view->start != value)
    ligma_histogram_view_set_range (box->view, value, box->view->end);
}

static void
ligma_histogram_box_high_adj_update (GtkAdjustment    *adjustment,
                                    LigmaHistogramBox *box)
{
  gdouble value = gtk_adjustment_get_value (adjustment);

  gtk_adjustment_set_upper (box->low_adj, value);

  if (box->n_bins != 256)
    value *= box->n_bins - 1;

  value = ROUND (value);

  if (box->view->end != value)
    ligma_histogram_view_set_range (box->view, box->view->start, value);
}

static void
ligma_histogram_box_histogram_range (LigmaHistogramView *view,
                                    gint               start,
                                    gint               end,
                                    LigmaHistogramBox  *box)
{
  gdouble s = start;
  gdouble e = end;

  if (box->n_bins != view->n_bins)
    {
      gdouble upper;
      gdouble page_increment;
      gdouble step_increment;
      guint   digits;

      box->n_bins = view->n_bins;

      if (box->n_bins == 256)
        {
          digits         = 0;
          upper          = 255.0;
          step_increment = 1.0;
          page_increment = 16.0;
        }
      else
        {
          digits         = 3;
          upper          = 1.0;
          step_increment = 0.01;
          page_increment = 0.1;
        }

      g_object_set (G_OBJECT (box->high_adj),
                    "upper", upper,
                    "step-increment", step_increment,
                    "page-increment", page_increment,
                    NULL);

      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (box->high_spinbutton), digits);

      g_object_set (G_OBJECT (box->low_adj),
                    "step-increment", step_increment,
                    "page-increment", page_increment,
                    NULL);

      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (box->low_spinbutton), digits);
    }

  if (box->n_bins != 256)
    {
      s /= box->n_bins - 1;
      e /= box->n_bins - 1;
    }

  gtk_adjustment_set_lower (box->high_adj, s);
  gtk_adjustment_set_upper (box->low_adj,  e);

  gtk_adjustment_set_value (box->low_adj,  s);
  gtk_adjustment_set_value (box->high_adj, e);
}

static void
ligma_histogram_box_channel_notify (LigmaHistogramView *view,
                                   GParamSpec        *pspec,
                                   LigmaHistogramBox  *box)
{
  ligma_color_bar_set_channel (LIGMA_COLOR_BAR (box->color_bar), view->channel);
}

static void
ligma_histogram_box_border_notify (LigmaHistogramView *view,
                                  GParamSpec        *pspec,
                                  LigmaHistogramBox  *box)
{
  GtkWidget *vbox = gtk_widget_get_parent (box->color_bar);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), view->border_width);
}


/*  public functions  */

GtkWidget *
ligma_histogram_box_new (void)
{
  return g_object_new (LIGMA_TYPE_HISTOGRAM_BOX, NULL);
}

void
ligma_histogram_box_set_channel (LigmaHistogramBox     *box,
                                LigmaHistogramChannel  channel)
{
  g_return_if_fail (LIGMA_IS_HISTOGRAM_BOX (box));

  if (box->view)
    ligma_histogram_view_set_channel (box->view, channel);
}
