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
#include "actionarea.h"

#include "config.h"
#include "libgimp/gimpintl.h"

void
build_action_area (GtkDialog *      dlg,
		   ActionAreaItem * actions,
		   int              num_actions,
		   int              default_action)
{
  GtkWidget *button;
  int i;
	GList* children;
	GtkWidget* hbbox;

  gtk_container_border_width (GTK_CONTAINER (dlg->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (dlg->action_area), FALSE);
	children = gtk_container_children (GTK_CONTAINER (dlg->action_area));
	if (children == NULL) {
		/* add a right packed hbbox */
		hbbox = gtk_hbutton_box_new();
		gtk_button_box_set_spacing(GTK_BUTTON_BOX (hbbox), 4);
		gtk_box_pack_end(GTK_BOX (dlg->action_area), hbbox, FALSE, FALSE, 0);
		gtk_widget_show(hbbox);
	} else {
		/* get the hbbox */
		hbbox = (GtkWidget*)(g_list_first(children)->data);
	}

  for (i = 0; i < num_actions; i++)
    {
      button = gtk_button_new_with_label (gettext(actions[i].label));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

      if (actions[i].callback)
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) actions[i].callback,
			    actions[i].user_data);

      if (default_action == i)
	gtk_widget_grab_default (button);
      gtk_widget_show (button);

      actions[i].widget = button;
    }
}
