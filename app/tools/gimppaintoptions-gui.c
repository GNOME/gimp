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

#include "apptypes.h"

#include "gimpcontext.h"
#include "gimprc.h"
#include "gtkhwrapbox.h"

#include "gimptoolinfo.h"
#include "paint_options.h"
#include "tool.h"
#include "tool_manager.h"

#include "gimppaintbrushtool.h"

#include "libgimp/gimpintl.h"


static PaintPressureOptions * paint_pressure_options_new (GtkType    tool_type);
static void   paint_pressure_options_reset (PaintPressureOptions *pressure_options);

static void   paint_options_opacity_adjustment_update (GtkAdjustment *adjustment,
						       gpointer       data);
static void   paint_options_opacity_changed           (GimpContext   *context,
						       gdouble        opacity,
						       gpointer       data);
static void   paint_options_paint_mode_update         (GtkWidget     *widget,
						       gpointer       data);
static void   paint_options_paint_mode_changed        (GimpContext   *context,
						       LayerModeEffects  paint_mode,
						       gpointer       data);


/*  declared extern in paint_options.h  */
PaintPressureOptions non_gui_pressure_options = 
{
  NULL,
  FALSE, FALSE, NULL,
  FALSE, FALSE, NULL,
  FALSE, FALSE, NULL,
  FALSE, FALSE, NULL,
  FALSE, FALSE, NULL
};

/*  a list of all PaintOptions  */
static GSList *paint_options_list = NULL;


void
paint_options_init (PaintOptions         *options,
		    GtkType               tool_type,
		    ToolOptionsResetFunc  reset_func)
{
  GimpToolInfo *tool_info;
  GtkWidget    *vbox;
  GtkWidget    *table;
  GtkWidget    *scale;
  GtkWidget    *separator;

  tool_info = tool_manager_get_info_by_type (tool_type);

  if (! tool_info)
    {
      g_warning ("%s(): no tool info registered for %s",
                 G_GNUC_FUNCTION, gtk_type_name (tool_type));
    }

  /*  initialize the tool options structure  */
  tool_options_init ((ToolOptions *) options,
		     reset_func);

  /*  initialize the paint options structure  */
  options->global           = NULL;
  options->opacity_w        = NULL;
  options->paint_mode_w     = NULL;
  options->context          = tool_info->context;
  options->incremental_w    = NULL;
  options->incremental      = options->incremental_d = FALSE;
  options->pressure_options = NULL;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox), vbox,
		      FALSE, FALSE, 0);
  options->paint_vbox = vbox;

  /*  the main table  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);

  /*  the opacity scale  */
  options->opacity_w =
    gtk_adjustment_new (gimp_context_get_opacity (tool_info->context) * 100,
			0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->opacity_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->opacity_w), "value_changed",
		      GTK_SIGNAL_FUNC (paint_options_opacity_adjustment_update),
		      tool_info->context);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Opacity:"), 1.0, 1.0,
			     scale, 1, FALSE);

  gtk_signal_connect (GTK_OBJECT (tool_info->context), "opacity_changed",
		      GTK_SIGNAL_FUNC (paint_options_opacity_changed),
		      options->opacity_w);

  /*  the paint mode menu  */
  if (tool_type == GIMP_TYPE_BUCKET_FILL_TOOL ||
      tool_type == GIMP_TYPE_BLEND_TOOL       ||
      tool_type == GIMP_TYPE_PENCIL_TOOL      ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL  ||
      tool_type == GIMP_TYPE_AIRBRUSH_TOOL    ||
      tool_type == GIMP_TYPE_CLONE_TOOL       ||
      tool_type == GIMP_TYPE_INK_TOOL)
    {
      gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);

      options->paint_mode_w =
	paint_mode_menu_new (paint_options_paint_mode_update, options,
			     gimp_context_get_paint_mode (tool_info->context));
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Mode:"), 1.0, 0.5,
				 options->paint_mode_w, 1, TRUE);

      gtk_signal_connect (GTK_OBJECT (tool_info->context), "paint_mode_changed",
			  GTK_SIGNAL_FUNC (paint_options_paint_mode_changed),
			  options->paint_mode_w);
    }

  /*  show the main table  */
  gtk_widget_show (table);

  /*  a separator after the common paint options which can be global  */
  if (tool_type == GIMP_TYPE_BUCKET_FILL_TOOL ||
      tool_type == GIMP_TYPE_BLEND_TOOL       ||
      tool_type == GIMP_TYPE_INK_TOOL)
    {
      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
    }

  if (! global_paint_options)
    gtk_widget_show (vbox);

  /*  the "incremental" toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      options->incremental_w =
	gtk_check_button_new_with_label (_("Incremental"));
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->incremental_w, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->incremental_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &options->incremental);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    options->incremental_d);
      gtk_widget_show (options->incremental_w);
    }

  options->pressure_options = paint_pressure_options_new (tool_type);

  if (options->pressure_options->frame)
    {
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->pressure_options->frame, FALSE, FALSE, 0);
      gtk_widget_show (options->pressure_options->frame);
    }

  /*  register this Paintoptions structure  */
  paint_options_list = g_slist_prepend (paint_options_list, options);
}

PaintOptions *
paint_options_new (GtkType               tool_type,
		   ToolOptionsResetFunc  reset_func)
{
  PaintOptions *options;

  options = g_new (PaintOptions, 1);
  paint_options_init (options, tool_type, reset_func);

  if (global_paint_options && options->global)
    gtk_widget_show (options->global);

  return options;
}

void
paint_options_reset (PaintOptions *options)
{
  GimpContext *default_context;

  default_context = gimp_context_get_default ();

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

  paint_pressure_options_reset (options->pressure_options);
}

void
paint_options_set_global (gboolean global)
{
  PaintOptions *options;
  GSList *list;

  global = global ? TRUE : FALSE;

  if (global_paint_options == global)
    return;

  global_paint_options = global;

  for (list = paint_options_list; list; list = list->next)
    {
      options = (PaintOptions *) list->data;

      if (global)
	{
	  if (options->paint_vbox && GTK_WIDGET_VISIBLE (options->paint_vbox))
	    gtk_widget_hide (options->paint_vbox);
	  if (options->global && ! GTK_WIDGET_VISIBLE (options->global))
	    gtk_widget_show (options->global);
	}
      else
	{
	  if (options->paint_vbox && ! GTK_WIDGET_VISIBLE (options->paint_vbox))
	    gtk_widget_show (options->paint_vbox);
	  if (options->global && GTK_WIDGET_VISIBLE (options->global))
	    gtk_widget_hide (options->global);
	}
    }
}

GtkWidget *
paint_mode_menu_new (GtkSignalFunc    callback,
		     gpointer         data,
		     LayerModeEffects initial)
{
  GtkWidget *menu;

  menu = gimp_option_menu_new2
    (FALSE, callback, data, (gpointer) initial,

     _("Normal"),          (gpointer) NORMAL_MODE, NULL,
     _("Dissolve"),        (gpointer) DISSOLVE_MODE, NULL,
     _("Behind"),          (gpointer) BEHIND_MODE, NULL,
     _("Multiply"),        (gpointer) MULTIPLY_MODE, NULL,
     _("Divide"),          (gpointer) DIVIDE_MODE, NULL,
     _("Screen"),          (gpointer) SCREEN_MODE, NULL,
     _("Overlay"),         (gpointer) OVERLAY_MODE, NULL,
     _("Dodge"),           (gpointer) DODGE_MODE, NULL,
     _("Burn"),            (gpointer) BURN_MODE, NULL,
     _("Hard Light"),      (gpointer) HARDLIGHT_MODE, NULL,
     _("Difference"),      (gpointer) DIFFERENCE_MODE, NULL,
     _("Addition"),        (gpointer) ADDITION_MODE, NULL,
     _("Subtract"),        (gpointer) SUBTRACT_MODE, NULL,
     _("Darken Only"),     (gpointer) DARKEN_ONLY_MODE, NULL,
     _("Lighten Only"),    (gpointer) LIGHTEN_ONLY_MODE, NULL,
     _("Hue"),             (gpointer) HUE_MODE, NULL,
     _("Saturation"),      (gpointer) SATURATION_MODE, NULL,
     _("Color"),           (gpointer) COLOR_MODE, NULL,
     _("Value"),           (gpointer) VALUE_MODE, NULL,

     NULL);

  return menu;
}


/*  private functions  */

static PaintPressureOptions *
paint_pressure_options_new (GtkType tool_type)
{
  PaintPressureOptions *pressure = NULL;
  GtkWidget            *frame = NULL;
  GtkWidget            *wbox = NULL;

  pressure = g_new (PaintPressureOptions, 1);

  pressure->opacity  = pressure->opacity_d  = TRUE;
  pressure->pressure = pressure->pressure_d = TRUE;
  pressure->rate     = pressure->rate_d     = FALSE;
  pressure->size     = pressure->size_d     = FALSE;
  pressure->color    = pressure->color_d    = FALSE;

  pressure->opacity_w  = NULL;
  pressure->pressure_w = NULL;
  pressure->rate_w     = NULL;
  pressure->size_w     = NULL;
  pressure->color_w    = NULL;

  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL     ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      frame = gtk_frame_new (_("Pressure Sensitivity"));
      wbox = gtk_hwrap_box_new (FALSE);
      gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (wbox), 6);
      gtk_container_add (GTK_CONTAINER (frame), wbox);
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
      gtk_signal_connect (GTK_OBJECT (pressure->opacity_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->opacity);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->opacity_w),
				    pressure->opacity_d);
      gtk_widget_show (pressure->opacity_w);
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
      gtk_signal_connect (GTK_OBJECT (pressure->pressure_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->pressure);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->pressure_w),
				    pressure->pressure_d);
      gtk_widget_show (pressure->pressure_w);
    }

  /*  the rate toggle */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      pressure->rate_w =
	gtk_check_button_new_with_label (_("Rate"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->rate_w);
      gtk_signal_connect (GTK_OBJECT (pressure->rate_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->rate);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->rate_w),
				    pressure->rate_d);
      gtk_widget_show (pressure->rate_w);
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
      gtk_signal_connect (GTK_OBJECT (pressure->size_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->size);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->size_w),
				    pressure->size_d);
      gtk_widget_show (pressure->size_w);
    }

  /*  the color toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      pressure->color_w =
	gtk_check_button_new_with_label (_("Color"));
      gtk_container_add (GTK_CONTAINER (wbox), pressure->color_w);
      gtk_signal_connect (GTK_OBJECT (pressure->color_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->color);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->color_w),
				    pressure->color_d);
      gtk_widget_show (pressure->color_w);
    }

  pressure->frame = frame;

  return pressure;
}

static void
paint_pressure_options_reset (PaintPressureOptions *pressure)
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
paint_options_opacity_adjustment_update (GtkAdjustment *adjustment,
					 gpointer       data)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (data), adjustment);
  gimp_context_set_opacity (GIMP_CONTEXT (data),
			    adjustment->value / 100);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (data), adjustment);
}

static void
paint_options_opacity_changed (GimpContext *context,
			       gdouble      opacity,
			       gpointer     data)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (data), context);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (data), opacity * 100);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (data), context);
}

static void
paint_options_paint_mode_update (GtkWidget *widget,
				 gpointer   data)
{
  LayerModeEffects  paint_mode;
  PaintOptions     *options;

  paint_mode = (LayerModeEffects) gtk_object_get_user_data (GTK_OBJECT (widget));
  options    = (PaintOptions *) data;

  gtk_signal_handler_block_by_data (GTK_OBJECT (options->context),
				    options->paint_mode_w);
  gimp_context_set_paint_mode (GIMP_CONTEXT (options->context), paint_mode);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (options->context),
				      options->paint_mode_w);
}

static void
paint_options_paint_mode_changed (GimpContext      *context,
				  LayerModeEffects  paint_mode,
				  gpointer          data)
{
  gimp_option_menu_set_history (GTK_OPTION_MENU (data), (gpointer) paint_mode);
}
