/* Watercolor color_select_module, Raph Levien <raph@acm.org>, February 1998
 *
 * Ported to loadable color-selector, Sven Neumann <sven@gimp.org>, May 1999
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimp/gimpuitypes.h"

#include "libgimp/gimpcolor.h"
#include "libgimp/gimpcolorspace.h"
#include "libgimp/gimpcolorselector.h"
#include "libgimp/gimpmodule.h"
#include "libgimp/gimpmath.h"
#include "libgimp/gimphelpui.h"

#include "gimpmodregister.h"

#include "libgimp/gimpintl.h"


/* definitions and variables */

#define IMAGE_SIZE   200

typedef struct
{
  GimpRGB                    rgb;
  gdouble                    last_x;
  gdouble                    last_y;
  gdouble                    last_pressure;

  gfloat                     pressure_adjust;
  guint32                    motion_time;
  gint                       button_state;

  GimpColorSelectorCallback  callback;
  gpointer                   data;
} ColorselWater;


/* prototypes */
static GtkWidget * colorsel_water_new         (const GimpHSV      *hsv,
					       const GimpRGB      *rgb,
					       gboolean            show_alpha,
				               GimpColorSelectorCallback,
				               gpointer            ,
				               gpointer           *);
static void        colorsel_water_free        (gpointer            data);
static void        colorsel_water_set_color   (gpointer            data,
					       const GimpHSV      *hsv,
					       const GimpRGB      *rgb);
static void        colorsel_water_set_channel (gpointer            data,
					       GimpColorSelectorChannelType  channel);
static void        colorsel_water_update      (ColorselWater      *colorsel);


/* local methods */
static GimpColorSelectorMethods methods = 
{
  colorsel_water_new,
  colorsel_water_free,
  colorsel_water_set_color,
  colorsel_water_set_channel
};


static GimpModuleInfo info =
{
  NULL,
  N_("Watercolor style color selector as a pluggable module"),
  "Raph Levien <raph@acm.org>, Sven Neumann <sven@gimp.org>",
  "v0.3",
  "(c) 1998-1999, released under the GPL",
  "May, 10 1999"
};


static const GtkTargetEntry targets[] =
{
  { "application/x-color", 0 }
};

/*************************************************************/

/* globaly exported init function */
G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
{
  GimpColorSelectorID id;

#ifndef __EMX__
  id = gimp_color_selector_register (_("Watercolor"), "watercolor.html",
				     &methods);
#else
  id = mod_color_selector_register (_("Watercolor"), "watercolor.html",
				    &methods);
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


static gdouble
calc (gdouble x,
      gdouble y,
      gdouble angle)
{
  gdouble s, c;

  s = 1.6 * sin (angle * G_PI / 180) * 256.0 / IMAGE_SIZE;
  c = 1.6 * cos (angle * G_PI / 180) * 256.0 / IMAGE_SIZE;

  return 128 + (x - (IMAGE_SIZE >> 1)) * c - (y - (IMAGE_SIZE >> 1)) * s;
}


/* Initialize the preview */
static void
select_area_draw (GtkWidget *preview)
{
  guchar  buf[3 * IMAGE_SIZE];
  gint    x, y;
  gdouble r, g, b;
  gdouble dr, dg, db;

  for (y = 0; y < IMAGE_SIZE; y++)
    {
      r = calc (0, y, 0);
      g = calc (0, y, 120);
      b = calc (0, y, 240);

      dr = calc (1, y, 0) - r;
      dg = calc (1, y, 120) - g;
      db = calc (1, y, 240) - b;

      for (x = 0; x < IMAGE_SIZE; x++)
	{
	  buf[x * 3]     = CLAMP ((gint) r, 0, 255);
	  buf[x * 3 + 1] = CLAMP ((gint) g, 0, 255);
	  buf[x * 3 + 2] = CLAMP ((gint) b, 0, 255);
	  r += dr;
	  g += dg;
	  b += db;
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, y, IMAGE_SIZE);
    }
}


static void
add_pigment (ColorselWater *colorsel,
	     gboolean       erase,
	     gdouble        x,
	     gdouble        y,
	     gdouble        much)
{
  gdouble r, g, b;

  much *= (gdouble) colorsel->pressure_adjust; 

 if (erase)
    {
      colorsel->rgb.r = 1 - (1 - colorsel->rgb.r) * (1 - much);
      colorsel->rgb.g = 1 - (1 - colorsel->rgb.g) * (1 - much);
      colorsel->rgb.b = 1 - (1 - colorsel->rgb.b) * (1 - much);
    }
  else
    {
      r = calc (x, y, 0) / 255.0;
      if (r < 0) r = 0;
      if (r > 1) r = 1;

      g = calc (x, y, 120) / 255.0;
      if (g < 0) g = 0;
      if (g > 1) g = 1;

      b = calc (x, y, 240) / 255.0;
      if (b < 0) b = 0;
      if (b > 1) b = 1;

      colorsel->rgb.r *= (1 - (1 - r) * much);
      colorsel->rgb.g *= (1 - (1 - g) * much);
      colorsel->rgb.b *= (1 - (1 - b) * much);
    }

  colorsel_water_update (colorsel);
}

static void
draw_brush (ColorselWater *colorsel,
	    GtkWidget     *widget,
	    gboolean       erase,
	    gdouble        x,
	    gdouble        y,
	    gdouble        pressure)
{
  gdouble much; /* how much pigment to mix in */

  if (pressure < colorsel->last_pressure)
    colorsel->last_pressure = pressure;

  much = sqrt ((x - colorsel->last_x) * (x - colorsel->last_x) +
	       (y - colorsel->last_y) * (y - colorsel->last_y) +
	       1000 *
	       (pressure - colorsel->last_pressure) *
	       (pressure - colorsel->last_pressure));

  much *= pressure * 0.05;

  add_pigment (colorsel, erase, x, y, much);

  colorsel->last_x = x;
  colorsel->last_y = y;
  colorsel->last_pressure = pressure;
}

static gint
button_press_event (GtkWidget      *widget,
		    GdkEventButton *event,
		    gpointer        data)
{
  ColorselWater *colorsel;
  gboolean       erase;

  colorsel = (ColorselWater *) data;

  colorsel->last_x = event->x;
  colorsel->last_y = event->y;
  colorsel->last_pressure = event->pressure;

  colorsel->button_state |= 1 << event->button;

  erase = (event->button != 1) ||
    (event->source == GDK_SOURCE_ERASER);

  add_pigment (colorsel, erase, event->x, event->y, 0.05);
  colorsel->motion_time = event->time;

  return FALSE;
}

static gint
button_release_event (GtkWidget      *widget,
		      GdkEventButton *event,
		      gpointer        data)
{
  ColorselWater *colorsel;

  colorsel = (ColorselWater *) data;

  colorsel->button_state &= ~(1 << event->button);

  return TRUE;
}

static gint
motion_notify_event (GtkWidget      *widget,
		     GdkEventMotion *event,
		     gpointer        data)
{
  ColorselWater *colorsel;
  GdkTimeCoord  *coords;
  gint           nevents;
  gint           i;
  gboolean       erase;

  colorsel = (ColorselWater *) data;

  if (event->state & (GDK_BUTTON1_MASK |
		      GDK_BUTTON2_MASK |
		      GDK_BUTTON3_MASK |
		      GDK_BUTTON4_MASK))
    {
      coords = gdk_input_motion_events (event->window, event->deviceid,
					colorsel->motion_time, event->time,
					&nevents);
      erase = (event->state & 
	       (GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK)) ||
	(event->source == GDK_SOURCE_ERASER);

      colorsel->motion_time = event->time;

      if (coords)
	{
	  for (i=0; i<nevents; i++)
	    draw_brush (colorsel, widget,
			erase,
			coords[i].x,
			coords[i].y,
			coords[i].pressure);

	  g_free (coords);
	}
      else
	{
	  if (event->is_hint)
	    gdk_input_window_get_pointer (event->window, event->deviceid,
#ifdef GTK_HAVE_SIX_VALUATORS
                                          NULL, NULL, NULL, NULL, NULL, NULL, NULL
#else /* !GTK_HAVE_SIX_VALUATORS */
					  NULL, NULL, NULL, NULL, NULL, NULL
#endif /* GTK_HAVE_SIX_VALUATORS */
					  );

	  draw_brush (colorsel, widget,
		      erase,
		      event->x,
		      event->y,
		      event->pressure);
	}
    }
  else
    {
      gdk_input_window_get_pointer (event->window, event->deviceid,
				    &event->x, &event->y,
#ifdef GTK_HAVE_SIX_VALUATORS
                                    NULL, NULL, NULL, NULL, NULL
#else /* !GTK_HAVE_SIX_VALUATORS */
				    NULL, NULL, NULL, NULL
#endif /* GTK_HAVE_SIX_VALUATORS */
				    );
    }

  return TRUE;
}

static gint
proximity_out_event (GtkWidget         *widget,
                     GdkEventProximity *event,
		     gpointer           data)
{
  ColorselWater *colorsel;

  colorsel = (ColorselWater *) data;

  return TRUE;
}

static void
pressure_adjust_update (GtkAdjustment *adj,
			gpointer       data)
{
  ColorselWater *colorsel;

  colorsel = (ColorselWater *) data;

  colorsel->pressure_adjust = adj->value / 100;
}


/***********/
/* methods */


static GtkWidget*
colorsel_water_new (const GimpHSV             *hsv,
		    const GimpRGB             *rgb,
		    gboolean                   show_alpha,
		    GimpColorSelectorCallback  callback,
		    gpointer                   callback_data,
		    /* RETURNS: */
		    gpointer                  *selector_data)
{
  ColorselWater *coldata;
  GtkWidget     *preview;
  GtkWidget     *event_box;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkObject     *adj;
  GtkWidget     *scale;

  coldata = g_new (ColorselWater, 1);

  coldata->pressure_adjust = 1.0;

  coldata->callback        = callback;
  coldata->data            = callback_data;

  *selector_data = coldata;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
  
  /* the event box */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0); 

  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (frame), event_box);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), IMAGE_SIZE, IMAGE_SIZE);
  gtk_container_add (GTK_CONTAINER (event_box), preview);
  select_area_draw (preview);

  /* Event signals */
  gtk_signal_connect (GTK_OBJECT (event_box), "motion_notify_event",
		      GTK_SIGNAL_FUNC (motion_notify_event),
		      coldata);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_press_event",
		      GTK_SIGNAL_FUNC (button_press_event),
		      coldata);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_release_event",
		      GTK_SIGNAL_FUNC (button_release_event),
		      coldata);
  gtk_signal_connect (GTK_OBJECT (event_box), "proximity_out_event",
		      GTK_SIGNAL_FUNC (proximity_out_event),
		      coldata);

  gtk_widget_set_events (event_box,
			 GDK_EXPOSURE_MASK            |
			 GDK_LEAVE_NOTIFY_MASK        |
			 GDK_BUTTON_PRESS_MASK        |
			 GDK_KEY_PRESS_MASK           |
			 GDK_POINTER_MOTION_MASK      |
			 GDK_POINTER_MOTION_HINT_MASK |
			 GDK_PROXIMITY_OUT_MASK);

  /* The following call enables tracking and processing of extension
   * events for the drawing area
   */
  gtk_widget_set_extension_events (event_box, GDK_EXTENSION_EVENTS_ALL);
  gtk_widget_grab_focus (event_box);

  adj = gtk_adjustment_new (100.0, 0.0, 200.0, 1.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (pressure_adjust_update),
		      coldata);
  scale = gtk_vscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gimp_help_set_help_data (scale, _("Pressure"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), scale, FALSE, FALSE, 0);

  gtk_widget_show_all (vbox);

  colorsel_water_set_color (coldata, hsv, rgb);

  return vbox;
}


static void
colorsel_water_free (gpointer  selector_data)
{
  g_free (selector_data);
}

static void
colorsel_water_set_color (gpointer       data,
			  const GimpHSV *hsv,
			  const GimpRGB *rgb)
{
  ColorselWater *colorsel;

  colorsel = (ColorselWater *) data;

  colorsel->rgb = *rgb;
}

static void
colorsel_water_set_channel (gpointer                      data,
			    GimpColorSelectorChannelType  channel)
{
}

static void
colorsel_water_update (ColorselWater *colorsel)
{
  GimpHSV hsv;

  gimp_rgb_to_hsv (&colorsel->rgb, &hsv);

  colorsel->callback (colorsel->data, &hsv, &colorsel->rgb);
}
