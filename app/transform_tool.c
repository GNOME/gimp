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

typedef struct _TransformOptions TransformOptions;

struct _TransformOptions
{
  int	      direction;
  int         smoothing;
  int	      clip;
  int	      grid_size;
  int         show_grid;
  ToolType    type;
};

/*  local functions  */
static void         transform_change_type     (int);

/*  Static variables  */
static TransformOptions *transform_options = NULL;

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
				 GtkSpinButton *spin)
{
  transform_options->grid_size =
    (int) pow (2.0, 7.0 - gtk_spin_button_get_value_as_int (spin));
  transform_core_grid_density_changed ();
}

static TransformOptions *
create_transform_options (void)
{
  TransformOptions *options;
  GtkWidget *main_box, *box, *vbox, *hbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkWidget *toggle;
  GtkAdjustment *grid_adj;
  GtkWidget *grid_density;
  GSList *group;
  int i;
  char *transform_button_names[4] =
  {
    N_("Rotation"),
    N_("Scaling"),
    N_("Shearing"),
    N_("Perspective")
  };
  char *direction_button_names[2] =
  {
    N_("Traditional"),
    N_("Corrective")
  };

  /*  the new options structure  */
  options = (TransformOptions *) g_malloc (sizeof (TransformOptions));
  options->type = ROTATE;
  options->smoothing = TRUE;
  options->clip = FALSE;
  options->direction = TRANSFORM_TRADITIONAL;
  options->grid_size = 32;
  options->show_grid = TRUE;

  /* the main vbox */
  main_box = gtk_vbox_new (FALSE, 1);
 
  /*  the main label  */
  label = gtk_label_new (_("Transform Tool Options"));
  gtk_box_pack_start (GTK_BOX (main_box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the hbox holding the left and right vboxes*/
  box = gtk_hbox_new (0, 2);
  gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, FALSE, 0);

  /*  the left vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);

  /*  the first radio frame and box, for transform type  */
  radio_frame = gtk_frame_new (_("Transform"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);
  
  /*  the radio buttons  */
  group = NULL;
  for (i = 0; i < 4; i++)
    {
      radio_button =
	gtk_radio_button_new_with_label (group, gettext(transform_button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) transform_type_callback,
			  (gpointer) ((long) ROTATE + i));
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /*  the smoothing toggle button  */
  toggle = gtk_check_button_new_with_label (_("Smoothing"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) transform_toggle_update,
		      &options->smoothing);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                      options->smoothing);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);

  /*  the right vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);

  /*  the second radio frame and box, for transform direction  */
  radio_frame = gtk_frame_new (_("Tool paradigm"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  group = NULL;
  for (i = 0; i < 2; i++)
    {
      radio_button =
	gtk_radio_button_new_with_label (group, gettext(direction_button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) transform_direction_callback,
			  (gpointer) ((long) i));
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /* the show grid toggle button */
  toggle = gtk_check_button_new_with_label (_("Show grid"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                      options->show_grid);
  /* important: connect the signal after setting the state, because calling
     transform_show_grid_update before the tool is created will fail */
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) transform_show_grid_update,
		      &options->show_grid);
  gtk_widget_show (toggle);
  
  /*  the grid density entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Grid density: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  grid_adj = (GtkAdjustment *) gtk_adjustment_new (2.0, 0.0, 5.0, 1.0, 1.0, 0.0);
  grid_density = gtk_spin_button_new (grid_adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (grid_density), TRUE);
  gtk_signal_connect (GTK_OBJECT (grid_adj), "value_changed",
		      (GtkSignalFunc) transform_grid_density_callback,
		      grid_density);
  gtk_box_pack_start (GTK_BOX (hbox), grid_density, FALSE, FALSE, 0);
  gtk_widget_show (grid_density);

  /*  the clip resulting image toggle button  */
  toggle = gtk_check_button_new_with_label (_("Clip result"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) transform_toggle_update,
		      &options->clip);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle),
                      options->clip);
  gtk_widget_show (toggle);

  gtk_widget_show (vbox);
  gtk_widget_show (box);
  

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (ROTATE, main_box);
  tools_register_options (SCALE, main_box);
  tools_register_options (SHEAR, main_box);
  tools_register_options (PERSPECTIVE, main_box);

  return options;
}

Tool *
tools_new_transform_tool ()
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
