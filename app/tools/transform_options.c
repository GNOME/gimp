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

#include "core/gimp.h"
#include "core/gimpcoreconfig.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpenummenu.h"

#include "gimprotatetool.h"
#include "gimpscaletool.h"
#include "gimptransformtool.h"
#include "transform_options.h"
#include "tool_manager.h"

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

  options = g_new0 (TransformOptions, 1);

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
  GtkWidget *vbox2;
  GtkWidget *fbox;
  GtkWidget *grid_density;

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = transform_options_reset;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  options->direction     = options->direction_d   = GIMP_TRANSFORM_FORWARD;
  options->interpolation = tool_info->gimp->config->interpolation_type;
  options->show_path     = options->show_path_d   = TRUE;
  options->clip          = options->clip_d        = FALSE;
  options->grid_size     = options->grid_size_d   = 32;
  options->show_grid     = options->show_grid_d   = TRUE;
  options->constrain_1   = options->constrain_1_d = FALSE;
  options->constrain_2   = options->constrain_2_d = FALSE;

  frame = gimp_enum_radio_frame_new (GIMP_TYPE_TRANSFORM_DIRECTION,
                                     gtk_label_new (_("Transform Direction")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->direction,
                                     &options->direction_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->direction_w),
                               GINT_TO_POINTER (options->direction));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the interpolation menu  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Interpolation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->interpolation_w = 
    gimp_enum_option_menu_new (GIMP_TYPE_INTERPOLATION_TYPE,
                               G_CALLBACK (gimp_menu_item_update),
                               &options->interpolation);
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->interpolation_w),
                                GINT_TO_POINTER (options->interpolation));
  gtk_box_pack_start (GTK_BOX (hbox), 
                      options->interpolation_w, FALSE, FALSE, 0);
  gtk_widget_show (options->interpolation_w);

  /*  the clip resulting image toggle button  */
  options->clip_w = gtk_check_button_new_with_label (_("Clip Result"));
  gtk_box_pack_start (GTK_BOX (vbox), options->clip_w, FALSE, FALSE, 0);
  gtk_widget_show (options->clip_w);

  g_signal_connect (G_OBJECT (options->clip_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->clip);

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
  gtk_frame_set_label_widget (GTK_FRAME (frame), options->show_grid_w);
  gtk_widget_show (options->show_grid_w);
  
  g_signal_connect (G_OBJECT (options->show_grid_w), "toggled",
                    G_CALLBACK (gimp_transform_tool_show_grid_update),
                    options);

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
  gtk_box_pack_start (GTK_BOX (hbox), grid_density, FALSE, FALSE, 0);
  gtk_widget_show (grid_density);

  g_signal_connect (G_OBJECT (options->grid_size_w), "value_changed",
                    G_CALLBACK (gimp_transform_tool_grid_density_update),
                    options);

  gtk_widget_set_sensitive (label, options->show_grid_d);
  gtk_widget_set_sensitive (grid_density, options->show_grid_d);
  g_object_set_data (G_OBJECT (options->show_grid_w), "set_sensitive",
                     grid_density);
  g_object_set_data (G_OBJECT (grid_density), "set_sensitive", label);  

  /*  the show_path toggle button  */
  options->show_path_w = gtk_check_button_new_with_label (_("Show Path"));
  gtk_box_pack_start (GTK_BOX (vbox), options->show_path_w, FALSE, FALSE, 0);
  gtk_widget_show (options->show_path_w);

  g_signal_connect (G_OBJECT (options->show_path_w), "toggled",
                    G_CALLBACK (gimp_transform_tool_show_path_update),
                    options);

  if (tool_info->tool_type == GIMP_TYPE_ROTATE_TOOL ||
      tool_info->tool_type == GIMP_TYPE_SCALE_TOOL)
    {
      /*  the constraints frame  */
      frame = gtk_frame_new (_("Constraints"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox2 = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      if (tool_info->tool_type == GIMP_TYPE_ROTATE_TOOL)
        {
          options->constrain_1_w =
            gtk_check_button_new_with_label (_("15 Degrees (<Ctrl>)"));
          gtk_box_pack_start (GTK_BOX (vbox2), options->constrain_1_w,
                              FALSE, FALSE, 0);
          gtk_widget_show (options->constrain_1_w);

          g_signal_connect (G_OBJECT (options->constrain_1_w), "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &options->constrain_1);
        }
      else if (tool_info->tool_type == GIMP_TYPE_SCALE_TOOL)
        {
          options->constrain_1_w =
            gtk_check_button_new_with_label (_("Keep Height (<Ctrl>)"));
          gtk_box_pack_start (GTK_BOX (vbox2), options->constrain_1_w,
                              FALSE, FALSE, 0);
          gtk_widget_show (options->constrain_1_w);

          gimp_help_set_help_data (options->constrain_1_w,
                                   _("Activate both the \"Keep Height\" and\n"
                                     "\"Keep Width\" toggles to constrain\n"
                                     "the aspect ratio"), NULL);

          g_signal_connect (G_OBJECT (options->constrain_1_w), "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &options->constrain_1);

          options->constrain_2_w =
            gtk_check_button_new_with_label (_("Keep Width (<Alt>)"));
          gtk_box_pack_start (GTK_BOX (vbox2), options->constrain_2_w,
                              FALSE, FALSE, 0);
          gtk_widget_show (options->constrain_2_w);

          gimp_help_set_help_data (options->constrain_2_w,
                                   _("Activate both the \"Keep Height\" and\n"
                                     "\"Keep Width\" toggles to constrain\n"
                                     "the aspect ratio"), NULL);

          g_signal_connect (G_OBJECT (options->constrain_2_w), "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &options->constrain_2);
        }
    }

  /* Set options to default values */
  transform_options_reset ((GimpToolOptions *) options);
}

void
transform_options_reset (GimpToolOptions *tool_options)
{
  TransformOptions *options;

  options = (TransformOptions *) tool_options;

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->direction_w),
                               GINT_TO_POINTER (options->direction_d));

  options->interpolation =
    tool_options->tool_info->gimp->config->interpolation_type;
  gimp_option_menu_set_history (GTK_OPTION_MENU (options->interpolation_w),
				GINT_TO_POINTER (options->interpolation));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->clip_w),
				options->clip_d);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_grid_w),
				options->show_grid_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->grid_size_w),
			    7.0 - log (options->grid_size_d) / log (2.0));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_path_w),
				options->show_path_d);

  if (options->constrain_1_w)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->constrain_1_w),
                                  options->constrain_1_d);

  if (options->constrain_2_w)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->constrain_2_w),
                                  options->constrain_2_d);
}


/*  private functions  */

static void
gimp_transform_tool_grid_density_update (GtkWidget *widget,
                                         gpointer   data)
{
  TransformOptions *options;
  GimpToolOptions  *tool_options;
  GimpTool         *active_tool;

  options      = (TransformOptions *) data;
  tool_options = (GimpToolOptions *) data;

  options->grid_size =
    (gint) (pow (2.0, 7.0 - GTK_ADJUSTMENT (widget)->value) + 0.5);

  active_tool = tool_manager_get_active (tool_options->tool_info->gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    gimp_transform_tool_grid_density_changed (GIMP_TRANSFORM_TOOL (active_tool));
}

static void
gimp_transform_tool_show_grid_update (GtkWidget *widget,
        	 		      gpointer   data)
{
  TransformOptions *options;
  GimpToolOptions  *tool_options;
  GimpTool         *active_tool;

  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfault */

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  options      = (TransformOptions *) data;
  tool_options = (GimpToolOptions *) data;

  gimp_toggle_button_update (widget, &options->show_grid);

  active_tool = tool_manager_get_active (tool_options->tool_info->gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    gimp_transform_tool_grid_density_changed (GIMP_TRANSFORM_TOOL (active_tool));
}

static void
gimp_transform_tool_show_path_update (GtkWidget *widget,
				      gpointer   data)
{
  TransformOptions  *options;
  GimpToolOptions   *tool_options;
  GimpTool          *active_tool;

  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfault */

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  options      = (TransformOptions *) data;
  tool_options = (GimpToolOptions *) data;

  active_tool = tool_manager_get_active (tool_options->tool_info->gimp);

  if (GIMP_IS_TRANSFORM_TOOL (active_tool))
    {
      GimpTransformTool *transform_tool;

      transform_tool = GIMP_TRANSFORM_TOOL (active_tool);

      gimp_transform_tool_show_path_changed (transform_tool, 1); /* pause */

      gimp_toggle_button_update (widget, &options->show_path);

      gimp_transform_tool_show_path_changed (transform_tool, 0); /* resume */
    }
  else
    {
      gimp_toggle_button_update (widget, &options->show_path);
    }
}
