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
#include "appenv.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "perspective_tool.h"
#include "rotate_tool.h"
#include "scale_tool.h"
#include "shear_tool.h"
#include "transform_core.h"
#include "transform_tool.h"

#include "config.h"
#include "libgimp/gimpmath.h"
#include "libgimp/gimpintl.h"

/*  the transform structures  */

typedef struct _TransformOptions TransformOptions;

struct _TransformOptions
{
  ToolOptions  tool_options;

  ToolType     type;
  ToolType     type_d;
  GtkWidget   *type_w[4];  /* 4 radio buttons */

  gboolean     smoothing;
  gboolean     smoothing_d;
  GtkWidget   *smoothing_w;

  gint	       direction;
  gint         direction_d;
  GtkWidget   *direction_w[2];  /* 2 radio buttons */

  gboolean     show_grid;
  gboolean     show_grid_d;
  GtkWidget   *show_grid_w;

  gint	       grid_size;
  gint         grid_size_d;
  GtkObject   *grid_size_w;

  gboolean     clip;
  gboolean     clip_d;
  GtkWidget   *clip_w;

  gboolean     showpath;
  gboolean     showpath_d;
  GtkWidget   *showpath_w;
};


/*  the transform tool options  */
static TransformOptions *transform_options = NULL;


/*  local functions  */
static void   transform_change_type (int);


/*  functions  */

static void
transform_show_grid_update (GtkWidget *widget,
			    gpointer   data)
{
  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfult */

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  tool_options_toggle_update (widget, data);
  transform_core_grid_density_changed ();
}

static void
transform_show_path_update (GtkWidget *widget,
			    gpointer   data)
{
  static gboolean first_call = TRUE;  /* eek, this hack avoids a segfult */

  if (first_call)
    {
      first_call = FALSE;
      return;
    }

  transform_core_showpath_changed (1); /* pause */
  tool_options_toggle_update (widget, data);
  transform_core_showpath_changed (0); /* resume */
}

static void
transform_type_callback (GtkWidget *widget,
			 gpointer   data)
{
  transform_change_type ((long) data);
}

static void
transform_direction_callback (GtkWidget *widget,
			      gpointer   data)
{
  long dir = (long) data;
  
  if (dir == TRANSFORM_TRADITIONAL)
    transform_options->direction = TRANSFORM_TRADITIONAL;
  else
    transform_options->direction = TRANSFORM_CORRECTIVE;
}

static void
transform_grid_density_callback (GtkWidget *widget,
				 gpointer   data)
{
  transform_options->grid_size =
    (int) (pow (2.0, 7.0 - GTK_ADJUSTMENT (widget)->value) + 0.5);

  transform_core_grid_density_changed ();
}

static void
transform_options_reset (void)
{
  TransformOptions *options = transform_options;

  gtk_toggle_button_set_active (((options->type_d == ROTATE) ?
				 GTK_TOGGLE_BUTTON (options->type_w[0]) :
				 ((options->type_d == SCALE) ?
				  GTK_TOGGLE_BUTTON (options->type_w[1]) :
				  ((options->type_d == SHEAR) ?
				   GTK_TOGGLE_BUTTON (options->type_w[2]) :
				   GTK_TOGGLE_BUTTON (options->type_w[3])))),
				TRUE);
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

static TransformOptions *
transform_options_new (void)
{
  TransformOptions *options;

  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *fbox;
  GtkWidget *grid_density;

  /*  the new transform tool options structure  */
  options = (TransformOptions *) g_malloc (sizeof (TransformOptions));
  tool_options_init ((ToolOptions *) options,
		     _("Transform Tool Options"),
		     transform_options_reset);
  options->type      = options->type_d      = ROTATE;
  options->smoothing = options->smoothing_d = TRUE;
  options->showpath  = options->showpath_d  = TRUE;
  options->clip      = options->clip_d      = FALSE;
  options->direction = options->direction_d = TRANSFORM_TRADITIONAL;
  options->grid_size = options->grid_size_d = 32;
  options->show_grid = options->show_grid_d = TRUE;

  /* the main table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_start (GTK_BOX (options->tool_options.main_vbox), table,
		      FALSE, FALSE, 0);

  /*  the left vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), vbox, 0, 1, 0, 1);

  /*  the transform type radio buttons  */
  frame = gimp_radio_group_new (TRUE, _("Transform"),

				_("Roatation"), transform_type_callback,
				ROTATE, NULL, &options->type_w[0], TRUE,
				_("Scaling"), transform_type_callback,
				SCALE, NULL, &options->type_w[1], FALSE,
				_("Shearing"), transform_type_callback,
				SHEAR, NULL, &options->type_w[2], FALSE,
				_("Perspective"), transform_type_callback,
				PERSPECTIVE, NULL, &options->type_w[3], FALSE,

				NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);

  /*  the right vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), vbox, 1, 2, 0, 1);

  /*  the second radio frame and box, for transform direction  */
  frame = gimp_radio_group_new (TRUE, _("Tool Paradigm"),

				_("Traditional"), transform_direction_callback,
				TRANSFORM_TRADITIONAL, NULL,
				&options->direction_w[0], TRUE,
				_("Corrective"), transform_direction_callback,
				TRANSFORM_CORRECTIVE, NULL,
				&options->direction_w[1], FALSE,

				NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the grid frame  */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  fbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (fbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), fbox);

  /*  the show grid toggle button  */
  options->show_grid_w = gtk_check_button_new_with_label (_("Show Grid"));
  gtk_signal_connect (GTK_OBJECT (options->show_grid_w), "toggled",
		      GTK_SIGNAL_FUNC (transform_show_grid_update),
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
		      GTK_SIGNAL_FUNC (transform_grid_density_callback),
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

  gtk_widget_show (vbox);

  /*  the left vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), vbox, 0, 1, 1, 2);

  /*  the smoothing toggle button  */
  options->smoothing_w = gtk_check_button_new_with_label (_("Smoothing"));
  gtk_signal_connect (GTK_OBJECT (options->smoothing_w), "toggled",
		      GTK_SIGNAL_FUNC (tool_options_toggle_update),
		      &options->smoothing);
  gtk_box_pack_start (GTK_BOX (vbox), options->smoothing_w, FALSE, FALSE, 0);
  gtk_widget_show (options->smoothing_w);

  /*  the clip resulting image toggle button  */
  options->clip_w = gtk_check_button_new_with_label (_("Clip Result"));
  gtk_signal_connect (GTK_OBJECT (options->clip_w), "toggled",
		      GTK_SIGNAL_FUNC (tool_options_toggle_update),
		      &options->clip);
  gtk_box_pack_start (GTK_BOX (vbox), options->clip_w, FALSE, FALSE, 0);
  gtk_widget_show (options->clip_w);

  gtk_widget_show (vbox);

  /*  the right vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), vbox, 1, 2, 1, 2);

  /*  the show_path toggle button  */
  options->showpath_w = gtk_check_button_new_with_label (_("Show Path"));
  gtk_signal_connect (GTK_OBJECT (options->showpath_w), "toggled",
		      GTK_SIGNAL_FUNC (transform_show_path_update),
		      &options->showpath);
  gtk_box_pack_start (GTK_BOX (vbox), options->showpath_w, FALSE, FALSE, 0);
  gtk_widget_show (options->showpath_w);

  gtk_widget_show (vbox);

  gtk_widget_show (table);
  
  return options;
}

Tool *
tools_new_transform_tool (void)
{
  /* The tool options */
  if (! transform_options)
    {
      transform_options = transform_options_new ();
      tools_register (ROTATE, (ToolOptions *) transform_options);
      tools_register (SCALE, (ToolOptions *) transform_options);
      tools_register (SHEAR, (ToolOptions *) transform_options);
      tools_register (PERSPECTIVE, (ToolOptions *) transform_options);

      /*  press all default buttons  */
      transform_options_reset ();
    }

  switch (transform_options->type)
    {
    case ROTATE:
      return tools_new_rotate_tool ();
      break;
    case SCALE:
      return tools_new_scale_tool ();
      break;
    case SHEAR:
      return tools_new_shear_tool ();
      break;
    case PERSPECTIVE:
      return tools_new_perspective_tool ();
      break;
    default :
      return NULL;
      break;
    }
}

void
tools_free_transform_tool (Tool *tool)
{
  switch (transform_options->type)
    {
    case ROTATE:
      tools_free_rotate_tool (tool);
      break;
    case SCALE:
      tools_free_scale_tool (tool);
      break;
    case SHEAR:
      tools_free_shear_tool (tool);
      break;
    case PERSPECTIVE:
      tools_free_perspective_tool (tool);
      break;
    default:
      break;
    }
}

static void
transform_change_type (gint new_type)
{
  if (transform_options->type != new_type)
    {
      /*  change the type, free the old tool, create the new tool  */
      transform_options->type = new_type;

      gimp_context_set_tool (gimp_context_get_user (), transform_options->type);
    }
}

int
transform_tool_smoothing (void)
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->smoothing;
}

int
transform_tool_showpath (void)
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->showpath;
}

int
transform_tool_clip (void)
{
  if (!transform_options)
    return FALSE;
  else
    return transform_options->clip;
}

int
transform_tool_direction (void)
{
  if (!transform_options)
    return TRANSFORM_TRADITIONAL;
  else
    return transform_options->direction;
}

int
transform_tool_grid_size (void)
{
  if (!transform_options)
    return 32;
  else
    return transform_options->grid_size;
}

int
transform_tool_show_grid (void)
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->show_grid;
}
