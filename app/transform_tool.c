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
#include <stdlib.h>
#include <math.h>
#include "appenv.h"
#include "gdisplay.h"
#include "tools.h"
#include "perspective_tool.h"
#include "rotate_tool.h"
#include "scale_tool.h"
#include "shear_tool.h"
#include "transform_core.h"
#include "transform_tool.h"

#include "config.h"
#include "libgimp/gimpintl.h"

/*  the transform structures  */

typedef struct _TransformOptions TransformOptions;
struct _TransformOptions
{
  int	      direction;
  int         direction_d;
  GtkWidget  *direction_w;

  int         smoothing;
  int         smoothing_d;
  GtkWidget  *smoothing_w;

  int	      clip;
  int         clip_d;
  GtkWidget  *clip_w;

  int	      grid_size;
  int         grid_size_d;
  GtkObject  *grid_size_w;

  int         show_grid;
  int         show_grid_d;
  GtkWidget  *show_grid_w;

  ToolType    type;
  ToolType    type_d;
  GtkWidget  *type_w;
};

/*  the transform tool options  */
static TransformOptions *transform_options = NULL;


/*  local functions  */
static void         transform_change_type     (int);


/*  functions  */

static void
transform_toggle_update (GtkWidget *w,
			 gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
transform_show_grid_update (GtkWidget *w,
			    gpointer   data)
{
  transform_toggle_update (w, data);
  transform_core_grid_density_changed ();
}

static void
transform_type_callback (GtkWidget *w,
			 gpointer   client_data)
{
  transform_change_type ((long) client_data);
}

static void
transform_direction_callback (GtkWidget *w,
			      gpointer   client_data)
{
  long dir = (long) client_data;
  
  if (dir == TRANSFORM_TRADITIONAL)
    transform_options->direction = TRANSFORM_TRADITIONAL;
  else
    transform_options->direction = TRANSFORM_CORRECTIVE;
}

static void
transform_grid_density_callback (GtkWidget *w,
				 gpointer   client_data)
{
  transform_options->grid_size =
    (int) (pow (2.0, 7.0 - GTK_ADJUSTMENT (w)->value) + 0.5);
  transform_core_grid_density_changed ();
}

static void
reset_transform_options (void)
{
  TransformOptions *options = transform_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w),
				options->type_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->smoothing_w),
				options->smoothing_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->direction_w), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_grid_w),
				options->show_grid_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->grid_size_w),
			    7.0 - log (options->grid_size_d) / log (2.0));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->clip_w),
				options->clip_d);
}

static TransformOptions *
create_transform_options (void)
{
  TransformOptions *options;
  GtkWidget *main_box, *vbox, *hbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkWidget *grid_density;
  GSList *group;
  int i;
  static const char *transform_button_names[] =
  {
    N_("Rotation"),
    N_("Scaling"),
    N_("Shearing"),
    N_("Perspective")
  };
  static const char *direction_button_names[] =
  {
    N_("Traditional"),
    N_("Corrective")
  };

  /*  the new options structure  */
  options = (TransformOptions *) g_malloc (sizeof (TransformOptions));
  options->type      = options->type_d      = ROTATE;
  options->smoothing = options->smoothing_d = TRUE;
  options->clip      = options->clip_d      = FALSE;
  options->direction = options->direction_d = TRANSFORM_TRADITIONAL;
  options->grid_size = options->grid_size_d = 32;
  options->show_grid = options->show_grid_d = TRUE;

  /* the main hbox */
  main_box = gtk_hbox_new (FALSE, 2);

  /*  the left vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 0);

  /*  the first radio frame and box, for transform type  */
  radio_frame = gtk_frame_new (_("Transform"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);
  
  /*  the radio buttons  */
  group = NULL;
  for (i = 0; i < 4; i++)
    {
      radio_button =
	gtk_radio_button_new_with_label (group,
					 gettext(transform_button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) transform_type_callback,
			  (gpointer) ((long) ROTATE + i));
      gtk_widget_show (radio_button);

      if (i == 0)
	options->type_w = radio_button;
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /*  the smoothing toggle button  */
  options->smoothing_w = gtk_check_button_new_with_label (_("Smoothing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->smoothing_w),
				options->smoothing);
  gtk_signal_connect (GTK_OBJECT (options->smoothing_w), "toggled",
		      (GtkSignalFunc) transform_toggle_update,
		      &options->smoothing);
  gtk_box_pack_start (GTK_BOX (vbox), options->smoothing_w, FALSE, FALSE, 0);
  gtk_widget_show (options->smoothing_w);

  gtk_widget_show (vbox);

  /*  the right vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 0);

  /*  the second radio frame and box, for transform direction  */
  radio_frame = gtk_frame_new (_("Tool paradigm"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  group = NULL;
  for (i = 0; i < 2; i++)
    {
      radio_button =
	gtk_radio_button_new_with_label (group,
					 gettext(direction_button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) transform_direction_callback,
			  (gpointer) ((long) i));
      gtk_widget_show (radio_button);

      if (i == 0)
	options->direction_w = radio_button;
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /* the show grid toggle button */
  options->show_grid_w = gtk_check_button_new_with_label (_("Show grid"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->show_grid_w),
				options->show_grid);
  /* important: connect the signal after setting the state, because calling
     transform_show_grid_update before the tool is created will fail */
  gtk_signal_connect (GTK_OBJECT (options->show_grid_w), "toggled",
		      (GtkSignalFunc) transform_show_grid_update,
		      &options->show_grid);
  gtk_box_pack_start (GTK_BOX (vbox), options->show_grid_w, FALSE, FALSE, 0);
  gtk_widget_show (options->show_grid_w);
  
  /*  the grid density entry  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Grid density:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->grid_size_w =
    gtk_adjustment_new (7.0 - log (options->grid_size) / log (2.0), 0.0, 5.0,
			1.0, 1.0, 0.0);
  grid_density =
    gtk_spin_button_new (GTK_ADJUSTMENT (options->grid_size_w), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (grid_density), TRUE);
  gtk_signal_connect (GTK_OBJECT (options->grid_size_w), "value_changed",
		      (GtkSignalFunc) transform_grid_density_callback,
		      &options->grid_size);
  gtk_box_pack_start (GTK_BOX (hbox), grid_density, FALSE, FALSE, 0);
  gtk_widget_show (grid_density);

  /*  the clip resulting image toggle button  */
  options->clip_w = gtk_check_button_new_with_label (_("Clip result"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->clip_w),
				options->clip);
  gtk_signal_connect (GTK_OBJECT (options->clip_w), "toggled",
		      (GtkSignalFunc) transform_toggle_update,
		      &options->clip);
  gtk_box_pack_start (GTK_BOX (vbox), options->clip_w, FALSE, FALSE, 0);
  gtk_widget_show (options->clip_w);

  gtk_widget_show (vbox);
  
  /*  Register this selection options widget with the main tools options dialog
   */
  tools_register (ROTATE, main_box, _("Transform Tool Options"),
		  reset_transform_options);
  tools_register (SCALE, main_box, _("Transform Tool Options"),
		  reset_transform_options);
  tools_register (SHEAR, main_box, _("Transform Tool Options"),
		  reset_transform_options);
  tools_register (PERSPECTIVE, main_box, _("Transform Tool Options"),
		  reset_transform_options);

  return options;
}

Tool *
tools_new_transform_tool (void)
{
  if (! transform_options)
    transform_options = create_transform_options ();

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
transform_change_type (int new_type)
{
  if (transform_options->type != new_type)
    {
      /*  change the type, free the old tool, create the new tool  */
      transform_options->type = new_type;

      tools_select (transform_options->type);
    }
}

int
transform_tool_smoothing ()
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->smoothing;
}

int
transform_tool_clip ()
{
  if (!transform_options)
    return FALSE;
  else
    return transform_options->clip;
}

int
transform_tool_direction ()
{
  if (!transform_options)
    return TRANSFORM_TRADITIONAL;
  else
    return transform_options->direction;
}

int
transform_tool_grid_size ()
{
  if (!transform_options)
    return 32;
  else
    return transform_options->grid_size;
}

int
transform_tool_show_grid ()
{
  if (!transform_options)
    return TRUE;
  else
    return transform_options->show_grid;
}
