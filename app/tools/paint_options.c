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

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gtkhwrapbox.h"

#include "gimptool.h"
#include "paint_options.h"
#include "tool_manager.h"

#include "gimpairbrushtool.h"
#include "gimpblendtool.h"
#include "gimpbucketfilltool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimpinktool.h"
#include "gimppaintbrushtool.h"
#include "gimppenciltool.h"
#include "gimpdodgeburntool.h"
#include "gimpsmudgetool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"

#include "libgimp/gimpintl.h"


static void   pressure_options_init  (GimpPressureOptions *pressure,
                                      GimpPaintOptions    *paint_options,
                                      GType                tool_type);
static void   pressure_options_reset (GimpPressureOptions *pressure);

static void   gradient_options_init  (GimpGradientOptions *gradient,
                                      GimpPaintOptions    *paint_options,
                                      GType                tool_type);
static void   gradient_options_reset (GimpGradientOptions *gradient);

static void   paint_options_opacity_adjustment_update (GtkAdjustment    *adjustment,
						       gpointer          data);
static void   paint_options_opacity_changed           (GimpContext      *context,
						       gdouble           opacity,
						       gpointer          data);
static void   paint_options_paint_mode_update         (GtkWidget        *widget,
						       gpointer          data);
static void   paint_options_paint_mode_changed        (GimpContext      *context,
						       GimpLayerModeEffects  paint_mode,
						       gpointer          data);
static void   paint_options_gradient_toggle_callback  (GtkWidget        *widget,
                                                       GimpPaintOptions *options);


GimpToolOptions *
paint_options_new (GimpToolInfo *tool_info)
{
  GimpPaintOptions *options;

  options = gimp_paint_options_new ();

  paint_options_init (options, tool_info);

  return (GimpToolOptions *) options;
}

void
paint_options_init (GimpPaintOptions *options,
                    GimpToolInfo     *tool_info)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *mode_label;

  /*  initialize the tool options structure  */
  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = paint_options_reset;

  options->context = tool_info->context;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the main table  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the opacity scale  */
  options->opacity_w =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("Opacity:"), -1, 50,
			  gimp_context_get_opacity (tool_info->context) * 100,
			  0.0, 100.0, 1.0, 10.0, 1,
			  TRUE, 0.0, 0.0,
			  NULL, NULL);

  g_signal_connect (G_OBJECT (options->opacity_w), "value_changed",
                    G_CALLBACK (paint_options_opacity_adjustment_update),
                    tool_info->context);

  g_signal_connect (G_OBJECT (tool_info->context), "opacity_changed",
                    G_CALLBACK (paint_options_opacity_changed),
                    options->opacity_w);

  /*  the paint mode menu  */
  options->paint_mode_w =
    gimp_paint_mode_menu_new (G_CALLBACK (paint_options_paint_mode_update),
                              options,
                              TRUE,
                              gimp_context_get_paint_mode (tool_info->context));
  mode_label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                          _("Mode:"), 1.0, 0.5,
                                          options->paint_mode_w, 2, TRUE);

  g_signal_connect (G_OBJECT (tool_info->context), "paint_mode_changed",
                    G_CALLBACK (paint_options_paint_mode_changed),
                    options->paint_mode_w);

  if (tool_info->tool_type == GIMP_TYPE_ERASER_TOOL    ||
      tool_info->tool_type == GIMP_TYPE_CONVOLVE_TOOL  ||
      tool_info->tool_type == GIMP_TYPE_DODGEBURN_TOOL ||
      tool_info->tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (options->paint_mode_w, FALSE);
      gtk_widget_set_sensitive (mode_label, FALSE);
    }

  /*  a separator after the common paint options  */
  if (tool_info->tool_type == GIMP_TYPE_BLEND_TOOL)
    {
      GtkWidget *separator;

      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
    }

  /*  the "incremental" toggle  */
  if (tool_info->tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_info->tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_info->tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_info->tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      options->incremental_w =
	gtk_check_button_new_with_label (_("Incremental"));
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->incremental_w, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    options->incremental_d);
      gtk_widget_show (options->incremental_w);

      g_signal_connect (G_OBJECT (options->incremental_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &options->incremental);
    }

  pressure_options_init (options->pressure_options,
                         options,
                         tool_info->tool_type);

  if (options->pressure_options->frame)
    {
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->pressure_options->frame, FALSE, FALSE, 0);
      gtk_widget_show (options->pressure_options->frame);
    }

  gradient_options_init (options->gradient_options,
                         options,
                         tool_info->tool_type);

  if (options->gradient_options->fade_frame)
    {
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->gradient_options->fade_frame,
                          FALSE, FALSE, 0);
      gtk_widget_show (options->gradient_options->fade_frame);
    }

  if (options->gradient_options->gradient_frame)
    {
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->gradient_options->gradient_frame,
                          FALSE, FALSE, 0);
      gtk_widget_show (options->gradient_options->gradient_frame);
    }
}

void
paint_options_reset (GimpToolOptions *tool_options)
{
  GimpPaintOptions *options;
  GimpContext      *default_context;

  options = (GimpPaintOptions *) tool_options;

  default_context = gimp_get_default_context (tool_options->tool_info->gimp);

  if (options->opacity_w)
    {
      gimp_context_set_opacity (GIMP_CONTEXT (options->context),
				gimp_context_get_opacity (default_context));
    }
  if (options->paint_mode_w)
    {
      gimp_context_set_paint_mode (GIMP_CONTEXT (options->context),
				   gimp_context_get_paint_mode (default_context));
    }
  if (options->incremental_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    options->incremental_d);
    }

  pressure_options_reset (options->pressure_options);
  gradient_options_reset (options->gradient_options);
}


/*  private functions  */

static void
pressure_options_init (GimpPressureOptions *pressure,
                       GimpPaintOptions    *paint_options,
                       GType                tool_type)
{
  GtkWidget *wbox = NULL;

  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGEBURN_TOOL  ||
      tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL     ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      pressure->frame = gtk_frame_new (_("Pressure Sensitivity"));

      wbox = gtk_hwrap_box_new (FALSE);
      gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (wbox), 6);
      gtk_container_add (GTK_CONTAINER (pressure->frame), wbox);
      gtk_widget_show (wbox);
    }

  /*  the opacity toggle  */
  if (tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_DODGEBURN_TOOL  ||
      tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      pressure->opacity_w =
	gtk_check_button_new_with_label (_("Opacity"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->opacity_w);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->opacity_w),
				    pressure->opacity_d);
      gtk_widget_show (pressure->opacity_w);

      g_signal_connect (G_OBJECT (pressure->opacity_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &pressure->opacity);
    }

  /*  the pressure toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGEBURN_TOOL  ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      pressure->pressure_w = gtk_check_button_new_with_label (_("Hardness"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->pressure_w);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->pressure_w),
				    pressure->pressure_d);
      gtk_widget_show (pressure->pressure_w);

      g_signal_connect (G_OBJECT (pressure->pressure_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &pressure->pressure);
    }

  /*  the rate toggle */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      pressure->rate_w =
	gtk_check_button_new_with_label (_("Rate"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->rate_w);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->rate_w),
				    pressure->rate_d);
      gtk_widget_show (pressure->rate_w);

      g_signal_connect (G_OBJECT (pressure->rate_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &pressure->rate);
    }

  /*  the size toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGEBURN_TOOL  ||
      tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      pressure->size_w =
	gtk_check_button_new_with_label (_("Size"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->size_w);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->size_w),
				    pressure->size_d);
      gtk_widget_show (pressure->size_w);

      g_signal_connect (G_OBJECT (pressure->size_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &pressure->size);
    }

  /*  the color toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      pressure->color_w =
	gtk_check_button_new_with_label (_("Color"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->color_w);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->color_w),
				    pressure->color_d);
      gtk_widget_show (pressure->color_w);

      g_signal_connect (G_OBJECT (pressure->color_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &pressure->color);
    }
}

static void
pressure_options_reset (GimpPressureOptions *pressure)
{
  if (pressure->opacity_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->opacity_w),
				    pressure->opacity_d);
    }
  if (pressure->pressure_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->pressure_w),
				    pressure->pressure_d);
    }
  if (pressure->rate_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->rate_w),
				    pressure->rate_d);
    }
  if (pressure->size_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->size_w),
				    pressure->size_d);
    }
  if (pressure->color_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->color_w),
				    pressure->color_d);
    }
}

static void
gradient_options_init (GimpGradientOptions *gradient,
                       GimpPaintOptions    *paint_options,
                       GType                tool_type)
{
  GtkWidget *table      = NULL;
  GtkWidget *spinbutton = NULL;

  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      gradient->fade_frame = gtk_frame_new (NULL);

      table = gtk_table_new (1, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_container_add (GTK_CONTAINER (gradient->fade_frame), table);
      gtk_widget_show (table);

      gradient->use_fade_w =
        gtk_check_button_new_with_label (_("Fade Out"));
      gtk_frame_set_label_widget (GTK_FRAME (gradient->fade_frame),
                                  gradient->use_fade_w);
      gtk_widget_show (gradient->use_fade_w);

      g_signal_connect (G_OBJECT (gradient->use_fade_w), "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &gradient->use_fade);

      gtk_widget_set_sensitive (table, gradient->use_fade_d);
      g_object_set_data (G_OBJECT (gradient->use_fade_w),
                           "set_sensitive", table);

     /*  the fade-out sizeentry  */
      gradient->fade_out_w =
        gtk_adjustment_new (gradient->fade_out_d,
                            1e-5, 32767.0, 1.0, 50.0, 0.0);

      spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (gradient->fade_out_w),
                                        1.0, 0.0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_size_request (spinbutton, 50, -1);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Length:"), 1.0, 0.5,
                                 spinbutton, 1, FALSE);

      g_signal_connect (G_OBJECT (gradient->fade_out_w), "value_changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &gradient->fade_out);

      /*  the fade-out unitmenu  */
      gradient->fade_unit_w =
        gimp_unit_menu_new ("%a", gradient->fade_unit_d, TRUE, TRUE, TRUE);
      gtk_table_attach (GTK_TABLE (table), gradient->fade_unit_w, 2, 3, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (gradient->fade_unit_w);

      g_object_set_data (G_OBJECT (gradient->fade_unit_w), "set_digits",
                         spinbutton);

      g_signal_connect (G_OBJECT (gradient->fade_unit_w), "unit_changed",
                        G_CALLBACK (gimp_unit_menu_update),
                        &gradient->fade_unit);
    }

  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      gradient->gradient_frame = gtk_frame_new (NULL);

      table = gtk_table_new (2, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_add (GTK_CONTAINER (gradient->gradient_frame), table);
      gtk_widget_show (table);

      gradient->use_gradient_w =
        gtk_check_button_new_with_label (_("Use Color from Gradient"));
      gtk_frame_set_label_widget (GTK_FRAME (gradient->gradient_frame),
                                  gradient->use_gradient_w);
      gtk_widget_show (gradient->use_gradient_w);

      g_signal_connect (G_OBJECT (gradient->use_gradient_w), "toggled",
                        G_CALLBACK (paint_options_gradient_toggle_callback),
                        paint_options);

      gtk_widget_set_sensitive (table, gradient->use_gradient_d);
      g_object_set_data (G_OBJECT (gradient->use_gradient_w), "set_sensitive",
                         table);
      g_object_set_data (G_OBJECT (gradient->use_gradient_w),
                         "inverse_sensitive",
                         paint_options->incremental_w);

      /*  the gradient length scale  */
      gradient->gradient_length_w =
        gtk_adjustment_new (gradient->gradient_length_d,
                            1e-5, 32767.0, 1.0, 50.0, 0.0);
      spinbutton =
        gtk_spin_button_new (GTK_ADJUSTMENT (gradient->gradient_length_w),
                             1.0, 0.0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_size_request (spinbutton, 50, -1);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Length:"), 1.0, 0.5,
                                 spinbutton, 1, FALSE);

      g_signal_connect (G_OBJECT (gradient->gradient_length_w), "value_changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &gradient->gradient_length);

      /*  the gradient unitmenu  */
      gradient->gradient_unit_w =
        gimp_unit_menu_new ("%a", gradient->gradient_unit_d, TRUE, TRUE, TRUE);
      gtk_table_attach (GTK_TABLE (table), gradient->gradient_unit_w, 2, 3, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (gradient->gradient_unit_w);

      g_object_set_data (G_OBJECT (gradient->gradient_unit_w), "set_digits",
                           spinbutton);

      g_signal_connect (G_OBJECT (gradient->gradient_unit_w), "unit_changed",
                        G_CALLBACK (gimp_unit_menu_update),
                        &gradient->gradient_unit);

      /*  the gradient type  */
      gradient->gradient_type_w =
	gimp_option_menu_new2 (FALSE,
			       G_CALLBACK (gimp_menu_item_update),
			       &gradient->gradient_type,
			       (gpointer) gradient->gradient_type_d,

			       _("Once Forward"),
			       GINT_TO_POINTER (ONCE_FORWARD), NULL,

			       _("Once Backward"),
			       GINT_TO_POINTER (ONCE_BACKWARDS), NULL,

			       _("Loop Sawtooth"),
			       GINT_TO_POINTER (LOOP_SAWTOOTH), NULL,

			       _("Loop Triangle"),
			       GINT_TO_POINTER (LOOP_TRIANGLE), NULL,

			       NULL);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("Type:"), 1.0, 0.5,
                                 gradient->gradient_type_w, 2,
                                 TRUE);

      gtk_widget_show (table);
    }
}

static void
gradient_options_reset (GimpGradientOptions *gradient)
{
  GtkWidget *spinbutton;
  gint       digits;

  if (gradient->use_fade_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gradient->use_fade_w),
                                    gradient->use_fade_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (gradient->fade_out_w),
                                gradient->fade_out_d);
      gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gradient->fade_unit_w),
                               gradient->fade_unit_d);
      digits = ((gradient->fade_unit_d == GIMP_UNIT_PIXEL) ? 0 :
                ((gradient->fade_unit_d == GIMP_UNIT_PERCENT) ? 2 :
                 (MIN (6, MAX (3, gimp_unit_get_digits (gradient->fade_unit_d))))));
      spinbutton = g_object_get_data (G_OBJECT (gradient->fade_unit_w),
                                        "set_digits");
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
    }

  if (gradient->use_gradient_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gradient->use_gradient_w),
                                    gradient->use_gradient_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (gradient->gradient_length_w),
                                gradient->gradient_length_d);
      gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gradient->gradient_unit_w),
                               gradient->gradient_unit_d);
      digits = ((gradient->gradient_unit_d == GIMP_UNIT_PIXEL) ? 0 :
                ((gradient->gradient_unit_d == GIMP_UNIT_PERCENT) ? 2 :
                 (MIN (6, MAX (3, gimp_unit_get_digits (gradient->gradient_unit_d))))));
      spinbutton = g_object_get_data (G_OBJECT (gradient->gradient_unit_w),
                                        "set_digits");
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);

      gradient->gradient_type = gradient->gradient_type_d;

      gtk_option_menu_set_history (GTK_OPTION_MENU (gradient->gradient_type_w),
                                   gradient->gradient_type_d);
    }
}

static void
paint_options_opacity_adjustment_update (GtkAdjustment *adjustment,
					 gpointer       data)
{
  g_signal_handlers_block_by_func (G_OBJECT (data), 
                                   paint_options_opacity_changed,
                                   adjustment);

  gimp_context_set_opacity (GIMP_CONTEXT (data),
			    adjustment->value / 100);

  g_signal_handlers_unblock_by_func (G_OBJECT (data), 
                                     paint_options_opacity_changed,
                                     adjustment);
}

static void
paint_options_opacity_changed (GimpContext *context,
			       gdouble      opacity,
			       gpointer     data)
{
  g_signal_handlers_block_by_func (G_OBJECT (data), 
                                   paint_options_opacity_adjustment_update,
                                   context);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (data), opacity * 100);

  g_signal_handlers_unblock_by_func (G_OBJECT (data), 
                                     paint_options_opacity_adjustment_update,
                                     context);
}

static void
paint_options_paint_mode_update (GtkWidget *widget,
				 gpointer   data)
{
  GimpLayerModeEffects  paint_mode;
  GimpPaintOptions     *options;

  paint_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), 
                                                   "gimp-item-data"));

  options = (GimpPaintOptions *) data;

  g_signal_handlers_block_by_func (G_OBJECT (options->context),
				   paint_options_paint_mode_changed,
				   options->paint_mode_w);

  gimp_context_set_paint_mode (GIMP_CONTEXT (options->context), paint_mode);

  g_signal_handlers_unblock_by_func (G_OBJECT (options->context),
				     paint_options_paint_mode_changed,
				     options->paint_mode_w);
}

static void
paint_options_paint_mode_changed (GimpContext          *context,
				  GimpLayerModeEffects  paint_mode,
				  gpointer              data)
{
  gimp_option_menu_set_history (GTK_OPTION_MENU (data),
                                GINT_TO_POINTER (paint_mode));
}

static void
paint_options_gradient_toggle_callback (GtkWidget        *widget,
                                        GimpPaintOptions *options)
{
  gimp_toggle_button_update (widget, &options->gradient_options->use_gradient);

  if (options->gradient_options->use_gradient)
    {
      options->incremental_save = options->incremental;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    TRUE);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    options->incremental_save);
    }
}
