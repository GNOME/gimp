/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */                                                                             
#include <stdio.h>
#include <string.h>

#include "gimp.h"
#include "gimpui.h"


static char* gimp_base_name     (char *str);
static void  gimp_menu_callback (GtkWidget *w,
				 gint32    *id);


GtkWidget*
gimp_image_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_image)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *filename;
  char *label;
  gint32 *images;
  int nimages;
  int i, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	filename = gimp_image_get_filename (images[i]);
	label = g_new (char, strlen (filename) + 16);
	sprintf (label, "%s-%d", gimp_base_name (filename), images[i]);
	g_free (filename);

	menuitem = gtk_menu_item_new_with_label (label);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    (GtkSignalFunc) gimp_menu_callback,
			    &images[i]);
	gtk_menu_append (GTK_MENU (menu), menuitem);
	gtk_widget_show (menuitem);

	g_free (label);

	if (images[i] == active_image)
	  gtk_menu_set_active (GTK_MENU (menu), k);

	k += 1;
      }

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (images)
    {
      if (active_image == -1)
	active_image = images[0];
      (* callback) (active_image, data);
    }

  return menu;
}

GtkWidget*
gimp_layer_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_layer)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *name;
  char *image_label;
  char *label;
  gint32 *images;
  gint32 *layers;
  gint32 layer;
  int nimages;
  int nlayers;
  int i, j, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  layer = -1;

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_filename (images[i]);
	image_label = g_new (char, strlen (name) + 16);
	sprintf (image_label, "%s-%d", gimp_base_name (name), images[i]);
	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);
	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      name = gimp_layer_get_name (layers[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &layers[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (layers[j] == active_layer)
		{
		  layer = active_layer;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (layer == -1)
		layer = layers[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (layer != -1)
    (* callback) (layer, data);

  return menu;
}

GtkWidget*
gimp_channel_menu_new (GimpConstraintFunc constraint,
		       GimpMenuCallback   callback,
		       gpointer           data,
		       gint32             active_channel)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *name;
  char *image_label;
  char *label;
  gint32 *images;
  gint32 *channels;
  gint32 channel;
  int nimages;
  int nchannels;
  int i, j, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  channel = -1;

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_filename (images[i]);
	image_label = g_new (char, strlen (name) + 16);
	sprintf (image_label, "%s-%d", gimp_base_name (name), images[i]);
	g_free (name);

	channels = gimp_image_get_channels (images[i], &nchannels);
	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      name = gimp_channel_get_name (channels[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &channels[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (channels[j] == active_channel)
		{
		  channel = active_channel;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (channel == -1)
		channel = channels[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (channel != -1)
    (* callback) (channel, data);

  return menu;
}

GtkWidget*
gimp_drawable_menu_new (GimpConstraintFunc constraint,
			GimpMenuCallback   callback,
			gpointer           data,
			gint32             active_drawable)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  char *name;
  char *image_label;
  char *label;
  gint32 *images;
  gint32 *layers;
  gint32 *channels;
  gint32 drawable;
  int nimages;
  int nlayers;
  int nchannels;
  int i, j, k;

  menu = gtk_menu_new ();
  gtk_object_set_user_data (GTK_OBJECT (menu), (gpointer) callback);
  gtk_object_set_data (GTK_OBJECT (menu), "gimp_callback_data", data);

  drawable = -1;

  images = gimp_query_images (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_filename (images[i]);
	image_label = g_new (char, strlen (name) + 16);
	sprintf (image_label, "%s-%d", gimp_base_name (name), images[i]);
	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);
	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      name = gimp_layer_get_name (layers[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &layers[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (layers[j] == active_drawable)
		{
		  drawable = active_drawable;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (drawable == -1)
		drawable = layers[j];

	      k += 1;
	    }

	channels = gimp_image_get_channels (images[i], &nchannels);
	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      name = gimp_channel_get_name (channels[j]);
	      label = g_new (char, strlen (image_label) + strlen (name) + 2);
	      sprintf (label, "%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new_with_label (label);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  (GtkSignalFunc) gimp_menu_callback,
				  &channels[j]);
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (channels[j] == active_drawable)
		{
		  drawable = active_drawable;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (drawable == -1)
		drawable = channels[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label ("none");
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (drawable != -1)
    (* callback) (drawable, data);

  return menu;
}


static char*
gimp_base_name (char *str)
{
  char *t;

  t = strrchr (str, '/');
  if (!t)
    return str;
  return t+1;
}

static void
gimp_menu_callback (GtkWidget *w,
		    gint32    *id)
{
  GimpMenuCallback callback;
  gpointer callback_data;

  callback = (GimpMenuCallback) gtk_object_get_user_data (GTK_OBJECT (w->parent));
  callback_data = gtk_object_get_data (GTK_OBJECT (w->parent), "gimp_callback_data");

  (* callback) (*id, callback_data);
}
