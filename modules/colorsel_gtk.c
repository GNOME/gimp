/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * colorsel_gtk module (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpmodregister.h"

#include "libgimp/gimpcolorselector.h"
#include "libgimp/gimpmodule.h"

#include "libgimp/gimpintl.h"


/* prototypes */
static GtkWidget * colorsel_gtk_new         (const GimpHSV             *hsv,
					     const GimpRGB             *rgb,
					     gboolean                   show_alpha,
					     GimpColorSelectorCallback  callback,
					     gpointer                   data,
					     gpointer                  *selector_data);
static void        colorsel_gtk_free        (gpointer                   data);
static void        colorsel_gtk_set_color   (gpointer                   data,
					     const GimpHSV             *hsv,
					     const GimpRGB             *rgb);
static void        colorsel_gtk_update      (GtkWidget                 *widget,
					     gpointer                   data);

/* EEK */
static gboolean    colorsel_gtk_widget_idle_hide (gpointer           data);
static void        colorsel_gtk_widget_hide      (GtkWidget         *widget,
						  gpointer           data);


/* local methods */
static GimpColorSelectorMethods methods = 
{
  colorsel_gtk_new,
  colorsel_gtk_free,
  colorsel_gtk_set_color,
  NULL  /*  set_channel  */
};

static GimpModuleInfo info =
{
  NULL,
  N_("GTK color selector as a pluggable color selector"),
  "Austin Donnelly <austin@gimp.org>",
  "v0.02",
  "(c) 1999, released under the GPL",
  "17 Jan 1999"
};


/* globaly exported init function */
G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
{
  GimpColorSelectorID id;

#ifndef __EMX__
  id = gimp_color_selector_register ("GTK", "gtk.html", &methods);
#else
  id = mod_color_selector_register  ("GTK", "gtk.html", &methods);
#endif   

  if (id)
    {
      info.shutdown_data = id;
      *inforet = &info;
      return GIMP_MODULE_OK;
    }
  else
    {
      return GIMP_MODULE_UNLOAD;
    }
}


G_MODULE_EXPORT void
module_unload (gpointer                     shutdown_data,
	       GimpColorSelectorFinishedCB  completed_cb,
	       gpointer                     completed_data)
{
#ifndef __EMX__
  gimp_color_selector_unregister (shutdown_data, completed_cb, completed_data);
#else
  mod_color_selector_unregister (shutdown_data, completed_cb, completed_data);
#endif
}


/******************************/
/* GTK color selector methods */

typedef struct
{
  GtkWidget                 *selector;
  GimpColorSelectorCallback  callback;
  void                      *client_data;
} ColorselGtk;


static GtkWidget *
colorsel_gtk_new (const GimpHSV             *hsv,
		  const GimpRGB             *rgb,
		  gboolean                   show_alpha,
		  GimpColorSelectorCallback  callback,
		  gpointer                   data,
		  /* RETURNS: */
		  gpointer                  *selector_data)
{
  GtkWidget   *hbox;
  GtkWidget   *vbox;
  ColorselGtk *p;

  p = g_new (ColorselGtk, 1);

  p->selector    = gtk_color_selection_new ();
  p->callback    = callback;
  p->client_data = data;

  gtk_color_selection_set_opacity (GTK_COLOR_SELECTION (p->selector),
				   show_alpha);

  gtk_widget_hide (GTK_COLOR_SELECTION (p->selector)->scales[0]->parent);

  colorsel_gtk_set_color (p, hsv, rgb);

  /* EEK: to be removed */
  gtk_signal_connect_object_after
    (GTK_OBJECT (GTK_COLOR_SELECTION (p->selector)->sample_area), "realize",
     GTK_SIGNAL_FUNC (colorsel_gtk_widget_hide),
     GTK_OBJECT (GTK_COLOR_SELECTION (p->selector)->sample_area->parent));

  gtk_signal_connect (GTK_OBJECT (p->selector), "color_changed",
		      GTK_SIGNAL_FUNC (colorsel_gtk_update),
		      p);

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

  gtk_box_pack_start (GTK_BOX (vbox), p->selector, FALSE, FALSE, 0);
  gtk_widget_show (p->selector);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  *selector_data = p;

  return hbox;
}

static void
colorsel_gtk_free (gpointer data)
{
  ColorselGtk *p = data;  

  /* don't need to gtk_widget_destroy() the selector, since that's
   * done for us.
   */

  g_free (p);
}

static void
colorsel_gtk_set_color (gpointer       data,
			const GimpHSV *hsv,
			const GimpRGB *rgb)
{
  ColorselGtk *p = data;

  gdouble color[4];

  color[0] = rgb->r;
  color[1] = rgb->g;
  color[2] = rgb->b;
  color[3] = rgb->a;

  gtk_color_selection_set_color (GTK_COLOR_SELECTION (p->selector), color);
}

static void
colorsel_gtk_update (GtkWidget *widget,
		     gpointer   data)
{
  ColorselGtk *p = data;
  GimpHSV      hsv;
  GimpRGB      rgb;
  gdouble      color[4];

  gtk_color_selection_get_color (GTK_COLOR_SELECTION (p->selector), color);

  rgb.r = color[0];
  rgb.g = color[1];
  rgb.b = color[2];
  rgb.a = color[3];

  gimp_rgb_to_hsv (&rgb, &hsv);

  p->callback (p->client_data, &hsv, &rgb);
}

/* EEK */
static gboolean
colorsel_gtk_widget_idle_hide (gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (data));

  return FALSE;
}

static void
colorsel_gtk_widget_hide (GtkWidget *widget,
			  gpointer   data)
{
  g_idle_add (colorsel_gtk_widget_idle_hide, widget);
}
