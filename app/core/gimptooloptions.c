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

#include "brush_select.h"
#include "gimprc.h"
#include "gimpui.h"
#include "paint_funcs.h"
#include "paint_options.h"
#include "selection_options.h"

#include "libgimp/gimpunitmenu.h"

#include "libgimp/gimpintl.h"


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
static GSList  *paint_options_list = NULL;

static PaintPressureOptions * paint_pressure_options_new   (ToolType);
static void                   paint_pressure_options_reset (PaintPressureOptions *);


/*  ui helper functions  ******************************************************/

static void
tool_options_opacity_adjustment_update (GtkAdjustment *adjustment,
					gpointer       data)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (data), adjustment);
  gimp_context_set_opacity (GIMP_CONTEXT (data),
			    adjustment->value / 100);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (data), adjustment);
}

static void
tool_options_opacity_changed (GimpContext *context,
			      gdouble      opacity,
			      gpointer     data)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (data), context);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (data), opacity * 100);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (data), context);
}

static void
tool_options_paint_mode_update (GtkWidget *widget,
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
tool_options_paint_mode_changed (GimpContext      *context,
				 LayerModeEffects  paint_mode,
				 gpointer          data)
{
  g_print ("tool_options_paint_mode_changed\n");

  gimp_option_menu_set_history (GTK_OPTION_MENU (data), (gpointer) paint_mode);
}

/*  tool options functions  ***************************************************/

void
tool_options_init (ToolOptions          *options,
		   gchar                *title,
		   ToolOptionsResetFunc  reset_func)
{
  options->main_vbox  = gtk_vbox_new (FALSE, 2);
  options->title      = title;
  options->reset_func = reset_func;
}

ToolOptions *
tool_options_new (gchar *title)
{
  ToolOptions *options;

  GtkWidget *label;

  options = g_new (ToolOptions, 1);
  tool_options_init (options, title, NULL);

  label = gtk_label_new (_("This tool has no options."));
  gtk_box_pack_start (GTK_BOX (options->main_vbox), label, FALSE, FALSE, 6);
  gtk_widget_show (label);

  return options;
}

/*  selection tool options functions  *****************************************/

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
		      _("Rectangular Selection") :
		      ((tool_type == ELLIPSE_SELECT) ?
		       _("Elliptical Selection") :
		       ((tool_type == FREE_SELECT) ?
			_("Free-Hand Selection") :
			((tool_type == FUZZY_SELECT) ?
			 _("Fuzzy Selection") :
			 ((tool_type == BEZIER_SELECT) ?
			  _("Bezier Selection") :
			  ((tool_type == ISCISSORS) ?
			   _("Intelligent Scissors") :
			   ((tool_type == BY_COLOR_SELECT) ?
			    _("By-Color Selection") :
			    "ERROR: Unknown Select Tool Type"))))))),
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
  options->fixed_unit     = options->fixed_unit_d     = GIMP_UNIT_PIXEL;

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
  if (tool_type != RECT_SELECT)
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
  switch (tool_type)
    {
    case FREE_SELECT:
    case BEZIER_SELECT:
    case ISCISSORS:
      break;
    case RECT_SELECT:
    case ELLIPSE_SELECT:
    case FUZZY_SELECT:
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
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
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
selection_options_new (ToolType              tool_type,
		       ToolOptionsResetFunc  reset_func)
{
  SelectionOptions *options;

  options = g_new (SelectionOptions, 1);
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
}


/*  paint tool options functions  *********************************************/

void
paint_options_init (PaintOptions         *options,
		    ToolType              tool_type,
		    ToolOptionsResetFunc  reset_func)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *scale;
  GtkWidget *separator;

  GimpContext *tool_context = tool_info[tool_type].tool_context;

  /*  initialize the tool options structure  */
  tool_options_init ((ToolOptions *) options,
		     ((tool_type == BUCKET_FILL) ?
		      _("Bucket Fill") :
		      ((tool_type == BLEND) ?
		       _("Blend Tool") :
		       ((tool_type == PENCIL) ?
			_("Pencil") :
			((tool_type == PAINTBRUSH) ?
			 _("Paintbrush") :
			 ((tool_type == ERASER) ?
			  _("Eraser") :
			  ((tool_type == AIRBRUSH) ?
			   _("Airbrush") :
			   ((tool_type == CLONE) ?
			    _("Clone Tool") :
			    ((tool_type == CONVOLVE) ?
			     _("Convolver") :
			     ((tool_type == INK) ?
			      _("Ink Tool") :
			      ((tool_type == DODGEBURN) ?
			       _("Dodge or Burn") :
			       ((tool_type == SMUDGE) ?
				_("Smudge Tool") :
/*  				((tool_type == XINPUT_AIRBRUSH) ? */
/*  				 _("Xinput Airbrush") : */
				 "ERROR: Unknown Paint Tool Type"))))))))))),
		     reset_func);

  /*  initialize the paint options structure  */
  options->global        = NULL;
  options->opacity_w     = NULL;
  options->paint_mode_w  = NULL;
  options->context       = tool_context;
  options->incremental_w = NULL;
  options->incremental = options->incremental_d = FALSE;

  options->pressure_options = paint_pressure_options_new (tool_type);

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
    gtk_adjustment_new (gimp_context_get_opacity (tool_context) * 100,
			0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->opacity_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->opacity_w), "value_changed",
		      GTK_SIGNAL_FUNC (tool_options_opacity_adjustment_update),
		      tool_context);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Opacity:"), 1.0, 1.0,
			     scale, 1, FALSE);

  gtk_signal_connect (GTK_OBJECT (tool_context), "opacity_changed",
		      GTK_SIGNAL_FUNC (tool_options_opacity_changed),
		      options->opacity_w);

  /*  the paint mode menu  */
  switch (tool_type)
    {
    case BUCKET_FILL:
    case BLEND:
    case PENCIL:
    case PAINTBRUSH:
    case AIRBRUSH:
    case CLONE:
    case INK:
/*      case XINPUT_AIRBRUSH: */
      gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);

      options->paint_mode_w =
	paint_mode_menu_new (tool_options_paint_mode_update, options,
			     gimp_context_get_paint_mode (tool_context));
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
				 _("Mode:"), 1.0, 0.5,
				 options->paint_mode_w, 1, TRUE);

      gtk_signal_connect (GTK_OBJECT (tool_context), "paint_mode_changed",
			  GTK_SIGNAL_FUNC (tool_options_paint_mode_changed),
			  options->paint_mode_w);
      break;
    case CONVOLVE:
    case ERASER:
    case DODGEBURN:
    case SMUDGE:
      break;
    default:
      break;
    }

  /*  show the main table  */
  gtk_widget_show (table);

  /*  a separator after the common paint options which can be global  */
  switch (tool_type)
    {
    case BUCKET_FILL:
    case BLEND:
    case CLONE:
    case CONVOLVE:
    case INK:
    case DODGEBURN:
    case SMUDGE:
/*      case XINPUT_AIRBRUSH: */
      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
      break;
    case AIRBRUSH:
    case ERASER:
    case PAINTBRUSH:
    case PENCIL:
      break;
    default:
      break;
    }

  if (! global_paint_options)
    gtk_widget_show (vbox);

  /*  the "incremental" toggle  */
  switch (tool_type)
    {
    case AIRBRUSH:
    case ERASER:
    case PAINTBRUSH:
    case PENCIL:
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

    case BUCKET_FILL:
    case BLEND:
    case CLONE:
    case CONVOLVE:
    case DODGEBURN:
    case SMUDGE:
      break;
    default:
      break;
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
paint_options_new (ToolType              tool_type,
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

static PaintPressureOptions *
paint_pressure_options_new (ToolType tool_type)
{
  PaintPressureOptions *pressure;
  GtkWidget *frame = NULL;
  GtkWidget *hbox = NULL;

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

  switch (tool_type)
    {
    case AIRBRUSH:
    case CLONE:
    case CONVOLVE:
    case DODGEBURN:
    case ERASER:
    case PAINTBRUSH:
    case PENCIL:
    case SMUDGE:
      frame = gtk_frame_new (_("Pressure Sensitivity"));
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      gtk_widget_show (hbox);
      break;
    default:
      break;
    }

  /*  the opacity toggle  */
  switch (tool_type)
    {
    case CLONE:
    case DODGEBURN:
    case ERASER:
    case PAINTBRUSH:
    case PENCIL:
      pressure->opacity_w =
	gtk_check_button_new_with_label (_("Opacity"));
      gtk_container_add (GTK_CONTAINER (hbox), pressure->opacity_w);
      gtk_signal_connect (GTK_OBJECT (pressure->opacity_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->opacity);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->opacity_w),
				    pressure->opacity_d);
      gtk_widget_show (pressure->opacity_w);
      break;
    default:
      break;
    }

 /*  the pressure toggle  */
  switch (tool_type)
    {
    case AIRBRUSH:
    case CLONE:
    case CONVOLVE:
    case DODGEBURN:
    case ERASER:
    case PAINTBRUSH:
    case SMUDGE:
      pressure->pressure_w = gtk_check_button_new_with_label (_("Pressure"));
      gtk_container_add (GTK_CONTAINER (hbox), pressure->pressure_w);
      gtk_signal_connect (GTK_OBJECT (pressure->pressure_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->pressure);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->pressure_w),
				    pressure->pressure_d);
      gtk_widget_show (pressure->pressure_w);
      break;
    default:
      break;
    }

  /*  the rate toggle */
  switch (tool_type)
    {
    case AIRBRUSH:
    case CONVOLVE:
    case SMUDGE:
      pressure->rate_w =
	gtk_check_button_new_with_label (_("Rate"));
      gtk_container_add (GTK_CONTAINER (hbox), pressure->rate_w);
      gtk_signal_connect (GTK_OBJECT (pressure->rate_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->rate);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->rate_w),
				    pressure->rate_d);
      gtk_widget_show (pressure->rate_w);
      break;
    default:
      break;
    }

  /*  the size toggle  */
  switch (tool_type)
    {
    case AIRBRUSH:
    case CLONE:
    case CONVOLVE:
    case DODGEBURN:
    case ERASER:
    case PAINTBRUSH:
    case PENCIL:
      pressure->size_w =
	gtk_check_button_new_with_label (_("Size"));
      gtk_container_add (GTK_CONTAINER (hbox), pressure->size_w);
      gtk_signal_connect (GTK_OBJECT (pressure->size_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->size);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->size_w),
				    pressure->size_d);
      gtk_widget_show (pressure->size_w);
      break;
    default:
      break;
    }

  /*  the color toggle  */
  switch (tool_type)
    {
    case AIRBRUSH:
    case PAINTBRUSH:
    case PENCIL:
      pressure->color_w =
	gtk_check_button_new_with_label (_("Color"));
      gtk_container_add (GTK_CONTAINER (hbox), pressure->color_w);
      gtk_signal_connect (GTK_OBJECT (pressure->color_w), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &pressure->color);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pressure->color_w),
				    pressure->color_d);
      gtk_widget_show (pressure->color_w);
      break;
    default:
      break;
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


/*  global paint options functions  *******************************************/

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

  /*  NULL means the main brush selection  */
  brush_select_show_paint_options (NULL, global);
}


/*  create a paint mode menu  *************************************************/

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
     _("Multiply (Burn)"), (gpointer) MULTIPLY_MODE, NULL,
     _("Divide (Dodge)"),  (gpointer) DIVIDE_MODE, NULL,
     _("Screen"),          (gpointer) SCREEN_MODE, NULL,
     _("Overlay"),         (gpointer) OVERLAY_MODE, NULL,
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
