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
#include "selection_options.h"
#include "tool_options_ui.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpunitmenu.h"


/*  ui helper functions  */

void
tool_options_toggle_update (GtkWidget *widget,
			    gpointer   data)
{
  GtkWidget *set_sensitive;
  int       *toggle_val;

  toggle_val = (int *) data;
  *toggle_val = (GTK_TOGGLE_BUTTON (widget)->active) ? TRUE : FALSE;

  /*  a tool options toggle button can set the sensitive state of
   *  an arbitrary list of widgets
   */
  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (widget), "set_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (set_sensitive), *toggle_val);
      set_sensitive =
	gtk_object_get_data (GTK_OBJECT (set_sensitive), "set_sensitive");
    }
}

void
tool_options_int_adjustment_update (GtkWidget *widget,
				    gpointer   data)
{
  int *val;

  val = (int *) data;
  *val = GTK_ADJUSTMENT (widget)->value;
}

void
tool_options_double_adjustment_update (GtkWidget *widget,
				       gpointer   data)
{
  double *val;

  val = (double *) data;
  *val = GTK_ADJUSTMENT (widget)->value;
}

void
tool_options_unitmenu_update (GtkWidget *widget,
			      gpointer   data)
{
  GUnit     *val;
  GtkWidget *spinbutton;
  int        digits;

  val = (GUnit *) data;
  *val = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  digits = ((*val == UNIT_PIXEL) ? 0 :
	    ((*val == UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (*val))))));

  spinbutton =
    gtk_object_get_data (GTK_OBJECT (widget), "set_digits");
  while (spinbutton)
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
      spinbutton = gtk_object_get_data (GTK_OBJECT (spinbutton), "set_digits");
    }
}


/*  tool options functions  */

void
tool_options_init (ToolOptions          *options,
		   gchar                *title,
		   ToolOptionsResetFunc  reset_func)
{
  options->main_vbox  = gtk_vbox_new (FALSE, 2);
  options->title = title;
  options->reset_func = reset_func;
}

ToolOptions *
tool_options_new (gchar *title)
{
  ToolOptions *options;

  GtkWidget *label;

  options = (ToolOptions *) g_malloc (sizeof (ToolOptions));
  tool_options_init (options, title, NULL);

  label = gtk_label_new (_("This tool has no options."));
  gtk_box_pack_start (GTK_BOX (options->main_vbox), label, FALSE, FALSE, 6);
  gtk_widget_show (label);

  return options;
}

/*  selection tool options functions  */

void
selection_options_init (SelectionOptions     *options,
			ToolType              tool_type,
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
		     ((tool_type == RECT_SELECT) ?
		      _("Rectangular Select Options") :
		      ((tool_type == ELLIPSE_SELECT) ?
		       _("Elliptical Selection Options") :
		       ((tool_type == FREE_SELECT) ?
			_("Free-hand Selection Options") :
			((tool_type == FUZZY_SELECT) ?
			 _("Fuzzy Selection Options") :
			 ((tool_type == BEZIER_SELECT) ?
			  _("Bezier Selection Options") :
			  ((tool_type == ISCISSORS) ?
			   _("Intelligent Scissors Options") :
			   ((tool_type == BY_COLOR_SELECT) ?
			    _("By-Color Select Options") :
			    _("Unknown Selection Type ???")))))))),
		     reset_func);

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  initialize the selection options structure  */
  options->feather        = options->feather_d        = FALSE;
  options->feather_radius = options->feather_radius_d = 10.0;
  options->antialias      = options->antialias_d      = TRUE;
  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->fixed_size     = options->fixed_size_d     = FALSE;
  options->fixed_height   = options->fixed_height_d   = 1;
  options->fixed_width    = options->fixed_width_d    = 1;
  options->fixed_unit     = options->fixed_unit_d     = UNIT_PIXEL;

  options->feather_w        = NULL;
  options->feather_radius_w = NULL;
  options->antialias_w      = NULL;
  options->sample_merged_w  = NULL;
  options->fixed_size_w     = NULL;
  options->fixed_height_w   = NULL;
  options->fixed_width_w    = NULL;
  options->fixed_unit_w     = NULL;

  /*  the feather toggle button  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->feather_w = gtk_check_button_new_with_label (_("Feather"));
  gtk_table_attach (GTK_TABLE (table), options->feather_w, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
				options->feather_d);
  gtk_signal_connect (GTK_OBJECT (options->feather_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
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
    gtk_adjustment_new (options->feather_radius_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->feather_radius_w));
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->feather_radius_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->feather_radius);
  gtk_widget_show (scale);

  /*  grey out label & scale if feather is off  */
  gtk_widget_set_sensitive (scale, options->feather_d);
  gtk_widget_set_sensitive (label, options->feather_d);
  gtk_object_set_data (GTK_OBJECT (options->feather_w), "set_sensitive", scale);
  gtk_object_set_data (GTK_OBJECT (scale), "set_sensitive", label);

  gtk_widget_show (table);

  /*  the antialias toggle button  */
  if (tool_type != RECT_SELECT)
    {
      options->antialias_w = gtk_check_button_new_with_label (_("Antialiasing"));
      gtk_box_pack_start (GTK_BOX (vbox), options->antialias_w, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				    options->antialias_d);
      gtk_signal_connect (GTK_OBJECT (options->antialias_w), "toggled",
			  (GtkSignalFunc) tool_options_toggle_update,
			  &options->antialias);
      gtk_widget_show (options->antialias_w);
    }

  /*  a separator between the common and tool-specific selection options  */
  switch (tool_type)
    {
    case FREE_SELECT:
    case BEZIER_SELECT:
      break;
    case RECT_SELECT:
    case ELLIPSE_SELECT:
    case FUZZY_SELECT:
    case ISCISSORS:
    case BY_COLOR_SELECT:
      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
      break;
    default:
      break;
    }

  /*  the sample merged option  */
  switch (tool_type)
    {
    case RECT_SELECT:
    case ELLIPSE_SELECT:
    case FREE_SELECT:
    case BEZIER_SELECT:
    case ISCISSORS:
      break;
    case FUZZY_SELECT:
    case BY_COLOR_SELECT:
      options->sample_merged_w =
	gtk_check_button_new_with_label (_("Sample Merged"));
      gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
			  (GtkSignalFunc) tool_options_toggle_update,
			  &options->sample_merged);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				    options->sample_merged_d);
      gtk_widget_show (options->sample_merged_w);
      break;
    default:
      break;
    }

  /*  widgets for fixed size select  */
  if (tool_type == RECT_SELECT || tool_type == ELLIPSE_SELECT) 
    {
      GtkWidget *alignment;
      GtkWidget *table;
      GtkWidget *width_spinbutton;
      GtkWidget *height_spinbutton;

      options->fixed_size_w =
	gtk_check_button_new_with_label (_("Fixed size / aspect ratio"));
      gtk_box_pack_start (GTK_BOX (vbox), options->fixed_size_w,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_size_w), "toggled",
			  (GtkSignalFunc) tool_options_toggle_update,
			  &options->fixed_size);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options->fixed_size_w),
				   options->fixed_size_d);
      gtk_widget_show(options->fixed_size_w);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
      gtk_widget_show (alignment);

      table = gtk_table_new (3, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_container_add (GTK_CONTAINER (alignment), table);

      /*  grey out the table if fixed size is off  */
      gtk_widget_set_sensitive (table, options->fixed_size_d);
      gtk_object_set_data (GTK_OBJECT (options->fixed_size_w), "set_sensitive",
			   table);

      label = gtk_label_new (_("Width:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

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
                          (GtkSignalFunc) tool_options_int_adjustment_update,
                          &options->fixed_width);
      gtk_table_attach (GTK_TABLE (table), width_spinbutton, 1, 2, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_widget_show (width_spinbutton);

      label = gtk_label_new (_("Height:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

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
                          (GtkSignalFunc) tool_options_int_adjustment_update,
                          &options->fixed_height);
      gtk_table_attach (GTK_TABLE (table), height_spinbutton, 1, 2, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_widget_show (height_spinbutton);

      label = gtk_label_new (_("Unit:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      options->fixed_unit_w =
	gimp_unit_menu_new ("%a", options->fixed_unit_d, TRUE, TRUE, TRUE);
      gtk_signal_connect (GTK_OBJECT (options->fixed_unit_w), "unit_changed",
                          (GtkSignalFunc) tool_options_unitmenu_update,
                          &options->fixed_unit);
      gtk_object_set_data (GTK_OBJECT (options->fixed_unit_w), "set_digits",
			   width_spinbutton);
      gtk_object_set_data (GTK_OBJECT (width_spinbutton), "set_digits",
			   height_spinbutton);
      gtk_table_attach (GTK_TABLE (table), options->fixed_unit_w, 1, 2, 2, 3,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_widget_show (options->fixed_unit_w);

      gtk_widget_show (table);
    }
}

SelectionOptions *
selection_options_new (ToolType              tool_type,
		       ToolOptionsResetFunc  reset_func)
{
  SelectionOptions *options;

  options = (SelectionOptions *) g_malloc (sizeof (SelectionOptions));
  selection_options_init (options, tool_type, reset_func);

  return options;
}

void
selection_options_reset (SelectionOptions *options)
{
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
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				  options->sample_merged_d);
  if (options->fixed_size_w)
    {
      GtkWidget *spinbutton;
      int        digits;

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
	((options->fixed_unit_d == UNIT_PIXEL) ? 0 :
	 ((options->fixed_unit_d == UNIT_PERCENT) ? 2 :
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
}
