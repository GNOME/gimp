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
#include "appenv.h"
#include "gdisplay.h"
#include "tools.h"
#include "perspective_tool.h"
#include "rotate_tool.h"
#include "scale_tool.h"
#include "shear_tool.h"
#include "transform_tool.h"

typedef struct _TransformOptions TransformOptions;

struct _TransformOptions
{
  int         smoothing;
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
transform_type_callback (GtkWidget *w,
			 gpointer   client_data)
{
  transform_change_type ((long) client_data);
}

static TransformOptions *
create_transform_options (void)
{
  TransformOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkWidget *smoothing_toggle;
  GSList *group = NULL;
  int i;
  char *button_names[4] =
  {
    "Rotation",
    "Scaling",
    "Shearing",
    "Perspective"
  };

  /*  the new options structure  */
  options = (TransformOptions *) g_malloc (sizeof (TransformOptions));
  options->smoothing = 1;
  options->type = ROTATE;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Transform Tool Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new ("Transform");
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 4; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
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
  smoothing_toggle = gtk_check_button_new_with_label ("Smoothing");
  gtk_box_pack_start (GTK_BOX (vbox), smoothing_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (smoothing_toggle), "toggled",
		      (GtkSignalFunc) transform_toggle_update,
		      &options->smoothing);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (smoothing_toggle),
                      options->smoothing);
  gtk_widget_show (smoothing_toggle);


  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (ROTATE, vbox);
  tools_register_options (SCALE, vbox);
  tools_register_options (SHEAR, vbox);
  tools_register_options (PERSPECTIVE, vbox);

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
    return 1;
  else
    return transform_options->smoothing;
}
