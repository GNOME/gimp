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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "gimprc.h"

#include "gimptool.h"
#include "gimptransformtool.h"
#include "transform_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


static TransformOptions  *transform_options     = NULL;

/* Callback functions - need prototypes */

static void
gimp_transform_tool_direction_callback (GtkWidget *widget,
			                gpointer   data)
{
  long dir = (long) data;

  if (dir == TRANSFORM_TRADITIONAL)
    transform_options->direction = TRANSFORM_TRADITIONAL;
  else
    transform_options->direction = TRANSFORM_CORRECTIVE;
}

static void
gimp_transform_tool_grid_density_callback (GtkWidget *widget,
				           gpointer   data)
{
  transform_options->grid_size =
    (int) (pow (2.0, 7.0 - GTK_ADJUSTMENT (widget)->value) + 0.5);

  gimp_transform_tool_grid_density_changed ();
}

static void
gimp_transform_tool_show_grid_update (GtkWidget *widget,
        	 		      gpointer   data)
{
  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfault */

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  gimp_toggle_button_update (widget, data);

  gimp_transform_tool_grid_density_changed ();
}

static void
gimp_transform_tool_show_path_update (GtkWidget *widget,
				      gpointer   data)
{
  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfault */

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  gimp_transform_tool_showpath_changed (1); /* pause */
  gimp_toggle_button_update (widget, data);
  gimp_transform_tool_showpath_changed (0); /* resume */
}

/* Global functions. No magic needed yet. */

/* Unhappy with this - making this stuff global is wrong. */

gboolean
gimp_transform_tool_smoothing (void)
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->smoothing;
}

gboolean
gimp_transform_tool_showpath (void)
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->showpath;
}

gboolean
gimp_transform_tool_clip (void)
{
  if (!transform_options)
    return FALSE;
  else
    return transform_options->clip;
}

gint
gimp_transform_tool_direction (void)
{
  if (!transform_options)
    return TRANSFORM_TRADITIONAL;
  else
    return transform_options->direction;
}

gint
gimp_transform_tool_grid_size (void)
{
  if (!transform_options)
    return 32;
  else
    return transform_options->grid_size;
}

gboolean
gimp_transform_tool_show_grid (void)
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->show_grid;
}

/* Main options stuff - the rest doesn't need to be here (default 
 * callbacks is good, but none of this stuff should depend on having 
 * access to the options)
 */

void
transform_options_reset (ToolOptions *tool_options)
{
  TransformOptions *options;

  options = (TransformOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->smoothing_w),
				options->smoothing_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->showpath_w),
				options->showpath_d);
  gtk_toggle_button_set_active (((options->direction_d == TRANSFORM_TRADITIONAL) ?
				 GTK_TOGGLE_BUTTON (options->direction_w[0]) :
				 GTK_TOGGLE_BUTTON (options->direction_w[1])),
				TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_grid_w),
				options->show_grid_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->grid_size_w),
			    7.0 - log (options->grid_size_d) / log (2.0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->clip_w),
				options->clip_d);
}

void
transform_options_init (TransformOptions     *options,
			GtkType               tool_type,
			ToolOptionsResetFunc  reset_func)
{

  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *fbox;
  GtkWidget *grid_density;

  tool_options_init ((ToolOptions *) options,
		     reset_func);

  options->smoothing = options->smoothing_d = TRUE;
  options->showpath  = options->showpath_d  = TRUE;
  options->clip      = options->clip_d      = FALSE;
  options->direction = options->direction_d = TRANSFORM_TRADITIONAL;
  options->grid_size = options->grid_size_d = 32;
  options->show_grid = options->show_grid_d = TRUE;


  /*  the top hbox (containing two frames) */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox), hbox2,
		      FALSE, FALSE, 0);

  /*  the second radio frame and box, for transform direction  */
  frame = gimp_radio_group_new (TRUE, _("Tool Paradigm"),

				_("Traditional"), gimp_transform_tool_direction_callback,
				TRANSFORM_TRADITIONAL, NULL,
				&options->direction_w[0], TRUE,

				_("Corrective"), gimp_transform_tool_direction_callback,
				TRANSFORM_CORRECTIVE, NULL,
				&options->direction_w[1], FALSE,

				NULL);

  gtk_box_pack_start (GTK_BOX (hbox2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the grid frame  */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox2), frame, FALSE, FALSE, 0);

  fbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (fbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), fbox);

  /*  the show grid toggle button  */
  options->show_grid_w = gtk_check_button_new_with_label (_("Show Grid"));
  gtk_signal_connect (GTK_OBJECT (options->show_grid_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_transform_tool_show_grid_update),
		      &options->show_grid);
  gtk_box_pack_start (GTK_BOX (fbox), options->show_grid_w, FALSE, FALSE, 0);
  gtk_widget_show (options->show_grid_w);
  
  /*  the grid density entry  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (fbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Density:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->grid_size_w =
    gtk_adjustment_new (7.0 - log (options->grid_size_d) / log (2.0), 0.0, 5.0,
			1.0, 1.0, 0.0);
  grid_density =
    gtk_spin_button_new (GTK_ADJUSTMENT (options->grid_size_w), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (grid_density), TRUE);
  gtk_signal_connect (GTK_OBJECT (options->grid_size_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_transform_tool_grid_density_callback),
		      &options->grid_size);
  gtk_box_pack_start (GTK_BOX (hbox), grid_density, FALSE, FALSE, 0);
  gtk_widget_show (grid_density);
  gtk_widget_set_sensitive (label, options->show_grid_d);
  gtk_widget_set_sensitive (grid_density, options->show_grid_d);
  gtk_object_set_data (GTK_OBJECT (options->show_grid_w), "set_sensitive",
		       grid_density);
  gtk_object_set_data (GTK_OBJECT (grid_density), "set_sensitive", label);  

  gtk_widget_show (fbox);
  gtk_widget_show (frame);

  gtk_widget_show (hbox2);

  /* the main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox), table,
		      FALSE, FALSE, 0);

  /*  the smoothing toggle button  */
  options->smoothing_w = gtk_check_button_new_with_label (_("Smoothing"));
  gtk_signal_connect (GTK_OBJECT (options->smoothing_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->smoothing);
  gtk_table_attach_defaults (GTK_TABLE (table), 
			     options->smoothing_w, 0, 1, 0, 1);
  gtk_widget_show (options->smoothing_w);

  /*  the showpath toggle button  */
  options->showpath_w = gtk_check_button_new_with_label (_("Show Path"));
  gtk_signal_connect (GTK_OBJECT (options->showpath_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_transform_tool_show_path_update),
		      &options->showpath);
  gtk_table_attach_defaults (GTK_TABLE (table), 
			     options->showpath_w, 1, 2, 0, 1);
  gtk_widget_show (options->showpath_w);

  gtk_widget_show (table);

  /*  the clip resulting image toggle button  */
  options->clip_w = gtk_check_button_new_with_label (_("Clip Result"));
  gtk_signal_connect (GTK_OBJECT (options->clip_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->clip);
  gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox), 
		      options->clip_w, FALSE, FALSE, 0);
  gtk_widget_show (options->clip_w);

  /* Set options to default values */
  transform_options_reset((ToolOptions *) options);
}

TransformOptions *
transform_options_new (GtkType               tool_type,
		       ToolOptionsResetFunc  reset_func)
{
  TransformOptions *options;

  options = g_new (TransformOptions, 1);
  transform_options_init (options, tool_type, reset_func);

  /* Set our local copy of the active tool's options (so that 
   * the methods/functions to access them work) */
  transform_options = options;

  return options;
}



