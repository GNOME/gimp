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
#include "gimprc.h"
#include "ops_buttons.h"


GtkWidget *ops_button_box_new (GtkWidget   *parent,
			       GtkTooltips *tool_tips,
			       OpsButton   *ops_buttons)   
{
  GtkWidget *button;
  GtkWidget *button_box;
  GtkWidget *box;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  gtk_widget_realize (parent);
  style = gtk_widget_get_style (parent);

  button_box = gtk_hbox_new (FALSE, 1);

  while (ops_buttons->xpm_data)
    {
      box = gtk_hbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (box), 0);
      
      pixmap = gdk_pixmap_create_from_xpm_d (parent->window,
					     &mask,
					     &style->bg[GTK_STATE_NORMAL],
					     ops_buttons->xpm_data);
      
      pixmapwid = gtk_pixmap_new (pixmap, mask);
      gtk_box_pack_start (GTK_BOX (box), pixmapwid, TRUE, TRUE, 3);
      gtk_widget_show (pixmapwid);
      gtk_widget_show (box);

      button = gtk_button_new ();
      gtk_container_add (GTK_CONTAINER (button), box);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 (GtkSignalFunc) ops_buttons->callback,
				 GTK_OBJECT (parent));

      if (tool_tips != NULL)
	gtk_tooltips_set_tip (tool_tips, button, ops_buttons->tooltip, NULL);

      gtk_box_pack_start (GTK_BOX(button_box), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      ops_buttons->widget = button;

      ops_buttons++;
    }
  return (button_box);
}


void
ops_button_box_set_insensitive (OpsButton *ops_buttons)
{
  while (ops_buttons->widget)
    {
      gtk_widget_set_sensitive (ops_buttons->widget, FALSE); 
      ops_buttons++;
    }
}




