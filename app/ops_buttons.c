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


#include "config.h"

#include "appenv.h"
#include "gimprc.h"
#include "ops_buttons.h"

#include "libgimp/gimpintl.h"

void ops_button_pressed_callback (GtkWidget*, GdkEventButton*, gpointer);
void ops_button_extended_callback (GtkWidget*, gpointer);


GtkWidget *ops_button_box_new (GtkWidget   *parent,
			       GtkTooltips *tool_tips,
			       OpsButton   *ops_button,
			       OpsButtonType ops_type)   
{
  GtkWidget *button;
  GtkWidget *button_box;
  GtkWidget *box;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;
  GSList *group = NULL;

  gtk_widget_realize (parent);
  style = gtk_widget_get_style (parent);

  button_box = gtk_hbox_new (FALSE, 1);

  while (ops_button->xpm_data)
    {
      box = gtk_hbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (box), 0);
      
      pixmap = gdk_pixmap_create_from_xpm_d (parent->window,
					     &mask,
					     &style->bg[GTK_STATE_NORMAL],
					     ops_button->xpm_data);
      
      pixmapwid = gtk_pixmap_new (pixmap, mask);
      gtk_box_pack_start (GTK_BOX (box), pixmapwid, TRUE, TRUE, 3);
      gtk_widget_show (pixmapwid);
      gtk_widget_show (box);
      
      switch(ops_type)
	{
	case OPS_BUTTON_NORMAL :
	  button = gtk_button_new ();
	  break;
	case OPS_BUTTON_RADIO :
	  button = gtk_radio_button_new (group);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_container_border_width (GTK_CONTAINER (button), 0);
	  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
	  break;
	default :
	  button = NULL; /*stop compiler complaints */
	  g_error("ops_button_box_new:: unknown type %d\n",ops_type);
	  break;
	}

      gtk_container_add (GTK_CONTAINER (button), box);
      
      if (ops_button->ext_callbacks == NULL)
	{
	  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				     (GtkSignalFunc) ops_button->callback,
				     NULL);
	} else {
	  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			      (GtkSignalFunc) ops_button_pressed_callback,
			      ops_button);	  
	  gtk_signal_connect (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) ops_button_extended_callback,
			      ops_button);
	}

      if (tool_tips != NULL)
	gtk_tooltips_set_tip (tool_tips, button, gettext (ops_button->tooltip), NULL);

      gtk_box_pack_start (GTK_BOX(button_box), button, TRUE, TRUE, 0); 

      gtk_widget_show (button);

      ops_button->widget = button;
      ops_button->modifier = OPS_BUTTON_MODIFIER_NONE;

      ops_button++;
    }
  return (button_box);
}


void
ops_button_box_set_insensitive (OpsButton *ops_button)
{
  g_return_if_fail (ops_button != NULL);

  while (ops_button->widget)
    {
      gtk_widget_set_sensitive (ops_button->widget, FALSE); 
      ops_button++;
    }
}

void
ops_button_pressed_callback (GtkWidget *widget, 
			     GdkEventButton *bevent,
			     gpointer   client_data)
{
  OpsButton *ops_button;

  g_return_if_fail (client_data != NULL);
  ops_button = (OpsButton*)client_data;

  if (bevent->state & GDK_SHIFT_MASK) 
    ops_button->modifier = OPS_BUTTON_MODIFIER_SHIFT;
  else if (bevent->state & GDK_CONTROL_MASK)
    ops_button->modifier = OPS_BUTTON_MODIFIER_CTRL;
  else if (bevent->state & GDK_MOD1_MASK)
    ops_button->modifier = OPS_BUTTON_MODIFIER_ALT;
  else 
    ops_button->modifier = OPS_BUTTON_MODIFIER_NONE;
}

void
ops_button_extended_callback (GtkWidget *widget, 
			      gpointer   client_data)
{
  OpsButton *ops_button;

  g_return_if_fail (client_data != NULL);
  ops_button = (OpsButton*)client_data;

  if (ops_button->modifier < 1 || ops_button->modifier > 3)
    (ops_button->callback) (widget, NULL);
  else {
    if (ops_button->ext_callbacks[ops_button->modifier - 1] != NULL)
      (ops_button->ext_callbacks[ops_button->modifier - 1]) (widget, NULL);
    else
      (ops_button->callback) (widget, NULL);
  } 

  ops_button->modifier = OPS_BUTTON_MODIFIER_NONE;
}
			    





