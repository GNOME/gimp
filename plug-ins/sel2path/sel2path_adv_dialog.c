/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Plugin to convert a selection to a path.
 *
 * Copyright (C) 1999 Andy Thomas  alt@gimp.org
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
 * 
 */

/* Change log:-
 * 0.1 First version.
 */

#include <stdio.h>
#include <stdlib.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "types.h"

static GSList * adjust_widgets = NULL;

typedef struct dVal
{
  gdouble value;
} dVal;

/* Reset to recommended defaults */
void
reset_adv_dialog()
{
  GSList *list = adjust_widgets;

  while(list)
    {
      GtkObject *w = GTK_OBJECT(list->data);
      dVal *valptr = (dVal *)gtk_object_get_data(GTK_OBJECT(w),"default_value");
      if(GTK_IS_ADJUSTMENT(w))
	{
	  (GTK_ADJUSTMENT(w))->value = valptr->value;
	  gtk_signal_emit_by_name(GTK_OBJECT(w), "value_changed");
	}
      else if(GTK_IS_TOGGLE_BUTTON(w))
	{
	  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w),valptr->value);
	}
      else
	g_warning("Internal widget list error");

      list = g_slist_next(list);
    }
}

gpointer
def_val(gdouble default_value)
{
  dVal *valptr = g_new0(dVal,1);
  valptr->value = default_value;
  return(valptr);
}

static void
dialog_scale_update(GtkAdjustment * adjustment, gdouble * value)
{
  if (*value != adjustment->value) {
    *value = adjustment->value;
  }
}


static void
on_keep_knees_checkbutton_toggled(GtkToggleButton *togglebutton,
				  gpointer         user_data)
{
  int *toggle_val;
  
  toggle_val = (int *) user_data;
  
  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    {
      /* Only do for event that sets a toggle button to true */
      /* This will break if any more toggles are added? */
      *toggle_val = TRUE;
    }
  else
    *toggle_val = FALSE;
}


GtkWidget *
dialog_create_selection_area(SELVALS *sels)
{
  GtkWidget *table1;
  GtkWidget *align_threshold_scale;
  GtkWidget *align_threshold;
  GtkWidget *line_reversion_threshold_scale;
  GtkWidget *subdivide_threshold_scale;
  GtkWidget *tangent_surround_scale;
  GtkWidget *subdivide_surround_scale;
  GtkWidget *reparameterize_threshold_scale;
  GtkWidget *corner_surround_scale;
  GtkWidget *corner_threshold_scale;
  GtkWidget *filter_percent_scale;
  GtkWidget *filter_secondary_surround_scale;
  GtkWidget *filter_surround_scale;
  GtkWidget *keep_knees_checkbutton;
  GtkWidget *subdivide_search_scale;
  GtkWidget *reparameterize_improvement_scale;
  GtkWidget *line_threshold_scale;
  GtkWidget *filter_iteration_count_scale;
  GtkWidget *filter_epsilon_scale;
  GtkWidget *subdivide_threshold;
  GtkWidget *tangent_surround;
  GtkWidget *subdivide_surround;
  GtkWidget *subdivide_search;
  GtkWidget *reparameterize_threshold;
  GtkWidget *reparameterize_improvement;
  GtkWidget *line_threshold;
  GtkWidget *line_reversion_threshold;
  GtkWidget *keep_knees;
  GtkWidget *filter_surround;
  GtkWidget *filter_secondary_surround;
  GtkWidget *filter_percent;
  GtkWidget *filter_iteration_count;
  GtkWidget *filter_epsilon;
  GtkWidget *corner_always_threshold_scale;
  GtkWidget *error_threshold_scale;
  GtkWidget *filter_alternative_surround_scale;
  GtkWidget *filter_alternative_surround;
  GtkWidget *error_threshold;
  GtkWidget *corner_threshold;
  GtkWidget *corner_surround;
  GtkWidget *corner_always_threshold;
  GtkTooltips *tooltips;
  GtkObject *size_data;

  tooltips = gtk_tooltips_new ();

  table1 = gtk_table_new (20, 2, FALSE);
  gtk_widget_show (table1);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);

  align_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->align_threshold, 0.2, 2, 0.1, 0.1, 0)));
  gtk_widget_show (align_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), align_threshold_scale, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, align_threshold_scale, "If two endpoints are closer than this, they are made to be equal.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (align_threshold_scale), GTK_POS_LEFT);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->align_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.5));

  align_threshold = gtk_label_new ("align_threshold: ");
  gtk_widget_show (align_threshold);
  gtk_table_attach (GTK_TABLE (table1), align_threshold, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  GTK_WIDGET_SET_FLAGS (align_threshold, GTK_CAN_FOCUS);
  gtk_tooltips_set_tip (tooltips, align_threshold, "If two endpoints are closer than this, they are made to be equal.\n   (-align-threshold)", NULL);
  gtk_misc_set_alignment (GTK_MISC (align_threshold), 1, 0.5);

  line_reversion_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->line_reversion_threshold, 0.01, 0.2, 0.01, 0.01, 0)));
  gtk_widget_show (line_reversion_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), line_reversion_threshold_scale, 1, 2, 12, 13,
                    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, line_reversion_threshold_scale, "If a spline is closer to a straight line than this, it remains a straight line, even if it would otherwise be changed back to a curve. This is weighted by the square of the curve length, to make shorter curves more likely to be reverted.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (line_reversion_threshold_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (line_reversion_threshold_scale), 3);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->line_reversion_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.01));


  subdivide_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->subdivide_threshold, 0.01, 1, 0.01, 0.01, 0)));
  gtk_widget_show (subdivide_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), subdivide_threshold_scale, 1, 2, 19, 20,
                    0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, subdivide_threshold_scale, "How many pixels a point can diverge from a straight line and still be considered a better place to subdivide.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (subdivide_threshold_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (subdivide_threshold_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->subdivide_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.03));


  tangent_surround_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->tangent_surround, 2, 10, 1, 1, 0)));
  gtk_widget_show (tangent_surround_scale);
  gtk_table_attach (GTK_TABLE (table1), tangent_surround_scale, 1, 2, 18, 19,
                    0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, tangent_surround_scale, "Number of points to look at on either side of a point when computing the approximation to the tangent at that point.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (tangent_surround_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (tangent_surround_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->tangent_surround);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(3.0));


  subdivide_surround_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->subdivide_surround, 2, 10, 1, 1, 0)));
  gtk_widget_show (subdivide_surround_scale);
  gtk_table_attach (GTK_TABLE (table1), subdivide_surround_scale, 1, 2, 17, 18,
                    0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, subdivide_surround_scale, "Number of points to consider when deciding whether a given point is a better place to subdivide.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (subdivide_surround_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (subdivide_surround_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->subdivide_surround);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(4.0));


  reparameterize_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->reparameterize_threshold, 1, 50, 0.5, 0.5, 0)));
  gtk_widget_show (reparameterize_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), reparameterize_threshold_scale, 1, 2, 15, 16,
                    0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, reparameterize_threshold_scale, "Amount of error at which it is pointless to reparameterize.  This happens, for example, when we are trying to fit the outline of the outside of an `O' with a single spline.  The initial fit is not good enough for the Newton-Raphson iteration to improve it.  It may be that it would be better to detect the cases where we didn't find any corners.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (reparameterize_threshold_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (reparameterize_threshold_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->reparameterize_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(1.0));


  corner_surround_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->corner_surround, 3, 8, 1, 1, 0)));
  gtk_widget_show (corner_surround_scale);
  gtk_table_attach (GTK_TABLE (table1), corner_surround_scale, 1, 2, 2, 3,
                    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, corner_surround_scale, "Number of points to consider when determining if a point is a corner or not.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (corner_surround_scale), GTK_POS_LEFT);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->corner_surround);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(4.0));


  corner_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->corner_threshold, 0, 180, 1, 1, 0)));
  gtk_widget_show (corner_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), corner_threshold_scale, 1, 2, 3, 4,
                    0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, corner_threshold_scale, "If a point, its predecessors, and its successors define an angle smaller than this, it's a corner. Should be in range 0..180.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (corner_threshold_scale), GTK_POS_LEFT);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->corner_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(100.0));


  filter_percent_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->filter_percent, 0, 1, 0.05, 0.01, 0)));
  gtk_widget_show (filter_percent_scale);
  gtk_table_attach (GTK_TABLE (table1), filter_percent_scale, 1, 2, 8, 9,
                    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_percent_scale, "To produce the new point, use the old point plus this times the neighbors.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (filter_percent_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (filter_percent_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->filter_percent);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.33));


  filter_secondary_surround_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->filter_secondary_surround, 3, 10, 1, 1, 0)));
  gtk_widget_show (filter_secondary_surround_scale);
  gtk_table_attach (GTK_TABLE (table1), filter_secondary_surround_scale, 1, 2, 9, 10,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_secondary_surround_scale, "Number of adjacent points to consider if `filter_surround' points defines a straight line.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (filter_secondary_surround_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (filter_secondary_surround_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->filter_secondary_surround);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(3.0));

  filter_surround_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->filter_surround, 2, 10, 1, 1, 0)));
  gtk_widget_show (filter_surround_scale);
  gtk_table_attach (GTK_TABLE (table1), filter_surround_scale, 1, 2, 10, 11,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_surround_scale, "Number of adjacent points to consider when filtering.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (filter_surround_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (filter_surround_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->filter_surround);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(2.0));


  keep_knees_checkbutton = gtk_check_button_new_with_label ("");
  gtk_widget_show (keep_knees_checkbutton);
  gtk_table_attach (GTK_TABLE (table1), keep_knees_checkbutton, 1, 2, 11, 12,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, keep_knees_checkbutton, "Says whether or not to remove ``knee'' points after finding the outline.", NULL);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (keep_knees_checkbutton), sels->keep_knees);
 gtk_signal_connect (GTK_OBJECT (keep_knees_checkbutton), "toggled",
		     GTK_SIGNAL_FUNC (on_keep_knees_checkbutton_toggled),
		     &sels->keep_knees);
  adjust_widgets = g_slist_append(adjust_widgets,keep_knees_checkbutton);
  gtk_object_set_data(GTK_OBJECT(keep_knees_checkbutton),"default_value",def_val(FALSE));

  subdivide_search_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new(sels->subdivide_search, 0.05, 1, 0.05, 0.01, 0)));
  gtk_widget_show (subdivide_search_scale);
  gtk_table_attach (GTK_TABLE (table1), subdivide_search_scale, 1, 2, 16, 17,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, subdivide_search_scale, "Percentage of the curve away from the worst point to look for a better place to subdivide.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (subdivide_search_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (subdivide_search_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->subdivide_search);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.10));


  reparameterize_improvement_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->reparameterize_improvement, 0, 1, 0.05, 0.01, 0)));
  gtk_widget_show (reparameterize_improvement_scale);
  gtk_table_attach (GTK_TABLE (table1), reparameterize_improvement_scale, 1, 2, 14, 15,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, reparameterize_improvement_scale, "If reparameterization doesn't improve the fit by this much percent, stop doing it. ", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (reparameterize_improvement_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (reparameterize_improvement_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->reparameterize_improvement);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.01));


  line_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->line_threshold, 0.2, 4, 0.1, 0.01, 0)));
  gtk_widget_show (line_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), line_threshold_scale, 1, 2, 13, 14,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, line_threshold_scale, "How many pixels (on the average) a spline can diverge from the line determined by its endpoints before it is changed to a straight line.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (line_threshold_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (line_threshold_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->line_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.5));


  filter_iteration_count_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->filter_iteration_count, 4, 70, 1, 0.01, 0)));
  gtk_widget_show (filter_iteration_count_scale);
  gtk_table_attach (GTK_TABLE (table1), filter_iteration_count_scale, 1, 2, 7, 8,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_iteration_count_scale, "Number of times to smooth original data points.  Increasing this number dramatically---to 50 or so---can produce vastly better results.  But if any points that ``should'' be corners aren't found, the curve goes to hell around that point.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (filter_iteration_count_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (filter_iteration_count_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->filter_iteration_count);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(4.0));


  filter_epsilon_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->filter_epsilon, 5, 40, 1, 1, 0)));
  gtk_widget_show (filter_epsilon_scale);
  gtk_table_attach (GTK_TABLE (table1), filter_epsilon_scale, 1, 2, 6, 7,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_epsilon_scale, "If the angles between the vectors produced by filter_surround and filter_alternative_surround points differ by more than this, use the one from filter_alternative_surround.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (filter_epsilon_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (filter_epsilon_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->filter_epsilon);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(10.0));


  subdivide_threshold = gtk_label_new ("subdivide_threshold: ");
  gtk_widget_show (subdivide_threshold);
  gtk_table_attach (GTK_TABLE (table1), subdivide_threshold, 0, 1, 19, 20,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (subdivide_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (subdivide_threshold), 0, 2);

  tangent_surround = gtk_label_new ("tangent_surround: ");
  gtk_widget_show (tangent_surround);
  gtk_table_attach (GTK_TABLE (table1), tangent_surround, 0, 1, 18, 19,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (tangent_surround), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (tangent_surround), 0, 2);

  subdivide_surround = gtk_label_new ("subdivide_surround: ");
  gtk_widget_show (subdivide_surround);
  gtk_table_attach (GTK_TABLE (table1), subdivide_surround, 0, 1, 17, 18,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (subdivide_surround), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (subdivide_surround), 0, 2);

  subdivide_search = gtk_label_new ("subdivide_search: ");
  gtk_widget_show (subdivide_search);
  gtk_table_attach (GTK_TABLE (table1), subdivide_search, 0, 1, 16, 17,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (subdivide_search), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (subdivide_search), 0, 2);

  reparameterize_threshold = gtk_label_new ("reparameterize_threshold: ");
  gtk_widget_show (reparameterize_threshold);
  gtk_table_attach (GTK_TABLE (table1), reparameterize_threshold, 0, 1, 15, 16,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (reparameterize_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (reparameterize_threshold), 0, 2);

  reparameterize_improvement = gtk_label_new ("reparameterize_improvement: ");
  gtk_widget_show (reparameterize_improvement);
  gtk_table_attach (GTK_TABLE (table1), reparameterize_improvement, 0, 1, 14, 15,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (reparameterize_improvement), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (reparameterize_improvement), 0, 2);

  line_threshold = gtk_label_new ("line_threshold: ");
  gtk_widget_show (line_threshold);
  gtk_table_attach (GTK_TABLE (table1), line_threshold, 0, 1, 13, 14,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (line_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (line_threshold), 0, 2);

  line_reversion_threshold = gtk_label_new ("line_reversion_threshold: ");
  gtk_widget_show (line_reversion_threshold);
  gtk_table_attach (GTK_TABLE (table1), line_reversion_threshold, 0, 1, 12, 13,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (line_reversion_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (line_reversion_threshold), 0, 2);

  keep_knees = gtk_label_new ("keep_knees: ");
  gtk_widget_show (keep_knees);
  gtk_table_attach (GTK_TABLE (table1), keep_knees, 0, 1, 11, 12,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (keep_knees), 1, 0.5);

  filter_surround = gtk_label_new ("filter_surround: ");
  gtk_widget_show (filter_surround);
  gtk_table_attach (GTK_TABLE (table1), filter_surround, 0, 1, 10, 11,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (filter_surround), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (filter_surround), 0, 2);

  filter_secondary_surround = gtk_label_new ("filter_secondary_surround: ");
  gtk_widget_show (filter_secondary_surround);
  gtk_table_attach (GTK_TABLE (table1), filter_secondary_surround, 0, 1, 9, 10,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (filter_secondary_surround), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (filter_secondary_surround), 0, 2);

  filter_percent = gtk_label_new ("filter_percent: ");
  gtk_widget_show (filter_percent);
  gtk_table_attach (GTK_TABLE (table1), filter_percent, 0, 1, 8, 9,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (filter_percent), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (filter_percent), 0, 2);

  filter_iteration_count = gtk_label_new ("filter_iteration_count: ");
  gtk_widget_show (filter_iteration_count);
  gtk_table_attach (GTK_TABLE (table1), filter_iteration_count, 0, 1, 7, 8,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (filter_iteration_count), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (filter_iteration_count), 0, 2);

  filter_epsilon = gtk_label_new ("filter_epsilon: ");
  gtk_widget_show (filter_epsilon);
  gtk_table_attach (GTK_TABLE (table1), filter_epsilon, 0, 1, 6, 7,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_epsilon, "If the angles between the vectors produced by filter_surround and\nfilter_alternative_surround points differ by more than this, use\nthe one from filter_alternative_surround.", NULL);
  gtk_label_set_justify (GTK_LABEL (filter_epsilon), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (filter_epsilon), 0.999999, 0.5);
  gtk_misc_set_padding (GTK_MISC (filter_epsilon), 0, 2);

  corner_always_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->corner_always_threshold, 30, 180, 1, 1, 0)));
  gtk_widget_show (corner_always_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), corner_always_threshold_scale, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, corner_always_threshold_scale, "If the angle defined by a point and its predecessors and successors is smaller than this, it's a corner, even if it's within `corner_surround' pixels of a point with a smaller angle.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (corner_always_threshold_scale), GTK_POS_LEFT);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->corner_always_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(60.0));


  error_threshold_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->error_threshold, 0.2, 10, 0.1, 0.1, 0)));
  gtk_widget_show (error_threshold_scale);
  gtk_table_attach (GTK_TABLE (table1), error_threshold_scale, 1, 2, 4, 5,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, error_threshold_scale, "Amount of error at which a fitted spline is unacceptable.  If any pixel is further away than this from the fitted curve, we try again.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (error_threshold_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (error_threshold_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->error_threshold);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(0.40));


  filter_alternative_surround_scale = gtk_hscale_new (GTK_ADJUSTMENT(size_data =gtk_adjustment_new (sels->filter_alternative_surround, 1, 10, 1, 1, 0)));
  gtk_widget_show (filter_alternative_surround_scale);
  gtk_table_attach (GTK_TABLE (table1), filter_alternative_surround_scale, 1, 2, 5, 6,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, filter_alternative_surround_scale, "A second number of adjacent points to consider when filtering.", NULL);
  gtk_scale_set_value_pos (GTK_SCALE (filter_alternative_surround_scale), GTK_POS_LEFT);
  gtk_scale_set_digits (GTK_SCALE (filter_alternative_surround_scale), 2);
  gtk_signal_connect(GTK_OBJECT(size_data), "value_changed",
		     (GtkSignalFunc) dialog_scale_update,
		     &sels->filter_alternative_surround);
  adjust_widgets = g_slist_append(adjust_widgets,size_data);
  gtk_object_set_data(GTK_OBJECT(size_data),"default_value",def_val(1.0));


  filter_alternative_surround = gtk_label_new ("filter_alternative_surround: ");
  gtk_widget_show (filter_alternative_surround);
  gtk_table_attach (GTK_TABLE (table1), filter_alternative_surround, 0, 1, 5, 6,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_label_set_justify (GTK_LABEL (filter_alternative_surround), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (filter_alternative_surround), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (filter_alternative_surround), 0, 2);

  error_threshold = gtk_label_new ("error_threshold: ");
  gtk_widget_show (error_threshold);
  gtk_table_attach (GTK_TABLE (table1), error_threshold, 0, 1, 4, 5,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (error_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (error_threshold), 0, 2);

  corner_threshold = gtk_label_new ("corner_threshold: ");
  gtk_widget_show (corner_threshold);
  gtk_table_attach (GTK_TABLE (table1), corner_threshold, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (corner_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (corner_threshold), 0, 2);

  corner_surround = gtk_label_new ("corner_surround: ");
  gtk_widget_show (corner_surround);
  gtk_table_attach (GTK_TABLE (table1), corner_surround, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (corner_surround), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (corner_surround), 0, 2);

  corner_always_threshold = gtk_label_new ("corner_always_threshold: ");
  gtk_widget_show (corner_always_threshold);
  gtk_table_attach (GTK_TABLE (table1), corner_always_threshold, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (corner_always_threshold), 1, 0.5);
  gtk_misc_set_padding (GTK_MISC (corner_always_threshold), 0, 2);


  return GTK_WIDGET(table1);
}
