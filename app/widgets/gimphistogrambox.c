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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/gimphistogram.h"

#include "gimphistogrambox.h"
#include "gimphistogramview.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gimp_histogram_box_class_init (GimpHistogramBoxClass *klass);
static void   gimp_histogram_box_init       (GimpHistogramBox      *histogram_box);

static void   gimp_histogram_box_low_adj_update   (GtkAdjustment     *adj,
                                                   GimpHistogramBox  *box);
static void   gimp_histogram_box_high_adj_update  (GtkAdjustment     *adj,
                                                   GimpHistogramBox  *box);
static void   gimp_histogram_box_histogram_range  (GimpHistogramView *view,
                                                   gint               start,
                                                   gint               end,
                                                   GimpHistogramBox  *box);


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
  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_histogram_box_init (GimpHistogramBox *box)
{
  GtkWidget *hbox;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkWidget *frame;

  gtk_box_set_spacing (GTK_BOX (box), 4);

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

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
                    G_CALLBACK (gimp_histogram_box_low_adj_update),
                    box);

  /*  high spinbutton  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     255.0, 0.0, 255.0, 1.0, 16.0, 0.0,
                                     1.0, 0);
  box->high_adj = GTK_ADJUSTMENT (adjustment);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
                    G_CALLBACK (gimp_histogram_box_high_adj_update),
                    box);

  /*  The histogram  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box->histogram = gimp_histogram_view_new (GIMP_HISTOGRAM_VIEW_WIDTH,
                                            GIMP_HISTOGRAM_VIEW_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (box->histogram));
  gtk_widget_show (GTK_WIDGET (box->histogram));

  g_signal_connect (G_OBJECT (box->histogram), "range_changed",
                    G_CALLBACK (gimp_histogram_box_histogram_range),
                    box);
}

static void
gimp_histogram_box_low_adj_update (GtkAdjustment    *adjustment,
                                   GimpHistogramBox *box)
{
  if ((gdouble) box->histogram->start != adjustment->value)
    {
      box->high_adj->lower = adjustment->value;
      gtk_adjustment_changed (box->high_adj);

      gimp_histogram_view_set_range (box->histogram,
                                     (gint) adjustment->value,
                                     box->histogram->end);
    }
}

static void
gimp_histogram_box_high_adj_update (GtkAdjustment    *adjustment,
                                    GimpHistogramBox *box)
{
  if ((gdouble) box->histogram->end != adjustment->value)
    {
      box->low_adj->upper = adjustment->value;
      gtk_adjustment_changed (box->low_adj);

      gimp_histogram_view_set_range (box->histogram,
                                     box->histogram->start,
                                     (gint) adjustment->value);
    }
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

GtkWidget *
gimp_histogram_box_new (const gchar *label)
{
  GimpHistogramBox *box;

  box = g_object_new (GIMP_TYPE_HISTOGRAM_BOX, NULL);

  gtk_label_set_text (GTK_LABEL (box->label), label);

  return GTK_WIDGET (box);
}
