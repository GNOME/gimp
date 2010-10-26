/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Plugin to convert a selection to a path.
 *
 * Copyright (C) 1999 Andy Thomas  alt@gimp.org
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Change log:-
 * 0.1 First version.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "types.h"

#include "selection-to-path.h"

#include "libgimp/stdplugins-intl.h"

#define SCALE_WIDTH  100
#define SCALE_DIGITS 8


static GSList * adjust_widgets = NULL;


/* Reset to recommended defaults */
void
reset_adv_dialog (void)
{
  GSList  *list;
  GObject *widget;
  gdouble *value;

  for (list = adjust_widgets; list; list = g_slist_next (list))
    {
      widget = G_OBJECT (list->data);
      value  = (gdouble *) g_object_get_data (G_OBJECT (widget),
                                              "default_value");

      if (GTK_IS_ADJUSTMENT (widget))
        {
          gtk_adjustment_set_value (GTK_ADJUSTMENT (widget),
                                    *value);
        }
      else if (GTK_IS_TOGGLE_BUTTON (widget))
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        (gboolean)(*value));
        }
      else
        g_warning ("Internal widget list error");
    }
}

static gpointer
def_val (gdouble default_value)
{
  gdouble *value = g_new0 (gdouble, 1);
  *value = default_value;
  return (value);
}

GtkWidget *
dialog_create_selection_area (SELVALS *sels)
{
  GtkWidget     *table;
  GtkWidget     *check;
  GtkAdjustment *adj;
  gint           row;

  table = gtk_table_new (20, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  row = 0;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Align Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->align_threshold,
                              0.2, 2.0, 0.1, 0.1, 2,
                              TRUE, 0, 0,
                              _("If two endpoints are closer than this, "
                                "they are made to be equal."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->align_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.5));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Corner Always Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->corner_always_threshold,
                              30, 180, 1, 1, 2,
                              TRUE, 0, 0,
                              _("If the angle defined by a point and its predecessors "
				"and successors is smaller than this, it's a corner, "
				"even if it's within `corner_surround' pixels of a "
				"point with a smaller angle."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->corner_always_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (60.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Corner Surround:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->corner_surround,
                              3, 8, 1, 1, 0,
                              TRUE, 0, 0,
                              _("Number of points to consider when determining if a "
				"point is a corner or not."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->corner_surround);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (4.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Corner Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->corner_threshold,
                              0, 180, 1, 1, 2,
                              TRUE, 0, 0,
                              _("If a point, its predecessors, and its successors "
				"define an angle smaller than this, it's a corner."),
                              NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->corner_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (100.0));


  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Error Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->error_threshold,
                              0.2, 10, 0.1, 0.1, 2,
                              TRUE, 0, 0,
                              _("Amount of error at which a fitted spline is "
				"unacceptable.  If any pixel is further away "
				"than this from the fitted curve, we try again."),
                              NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->error_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.40));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Filter Alternative Surround:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->filter_alternative_surround,
                              1, 10, 1, 1, 0,
                              TRUE, 0, 0,
                              _("A second number of adjacent points to consider "
				"when filtering."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->filter_alternative_surround);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (1.0));


  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Filter Epsilon:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->filter_epsilon,
                              5, 40, 1, 1, 2,
                              TRUE, 0, 0,
                              _("If the angles between the vectors produced by "
				"filter_surround and filter_alternative_surround "
				"points differ by more than this, use the one from "
				"filter_alternative_surround."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->filter_epsilon);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (10.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Filter Iteration Count:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->filter_iteration_count,
                              4, 70, 1, 1, 0,
                              TRUE, 0, 0,
                              _("Number of times to smooth original data points.  "
				"Increasing this number dramatically --- to 50 or "
				"so --- can produce vastly better results.  But if "
				"any points that ``should'' be corners aren't found, "
				"the curve goes to hell around that point."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->filter_iteration_count);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (4.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Filter Percent:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->filter_percent,
                              0, 1, 0.05, 0.01, 2,
                              TRUE, 0, 0,
                              _("To produce the new point, use the old point plus "
				"this times the neighbors."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->filter_percent);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.33));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Filter Secondary Surround:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->filter_secondary_surround,
                              3, 10, 1, 1, 0,
                              TRUE, 0, 0,
                              _("Number of adjacent points to consider if "
				"`filter_surround' points defines a straight line."),
                              NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->filter_secondary_surround);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (3.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Filter Surround:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->filter_surround,
                              2, 10, 1, 1, 0,
                              TRUE, 0, 0,
                              _("Number of adjacent points to consider when filtering."),
                              NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->filter_surround);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (2.0));

  check = gtk_check_button_new_with_label (_("Keep Knees"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), sels->keep_knees);
  gtk_table_attach (GTK_TABLE (table), check, 1, 3, row, row + 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gimp_help_set_help_data (GTK_WIDGET (check),
                           _("Says whether or not to remove ``knee'' "
			     "points after finding the outline."), NULL);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &sels->keep_knees);
  gtk_widget_show (check);
  adjust_widgets = g_slist_append (adjust_widgets, check);
  g_object_set_data (G_OBJECT (check), "default_value", def_val ((gdouble)FALSE));
  row++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Line Reversion Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->line_reversion_threshold,
                              0.01, 0.2, 0.01, 0.01, 3,
                              TRUE, 0, 0,
                              _("If a spline is closer to a straight line than this, "
				"it remains a straight line, even if it would otherwise "
				"be changed back to a curve. This is weighted by the "
				"square of the curve length, to make shorter curves "
				"more likely to be reverted."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->line_reversion_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.01));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Line Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->line_threshold,
                              0.2, 4, 0.1, 0.01, 2,
                              TRUE, 0, 0,
                              _("How many pixels (on the average) a spline can "
				"diverge from the line determined by its endpoints "
				"before it is changed to a straight line."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->line_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.5));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Reparametrize Improvement:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->reparameterize_improvement,
                              0, 1, 0.05, 0.01, 2,
                              TRUE, 0, 0,
                              _("If reparameterization doesn't improve the fit by this "
				"much percent, stop doing it. ""Amount of error at which "
				"it is pointless to reparameterize."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->reparameterize_improvement);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.01));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Reparametrize Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->reparameterize_threshold,
                              1, 50, 0.5, 0.5, 2,
                              TRUE, 0, 0,
                              _("Amount of error at which it is pointless to reparameterize.  "
				"This happens, for example, when we are trying to fit the "
				"outline of the outside of an `O' with a single spline.  "
				"The initial fit is not good enough for the Newton-Raphson "
				"iteration to improve it.  It may be that it would be better "
				"to detect the cases where we didn't find any corners."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->reparameterize_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (1.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Subdivide Search:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->subdivide_search,
                              0.05, 1, 0.05, 0.01, 2,
                              TRUE, 0, 0,
                              _("Percentage of the curve away from the worst point "
				"to look for a better place to subdivide."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->subdivide_search);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.1));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Subdivide Surround:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->subdivide_surround,
                              2, 10, 1, 1, 0,
                              TRUE, 0, 0,
                              _("Number of points to consider when deciding whether "
				"a given point is a better place to subdivide."),
                              NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->subdivide_surround);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (4.0));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Subdivide Threshold:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->subdivide_threshold,
                              0.01, 1, 0.01, 0.01, 2,
                              TRUE, 0, 0,
                              _("How many pixels a point can diverge from a straight "
				"line and still be considered a better place to "
				"subdivide."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->subdivide_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (0.03));

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Tangent Surround:"), SCALE_WIDTH, SCALE_DIGITS,
                              sels->tangent_surround,
                              2, 10, 1, 1, 0,
                              TRUE, 0, 0,
                              _("Number of points to look at on either side of a "
				"point when computing the approximation to the "
				"tangent at that point."), NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &sels->tangent_surround);
  adjust_widgets = g_slist_append (adjust_widgets, adj);
  g_object_set_data (G_OBJECT (adj), "default_value", def_val (3.0));

  return table;
}
