/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "gimprc.h"

#include "gimpellipseselecttool.h"
#include "gimpfuzzyselecttool.h"
#include "gimpiscissorstool.h"
#include "gimprectselecttool.h"
#include "gimptool.h"
#include "selection_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


void
selection_options_init (SelectionOptions     *options,
			GtkType               tool_type,
			ToolOptionsResetFunc  reset_func)
{
  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *separator;

  /*  initialize the tool options structure  */
  tool_options_init ((ToolOptions *) options,
		     reset_func);

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  initialize the selection options structure  */
  options->feather        = options->feather_d        = FALSE;
  options->feather_radius = options->feather_radius_d = 10.0;
  options->antialias      = options->antialias_d      = TRUE;
  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->threshold                                  = default_threshold;
  options->fixed_size     = options->fixed_size_d     = FALSE;
  options->fixed_height   = options->fixed_height_d   = 1;
  options->fixed_width    = options->fixed_width_d    = 1;
  options->fixed_unit     = options->fixed_unit_d     = GIMP_UNIT_PIXEL;
  options->interactive    = options->interactive_d    = FALSE;

  options->feather_w        = NULL;
  options->feather_radius_w = NULL;
  options->antialias_w      = NULL;
  options->sample_merged_w  = NULL;
  options->threshold_w      = NULL;
  options->fixed_size_w     = NULL;
  options->fixed_height_w   = NULL;
  options->fixed_width_w    = NULL;
  options->fixed_unit_w     = NULL;
  options->interactive_w    = NULL;

  /*  the feather toggle button  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->feather_w = gtk_check_button_new_with_label (_("Feather"));
  gtk_table_attach (GTK_TABLE (table), options->feather_w, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
				options->feather_d);
  gtk_signal_connect (GTK_OBJECT (options->feather_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->feather);
  gtk_widget_show (options->feather_w);

  /*  the feather radius scale  */
  label = gtk_label_new (_("Radius:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 0, 2,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->feather_radius_w =
    gtk_adjustment_new (options->feather_radius_d, 0.0, 100.0, 1.0, 1.0, 1.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->feather_radius_w));
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->feather_radius_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->feather_radius);
  gtk_widget_show (scale);

  /*  grey out label & scale if feather is off  */
  gtk_widget_set_sensitive (scale, options->feather_d);
  gtk_widget_set_sensitive (label, options->feather_d);
  gtk_object_set_data (GTK_OBJECT (options->feather_w), "set_sensitive", scale);
  gtk_object_set_data (GTK_OBJECT (scale), "set_sensitive", label);

  gtk_widget_show (table);

  /*  the antialias toggle button  */
  if (tool_type != GIMP_TYPE_RECT_SELECT_TOOL)
    {
      options->antialias_w = gtk_check_button_new_with_label (_("Antialiasing"));
      gtk_box_pack_start (GTK_BOX (vbox), options->antialias_w, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				    options->antialias_d);
      gtk_signal_connect (GTK_OBJECT (options->antialias_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &options->antialias);
      gtk_widget_show (options->antialias_w);
    }

  /*  a separator between the common and tool-specific selection options  */
  if (tool_type == GIMP_TYPE_ISCISSORS_TOOL      ||
      tool_type == GIMP_TYPE_RECT_SELECT_TOOL    ||
      tool_type == GIMP_TYPE_ELLIPSE_SELECT_TOOL ||
      tool_type == GIMP_TYPE_FUZZY_SELECT_TOOL   ||
      tool_type == GIMP_TYPE_BY_COLOR_SELECT_TOOL)
    {
      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
    }

  /* selection tool with an interactive boundary that can be toggled */
  if (tool_type == GIMP_TYPE_ISCISSORS_TOOL)
    {
      options->interactive_w =
	gtk_check_button_new_with_label (_("Show Interactive Boundary"));

      gtk_box_pack_start (GTK_BOX (vbox), options->interactive_w,
			  FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->interactive_w),
				    options->interactive_d);
      gtk_signal_connect (GTK_OBJECT (options->interactive_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &options->interactive);
      gtk_widget_show (options->interactive_w);
    }

  /*  selection tools which operate on contiguous regions  */
  if (tool_type == GIMP_TYPE_FUZZY_SELECT_TOOL)
    {
      GtkWidget *hbox;

      /*  the sample merged toggle  */
      options->sample_merged_w =
	gtk_check_button_new_with_label (_("Sample Merged"));
      gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &options->sample_merged);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				    options->sample_merged_d);
      gtk_widget_show (options->sample_merged_w);

      /*  the threshold scale  */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
  
      label = gtk_label_new (_("Threshold:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
      gtk_widget_show (label);

      options->threshold_w = 
	gtk_adjustment_new (default_threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
      scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
      gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
      gtk_signal_connect (GTK_OBJECT (options->threshold_w), "value_changed",
			  GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
			  &options->threshold);
      gtk_widget_show (scale);
    }

  /*  widgets for fixed size select  */
  if (tool_type == GIMP_TYPE_RECT_SELECT_TOOL    ||
      tool_type == GIMP_TYPE_ELLIPSE_SELECT_TOOL)
    {
      GtkWidget *alignment;
      GtkWidget *table;
      GtkWidget *width_spinbutton;
      GtkWidget *height_spinbutton;

      options->fixed_size_w =
	gtk_check_button_new_with_label (_("Fixed Size / Aspect Ratio"));
      gtk_box_pack_start (GTK_BOX (vbox), options->fixed_size_w,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_size_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &options->fixed_size);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fixed_size_w),
				    options->fixed_size_d);
      gtk_widget_show (options->fixed_size_w);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
      gtk_widget_show (alignment);

      table = gtk_table_new (3, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_container_add (GTK_CONTAINER (alignment), table);

      /*  grey out the table if fixed size is off  */
      gtk_widget_set_sensitive (table, options->fixed_size_d);
      gtk_object_set_data (GTK_OBJECT (options->fixed_size_w), "set_sensitive",
			   table);

      options->fixed_width_w =
	gtk_adjustment_new (options->fixed_width_d, 1e-5, 32767.0,
			    1.0, 50.0, 0.0);
      width_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (options->fixed_width_w), 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(width_spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width_spinbutton), TRUE);
      gtk_widget_set_usize (width_spinbutton, 75, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_width_w), "value_changed",
                          GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                          &options->fixed_width);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Width:"), 1.0, 0.5,
				 width_spinbutton, 1, FALSE);

      options->fixed_height_w =
	gtk_adjustment_new (options->fixed_height_d, 1e-5, 32767.0,
			    1.0, 50.0, 0.0);
      height_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (options->fixed_height_w), 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(height_spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(height_spinbutton), TRUE);
      gtk_widget_set_usize (height_spinbutton, 75, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_height_w), "value_changed",
                          GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                          &options->fixed_height);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Height:"), 1.0, 0.5,
				 height_spinbutton, 1, FALSE);

      options->fixed_unit_w =
	gimp_unit_menu_new ("%a", options->fixed_unit_d, TRUE, TRUE, TRUE);
      gtk_signal_connect (GTK_OBJECT (options->fixed_unit_w), "unit_changed",
                          GTK_SIGNAL_FUNC (gimp_unit_menu_update),
                          &options->fixed_unit);
      gtk_object_set_data (GTK_OBJECT (options->fixed_unit_w), "set_digits",
			   width_spinbutton);
      gtk_object_set_data (GTK_OBJECT (width_spinbutton), "set_digits",
			   height_spinbutton);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
				 _("Unit:"), 1.0, 0.5,
				 options->fixed_unit_w, 1, FALSE);

      gtk_widget_show (table);
    }
}

SelectionOptions *
selection_options_new (GtkType               tool_type,
		       ToolOptionsResetFunc  reset_func)
{
  SelectionOptions *options;

  options = g_new (SelectionOptions, 1);
  selection_options_init (options, tool_type, reset_func);

  return options;
}

void
selection_options_reset (ToolOptions *tool_options)
{
  SelectionOptions *options;

  options = (SelectionOptions *) tool_options;

  if (options->feather_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
				    options->feather_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->feather_radius_w),
				options->feather_radius_d);
    }

  if (options->antialias_w)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				  options->antialias_d);

  if (options->sample_merged_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				    options->sample_merged_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
				default_threshold);
    }

  if (options->fixed_size_w)
    {
      GtkWidget *spinbutton;
      gint       digits;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(options->fixed_size_w),
				    options->fixed_size_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fixed_width_w),
				options->fixed_width_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fixed_height_w),
				options->fixed_height_d);

      options->fixed_unit = options->fixed_unit_d;
      gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->fixed_unit_w),
			       options->fixed_unit_d);

      digits =
	((options->fixed_unit_d == GIMP_UNIT_PIXEL) ? 0 :
	 ((options->fixed_unit_d == GIMP_UNIT_PERCENT) ? 2 :
	  (MIN (6, MAX (3, gimp_unit_get_digits (options->fixed_unit_d))))));

      spinbutton =
	gtk_object_get_data (GTK_OBJECT (options->fixed_unit_w), "set_digits");
      while (spinbutton)
	{
	  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
	  spinbutton =
	    gtk_object_get_data (GTK_OBJECT (spinbutton), "set_digits");
	}
    }

  if (options->interactive_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->interactive_w),
				    options->interactive_d);
    }
}
