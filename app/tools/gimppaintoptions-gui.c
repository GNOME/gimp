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

#include "app_procs.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define DEFAULT_USE_FADE        FALSE
#define DEFAULT_FADE_OUT        100.0
#define DEFAULT_FADE_UNIT       GIMP_UNIT_PIXEL
#define DEFAULT_USE_GRADIENT    FALSE
#define DEFAULT_GRADIENT_LENGTH 100.0
#define DEFAULT_GRADIENT_UNIT   GIMP_UNIT_PIXEL
#define DEFAULT_GRADIENT_TYPE   LOOP_TRIANGLE


static PaintPressureOptions * paint_pressure_options_new (GtkType       tool_type,
                                                          PaintOptions *paint_options);
static void   paint_pressure_options_reset (PaintPressureOptions *pressure_options,
                                            PaintOptions         *paint_options);

static PaintGradientOptions * paint_gradient_options_new (GtkType       tool_type,
                                                          PaintOptions *paint_options);
static void   paint_gradient_options_reset (PaintGradientOptions *gradient_options,
                                            PaintOptions         *paint_options);

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

static void   paint_gradient_options_gradient_toggle_callback (GtkWidget    *widget,
                                                               PaintOptions *options);


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

PaintGradientOptions non_gui_gradient_options = 
{
  NULL,
  DEFAULT_USE_FADE,        DEFAULT_USE_FADE,        NULL,
  DEFAULT_FADE_OUT,        DEFAULT_FADE_OUT,        NULL,
  DEFAULT_FADE_UNIT,       DEFAULT_FADE_UNIT,       NULL,
  DEFAULT_USE_GRADIENT,    DEFAULT_USE_GRADIENT,    NULL,
  DEFAULT_GRADIENT_LENGTH, DEFAULT_GRADIENT_LENGTH, NULL,
  DEFAULT_GRADIENT_UNIT,   DEFAULT_GRADIENT_UNIT,   NULL,
  DEFAULT_GRADIENT_TYPE,   DEFAULT_GRADIENT_TYPE,   NULL
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

  tool_info = tool_manager_get_info_by_type (the_gimp, tool_type);

  if (! tool_info)
    {
      g_warning ("%s(): no tool info registered for %s",
                 G_GNUC_FUNCTION, gtk_type_name (tool_type));
    }

  /*  initialize the tool options structure  */
  tool_options_init ((GimpToolOptions *) options,
		     reset_func);

  /*  initialize the paint options structure  */
  options->global           = NULL;
  options->opacity_w        = NULL;
  options->paint_mode_w     = NULL;
  options->context          = tool_info->context;
  options->incremental_w    = NULL;
  options->incremental      = options->incremental_d = FALSE;
  options->incremental_save = FALSE;
  options->pressure_options = NULL;
  options->gradient_options = NULL;

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
	gimp_paint_mode_menu_new (G_CALLBACK (paint_options_paint_mode_update),
				  options,
				  TRUE,
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

  if (! gimprc.global_paint_options)
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

  options->pressure_options = paint_pressure_options_new (tool_type, options);

  if (options->pressure_options->frame)
    {
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->pressure_options->frame, FALSE, FALSE, 0);
      gtk_widget_show (options->pressure_options->frame);
    }

  options->gradient_options = paint_gradient_options_new (tool_type, options);

  if (options->gradient_options->frame)
    {
      gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox),
			  options->gradient_options->frame, FALSE, FALSE, 0);
      gtk_widget_show (options->gradient_options->frame);
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

  if (gimprc.global_paint_options && options->global)
    gtk_widget_show (options->global);

  return options;
}

void
paint_options_reset (GimpToolOptions *tool_options)
{
  PaintOptions *options;
  GimpContext  *default_context;

  options = (PaintOptions *) tool_options;

  default_context = gimp_get_default_context (the_gimp);

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

  paint_pressure_options_reset (options->pressure_options, options);

  paint_gradient_options_reset (options->gradient_options, options);
}

void
paint_options_set_global (gboolean global)
{
  PaintOptions *options;
  GSList *list;

  global = global ? TRUE : FALSE;

  if (gimprc.global_paint_options == global)
    return;

  gimprc.global_paint_options = global;

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


/*  private functions  */

static PaintPressureOptions *
paint_pressure_options_new (GtkType       tool_type,
                            PaintOptions *paint_options)
{
  PaintPressureOptions *pressure = NULL;
  GtkWidget            *frame    = NULL;
  GtkWidget            *wbox     = NULL;

  pressure = g_new0 (PaintPressureOptions, 1);

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
      tool_type == GIMP_TYPE_DODGEBURN_TOOL  ||
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
paint_pressure_options_reset (PaintPressureOptions *pressure,
                              PaintOptions         *paint_options)
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

static PaintGradientOptions *
paint_gradient_options_new (GtkType       tool_type,
                            PaintOptions *paint_options)
{
  PaintGradientOptions *gradient   = NULL;
  GtkWidget            *abox       = NULL;
  GtkWidget            *table      = NULL;
  GtkWidget            *type_label = NULL;
  GtkWidget            *spinbutton = NULL;

  gradient = g_new0 (PaintGradientOptions, 1);

  gradient->use_fade        = gradient->use_fade_d        = DEFAULT_USE_FADE;
  gradient->fade_out        = gradient->fade_out_d        = DEFAULT_FADE_OUT;
  gradient->fade_unit       = gradient->fade_unit_d       = DEFAULT_FADE_UNIT;
  gradient->use_gradient    = gradient->use_gradient_d    = DEFAULT_USE_GRADIENT;
  gradient->gradient_length = gradient->gradient_length_d = DEFAULT_GRADIENT_LENGTH;
  gradient->gradient_unit   = gradient->gradient_unit_d   = DEFAULT_GRADIENT_UNIT;
  gradient->gradient_type   = gradient->gradient_type_d   = DEFAULT_GRADIENT_TYPE;

  gradient->use_fade_w        = NULL;
  gradient->fade_out_w        = NULL;
  gradient->fade_unit_w       = NULL;
  gradient->use_gradient_w    = NULL;
  gradient->gradient_length_w = NULL;
  gradient->gradient_unit_w   = NULL;
  gradient->gradient_type_w   = NULL;

  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      gradient->frame = gtk_frame_new (_("Gradient Options"));
      table = gtk_table_new (3, 3, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
      gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
      gtk_table_set_row_spacing (GTK_TABLE (table), 1, 3);
      gtk_container_add (GTK_CONTAINER (gradient->frame), table);
      gtk_widget_show (table);
    }

  /*  the fade options  */
  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
      gtk_table_attach (GTK_TABLE (table), abox, 0, 1, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (abox);

      /*  the use fade toggle  */
      gradient->use_fade_w =
        gtk_check_button_new_with_label (_("Fade Out"));
      gtk_container_add (GTK_CONTAINER (abox), gradient->use_fade_w);
      gtk_signal_connect (GTK_OBJECT (gradient->use_fade_w), "toggled",
                          GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                          &gradient->use_fade);
      gtk_widget_show (gradient->use_fade_w);

      /*  the fade-out sizeentry  */
      gradient->fade_out_w =
        gtk_adjustment_new (gradient->fade_out_d,
                            1e-5, 32767.0, 1.0, 50.0, 0.0);
      spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (gradient->fade_out_w),
                                        1.0, 0.0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_signal_connect (GTK_OBJECT (gradient->fade_out_w), "value_changed",
                          GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                          &gradient->fade_out);
      gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 0, 1);
      gtk_widget_show (spinbutton);

      /*  the fade-out unitmenu  */
      gradient->fade_unit_w =
        gimp_unit_menu_new ("%a", gradient->fade_unit_d, TRUE, TRUE, TRUE);
      gtk_signal_connect (GTK_OBJECT (gradient->fade_unit_w), "unit_changed",
                          GTK_SIGNAL_FUNC (gimp_unit_menu_update),
                          &gradient->fade_unit);
      g_object_set_data (G_OBJECT (gradient->fade_unit_w), "set_digits",
                           spinbutton);
      gtk_table_attach (GTK_TABLE (table), gradient->fade_unit_w, 2, 3, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (gradient->fade_unit_w);

      /*  automatically set the sensitive state of the fadeout stuff  */
      gtk_widget_set_sensitive (spinbutton, gradient->use_fade_d);
      gtk_widget_set_sensitive (gradient->fade_unit_w, gradient->use_fade_d);
      g_object_set_data (G_OBJECT (gradient->use_fade_w),
                           "set_sensitive", spinbutton);
      g_object_set_data (G_OBJECT (spinbutton),
                           "set_sensitive", gradient->fade_unit_w);
    }

  /*  the gradient options  */
  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
      gtk_table_attach (GTK_TABLE (table), abox, 0, 1, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (abox);

      /*  the use gradient toggle  */
      gradient->use_gradient_w =
        gtk_check_button_new_with_label (_("Gradient"));
      gtk_container_add (GTK_CONTAINER (abox), gradient->use_gradient_w);
      gtk_signal_connect (GTK_OBJECT (gradient->use_gradient_w), "toggled",
                          GTK_SIGNAL_FUNC (paint_gradient_options_gradient_toggle_callback),
                          paint_options);
      gtk_widget_show (gradient->use_gradient_w);

      /*  the gradient length scale  */
      gradient->gradient_length_w =
        gtk_adjustment_new (gradient->gradient_length_d,
                            1e-5, 32767.0, 1.0, 50.0, 0.0);
      spinbutton =
        gtk_spin_button_new (GTK_ADJUSTMENT (gradient->gradient_length_w),
                             1.0, 0.0);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_signal_connect (GTK_OBJECT (gradient->gradient_length_w), "value_changed",
                          GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
                          &gradient->gradient_length);
      gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 1, 2);
      gtk_widget_show (spinbutton);

      /*  the gradient unitmenu  */
      gradient->gradient_unit_w =
        gimp_unit_menu_new ("%a", gradient->gradient_unit_d, TRUE, TRUE, TRUE);
      gtk_signal_connect (GTK_OBJECT (gradient->gradient_unit_w), "unit_changed",
                          GTK_SIGNAL_FUNC (gimp_unit_menu_update),
                          &gradient->gradient_unit);
      g_object_set_data (G_OBJECT (gradient->gradient_unit_w), "set_digits",
                           spinbutton);
      gtk_table_attach (GTK_TABLE (table), gradient->gradient_unit_w, 2, 3, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (gradient->gradient_unit_w);

      /*  the gradient type  */
      type_label = gtk_label_new (_("Type:"));
      gtk_misc_set_alignment (GTK_MISC (type_label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), type_label, 0, 1, 2, 3,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (type_label);

      abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 3, 2, 3);
      gtk_widget_show (abox);

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

      gtk_container_add (GTK_CONTAINER (abox), gradient->gradient_type_w);
      gtk_widget_show (gradient->gradient_type_w);

      gtk_widget_show (table);

      /*  automatically set the sensitive state of the gradient stuff  */
      gtk_widget_set_sensitive (spinbutton, gradient->use_gradient_d);
      gtk_widget_set_sensitive (spinbutton, gradient->use_gradient_d);
      gtk_widget_set_sensitive (gradient->gradient_unit_w,
                                gradient->use_gradient_d);
      gtk_widget_set_sensitive (gradient->gradient_type_w,
                                gradient->use_gradient_d);
      gtk_widget_set_sensitive (type_label, gradient->use_gradient_d);
      gtk_widget_set_sensitive (paint_options->incremental_w,
                                ! gradient->use_gradient_d);
      g_object_set_data (G_OBJECT (gradient->use_gradient_w),
                           "set_sensitive",
                           spinbutton);
      g_object_set_data (G_OBJECT (spinbutton), "set_sensitive",
                           gradient->gradient_unit_w);
      g_object_set_data (G_OBJECT (gradient->gradient_unit_w),
                           "set_sensitive",
                           gradient->gradient_type_w);
      g_object_set_data (G_OBJECT (gradient->gradient_type_w),
                           "set_sensitive",
                           type_label);
      g_object_set_data (G_OBJECT (gradient->use_gradient_w),
                           "inverse_sensitive",
                           paint_options->incremental_w);
    }

  return gradient;
}

static void
paint_gradient_options_reset (PaintGradientOptions *gradient,
                              PaintOptions         *paint_options)
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

static void
paint_gradient_options_gradient_toggle_callback (GtkWidget    *widget,
                                                 PaintOptions *options)
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
