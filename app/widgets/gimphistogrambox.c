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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/gimphistogram.h"

#include "gimphistogrambox.h"
#include "gimphistogramview.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


/* #define DEBUG_VIEW */

#define GRADIENT_HEIGHT 10


/*  local function prototypes  */

static void   gimp_histogram_box_class_init  (GimpHistogramBoxClass *klass);
static void   gimp_histogram_box_init        (GimpHistogramBox      *histogram_box);
static void   gimp_histogram_box_finalize    (GObject               *object);

static void   gimp_histogram_box_low_adj_update  (GtkAdjustment     *adj,
                                                  GimpHistogramBox  *box);
static void   gimp_histogram_box_high_adj_update (GtkAdjustment     *adj,
                                                  GimpHistogramBox  *box);
static void   gimp_histogram_box_histogram_range (GimpHistogramView *view,
                                                  gint               start,
                                                  gint               end,
                                                  GimpHistogramBox  *box);

static void     gimp_histogram_box_gradient_size_allocate (GtkWidget      *widget,
                                                           GtkAllocation  *allocation,
                                                           gpointer        data);
static gboolean gimp_histogram_box_gradient_expose        (GtkWidget      *widget,
                                                           GdkEventExpose *event,
                                                           gpointer        data);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_histogram_box_get_type (void)
{
  static GType box_type = 0;

  if (! box_type)
    {
      static const GTypeInfo box_info =
      {
        sizeof (GimpHistogramBoxClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_histogram_box_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpHistogramBox),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_histogram_box_init,
      };

      box_type = g_type_register_static (GTK_TYPE_VBOX,
                                         "GimpHistogramBox",
                                         &box_info, 0);
    }

  return box_type;
}

static void
gimp_histogram_box_class_init (GimpHistogramBoxClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_histogram_box_finalize;
}

static void
gimp_histogram_box_init (GimpHistogramBox *box)
{
  GtkWidget *hbox;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkWidget *frame;
  GtkWidget *view;

  gtk_box_set_spacing (GTK_BOX (box), 2);

  /*  The histogram  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  view = gimp_histogram_view_new (TRUE);
  gtk_container_add (GTK_CONTAINER (frame), view);
  gtk_widget_show (view);

  g_signal_connect (view, "range_changed",
                    G_CALLBACK (gimp_histogram_box_histogram_range),
                    box);

  box->view = GIMP_HISTOGRAM_VIEW (view);

  /*  The gradient below the histogram */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box->gradient = gtk_drawing_area_new ();
  gtk_widget_set_size_request (box->gradient, -1,
                               GRADIENT_HEIGHT + 2 * box->view->border_width);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (box->gradient));
  gtk_widget_show (box->gradient);

  g_signal_connect (box->gradient, "size_allocate",
                    G_CALLBACK (gimp_histogram_box_gradient_size_allocate),
                    box);
  g_signal_connect (box->gradient, "expose_event",
                    G_CALLBACK (gimp_histogram_box_gradient_expose),
                    box);

  g_signal_connect_swapped (view, "notify::histogram-channel",
			    G_CALLBACK (gtk_widget_queue_draw),
			    box->gradient);
  g_signal_connect_swapped (view, "notify::border-width",
			    G_CALLBACK (gtk_widget_queue_resize),
			    box->gradient);

  /*  The range selection */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  box->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), box->label, FALSE, FALSE, 0);
  gtk_widget_show (box->label);

  /*  low spinbutton  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     0.0, 0.0, 255.0, 1.0, 16.0, 0.0,
                                     1.0, 0);
  box->low_adj = GTK_ADJUSTMENT (adjustment);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adjustment, "value_changed",
                    G_CALLBACK (gimp_histogram_box_low_adj_update),
                    box);

  /*  high spinbutton  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     255.0, 0.0, 255.0, 1.0, 16.0, 0.0,
                                     1.0, 0);
  box->high_adj = GTK_ADJUSTMENT (adjustment);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adjustment, "value_changed",
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
gimp_histogram_box_finalize (GObject *object)
{
  GimpHistogramBox *box = GIMP_HISTOGRAM_BOX (object);

  if (box->gradient_buf)
    {
      g_free (box->gradient_buf);
      box->gradient_buf = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
}

static void
gimp_histogram_box_histogram_range (GimpHistogramView *widget,
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
}

static void
gimp_histogram_box_gradient_size_allocate (GtkWidget     *widget,
                                           GtkAllocation *allocation,
                                           gpointer       data)
{
  GimpHistogramBox *box = GIMP_HISTOGRAM_BOX (data);
  gint              border;
  gint              width, height;

  border = box->view->border_width;
  width  = allocation->width  - 2 * border;
  height = allocation->height - 2 * border;

  box->gradient_buf = g_realloc (box->gradient_buf, MAX (0, 3 * width * height));
}

static gboolean
gimp_histogram_box_gradient_expose (GtkWidget      *widget,
                                    GdkEventExpose *event,
                                    gpointer        data)
{
  GimpHistogramBox     *box = GIMP_HISTOGRAM_BOX (data);
  GimpHistogramChannel  channel;

  gint    i;
  gint    border;
  gint    width, height;
  guchar  r, g, b;

  border = box->view->border_width;
  width  = widget->allocation.width  - 2 * border;
  height = widget->allocation.height - 2 * border;

  if (width <= 0 || height <= 0)
    return TRUE;

  if (box->view)
    channel = box->view->channel;
  else
    channel = GIMP_HISTOGRAM_VALUE;

  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
    case GIMP_HISTOGRAM_ALPHA:  r = g = b = 1;
      break;
    case GIMP_HISTOGRAM_RED:    r = 1; g = b = 0;
      break;
    case GIMP_HISTOGRAM_GREEN:  g = 1; r = b = 0;
      break;
    case GIMP_HISTOGRAM_BLUE:   b = 1; r = g = 0;
      break;
    default:
      r = g = b = 0;
      g_assert_not_reached ();
      break;
    }

  for (i = 0; i < width; i++)
    {
      guchar *buffer = box->gradient_buf + 3 * i;
      gint    x      = (i * 256) / width;

      buffer[0] = x * r;
      buffer[1] = x * g;
      buffer[2] = x * b;
    }

  for (i = 1; i < height; i++)
    memcpy (box->gradient_buf + 3 * i * width, box->gradient_buf, 3 * width);

  gdk_draw_rgb_image (widget->window, widget->style->black_gc,
                      border, border, width, height, GDK_RGB_DITHER_NORMAL,
                      box->gradient_buf, 3 * width);

  return TRUE;
}

GtkWidget *
gimp_histogram_box_new (const gchar *label)
{
  GimpHistogramBox *box = g_object_new (GIMP_TYPE_HISTOGRAM_BOX, NULL);

  gtk_label_set_text (GTK_LABEL (box->label), label);

  return GTK_WIDGET (box);
}

void
gimp_histogram_box_set_channel (GimpHistogramBox     *box,
                                GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_BOX (box));

  if (box->view)
    gimp_histogram_view_set_channel (box->view, channel);
}
