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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "gimprc.h"

#include "gimpellipseselecttool.h"
#include "gimpfuzzyselecttool.h"
#include "gimpiscissorstool.h"
#include "gimprectselecttool.h"
#include "gimpbycolorselecttool.h"
#include "gimptool.h"
#include "selection_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


void
selection_options_init (SelectionOptions *options,
                        GimpToolInfo     *tool_info)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scale;

  /*  initialize the tool options structure  */
  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = selection_options_reset;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  initialize the selection options structure  */
  options->op                 = options->op_d                 = SELECTION_REPLACE;
  options->feather            = options->feather_d            = FALSE;
  options->feather_radius     = options->feather_radius_d     = 10.0;

  if (tool_info->tool_type == GIMP_TYPE_RECT_SELECT_TOOL)
    options->antialias        = options->antialias_d          = FALSE;
  else
    options->antialias        = options->antialias_d          = TRUE;

  options->select_transparent = options->select_transparent_d = TRUE;
  options->sample_merged      = options->sample_merged_d      = FALSE;
  options->threshold          = gimprc.default_threshold;
  options->auto_shrink        = options->auto_shrink_d        = FALSE;
  options->shrink_merged      = options->shrink_merged_d      = FALSE;
  options->fixed_size         = options->fixed_size_d         = FALSE;
  options->fixed_height       = options->fixed_height_d       = 1;
  options->fixed_width        = options->fixed_width_d        = 1;
  options->fixed_unit         = options->fixed_unit_d         = GIMP_UNIT_PIXEL;
  options->interactive        = options->interactive_d        = FALSE;

  options->feather_w            = NULL;
  options->feather_radius_w     = NULL;
  options->antialias_w          = NULL;
  options->select_transparent_w = NULL;
  options->sample_merged_w      = NULL;
  options->threshold_w          = NULL;
  options->auto_shrink_w        = NULL;
  options->shrink_merged_w      = NULL;
  options->fixed_size_w         = NULL;
  options->fixed_height_w       = NULL;
  options->fixed_width_w        = NULL;
  options->fixed_unit_w         = NULL;
  options->interactive_w        = NULL;

  /*  the selection operation radio buttons  */
  {
    SelectOps radio_ops[] =
    {
      SELECTION_REPLACE,
      SELECTION_ADD,
      SELECTION_SUB,
      SELECTION_INTERSECT
    };

    const gchar *radio_stock_ids[] =
    {
      GIMP_STOCK_SELECTION_REPLACE,
      GIMP_STOCK_SELECTION_ADD,
      GIMP_STOCK_SELECTION_SUBTRACT,
      GIMP_STOCK_SELECTION_INTERSECT
    };

    const gchar *radio_tooltips[] =
    {
      _("Replace the current selection"),
      _("Add to the current selection"),
      _("Subtract from the current selection"),
      _("Intersect with the current selection")
    };

    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *image;
    GSList    *group = NULL;
    gint       i;

    hbox = gtk_hbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Mode:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    for (i = 0; i < G_N_ELEMENTS (radio_ops); i++)
      {
        options->op_w[i] = gtk_radio_button_new (group);
        group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (options->op_w[i]));
        gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (options->op_w[i]), FALSE);
        gtk_box_pack_start (GTK_BOX (hbox), options->op_w[i], FALSE, FALSE, 0);
        gtk_widget_show (options->op_w[i]);

        gimp_help_set_help_data (options->op_w[i], radio_tooltips[i], NULL);

        image = gtk_image_new_from_stock (radio_stock_ids[i],
                                          GTK_ICON_SIZE_BUTTON);
        gtk_container_add (GTK_CONTAINER (options->op_w[i]), image);
        gtk_widget_show (image);

        if (radio_ops[i] == options->op)
          {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->op_w[i]),
                                          TRUE);
          }

        g_object_set_data (G_OBJECT (options->op_w[i]), "gimp-item-data",
                           GINT_TO_POINTER (radio_ops[i]));
        g_signal_connect (G_OBJECT (options->op_w[i]), "toggled",
                          G_CALLBACK (gimp_radio_button_update),
                          &options->op);
      }
  }

  /*  the antialias toggle button  */
  options->antialias_w = gtk_check_button_new_with_label (_("Antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
                                options->antialias_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->antialias_w, FALSE, FALSE, 0);
  gtk_widget_show (options->antialias_w);

  gimp_help_set_help_data (options->antialias_w, _("Smooth edges"), NULL);

  if (tool_info->tool_type == GIMP_TYPE_RECT_SELECT_TOOL)
    {
      gtk_widget_set_sensitive (options->antialias_w, FALSE);
    }
  else
    {
      g_signal_connect (G_OBJECT (options->antialias_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->antialias);
    }

  /*  the feather frame  */
  {
    GtkWidget *frame;
    GtkWidget *hbox;

    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    options->feather_w = gtk_check_button_new_with_label (_("Feather Edges"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
                                  options->feather_d);
    gtk_frame_set_label_widget (GTK_FRAME (frame), options->feather_w);
    gtk_widget_show (options->feather_w);

    g_signal_connect (G_OBJECT (options->feather_w), "toggled",
                      G_CALLBACK (gimp_toggle_button_update),
                      &options->feather);

    /*  the feather radius scale  */
    hbox = gtk_hbox_new (FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    gtk_widget_show (hbox);
  
    label = gtk_label_new (_("Radius:"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    options->feather_radius_w =
      gtk_adjustment_new (options->feather_radius_d, 0.0, 100.0, 1.0, 1.0, 1.0);

    scale = gtk_hscale_new (GTK_ADJUSTMENT (options->feather_radius_w));
    gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
    gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
    gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
    gtk_widget_show (scale);

    g_signal_connect (G_OBJECT (options->feather_radius_w), "value_changed",
                      G_CALLBACK (gimp_double_adjustment_update),
                      &options->feather_radius);

    /*  grey out label & scale if feather is off  */
    gtk_widget_set_sensitive (hbox, options->feather_d);
    g_object_set_data (G_OBJECT (options->feather_w), "set_sensitive", hbox);
  }

#if 0
  /*  a separator between the common and tool-specific selection options  */
  if (tool_info->tool_type == GIMP_TYPE_ISCISSORS_TOOL      ||
      tool_info->tool_type == GIMP_TYPE_RECT_SELECT_TOOL    ||
      tool_info->tool_type == GIMP_TYPE_ELLIPSE_SELECT_TOOL ||
      tool_info->tool_type == GIMP_TYPE_FUZZY_SELECT_TOOL   ||
      tool_info->tool_type == GIMP_TYPE_BY_COLOR_SELECT_TOOL)
    {
      GtkWidget *separator;

      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
    }
#endif

  /* selection tool with an interactive boundary that can be toggled */
  if (tool_info->tool_type == GIMP_TYPE_ISCISSORS_TOOL)
    {
      options->interactive_w =
	gtk_check_button_new_with_label (_("Show Interactive Boundary"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->interactive_w),
				    options->interactive_d);
      gtk_box_pack_start (GTK_BOX (vbox), options->interactive_w,
			  FALSE, FALSE, 0);
      gtk_widget_show (options->interactive_w);

      g_signal_connect (G_OBJECT (options->interactive_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->interactive);
    }

  /*  selection tools which operate on colors or contiguous regions  */
  if (tool_info->tool_type == GIMP_TYPE_FUZZY_SELECT_TOOL ||
      tool_info->tool_type == GIMP_TYPE_BY_COLOR_SELECT_TOOL)
    {
      GtkWidget *frame;
      GtkWidget *vbox2;
      GtkWidget *hbox;

      frame = gtk_frame_new (_("Finding Similar Colors"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox2 = gtk_vbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      /*  the select transparent areas toggle  */
      options->select_transparent_w =
	gtk_check_button_new_with_label (_("Select Transparent Areas"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->select_transparent_w),
				    options->select_transparent_d);
      gtk_box_pack_start (GTK_BOX (vbox2), options->select_transparent_w,
			  FALSE, FALSE, 0);
      gtk_widget_show (options->select_transparent_w);

      gimp_help_set_help_data (options->select_transparent_w,
                               _("Allow completely transparent regions "
                                 "to be selected"), NULL);

      g_signal_connect (G_OBJECT (options->select_transparent_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->select_transparent);

      /*  the sample merged toggle  */
      options->sample_merged_w =
	gtk_check_button_new_with_label (_("Sample Merged"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				    options->sample_merged_d);
      gtk_box_pack_start (GTK_BOX (vbox2), options->sample_merged_w,
			  FALSE, FALSE, 0);
      gtk_widget_show (options->sample_merged_w);

      gimp_help_set_help_data (options->sample_merged_w,
                               _("Base selection on all visible layers"), NULL);

      g_signal_connect (G_OBJECT (options->sample_merged_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->sample_merged);

      /*  the threshold scale  */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
  
      label = gtk_label_new (_("Threshold:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
      gtk_widget_show (label);

      options->threshold_w = 
	gtk_adjustment_new (gimprc.default_threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
      scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
      gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
      gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      gimp_help_set_help_data (scale, _("Maximum color difference"), NULL);

      g_signal_connect (G_OBJECT (options->threshold_w), "value_changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &options->threshold);
    }

  /*  widgets for fixed size select  */
  if (tool_info->tool_type == GIMP_TYPE_RECT_SELECT_TOOL    ||
      tool_info->tool_type == GIMP_TYPE_ELLIPSE_SELECT_TOOL)
    {
      GtkWidget *frame;
      GtkWidget *vbox2;
      GtkWidget *alignment;
      GtkWidget *table;
      GtkWidget *width_spinbutton;
      GtkWidget *height_spinbutton;

      frame = gtk_frame_new (NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox2 = gtk_vbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      options->auto_shrink_w =
	gtk_check_button_new_with_label (_("Auto Shrink Selection"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->auto_shrink_w),
				    options->auto_shrink_d);
      gtk_frame_set_label_widget (GTK_FRAME (frame), options->auto_shrink_w);
      gtk_widget_show (options->auto_shrink_w);

      g_signal_connect (G_OBJECT (options->auto_shrink_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->auto_shrink);

      gtk_widget_set_sensitive (vbox2, options->auto_shrink);
      g_object_set_data (G_OBJECT (options->auto_shrink_w), "set_sensitive",
                         vbox2);

      options->shrink_merged_w =
	gtk_check_button_new_with_label (_("Sample Merged"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->shrink_merged_w),
				    options->shrink_merged_d);
      gtk_box_pack_start (GTK_BOX (vbox2), options->shrink_merged_w,
                          FALSE, FALSE, 0);
      gtk_widget_show (options->shrink_merged_w);

      gimp_help_set_help_data (options->shrink_merged_w,
                               _("Use all visible layers when shrinking "
                                 "the selection"), NULL);

      g_signal_connect (G_OBJECT (options->shrink_merged_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->shrink_merged);

      frame = gtk_frame_new (NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_container_set_border_width (GTK_CONTAINER (alignment), 2);
      gtk_container_add (GTK_CONTAINER (frame), alignment);
      gtk_widget_show (alignment);

      options->fixed_size_w =
	gtk_check_button_new_with_label (_("Fixed Size / Aspect Ratio"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fixed_size_w),
				    options->fixed_size_d);
      gtk_frame_set_label_widget (GTK_FRAME (frame), options->fixed_size_w);
      gtk_widget_show (options->fixed_size_w);

      g_signal_connect (G_OBJECT (options->fixed_size_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->fixed_size);

      gtk_widget_set_sensitive (alignment, options->fixed_size_d);
      g_object_set_data (G_OBJECT (options->fixed_size_w), "set_sensitive",
                         alignment);

      table = gtk_table_new (3, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_container_add (GTK_CONTAINER (alignment), table);

      options->fixed_width_w =
	gtk_adjustment_new (options->fixed_width_d, 1e-5, 32767.0,
			    1.0, 50.0, 0.0);
      width_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (options->fixed_width_w), 1.0, 0.0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width_spinbutton), TRUE);
      gtk_widget_set_size_request (width_spinbutton, 75, -1);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
				 _("Width:"), 1.0, 0.5,
				 width_spinbutton, 1, FALSE);

      g_signal_connect (G_OBJECT (options->fixed_width_w), "value_changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &options->fixed_width);

      options->fixed_height_w =
	gtk_adjustment_new (options->fixed_height_d, 1e-5, 32767.0,
			    1.0, 50.0, 0.0);
      height_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (options->fixed_height_w), 1.0, 0.0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (height_spinbutton), TRUE);
      gtk_widget_set_size_request (height_spinbutton, 75, -1);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Height:"), 1.0, 0.5,
				 height_spinbutton, 1, FALSE);

      g_signal_connect (G_OBJECT (options->fixed_height_w), "value_changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &options->fixed_height);

      options->fixed_unit_w =
	gimp_unit_menu_new ("%a", options->fixed_unit_d, TRUE, TRUE, TRUE);
      g_object_set_data (G_OBJECT (options->fixed_unit_w), "set_digits",
			   width_spinbutton);
      g_object_set_data (G_OBJECT (width_spinbutton), "set_digits",
			   height_spinbutton);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
				 _("Unit:"), 1.0, 0.5,
				 options->fixed_unit_w, 1, FALSE);

      g_signal_connect (G_OBJECT (options->fixed_unit_w), "unit_changed",
                        G_CALLBACK (gimp_unit_menu_update),
                        &options->fixed_unit);

      gtk_widget_show (table);
    }
}

GimpToolOptions *
selection_options_new (GimpToolInfo *tool_info)
{
  SelectionOptions *options;

  options = g_new0 (SelectionOptions, 1);

  selection_options_init (options, tool_info);

  return (GimpToolOptions *) options;
}

void
selection_options_reset (GimpToolOptions *tool_options)
{
  SelectionOptions *options;

  options = (SelectionOptions *) tool_options;

  if (options->op_w[0])
    {
      gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->op_w[0]),
                                   GINT_TO_POINTER (options->op_d));
    }

  if (options->antialias_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
                                    options->antialias_d);
    }

  if (options->feather_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
				    options->feather_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->feather_radius_w),
				options->feather_radius_d);
    }

  if (options->sample_merged_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				    options->sample_merged_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
				gimprc.default_threshold);
    }

  if (options->auto_shrink_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->auto_shrink_w),
				    options->auto_shrink_d);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->shrink_merged_w),
				    options->shrink_merged_d);
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
	g_object_get_data (G_OBJECT (options->fixed_unit_w), "set_digits");
      while (spinbutton)
	{
	  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
	  spinbutton =
	    g_object_get_data (G_OBJECT (spinbutton), "set_digits");
	}
    }

  if (options->interactive_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->interactive_w),
				    options->interactive_d);
    }
}
