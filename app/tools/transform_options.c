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

#include "gimptransformtool.h"
#include "transform_options.h"
#include "tool_manager.h"

#include "app_procs.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gimp_transform_tool_grid_density_update (GtkWidget *widget,
                                                       gpointer   data);
static void   gimp_transform_tool_show_grid_update    (GtkWidget *widget,
                                                       gpointer   data);
static void   gimp_transform_tool_show_path_update    (GtkWidget *widget,
                                                       gpointer   data);


/*  public functions  */

GimpToolOptions *
transform_options_new (GimpToolInfo *tool_info)
{
  TransformOptions *options;

  options = g_new (TransformOptions, 1);

  transform_options_init (options, tool_info);

  return (GimpToolOptions *) options;
}

void
transform_options_init (TransformOptions *options,
                        GimpToolInfo     *tool_info)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *fbox;
  GtkWidget *grid_density;

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = transform_options_reset;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  options->smoothing = options->smoothing_d = TRUE;
  options->show_path = options->show_path_d = TRUE;
  options->clip      = options->clip_d      = FALSE;
  options->direction = options->direction_d = GIMP_TRANSFORM_FORWARD;
  options->grid_size = options->grid_size_d = 32;
  options->show_grid = options->show_grid_d = TRUE;

  frame = gimp_radio_group_new2 (TRUE, _("Tool Paradigm"),
                                 G_CALLBACK (gimp_radio_button_update),
                                 &options->direction,
                                 GINT_TO_POINTER (options->direction),

                                 _("Traditional"),
                                 GINT_TO_POINTER (GIMP_TRANSFORM_FORWARD),
                                 &options->direction_w[0],

                                 _("Corrective"),
                                 GINT_TO_POINTER (GIMP_TRANSFORM_BACKWARD),
                                 &options->direction_w[1],

                                 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the grid frame  */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  fbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (fbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), fbox);
  gtk_widget_show (fbox);

  /*  the show grid toggle button  */
  options->show_grid_w = gtk_check_button_new_with_label (_("Show Grid"));
  g_signal_connect (G_OBJECT (options->show_grid_w), "toggled",
                    G_CALLBACK (gimp_transform_tool_show_grid_update),
                    &options->show_grid);
  gtk_frame_set_label_widget (GTK_FRAME (frame), options->show_grid_w);
  gtk_widget_show (options->show_grid_w);
  
  /*  the grid density entry  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (fbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Density:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->grid_size_w =
    gtk_adjustment_new (7.0 - log (options->grid_size_d) / log (2.0), 0.0, 5.0,
			1.0, 1.0, 0.0);
  grid_density =
    gtk_spin_button_new (GTK_ADJUSTMENT (options->grid_size_w), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (grid_density), TRUE);
  g_signal_connect (G_OBJECT (options->grid_size_w), "value_changed",
                    G_CALLBACK (gimp_transform_tool_grid_density_update),
                    options);
  gtk_box_pack_start (GTK_BOX (hbox), grid_density, FALSE, FALSE, 0);
  gtk_widget_show (grid_density);

  gtk_widget_set_sensitive (label, options->show_grid_d);
  gtk_widget_set_sensitive (grid_density, options->show_grid_d);
  g_object_set_data (G_OBJECT (options->show_grid_w), "set_sensitive",
                     grid_density);
  g_object_set_data (G_OBJECT (grid_density), "set_sensitive", label);  

  /*  the show_path toggle button  */
  options->show_path_w = gtk_check_button_new_with_label (_("Show Path"));
  g_signal_connect (G_OBJECT (options->show_path_w), "toggled",
                    G_CALLBACK (gimp_transform_tool_show_path_update),
                    &options->show_path);
  gtk_box_pack_start (GTK_BOX (vbox), options->show_path_w, FALSE, FALSE, 0);
  gtk_widget_show (options->show_path_w);

  /*  the smoothing toggle button  */
  options->smoothing_w = gtk_check_button_new_with_label (_("Smoothing"));
  g_signal_connect (G_OBJECT (options->smoothing_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->smoothing);
  gtk_box_pack_start (GTK_BOX (vbox), options->smoothing_w, FALSE, FALSE, 0);
  gtk_widget_show (options->smoothing_w);

  /*  the clip resulting image toggle button  */
  options->clip_w = gtk_check_button_new_with_label (_("Clip Result"));
  g_signal_connect (G_OBJECT (options->clip_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->clip);
  gtk_box_pack_start (GTK_BOX (vbox), options->clip_w, FALSE, FALSE, 0);
  gtk_widget_show (options->clip_w);

  /* Set options to default values */
  transform_options_reset ((GimpToolOptions *) options);
}

void
transform_options_reset (GimpToolOptions *tool_options)
{
  TransformOptions *options;

  options = (TransformOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->smoothing_w),
				options->smoothing_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_path_w),
				options->show_path_d);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->direction_w[0]),
                               GINT_TO_POINTER (options->direction_d));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_grid_w),
				options->show_grid_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->grid_size_w),
			    7.0 - log (options->grid_size_d) / log (2.0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->clip_w),
				options->clip_d);
}


/*  private functions  */

static void
gimp_transform_tool_grid_density_update (GtkWidget *widget,
                                         gpointer   data)
{
  TransformOptions *options;
  GimpTool         *active_tool;

  options = (TransformOptions *) data;

  options->grid_size =
    (gint) (pow (2.0, 7.0 - GTK_ADJUSTMENT (widget)->value) + 0.5);

  active_tool = tool_manager_get_active (the_gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    gimp_transform_tool_grid_density_changed (GIMP_TRANSFORM_TOOL (active_tool));
}

static void
gimp_transform_tool_show_grid_update (GtkWidget *widget,
        	 		      gpointer   data)
{
  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfault */

  GimpTool *active_tool;

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  gimp_toggle_button_update (widget, data);

  active_tool = tool_manager_get_active (the_gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    gimp_transform_tool_grid_density_changed (GIMP_TRANSFORM_TOOL (active_tool));
}

static void
gimp_transform_tool_show_path_update (GtkWidget *widget,
				      gpointer   data)
{
  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfault */

  GimpTool          *active_tool;
  GimpTransformTool *transform_tool = NULL;

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  active_tool = tool_manager_get_active (the_gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    transform_tool = GIMP_TRANSFORM_TOOL (active_tool);

  if (transform_tool)
    gimp_transform_tool_show_path_changed (transform_tool, 1); /* pause */

  gimp_toggle_button_update (widget, data);

  if (transform_tool)
    gimp_transform_tool_show_path_changed (transform_tool, 0); /* resume */
}
