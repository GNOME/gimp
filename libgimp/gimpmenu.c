/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmenu.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


#define MENU_THUMBNAIL_WIDTH   24
#define MENU_THUMBNAIL_HEIGHT  24


static void      gimp_menu_callback         (GtkWidget         *widget,
                                             gint32            *id);
static void      fill_preview_with_thumb    (GtkWidget         *widget,
                                             gint32             drawable_ID,
                                             gint               width,
                                             gint               height);


GtkWidget *
gimp_image_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_image)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *label;
  gint32    *images;
  gint       nimages;
  gint       i, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", (gpointer) callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  images = gimp_image_list (&nimages);

  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);
	label = g_strdup_printf ("%s-%d", name, images[i]);
	g_free (name);

	menuitem = gtk_menu_item_new_with_label (label);
	g_signal_connect (menuitem, "activate",
                          G_CALLBACK (gimp_menu_callback),
                          &images[i]);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	gtk_widget_show (menuitem);

	g_free (label);

	if (images[i] == active_image)
	  gtk_menu_set_active (GTK_MENU (menu), k);

	k += 1;
      }

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (images)
    {
      if (active_image == -1)
	active_image = images[0];

      (* callback) (active_image, data);

      g_free (images);
    }

  return menu;
}

GtkWidget *
gimp_layer_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_layer)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *image_label;
  gchar     *label;
  gint32    *images;
  gint32    *layers;
  gint32     layer;
  gint       nimages;
  gint       nlayers;
  gint       i, j, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  layer = -1;

  images = gimp_image_list (&nimages);

  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);
	image_label = g_strdup_printf ("%s-%d", name, images[i]);
	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);

	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_layer_get_name (layers[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &layers[j]);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);
	      
	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box),
                                      GDK_RGB_DITHER_MAX);

	      fill_preview_with_thumb (wcolor_box,
				       layers[j],
				       MENU_THUMBNAIL_WIDTH,
				       MENU_THUMBNAIL_HEIGHT);

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box),
                                           MENU_THUMBNAIL_WIDTH,
                                           MENU_THUMBNAIL_HEIGHT);

	      gtk_container_add (GTK_CONTAINER (vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
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
        g_free (layers);
      }

  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (layer != -1)
    (* callback) (layer, data);

  return menu;
}

GtkWidget *
gimp_channel_menu_new (GimpConstraintFunc constraint,
		       GimpMenuCallback   callback,
		       gpointer           data,
		       gint32             active_channel)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *image_label;
  gchar     *label;
  gint32    *images;
  gint32    *channels;
  gint32     channel;
  gint       nimages;
  gint       nchannels;
  gint       i, j, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  channel = -1;

  images = gimp_image_list (&nimages);

  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);
	image_label = g_strdup_printf ("%s-%d", name, images[i]);
	g_free (name);

	channels = gimp_image_get_channels (images[i], &nchannels);

	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_channel_get_name (channels[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new ();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &channels[j]);
	      
	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);

	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box),
				      GDK_RGB_DITHER_MAX);

 	      fill_preview_with_thumb (wcolor_box, 
				       channels[j], 
				       MENU_THUMBNAIL_WIDTH, 
				       MENU_THUMBNAIL_HEIGHT); 

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box),
                                           MENU_THUMBNAIL_WIDTH,
                                           MENU_THUMBNAIL_HEIGHT);

	      gtk_container_add (GTK_CONTAINER(vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
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
        g_free (channels);
      }

  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (channel != -1)
    (* callback) (channel, data);

  return menu;
}

GtkWidget *
gimp_drawable_menu_new (GimpConstraintFunc constraint,
			GimpMenuCallback   callback,
			gpointer           data,
			gint32             active_drawable)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *image_label;
  gchar     *label;
  gint32    *images;
  gint32    *layers;
  gint32    *channels;
  gint32     drawable;
  gint       nimages;
  gint       nlayers;
  gint       nchannels;
  gint       i, j, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  drawable = -1;

  images = gimp_image_list (&nimages);

  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);
	image_label = g_strdup_printf ("%s-%d", name, images[i]);
	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);

	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_layer_get_name (layers[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new ();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &layers[j]);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER(menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);
	      
	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box), GDK_RGB_DITHER_MAX);

	      fill_preview_with_thumb (wcolor_box,
				       layers[j],
				       MENU_THUMBNAIL_WIDTH,
				       MENU_THUMBNAIL_HEIGHT);

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box), 
                                           MENU_THUMBNAIL_WIDTH , 
                                           MENU_THUMBNAIL_HEIGHT);

	      gtk_container_add (GTK_CONTAINER(vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
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
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_channel_get_name (channels[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new ();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &channels[j]);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);
	      
	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box),
				      GDK_RGB_DITHER_MAX);

 	      fill_preview_with_thumb (wcolor_box, 
				       channels[j], 
				       MENU_THUMBNAIL_WIDTH, 
				       MENU_THUMBNAIL_HEIGHT); 

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box), 
                                           MENU_THUMBNAIL_WIDTH , 
                                           MENU_THUMBNAIL_HEIGHT);
	      
	      gtk_container_add (GTK_CONTAINER (vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
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
        g_free (layers);
        g_free (channels);
      }

  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (drawable != -1)
    (* callback) (drawable, data);

  return menu;
}


/*  private functions  */

static void
gimp_menu_callback (GtkWidget *widget,
		    gint32    *id)
{
  GimpMenuCallback callback;
  gpointer         callback_data;

  callback = (GimpMenuCallback) g_object_get_data (G_OBJECT (widget->parent),
                                                   "gimp_callback");
  callback_data = g_object_get_data (G_OBJECT (widget->parent),
                                     "gimp_callback_data");

  (* callback) (*id, callback_data);
}

static void
fill_preview_with_thumb (GtkWidget *widget,
			 gint32     drawable_ID,
			 gint       width,
			 gint       height)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     x, y;
  guchar  *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0, *p1, *even, *odd;

  bpp = 0; /* Only returned */

  drawable_data = gimp_drawable_get_thumbnail_data (drawable_ID,
                                                    &width, &height, &bpp);

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src  = drawable_data;

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;

      for (x = 0; x < width; x++) 
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble) src[x*4+0]) / 255.0;
	      g = ((gdouble) src[x*4+1]) / 255.0;
	      b = ((gdouble) src[x*4+2]) / 255.0;
	      a = ((gdouble) src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble) src[x*3+0]) / 255.0;
	      g = ((gdouble) src[x*3+1]) / 255.0;
	      b = ((gdouble) src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = g = b = ((gdouble) src[x*bpp+0]) / 255.0;

	      if (bpp == 2)
		a = ((gdouble) src[x*bpp+1]) / 255.0;
	      else
		a = 1.0;
	    }

	  if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	    {
	      c0 = GIMP_CHECK_LIGHT;
	      c1 = GIMP_CHECK_DARK;
	    } 
	  else 
	    {
	      c0 = GIMP_CHECK_DARK;
	      c1 = GIMP_CHECK_LIGHT;
	    }

	  *p0++ = (c0 + (r - c0) * a) * 255.0;
	  *p0++ = (c0 + (g - c0) * a) * 255.0;
	  *p0++ = (c0 + (b - c0) * a) * 255.0;

	  *p1++ = (c1 + (r - c1) * a) * 255.0;
	  *p1++ = (c1 + (g - c1) * a) * 255.0;
	  *p1++ = (c1 + (b - c1) * a) * 255.0;  
	}

      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (widget), odd,  0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (widget), even, 0, y, width);

      src += width * bpp;
    }

  g_free (drawable_data);
  g_free (even);
  g_free (odd);
}
